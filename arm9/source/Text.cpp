#include <nds.h>
#include <fat.h>
#include "Text.h"
#include "App.h"
#include "main.h"

Text::Text()
{
	cachenext = 0;
	fontfilename = FONTFILEPATH;
	ftc = false;
	invert = false;
	justify = false;
	pixelsize = PIXELSIZE;
	screenleft = (u16*)BG_BMP_RAM_SUB(0);
	screenright = (u16*)BG_BMP_RAM(0);
	screen = screenleft;
}

Text::Text(App *parent)
{
	app = parent;
	Text();
}

static FT_Error
TextFaceRequester(    FTC_FaceID   face_id,
                      FT_Library   library,
                      FT_Pointer   request_data,
                      FT_Face     *aface )
{
	TextFace face = (TextFace) face_id;   // simple typecase
	return FT_New_Face( library, face->file_path, face->face_index, aface );
}

int Text::InitWithCacheManager(void) {
	if(error = FT_Init_FreeType(&library)) return error;

	FTC_Manager_New(library,0,0,0,
		&TextFaceRequester,NULL,&cache.manager);
	FTC_ImageCache_New(cache.manager,&cache.image);
	FTC_SBitCache_New(cache.manager,&cache.sbit);
	FTC_CMapCache_New(cache.manager,&cache.cmap);

	face_id.file_path = "dslibris.ttf";
	face_id.face_index = 0;
	error =	FTC_Manager_LookupFace(cache.manager, (FTC_FaceID)&face_id, &face);
	if(error) return error;
	FT_Select_Charmap(face,FT_ENCODING_UNICODE);
	charmap_index = FT_Get_Charmap_Index(face->charmap);
/*
	charmap_index = 0;
	for(int i=0; i<face->num_charmaps;i++)
	{
		if((face->charmaps)[i]->encoding == FT_ENCODING_UNICODE)
		{
			charmap_index = i;
		}
	}
*/
	imagetype.face_id = (FTC_FaceID)&face_id;
	imagetype.height = pixelsize;
	imagetype.width = pixelsize;
	ftc = true;

	return 0;
}

int Text::InitDefault(void) {
	if (FT_Init_FreeType(&library)) return 1;
	if (FT_New_Face(library, fontfilename.c_str(), 0, &face)) return 2;
	FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	FT_Set_Pixel_Sizes(face, 0, pixelsize);
	screen = screenleft;
	ClearCache();
	InitPen();
	ftc = false;
	return(0);
}

int Text::CacheGlyph(u32 ucs)
{
	// Cache glyph at ucs if there's space.
	// Does not check if this is a duplicate entry.

	if(cachenext == CACHESIZE) return -1;

	FT_Load_Char(face, ucs,
		FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL);
	FT_GlyphSlot src = face->glyph;
	FT_GlyphSlot dst = &glyphs[cachenext];
	int x = src->bitmap.rows;
	int y = src->bitmap.width;
	dst->bitmap.buffer = new unsigned char[x*y];
	memcpy(dst->bitmap.buffer, src->bitmap.buffer, x*y);
	dst->bitmap.rows = src->bitmap.rows;
	dst->bitmap.width = src->bitmap.width;
	dst->bitmap_top = src->bitmap_top;
	dst->bitmap_left = src->bitmap_left;
	dst->advance = src->advance;
	cache_ucs[cachenext] = ucs;
	cachenext++;
	return cachenext-1;
}

FT_UInt Text::GetGlyphIndex(u32 ucs)
{
	if(!ftc) return ucs;
	return FTC_CMapCache_Lookup(cache.cmap,(FTC_FaceID)&face_id,
		charmap_index,ucs);	
}

int Text::GetGlyphBitmap(u32 ucs, FTC_SBit *sbit)
{
	imagetype.flags = FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL;
	error = FTC_SBitCache_Lookup(cache.sbit,&imagetype,
		GetGlyphIndex(ucs),sbit,NULL);
	if(error) return error;
	return 0;
}

FT_GlyphSlot Text::GetGlyph(u32 ucs, int flags)
{
	if(ftc) return NULL;
	int i;
	for(i=0;i<cachenext;i++)
	{
		if(cache_ucs[i] == ucs) return &glyphs[i];
	}

	i = CacheGlyph(ucs);
	if(i > -1) return &glyphs[i];

	FT_Load_Char(face, ucs, flags);
	return face->glyph;
}

void Text::ClearCache()
{
	cachenext = 0;
}

void Text::ClearScreen()
{
	if(invert) memset((void*)screen,0,PAGE_WIDTH*PAGE_HEIGHT*4);
	else memset((void*)screen,255,PAGE_WIDTH*PAGE_HEIGHT*4);
}

void Text::ClearRect(u16 xl, u16 yl, u16 xh, u16 yh)
{
	u16 clearcolor;
	if(invert) clearcolor = RGB15(0,0,0) | BIT(15);
	else clearcolor = RGB15(31,31,31) | BIT(15);
	for(u16 y=yl; y<yh; y++) {
		for(u16 x=xl; x<xh; x++) {
			screen[y*PAGE_WIDTH+x] = clearcolor;
		}
	}
}

u8 Text::GetStringWidth(const char *txt)
{
	u8 width = 0;
	const char *c;
	for(c = txt; c != NULL; c++)
	{
		u32 ucs;
		GetCharCode(c, &ucs);
		width += GetAdvance(ucs);
	}
	return width;
}	


u8 Text::GetCharCode(const char *utf8, u32 *ucs) {
	// given a UTF-8 encoding, fill in the Unicode/UCS code point.
	// returns the bytelength of the encoding, for advancing
	// to the next character.
	// returns 0 if encoding could not be translated.
	// TODO - handle 4 byte encodings.

	if (utf8[0] < 0x80) { // ASCII
		*ucs = utf8[0];
		return 1;

	} else if (utf8[0] > 0xc1 && utf8[0] < 0xe0) { // latin
		*ucs = ((utf8[0]-192)*64) + (utf8[1]-128);
		return 2;

	} else if (utf8[0] > 0xdf && utf8[0] < 0xf0) { // asian
		*ucs = (utf8[0]-224)*4096 + (utf8[1]-128)*64 + (utf8[2]-128);
		return 3;

	} else if (utf8[0] > 0xef) { // rare
		return 4;

	}
	return 0;
}

u8 Text::GetHeight() {
	return (face->size->metrics.height >> 6);
}

void Text::GetPen(u16 *x, u16 *y) {
	*x = pen.x;
	*y = pen.y;
}

void Text::SetPen(u16 x, u16 y) {
	pen.x = x;
	pen.y = y;
}

void Text::GetPen(u16 &x, u16 &y) {
	x = pen.x;
	y = pen.y;
}

void Text::SetInvert(bool state) {
	invert = state;
}

bool Text::GetInvert() {
	return invert;
}

u8 Text::GetPenX() {
	return pen.x;
}

u8 Text::GetPenY() {
	return pen.y;
}

u8 Text::GetPixelSize()
{
	return pixelsize;
}

u16* Text::GetScreen()
{
	return screen;
}

void Text::SetPixelSize(u8 size)
{
	if(ftc) {
		imagetype.height = size;
		imagetype.width = size;
		pixelsize = size;
		return;
	}

	if (!size) {
		FT_Set_Pixel_Sizes(face, 0, PIXELSIZE);
		pixelsize = PIXELSIZE;
	} else {
		FT_Set_Pixel_Sizes(face, 0, size);
		pixelsize = size;
	}
	ClearCache();
}

void Text::SetScreen(u16 *inscreen)
{
	screen = inscreen;
}

u8 Text::GetAdvance(u32 ucs) {
	if(!ftc)
		// Caches this glyph if possible.
		return GetGlyph(ucs, FT_LOAD_DEFAULT)->advance.x >> 6;

	imagetype.flags = FT_LOAD_DEFAULT;
	error = FTC_SBitCache_Lookup(cache.sbit,&imagetype,
		GetGlyphIndex(ucs),&sbit,NULL);
	return sbit->xadvance;
}

void Text::InitPen(void) {
	pen.x = MARGINLEFT;
	pen.y = MARGINTOP + GetHeight();
}

void Text::PrintChar(u32 ucs) {
	// Draw a character for the given UCS codepoint,
	// into the current screen buffer at the current pen position.

	u16 bx, by, width, height = 0;
	FT_Byte *buffer = NULL;
	FT_UInt advance = 0;

	if(ftc)
	{
		error = GetGlyphBitmap(ucs,&sbit);
		buffer = sbit->buffer;
		bx = sbit->left;
		by = sbit->top;
		height = sbit->height;
		width = sbit->width;
		advance = sbit->xadvance;
	}
	else
	{
		// Consult the cache for glyph data and cache it on a miss
		// if space is available.
		FT_GlyphSlot glyph = GetGlyph(ucs, 
			FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL);
		FT_Bitmap bitmap = glyph->bitmap;
		bx = glyph->bitmap_left;
		by = glyph->bitmap_top;
		width = bitmap.width;
		height = bitmap.rows;
		advance = glyph->advance.x >> 6;
		buffer = bitmap.buffer;
	}

	u16 gx, gy;
	for (gy=0; gy<height; gy++) {
		for (gx=0; gx<width; gx++) {
			u16 a = buffer[gy*width+gx];
			if (a) {
				u16 sx = (pen.x+gx+bx);
				u16 sy = (pen.y+gy-by);
				int l;
				if (invert) l = a >> 3;
				else l = (255-a) >> 3;
				screen[sy*SCREEN_WIDTH+sx] = RGB15(l,l,l) | BIT(15);
			}
		}
	}
	pen.x += advance;
}

bool Text::PrintNewLine(void) {
	pen.x = MARGINLEFT;
	int height = GetHeight();
	int y = pen.y + height + LINESPACING;
	if (y > (PAGE_HEIGHT - MARGINBOTTOM)) {
		if (screen == screenleft)
		{
			screen = screenright;
			pen.y = MARGINTOP + height;
			return true;
		}
		else
			return false;
	}
	else
	{
		pen.y += height + LINESPACING;
		return true;
	}
}

void Text::PrintString(const char *s) {
	// draw a character string starting at the pen position.
	u8 i;
	for (i=0;i<strlen((char*)s);i++) {
		u32 c = s[i];
		if (c == '\n') PrintNewLine();
		else {
			i+=GetCharCode(&(s[i]),&c);
			i--;
			PrintChar(c);
		}
	}
}

void Text::PrintStatusMessage(const char *msg)
{
	u16 x,y;
	u16 *s = screen;
	GetPen(&x,&y);
	screen = screenleft;
	SetPen(10,10);
	PrintString(msg);
	screen = s;
	SetPen(x,y);
}

