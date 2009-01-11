#include <errno.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include <expat.h>

#include <fat.h>

#include "main.h"
#include "parse.h"
#include "App.h"
#include "Book.h"
#include "Button.h"
#include "Text.h"

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

void App::PrefsInit()
{	
	prefsButtonBooks.Init(ts);
	prefsButtonBooks.Move(2, (PREFS_BUTTON_BOOKS + 1) * 32 - 16);
	PrefsRefreshButtonBooks();
	prefsButtons[PREFS_BUTTON_BOOKS] = &prefsButtonBooks;
	
	prefsButtonFonts.Init(ts);
	prefsButtonFonts.Move(2, (PREFS_BUTTON_FONTS + 1) * 32 - 16);
	PrefsRefreshButtonFonts();
	prefsButtons[PREFS_BUTTON_FONTS] = &prefsButtonFonts;
	
	prefsButtonFont.Init(ts);
	prefsButtonFont.Move(2, (PREFS_BUTTON_FONT + 1) * 32 - 16);
	PrefsRefreshButtonFont();
	prefsButtons[PREFS_BUTTON_FONT] = &prefsButtonFont;
	
	prefsButtonFontBold.Init(ts);
	prefsButtonFontBold.Move(2, (PREFS_BUTTON_FONT_BOLD + 1) * 32 - 16);
	PrefsRefreshButtonFontBold();
	prefsButtons[PREFS_BUTTON_FONT_BOLD] = &prefsButtonFontBold;
		
	prefsButtonFontItalic.Init(ts);
	prefsButtonFontItalic.Move(2, (PREFS_BUTTON_FONT_ITALIC + 1) * 32 - 16);
	PrefsRefreshButtonFontItalic();
	prefsButtons[PREFS_BUTTON_FONT_ITALIC] = &prefsButtonFontItalic;
	
	prefsButtonFontSize.Init(ts);
	prefsButtonFontSize.Move(2, (PREFS_BUTTON_FONTSIZE + 1) * 32 - 16);
	PrefsRefreshButtonFontSize();
	prefsButtons[PREFS_BUTTON_FONTSIZE] = &prefsButtonFontSize;
	
	prefsButtonParaspacing.Init(ts);
	prefsButtonParaspacing.Move(2, (PREFS_BUTTON_PARASPACING + 1) * 32 - 16);
	PrefsRefreshButtonParaspacing();
	prefsButtons[PREFS_BUTTON_PARASPACING] = &prefsButtonParaspacing;
	
	prefsSelected = 0;
}

void App::PrefsRefreshButtonBooks()
{
	sprintf((char*)msg, "Book Folder\n    %s", bookdir.c_str());
	prefsButtonBooks.Label(msg);
}

void App::PrefsRefreshButtonFonts()
{
	sprintf((char*)msg, "Font Folder\n    %s", fontdir.c_str());
	prefsButtonFonts.Label(msg);
}

void App::PrefsRefreshButtonFont()
{
	sprintf((char*)msg, "Change Font\n    %s", ts->GetFontFile(TEXT_STYLE_NORMAL).c_str());
	prefsButtonFont.Label(msg);
}

void App::PrefsRefreshButtonFontBold()
{
	sprintf((char*)msg, "Change Bold Font\n    %s", ts->GetFontFile(TEXT_STYLE_BOLD).c_str());
	prefsButtonFontBold.Label(msg);
}

void App::PrefsRefreshButtonFontItalic()
{
	sprintf((char*)msg, "Change Italic Font\n    %s", ts->GetFontFile(TEXT_STYLE_ITALIC).c_str());
	prefsButtonFontItalic.Label(msg);
}

void App::PrefsRefreshButtonFontSize()
{
	if (ts->GetPixelSize() == 1)
		sprintf((char*)msg, "Change Font Size\n    [ %d >", ts->GetPixelSize());
	else if (ts->GetPixelSize() == 255)
		sprintf((char*)msg, "Change Font Size\n    < %d ]", ts->GetPixelSize());
	else
		sprintf((char*)msg, "Change Font Size\n    < %d >", ts->GetPixelSize());
	prefsButtonFontSize.Label(msg);
}

void App::PrefsRefreshButtonParaspacing()
{
	if (paraspacing == 0)
		sprintf((char*)msg, "Change Paragraph Spacing\n    [ %d >", paraspacing);
	else if (paraspacing == 255)
		sprintf((char*)msg, "Change Paragraph Spacing\n    < %d ]", paraspacing);
	else
		sprintf((char*)msg, "Change Paragraph Spacing\n    < %d >", paraspacing);
	prefsButtonParaspacing.Label(msg);
}

void App::PrefsDraw()
{
	PrefsDraw(true);
}

void App::PrefsDraw(bool redraw)
{
	// save state.
	bool invert = ts->GetInvert();
	u8 size = ts->GetPixelSize();
	u16* screen = ts->GetScreen();

	ts->SetScreen(screenright);
	ts->SetInvert(false);
	if (redraw) ts->ClearScreen();
	ts->SetPixelSize(PIXELSIZE);
	for (u8 i = 0; i < PREFS_BUTTON_COUNT; i++)
	{
		prefsButtons[i]->Draw(screen, i == prefsSelected);
	}
	
	buttonprefs.Label("return");
	buttonprefs.Draw(screen, false);

	// restore state.
	ts->SetInvert(invert);
	ts->SetPixelSize(size);
	ts->SetScreen(screen);
}

void App::HandleEventInPrefs()
{
	if (keysDown() & (KEY_START | KEY_SELECT | KEY_B)) {
		mode = APP_MODE_BROWSER;
		browser_draw();
	} else if (prefsSelected > 0 && (keysDown() & (KEY_RIGHT | KEY_R))) {
		prefsSelected--;
		PrefsDraw(false);
	} else if (prefsSelected < PREFS_BUTTON_COUNT - 1 && (keysDown() & (KEY_LEFT | KEY_L))) {
		prefsSelected++;
		PrefsDraw(false);
	} else if (keysDown() & KEY_A) {
		PrefsButton();
	} else if (keysDown() & KEY_TOUCH) {
		touchPosition touch;
		touchRead(&touch);
		touchPosition coord;
		u8 regionprev[2];
		regionprev[0] = 0;
		regionprev[1] = 16;

		if(!orientation)
		{
			coord.px = 256 - touch.px;
			coord.py = 192 - touch.py;
		} else {
			coord.px = touch.px;
			coord.py = touch.py;
		}
		
		if (buttonprefs.EnclosesPoint(coord.py, coord.px)) {
			buttonprefs.Label("prefs");
			mode = APP_MODE_BROWSER;
			browser_draw();
		} else {
			for(u8 i = 0; i < PREFS_BUTTON_COUNT; i++) {
				if (prefsButtons[i]->EnclosesPoint(coord.py, coord.px))
				{
					if (i != prefsSelected) {
						prefsSelected = i;
						PrefsDraw(false);
					}
					
					if (i == PREFS_BUTTON_FONTSIZE) {
						if (coord.py < 2 + 188 / 2) {
							PrefsIncreasePixelSize();
						} else {
							PrefsDecreasePixelSize();
						}
					} else if (i == PREFS_BUTTON_PARASPACING) {
						if (coord.py < 2 + 188 / 2) {
							PrefsIncreaseParaspacing();
						} else {
							PrefsDecreaseParaspacing();
						}
					} else {
						PrefsButton();
					}
					
					break;
				}
			}
		}
	} else if (prefsSelected == PREFS_BUTTON_FONTSIZE && (keysDown() & KEY_UP)) {
		PrefsDecreasePixelSize();
	} else if (prefsSelected == PREFS_BUTTON_FONTSIZE && (keysDown() & KEY_DOWN)) {
		PrefsIncreasePixelSize();
	} else if (prefsSelected == PREFS_BUTTON_PARASPACING && (keysDown() & KEY_UP)) {
		PrefsDecreaseParaspacing();
	} else if (prefsSelected == PREFS_BUTTON_PARASPACING && (keysDown() & KEY_DOWN)) {
		PrefsIncreaseParaspacing();
	}
}

void App::PrefsIncreasePixelSize()
{
	if (ts->pixelsize < 255) {
		ts->pixelsize++;
		PrefsRefreshButtonFontSize();
		PrefsDraw();
		bookcurrent = -1;
		prefs->Write();
	}
}

void App::PrefsDecreasePixelSize()
{
	if (ts->pixelsize > 1) {
		ts->pixelsize--;
		PrefsRefreshButtonFontSize();
		PrefsDraw();
		bookcurrent = -1;
		prefs->Write();
	}
}

void App::PrefsIncreaseParaspacing()
{
	if (paraspacing < 255) {
		paraspacing++;
		PrefsRefreshButtonParaspacing();
		PrefsDraw();
		bookcurrent = -1;
		prefs->Write();
	}
}

void App::PrefsDecreaseParaspacing()
{
	if (paraspacing > 0) {
		paraspacing--;
		PrefsRefreshButtonParaspacing();
		PrefsDraw();
		bookcurrent = -1;
		prefs->Write();
	}
}

void App::PrefsButton()
{
	if (prefsSelected != PREFS_BUTTON_BOOKS)
	{ 
		if(prefsSelected == PREFS_BUTTON_FONTS) {
		} else if (prefsSelected == PREFS_BUTTON_FONT) {
			mode = APP_MODE_PREFS_FONT;
		} else if (prefsSelected == PREFS_BUTTON_FONT_BOLD) {
			mode = APP_MODE_PREFS_FONT_BOLD;
		} else if (prefsSelected == PREFS_BUTTON_FONT_ITALIC) {
			mode = APP_MODE_PREFS_FONT_ITALIC;
		}
/*		ts->SetScreen(screenright);
		ts->ClearScreen();
		ts->SetPen(marginleft,PAGE_HEIGHT/2);
		ts->PrintString("[loading fonts...]");
*/
		PrintStatus("[loading fonts...]");
		FontInit();
		FontDraw();
		PrintStatus("");
	}
}
