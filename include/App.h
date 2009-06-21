/*
 Copyright (C) 2007-2009 Ray Haleblian
 
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
 
 To contact the copyright holder: rayh23@sourceforge.net
 */

#ifndef APP_H
#define APP_H

/*!
\mainpage

Welcome to the documentation for dslibris, a book reader for Nintendo DS.

\section Prerequisites

Fedora, Ubuntu, Arch, OS X, and Windows XP have all been used as build platforms; currently we're on Ubuntu 8.04 and Arch Linux. Have
  - devkitPro v20 (circa 10/23/2007) installed, including
	devkitARM,
	libnds,
	libfat,
	libwifi,
	and masscat's DS wifi debug stub library.
  - On Windows XP, MSYS/MINGW. The MSYS provided with devkitPro is fine.
  - Optionally, desmume 0.7.3 or better and Insight, if you want to debug with gdb (see Debugging).
  - A media card and a DLDI patcher, but you knew that.

Set DEVKITPRO and DEVKITARM in your environment.

\section Building

\code
cd ndslibris/trunk  # or wherever you put the SVN trunk
make
\endcode

dslibris.nds should show up in your current directory.
 
Note the libraries in 'external' are required prebuilts for arm-eabi; make sure you don't have conflicting libs in your path.

\section Installation

see INSTALL.txt.

\section Debugging

gdb and insight-6.6 have been known to work for debugging. See online forums for means to build an arm-eabi targeted Insight for your platform.

\section Homepage

http://sourceforge.net/projects/ndslibris

\author ray haleblian

*/


#include <nds.h>
#include <expat.h>
#include <unistd.h>
#include <DSGUI/BGUI.h>
#include <DSGUI/BImage.h>
#include <DSGUI/BScreen.h>
#include <DSGUI/BProgressBar.h>

#include "Book.h"
#include "Button.h"
#include "Prefs.h"
#include "Text.h"

#include "main.h"
#include "parse.h"

#include <vector>
#include <list>

#define APP_BROWSER_BUTTON_COUNT 6
#define APP_LOGFILE "dslibris.log"
#define APP_MODE_BOOK 0
#define APP_MODE_BROWSER 1
#define APP_MODE_PREFS 2
#define APP_MODE_PREFS_FONT 3
#define APP_MODE_PREFS_FONT_BOLD 4
#define APP_MODE_PREFS_FONT_ITALIC 5
#define APP_URL "http://sourceforge.net/projects/ndslibris"

#define PREFS_BUTTON_COUNT 7
#define PREFS_BUTTON_BOOKS 0
#define PREFS_BUTTON_FONTS 1
#define PREFS_BUTTON_FONT 2
#define PREFS_BUTTON_FONT_ITALIC 3
#define PREFS_BUTTON_FONT_BOLD 4
#define PREFS_BUTTON_FONTSIZE 5
#define PREFS_BUTTON_PARASPACING 6


//! \brief Main application.
//!
//!	\detail Top-level singleton class that handles application initialization,
//! interaction loop, drawing everything but text, and logging.

class App {
	private:
	void InitScreens();
	void SetBrightness(int b);
	void SetOrientation(bool flip);
	void WifiInit();
	bool WifiConnect();
	void Fatal(const char *msg);

	public:
	Text *ts;
	Prefs myprefs;   //?
	Prefs *prefs;    //?
	u8 brightness;   //! 4 levels for the Lite.
	u8 mode; 	     //! Are we in book or browser mode?
	string fontdir;  //! Default location to search for TTFs.
	bool console;    //! Can we print to console at the moment?
	
	//! key functions are remappable to support screen flipping.
	struct {
		u16 up,down,left,right,l,r,a,b,x,y,start,select;
	} key;
	
	vector<Button*> buttons;
	Button buttonprev, buttonnext, buttonprefs; //! Buttons on browser bottom.
	//! index into book vector denoting first book visible on library screen. 
	u8 browserstart; 
	string bookdir;  //! Search here for XHTML.
	vector<Book*> books;
	u8 bookcount;
	//! which book is currently selected in browser? -1=none.
	Book* bookselected;
	//! which book is currently being read? -1=none.
	Book* bookcurrent;
	//! reopen book from last session on startup?
	bool reopen;
	//! user data block passed to expat callbacks.
	parsedata_t parsedata;
	//! not used yet; will contain pagination indices for caching.
	vector<u16> pageindices;
	u8 orientation;
	u8 paraspacing, paraindent;
	
	Button prefsButtonBooks;
	Button prefsButtonFonts;
	Button prefsButtonFont;
	Button prefsButtonFontBold;
	Button prefsButtonFontItalic;
	Button prefsButtonFontSize;
	Button prefsButtonParaspacing;
	Button* prefsButtons[PREFS_BUTTON_COUNT];
	u8 prefsSelected;
	
	u8 fontSelected;
	Button* fontButtons;
	vector<Text*> fontTs;
	u8 fontPage;

	BImage *image0;
	BScreen *bscreen0;
	BProgressBar *progressbar;

	App();
	~App();
	
	//! in App.cpp
	void CycleBrightness();
	void PrintStatus(const char *msg);
	void PrintStatus(string msg);
	void Flip();
	void SetProgress(int amount);
	void UpdateClock();
	void Log(const char*);
	void Log(std::string);
	void Log(const char* format, const char *msg);
	int  Run(void);
	bool parse_in(parsedata_t *data, context_t context);
	void parse_init(parsedata_t *data);
	context_t parse_pop(parsedata_t *data);
	void parse_error(XML_ParserStruct *ps);
	void parse_push(parsedata_t *data, context_t context);

	//! in App_Browser.cpp
	void HandleEventInBrowser();
	void browser_init(void);
	void browser_draw(void);
	void browser_nextpage(void);
	void browser_prevpage(void);
	void browser_redraw(void);
	
	//! in App_Book.cpp
	void HandleEventInBook();
	int  GetBookIndex(Book*);
	void AttemptBookOpen();
	u8   OpenBook(void);
	
	//! in App_Prefs.cpps
	void HandleEventInPrefs();
	void PrefsInit();
	void PrefsDraw();
	void PrefsDraw(bool redraw);
	void PrefsButton();
	void PrefsIncreasePixelSize();
	void PrefsDecreasePixelSize();
	void PrefsIncreaseParaspacing();
	void PrefsDecreaseParaspacing();
	void PrefsRefreshButtonBooks();
	void PrefsRefreshButtonFonts();
	void PrefsRefreshButtonFont();
	void PrefsRefreshButtonFontBold();
	void PrefsRefreshButtonFontItalic();
	void PrefsRefreshButtonFontSize();
	void PrefsRefreshButtonParaspacing();
	
	//! in App_Font.cpp
	void HandleEventInFont();
	void FontInit();
	void FontDeinit();
	void FontDraw();
	void FontDraw(bool redraw);
	void FontNextPage();
	void FontPreviousPage();
	void FontButton();
};

#endif

