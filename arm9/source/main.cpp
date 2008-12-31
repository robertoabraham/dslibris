/* 

dslibris - an ebook reader for the Nintendo DS.

 Copyright (C) 2007-2008 Ray Haleblian

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <nds.h>
#include <stdio.h>
#include <expat.h>

#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"
#include "main.h"
#include "parse.h"
#include "splash.h"

App *app;
bool linebegan = false;
// Static vars needed for state in the XML config callback prefs_start_hndl
//char reopenName[MAXPATHLEN];
int book = -1; //! Current book context.
bool parseFontBold = false;
bool parseFontItalic = false;

/*---------------------------------------------------------------------------*/

void spin()
{
	while (1) swiWaitForVBlank();
}

void greenblue()
{
	u16 green = ARGB16(1,0,15,0);
	u16 blue = ARGB16(1,0,0,15);
	for(int i=0; i<256; i++)
	{
		for(int j=0; j<256; j++) {
			BG_BMP_RAM(0)[j+i*256] = green;
			BG_BMP_RAM_SUB(0)[j+i*256] = blue;
		}
	}
}

bool iswhitespace(u8 c)
{
	switch (c)
	{
		case ' ':
		case '\t':
		case '\n':
			return true;
			break;
		default:
			return false;
			break;
	}
}

void initScreenLeft()
{	
	//! Bring up left screen.
	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	int bgMain = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);
	bgSetCenter(bgMain,0,0);
	bgRotate(bgMain,-8192);
	bgScroll(bgMain,0,256);
	bgUpdate(bgMain);
	decompress(splashBitmap, BG_GFX, LZ77Vram);
}

void splash()
{
	//! Draw graphic, the left screen must be initialized. 
	decompress(splashBitmap, BG_GFX, LZ77Vram);
}

int main(void)
{
	defaultExceptionHandler();

	initScreenLeft();
	
	consoleDemoInit();
	iprintf("dslibris 1.3.\n");

	if(!fatInitDefault())
	{
		iprintf("fatal: no filesystem.\n");
	//	spin();
	}

//	strcpy(reopenName,"");
	app = new App();
	return app->Run();
}

u8 GetParserFace()
{
	if (parseFontItalic)
		return TEXT_STYLE_ITALIC;
	else if (parseFontBold)
		return TEXT_STYLE_BOLD;
	else
		return TEXT_STYLE_NORMAL;
}

void prefs_start_hndl(	void *userdata,
						const XML_Char *name,
						const XML_Char **attr)
{
	vector<Book*> *data = (vector<Book*>*)userdata;
	int position = 0; //! Page position in book.
	char filename[MAXPATHLEN];
	bool current = FALSE;
	int i;
 
	char msg[256];
	sprintf(msg,"info : found <%s>\n",name);
	app->Log(msg);
	
	if (!stricmp(name,"library"))
	{
		for(i=0;attr[i];i+=2) {
			if(!strcmp(attr[i],"folder"))
				app->bookdir = std::string(attr[i+1]);
		}
	}
	else if (!stricmp(name,"screen"))
	{
		for(i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"brightness"))
			{
				app->brightness = atoi(attr[i+1]);
				app->brightness = app->brightness % 4;
			}
			else if(!strcmp(attr[i],"invert"))
			{
				app->ts->SetInvert(atoi(attr[i+1]));
			}
			else if(!strcmp(attr[i],"clockwise"))
			{
				app->orientation = atoi(attr[i+1]);
			}
		}
	}
	else if (!stricmp(name,"paragraph"))
	{
		for(i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"spacing")) app->paraspacing = atoi(attr[i+1]);
			if(!strcmp(attr[i],"indent")) app->paraindent = atoi(attr[i+1]);
		}
	}
	else if (!stricmp(name,"font"))
	{
		for(i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"size"))
				app->ts->pixelsize = atoi(attr[i+1]);
			else if(!strcmp(attr[i],"normal"))
				app->ts->SetFontFile((char *)attr[i+1], TEXT_STYLE_NORMAL);
			else if(!strcmp(attr[i],"bold"))
				app->ts->SetFontFile((char *)attr[i+1], TEXT_STYLE_BOLD);
			else if(!strcmp(attr[i],"italic"))
				app->ts->SetFontFile((char *)attr[i+1], TEXT_STYLE_ITALIC);
			else if (!strcmp(attr[i], "path")) {
				if (strlen(attr[i+1]))
					app->fontdir = string(attr[i+1]);
			}
		}
	}
	else if (!stricmp(name, "books"))
	{
		for (i = 0; attr[i]; i+=2) {
			if (!strcmp(attr[i], "reopen"))
				// For prefs where reopen was a string, reopen will get turned off.
				app->reopen = atoi(attr[i+1]);
			else if (!strcmp(attr[i], "path")) {
				if (strlen(attr[i+1]))
					app->bookdir = string(attr[i+1]);
			}
        }
	}
	else if (!stricmp(name, "book"))
	{
		strcpy(filename,"");
		current = FALSE;

		for (i = 0; attr[i]; i+=2) {
			if (!strcmp(attr[i], "file"))
				strcpy(filename, attr[i+1]);
			if (!strcmp(attr[i], "page"))
				position = atoi(attr[i+1]);
			if (!strcmp(attr[i], "current"))
			{
				// Should warn if multiple books are current...
				// the last current book will win.
				if(atoi(attr[i+1])) current = TRUE;
			}
        }
		
		// Find the book index for this library entry
		// and set context for later bookmarks.
		for(i = 0; i < app->bookcount; i++)
		{
			const char *bookname = (*data)[i]->GetFileName();
			if(!stricmp(bookname, filename))
			{
				char msg[128];
				sprintf(msg,"info : matched extant book '%s'.\n",bookname);
				app->Log(msg);
				
				book = i;	// bookmark tags will refer to this.
				if (current)
				{
					// Set this book as current.
					app->bookcurrent = i;
					app->bookselected = i;
				}
				if (position)
					// Set current page in this book.
					(*data)[i]->SetPosition(position - 1);

				break;
			}
		}
	}
	else if (!stricmp(name, "bookmark"))
	{
		for (i = 0; attr[i]; i+=2) {
			if (!strcmp(attr[i], "page"))
				position = atoi(attr[i+1]);
        }
		
		if (book >= 0)
		{
			app->books[book]->GetBookmarks()->push_back(position - 1);
		}
	}
	else if (!stricmp(name,"margin"))
	{
		for (i=0;attr[i];i+=2)
		{
			if (!strcmp(attr[i],"left")) app->marginleft = atoi(attr[i+1]);
			if (!strcmp(attr[i],"right")) app->marginright = atoi(attr[i+1]);
			if (!strcmp(attr[i],"top")) app->margintop = atoi(attr[i+1]);
			if (!strcmp(attr[i],"bottom")) app->marginbottom = atoi(attr[i+1]);
		}
	}
}

void prefs_end_hndl(void *data, const char *name)
{
	if (!stricmp(name,"book")) book = -1;
}

int unknown_hndl(void *encodingHandlerData,
                 const XML_Char *name,
                 XML_Encoding *info)
{
	return(XML_STATUS_ERROR);
}

void default_hndl(void *data, const XML_Char *s, int len)
{
	parsedata_t *p = (parsedata_t *)data;
	if (s[0] == '&')
	{
		page_t *page = &(app->pages[app->pagecurrent]);

		/** handle only common iso-8859-1 character codes. */
		if (!strnicmp(s,"&nbsp;",5))
		{
			app->pagebuf[page->length++] = ' ';
			p->pen.x += app->ts->GetAdvance(' ', GetParserFace());
			
			return;
		}

		/** if it's decimal, convert the UTF-16 to UTF-8. */
		int code=0;
		sscanf(s,"&#%d;",&code);
		if (code)
		{
			if (code>=128 && code<=2047)
			{
				app->pagebuf[page->length++] = 192 + (code/64);
				app->pagebuf[page->length++] = 128 + (code%64);
			}
			else if (code>=2048 && code<=65535)
			{
				app->pagebuf[page->length++] = 224 + (code/4096);
				app->pagebuf[page->length++] = 128 + ((code/64)%64);
				app->pagebuf[page->length++] = 128 + (code%64);
			}
			// TODO - support 4-byte codes
			
			p->pen.x += app->ts->GetAdvance(code, GetParserFace());
		}
	}
}  /* End default_hndl */

static char title[32];

void title_start_hndl(void *userdata, const char *el, const char **attr)
{
	if(!stricmp(el,"title"))
	{
		app->parse_push((parsedata_t *)userdata,TAG_TITLE);
		strcpy(title,"");
	}
}

void title_char_hndl(void *userdata, const char *txt, int txtlen)
{
	if(strlen(title) > 27) return;
	if (!app->parse_in((parsedata_t*)userdata,TAG_TITLE)) return;

	for(u8 t=0;t<txtlen;t++)
	{
		if(iswhitespace(txt[t])) 
		{
			if(strlen(title)) strncat(title," ",1);
		}
		else strncat(title,txt+t,1);

		if (strlen(title) > 27)
		{
			strcat(title+27, "...");
			break;
		}
	}
}

void title_end_hndl(void *userdata, const char *el)
{
	parsedata_t *data = (parsedata_t*)userdata;
	if(!stricmp(el,"title")) data->book->SetTitle(title);
	app->parse_pop(data);	
}

void start_hndl(void *data, const char *el, const char **attr)
{
	parsedata_t *pdata = (parsedata_t*)data;
	if (!stricmp(el,"html")) app->parse_push(pdata,TAG_HTML);
	else if (!stricmp(el,"body")) app->parse_push(pdata,TAG_BODY);
	else if (!stricmp(el,"div")) app->parse_push(pdata,TAG_DIV);
	else if (!stricmp(el,"dt")) app->parse_push(pdata,TAG_DT);
	else if (!stricmp(el,"h1")) app->parse_push(pdata,TAG_H1);
	else if (!stricmp(el,"h2")) app->parse_push(pdata,TAG_H2);
	else if (!stricmp(el,"h3")) app->parse_push(pdata,TAG_H3);
	else if (!stricmp(el,"h4")) app->parse_push(pdata,TAG_H4);
	else if (!stricmp(el,"h5")) app->parse_push(pdata,TAG_H5);
	else if (!stricmp(el,"h6")) app->parse_push(pdata,TAG_H6);
	else if (!stricmp(el,"head")) app->parse_push(pdata,TAG_HEAD);
	else if (!stricmp(el,"ol")) app->parse_push(pdata,TAG_OL);
	else if (!stricmp(el,"p")) app->parse_push(pdata,TAG_P);
	else if (!stricmp(el,"pre")) app->parse_push(pdata,TAG_PRE);
	else if (!stricmp(el,"script")) app->parse_push(pdata,TAG_SCRIPT);
	else if (!stricmp(el,"style")) app->parse_push(pdata,TAG_STYLE);
	else if (!stricmp(el,"title")) app->parse_push(pdata,TAG_TITLE);
	else if (!stricmp(el,"td")) app->parse_push(pdata,TAG_TD);
	else if (!stricmp(el,"ul")) app->parse_push(pdata,TAG_UL);
	else if (!stricmp(el,"strong") || !stricmp(el, "b")) {
		page_t *page = &(app->pages[app->pagecurrent]);
		app->parse_push(pdata,TAG_STRONG);
		app->pagebuf[page->length] = TEXT_BOLD_ON;
		page->length++;
		parseFontBold = true;
	}
	else if (!stricmp(el,"em") || !stricmp(el, "i")) {
		page_t *page = &(app->pages[app->pagecurrent]);
		app->parse_push(pdata,TAG_EM);
		app->pagebuf[page->length] = TEXT_ITALIC_ON;
		page->length++;
		parseFontItalic = true;
	}
	else app->parse_push(pdata,TAG_UNKNOWN);
}  /* End of start_hndl */


void char_hndl(void *data, const XML_Char *txt, int txtlen)
{
	/** reflow text on the fly, into page data structure. **/

	parsedata_t *pdata = (parsedata_t *)data;
	if (app->parse_in(pdata,TAG_TITLE)) return;	
	if (app->parse_in(pdata,TAG_SCRIPT)) return;
	if (app->parse_in(pdata,TAG_STYLE)) return;
	if (app->pagecount == MAXPAGES) return;

	page_t *page = &(app->pages[app->pagecurrent]);
	if (page->length == 0)
	{
		/** starting a new page. **/
		pdata->pen.x = app->marginleft;
		pdata->pen.y = app->margintop + app->ts->GetHeight();
		linebegan = false;
	}

	u8 advance=0;
	int i=0;
	while (i<txtlen)
	{
		if (txt[i] == '\r')
		{
			i++;
			continue;
		}

		if (iswhitespace(txt[i]))
		{
			if(app->parse_in(pdata,TAG_PRE))
			{
				app->pagebuf[page->length++] = txt[i];
				if(txt[i] == '\n')
				{
					pdata->pen.x = app->marginleft;
					pdata->pen.y += (app->ts->GetHeight() + app->linespacing);
				}
				else {
					pdata->pen.x += app->ts->GetAdvance((u16)' ', GetParserFace());
				}
			}
			else if(linebegan && page->length
				&& !iswhitespace(app->pagebuf[page->length-1]))
			{
				app->pagebuf[page->length++] = ' ';
				pdata->pen.x += app->ts->GetAdvance((u16)' ', GetParserFace());	
			}
			i++;
		}
		else
		{
			linebegan = true;
			int j;
			advance = 0;
			u8 bytes = 1;
			for (j=i;(j<txtlen) && (!iswhitespace(txt[j]));j+=bytes)
			{

				/** set type until the end of the next word.
				    account for UTF-8 characters when advancing. **/
				u32 code;
				if (txt[j] > 127)
					bytes = app->ts->GetCharCode((char*)&(txt[j]),&code);
				else
				{
					code = txt[j];
					bytes = 1;
				}

				advance += app->ts->GetAdvance(code, GetParserFace());
					
				if(advance > PAGE_WIDTH-MARGINRIGHT-MARGINLEFT)
				{
					// here's a line-long word, need to break it now.
					break;
				}
			}

			// reflow - if we overrun the margin, insert a break.

			if ((pdata->pen.x + advance) > (PAGE_WIDTH-app->marginright))
			{
				app->pagebuf[page->length++] = '\n';
				pdata->pen.x = app->marginleft;
				pdata->pen.y += (app->ts->GetHeight() + app->linespacing);

				if (pdata->pen.y > (PAGE_HEIGHT-app->marginbottom))
				{
					if (app->parse_pagefeed(pdata,page))
					{
						page++;
						app->page_init(page);
						app->pagecurrent++;
						app->pagecount++;
						if (app->pagecount == MAXPAGES)
							return;
						
						if (parseFontItalic)
							app->pagebuf[page->length++] = TEXT_ITALIC_ON;
						if (parseFontBold)
							app->pagebuf[page->length++] = TEXT_BOLD_ON;
					}
				}
				linebegan = false;
			}

			/** append this word to the page. to save space,
			chars will stay UTF-8 until they are rendered. **/

			for (;i<j;i++)
			{
				if (iswhitespace(txt[i]))
				{
					if (linebegan)
					{
						app->pagebuf[page->length] = ' ';
						page->length++;
					}
				}
				else
				{
					linebegan = true;
					app->pagebuf[page->length] = txt[i];
					page->length++;
				}
			}
			pdata->pen.x += advance;
		}
	}
}  /* End char_hndl */

void end_hndl(void *data, const char *el)
{
	page_t *page = &(app->pages[app->pagecurrent]);
	parsedata_t *p = (parsedata_t *)data;
	if (
	    !stricmp(el,"br")
	    || !stricmp(el,"div")
	    || !stricmp(el,"dt")
	    || !stricmp(el,"h1")
	    || !stricmp(el,"h2")
	    || !stricmp(el,"h3")
	    || !stricmp(el,"h4")
	    || !stricmp(el,"h5")
	    || !stricmp(el,"h6")
	    || !stricmp(el,"hr")
	    || !stricmp(el,"li")
	    || !stricmp(el,"p")
	    || !stricmp(el,"pre")
	    || !stricmp(el,"ol")
	    || !stricmp(el,"td")
	    || !stricmp(el,"ul")
	)
	{
		if(linebegan) {
			linebegan = false;
			app->pagebuf[page->length] = '\n';
			page->length++;
			p->pen.x = MARGINLEFT;
			p->pen.y += app->ts->GetHeight() + app->linespacing;
			if (!stricmp(el,"p"))
			{
				for(int i=0;i<app->paraspacing;i++)
				{
					app->pagebuf[page->length++] = '\n';
					p->pen.x = MARGINLEFT;
					p->pen.y += app->ts->GetHeight() + app->linespacing;
				}
				for(int i=0;i<app->paraindent;i++)
				{
					app->pagebuf[page->length++] = ' ';
					p->pen.x += app->ts->GetAdvance(' ', GetParserFace());
				}
			}
			else if (	
				   !strcmp(el,"h1")
				|| !strcmp(el,"h2")
				|| !strcmp(el,"h3")
				|| !strcmp(el,"h4")
				|| !strcmp(el,"h5")
				|| !strcmp(el,"h6")
				|| !strcmp(el,"hr")
				|| !strcmp(el,"pre")
			)
			{
				app->pagebuf[page->length] = '\n';
				page->length++;
				p->pen.x = app->marginleft;
				p->pen.y += app->ts->GetHeight() + app->linespacing;
			}
			if (p->pen.y > (PAGE_HEIGHT-app->marginbottom))
			{
				if (app->ts->GetScreen() == app->screenright)
				{
					app->ts->SetScreen(app->screenleft);
					if (!page->buf)
						page->buf = (u8*)new u8[page->length];
					strncpy((char*)page->buf,(char *)app->pagebuf,page->length);
					page++;
					app->page_init(page);
					app->pagecurrent++;
					app->pagecount++;
					if(app->pagecount == MAXPAGES) return;
				}
				else
				{
					app->ts->SetScreen(app->screenright);
				}
				p->pen.x = app->marginleft;
				p->pen.y = app->margintop + app->ts->GetHeight();
			}
		}
	} else if (!stricmp(el,"body")) {
		if (!page->buf)
		{
			page->buf = new u8[page->length];
			if (!page->buf)
				app->ts->PrintStatusMessage("error: no memory to store page.\n");
		}
		strncpy((char*)page->buf,(char*)app->pagebuf,page->length);
		app->parse_pop(p);
	} else if (!stricmp(el, "strong") || !stricmp(el, "b")) {
		app->pagebuf[page->length] = TEXT_BOLD_OFF;
		page->length++;
		parseFontBold = false;
	} else if (!stricmp(el, "em") || !stricmp(el, "i")) {
		app->pagebuf[page->length] = TEXT_ITALIC_OFF;
		page->length++;
		parseFontItalic = false;
	}

	app->parse_pop(p);


}  /* End of end_hndl */

void proc_hndl(void *data, const char *target, const char *pidata)
{
}  /* End proc_hndl */
