#ifndef _text_h
#define _text_h

#include <string>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

using namespace std;

//! Reference: FreeType2 online documentation
#define EMTOPIXEL (float)(POINTSIZE * DPI/72.0)
#define CACHESIZE 256
#define PIXELSIZE 12
//! Reference: http://www.displaymate.com/psp_ds_shootout.htm
#define DPI 110
#define TEXT_BOLD_ON 2
#define TEXT_BOLD_OFF 3
#define TEXT_ITALIC_ON 4
#define TEXT_ITALIC_OFF 5
#define TEXT_STYLE_NORMAL (u8)0
#define TEXT_STYLE_BOLD (u8)1
#define TEXT_STYLE_ITALIC (u8)2
#define TEXT_STYLE_BROWSER (u8)3
#define TEXT_STYLE_SPLASH (u8)4

class App;

typedef struct TextFaceRec_ {
	const char* file_path;
	int face_index;
} TextFaceRec, *TextFace;

typedef struct TextCache_ {
	FTC_Manager manager;
	FTC_CMapCache cmap;
	FTC_ImageCache image;
	FTC_SBitCache sbit;
} TextCache;

class Cache {
public:
	// associates each glyph cache index (value)
	// with its Unicode code point (key).
	map<u16, FT_GlyphSlot> cacheMap;
	u16 cachenext;
	
	Cache() {
		cachenext = 0;
	}
};

#if 0
class Face {
	Cache cache;
	FT_Face ft_face;
	Face() {
		ft_face = NULL;
		cache = new Cache();
	}
	Face(FT_Library library, std::string path, int index) {
		FT_New_Face( library, path.c_str(), index, &ft_face );
		cache = new Cache();
	}
	~Face() {
		if(face) FT_Done_Face(face);
		delete cache;
	}
};

// Maps a style ID to a face.
// Multiple styles might use the same face.
class Style {
	u8 id;
	Face *face;

	Style(Face *f) {
		style = TEXT_STYLE_NORMAL;
		face = f;
	}
};
#endif

//! Typesetter singleton that provides all text rendering services.

//! Implemented atop FreeType 2.
//! Attempts to cache for performance,
//! but the caching is now a bit broken
//! since font styles were introduced.

class Text {
	FT_Library library;
	FT_Error error;

	//! Use the FreeType cache?
	bool ftc;
	TextCache cache;
	TextFaceRec face_id;
	FTC_SBit sbit;
	FTC_ImageTypeRec imagetype;
	FT_Int charmap_index;
	
	map<FT_Face, Cache*> textCache;
	map<u8, FT_Face> faces;
	map<u8, string> filenames;
//	vector<Face> faces;
//	map<u8, Face*> styles;

	FT_Vector pen;
	//! Draw white text on black?
	bool invert;
	//! We wish this worked.
	bool justify;
	u32 codeprev; // last printed char code

	int CacheGlyph(u32 ucs);
	int CacheGlyph(u32 ucs, u8 style);
	int CacheGlyph(u32 ucs, FT_Face face);
	int InitDefault();
	int InitWithCacheManager();
	FT_GlyphSlot GetGlyph(u32 ucs, int flags);
	FT_GlyphSlot GetGlyph(u32 ucs, int flags, u8 style);
	FT_GlyphSlot GetGlyph(u32 ucs, int flags, FT_Face face);
	FT_Error GetGlyphBitmap(u32 ucs, FTC_SBit *asbit);
	FT_UInt GetGlyphIndex(u32 ucs);

	u8   GetAdvance(u32 ucs, FT_Face face);
	u8   GetStringWidth(const char *txt, FT_Face face);
	void ClearCache(FT_Face face);
	void PrintChar(u32 ucs, FT_Face face);
	void PrintString(const char *string, FT_Face face);
	
public:
	App *app;
	int pixelsize;
	//! Not used ... really.
	struct { u8 r; u8 g; u8 b; } bgcolor;
	bool usebgcolor;
	u16 *screen, *screenleft, *screenright;
	struct {
		int left, right, top, bottom;
	} margin;
	struct {
		int width, height;
	} display;
	int linespacing;
	bool linebegan, bold, italic;
	
	// keeps stats to check efficiency of caching.
	int stats_hits;
	int stats_misses;

	Text();
	Text(class App *parent) { app = parent; }
	~Text();
	int  Init();
	void InitPen(void);
	void Begin();
	void End();
	
	u8   GetAdvance(u32 ucs);
	u8   GetAdvance(u32 ucs, u8 style);
	u8   GetCharCode(const char* txt, u32* code);
	FT_Face GetFace(u8 style);
	string GetFontFile(u8 style);
	u8   GetHeight(void);
	bool GetInvert();
	void GetPen(u16 *x, u16 *y);
	void GetPen(u16 &x, u16 &y);
	u8   GetPenX();
	u8   GetPenY();
	u8   GetPixelSize();
	u16* GetScreen();
	u8   GetStringWidth(const char *txt, u8 style);

	void SetInvert(bool invert);
	void SetPen(u16 x, u16 y);
	void SetPixelSize(u8 size);
	bool SetFace(u8 style);
	void SetFontFile(const char *filename, u8 style);
	void SetScreen(u16 *s);
	
	void ClearCache();
	void ClearCache(u8 style);
	void ClearRect(u16 xl, u16 yl, u16 xh, u16 yh);
	void ClearScreen();
	void ClearScreen(u16*, u8, u8, u8);
	void SwapScreens();

	void PrintChar(u32 ucs);
	void PrintChar(u32 ucs, u8 style);
	bool PrintNewLine(void);
	void PrintStats();
	void PrintStatusMessage(const char *msg);
	void PrintString(const char *string);
	void PrintString(const char *string, u8 style);
	void PrintSplash(u16 *screen);
};

#endif

