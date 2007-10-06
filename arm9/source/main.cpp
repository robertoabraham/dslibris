/** ---------------------------------------------------------------------------
dslibris - an ebook reader for Nintendo DS
<gpl />
--------------------------------------------------------------------------- **/

#include <nds.h>

#include <expat.h>
#include "types.h"
#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"
#include "main.h"
#include "parse.h"
#include "wifi.h"

App *app;
bool linebegan = false;

/*---------------------------------------------------------------------------*/

int main(void)
{
	app = new App();
	return app->main();
}

void prefs_start_hndl(	void *userdata,
						const XML_Char *name,
						const XML_Char **attr)
{
	Book *data = (Book*)userdata;
	char filename[64];
	strcpy(filename,"");
	u16 position = 0;
	if (!stricmp(name,"font"))
	{
		for(int i=0;attr[i];i+=2)
		{
			if(!strcmp(attr[i],"size"))
				app->ts->SetPixelSize(atoi(attr[i+1]));
		}
	}
	if (!stricmp(name,"bookmark") || !stricmp(name,"book"))
	{
		u8 i;
		for (i=0;attr[i];i+=2)
		{
			if (!strcmp(attr[i],"file")) strcpy(filename, attr[i+1]);
			if (!strcmp(attr[i],"position")) position = atoi(attr[i+1]);
			if (!strcmp(attr[i],"page")) position = atoi(attr[i+1]);
		}
		for(i=0; i<app->bookcount; i++)
		{
			if(!stricmp(data[i].GetFilename(),filename))
			{
				if(position) data[i].SetPosition(position-1);
				if(!stricmp(name,"book")) app->bookcurrent = i;
				break;
			}
		}
	}
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
			p->pen.x += app->ts->Advance(' ');
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

			if (code>=2048 && code<=65535)
			{
				app->pagebuf[page->length++] = 224 + (code/4096);
				app->pagebuf[page->length++] = 128 + ((code/64)%64);
				app->pagebuf[page->length++] = 128 + (code%64);
			}

			p->pen.x += app->ts->Advance(code);
		}
	}
}  /* End default_hndl */

void start_hndl(void *data, const char *el, const char **attr)
{
	parsedata_t *pdata = (parsedata_t*)data;
	if (!stricmp(el,"html")) app->parse_push(pdata,HTML);
	if (!stricmp(el,"body")) app->parse_push(pdata,BODY);
	if (!stricmp(el,"title")) app->parse_push(pdata,TITLE);
	if (!stricmp(el,"head")) app->parse_push(pdata,HEAD);
	if (!stricmp(el,"pre")) app->parse_push(pdata,PRE);
}  /* End of start_hndl */

void title_hndl(void *userdata, const char *txt, int txtlen)
{
	parsedata_t *data = (parsedata_t*)userdata;
	char title[32];
	if (app->parse_in(data,TITLE))
	{
		if (txtlen > 30)
		{
			strncpy(title,txt,27);
			strcpy(title+27, "...");
		}
		else
		{
			strncpy(title,txt,txtlen);
			title[txtlen] = 0;
		}
		data->book->SetTitle(title);
	}
}

void char_hndl(void *data, const XML_Char *txt, int txtlen)
{
	/** flow text on the fly, into page data structure. **/

	parsedata_t *pdata = (parsedata_t *)data;
	if (!app->parse_in(pdata,BODY)) return;

	int i=0;
	u8 advance=0;
	u8 linelength=PAGE_WIDTH-MARGINRIGHT;
	static u8 breakchar=0;

	page_t *page = &(app->pages[app->pagecurrent]);
	if (page->length == 0)
	{
		/** starting a new page. **/
		breakchar = 0;
		pdata->pen.x = MARGINLEFT;
		pdata->pen.y = MARGINTOP + app->ts->GetHeight();
		linebegan = false;
	}

	while (i<txtlen)
	{
		if (txt[i] == '\r')
		{
			i++;
			continue;
		}

		if (iswhitespace(txt[i]))
		{

			// TODO check this <PRE> code, i don't trust it
			if (app->parse_in(pdata,PRE) && txt[i] == '\n')
			{
				app->pagebuf[page->length++] = txt[i];
				pdata->pen.x += app->ts->Advance((u16)txt[i]);
				pdata->pen.y += (app->ts->GetHeight() + LINESPACING);
				if (pdata->pen.y > (PAGE_HEIGHT-MARGINBOTTOM))
				{
					if (app->parse_pagefeed(pdata,page))
					{
						page++;
						app->page_init(page);
						app->pagecurrent++;
						app->pagecount++;
					}
					linebegan = false;
				}

			}
			else
			{
				if (linebegan)
				{
					breakchar = page->length;
					app->pagebuf[page->length++] = ' ';
					pdata->pen.x += app->ts->Advance((u16)' ');
				}

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
				u16 code;
				if (txt[j] > 127)
					bytes = app->ts->GetUCS((char*)&(txt[j]),&code);
				else
				{
					code = txt[j];
					bytes = 1;
				}
				advance += app->ts->Advance(code);
			}

			/** reflow. if we overrun the margin, 
				insert a break. **/

			int overrun = (pdata->pen.x + advance) - linelength;

			if (overrun > 0)
			{
				app->pagebuf[page->length] = '\n';
				page->length++;
				pdata->pen.x = MARGINLEFT;
				pdata->pen.y += (app->ts->GetHeight() + LINESPACING);

				if (pdata->pen.y > (PAGE_HEIGHT-MARGINBOTTOM))
				{
					if (app->parse_pagefeed(pdata,page))
					{
						page++;
						app->page_init(page);
						app->pagecurrent++;
						app->pagecount++;
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
	    || !stricmp(el,"p")
	    || !stricmp(el,"h1")
	    || !stricmp(el,"h2")
	    || !stricmp(el,"h3")
	    || !stricmp(el,"h4")
	    || !stricmp(el,"hr")
	)
	{
		app->pagebuf[page->length] = '\n';
		page->length++;
		p->pen.x = MARGINLEFT;
		p->pen.y += app->ts->GetHeight() + LINESPACING;
		if ( !stricmp(el,"p"))
		{
			app->pagebuf[page->length] = '\n';
			page->length++;
			p->pen.y += app->ts->GetHeight() + LINESPACING;
		}
		if (p->pen.y > (PAGE_HEIGHT-MARGINBOTTOM))
		{
			if (app->fb == app->screen1)
			{
				app->fb = app->screen0;
				if (!page->buf)
					page->buf = (u8*)new u8[page->length];
				strncpy((char*)page->buf,(char *)app->pagebuf,page->length);
				page++;
				app->page_init(page);
				app->pagecurrent++;
				app->pagecount++;
			}
			else
			{
				app->fb = app->screen1;
			}
			p->pen.x = MARGINLEFT;
			p->pen.y = MARGINTOP + app->ts->GetHeight();
		}
		linebegan = false;
	}
	if (!stricmp(el,"body"))
	{
		if (!page->buf)
		{
			page->buf = new u8[page->length];
			if (!page->buf) app->ts->PrintString("[memory error]\n");
		}
		strncpy((char*)page->buf,(char*)app->pagebuf,page->length);
		app->parse_pop(p);
	}
	if (!(stricmp(el,"title") && stricmp(el,"head")
	        && stricmp(el,"pre") && stricmp(el,"html")))
	{
		app->parse_pop(p);
	}

}  /* End of end_hndl */

void proc_hndl(void *data, const char *target, const char *pidata)
{
}  /* End proc_hndl */

