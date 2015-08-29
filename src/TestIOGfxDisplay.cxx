/**
 * Test graphics display

 * Copyright (C) 2015  Sylvain Beucler

 * This file is part of GNU FreeDink

 * GNU FreeDink is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.

 * GNU FreeDink is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public
 * License along with GNU FreeDink.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cxxtest/TestSuite.h>

#include "IOGfxDisplaySW.h"
#include "IOGfxDisplayGL2.h"
#include "IOGfxSurfaceSW.h"
#include "SDL_image.h"
#include "freedink_xpm.h"
#include "log.h"
#include "ImageLoader.h" /* GFX_ref_pal */ // TODO: break dep
#include "gfx_palette.h"

/**
 * Test graphics output
 * Shortcomings:
 * - Buggy driver init
 * - SDL_Renderer drops all ops when window is hidden; use a visible window
 * - Android display is 16-bit, so use fuzzy color comparisons
 * - Stretched display (fullscreen and Android) use linear interpolation, so
 *   check fuzzy pixel positions after flipStretch()
 * - No OpenGL ES 2.0 API to retrieve texture contents, only retrieve main buffer
 */
class TestIOGfxDisplay : public CxxTest::TestSuite {
public:
	IOGfxDisplay* display;
	struct {
		bool gl;
		bool truecolor;
	} lastDisplay;

	SDL_Color white;
	SDL_Color black;
	SDL_Color green;
	SDL_Color blue;



	TestIOGfxDisplay() {
		TS_ASSERT_EQUALS(SDL_InitSubSystem(SDL_INIT_VIDEO), 0);
		// SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);

		display = NULL;
		lastDisplay.gl = false;
		lastDisplay.truecolor = false;

		white = {255, 255, 255, 255};
		black = {  0,   0,   0, 255};
		green = {255, 255,   0, 255};
		blue  = {  0,   0, 255, 255};
	}
	~TestIOGfxDisplay() {
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}



	void setUp() {
		// init palette; hopefully can be removed if we get rid of blitFormat
		for (int i = 0; i < 256; i++) {
			GFX_ref_pal[i].r = i;
			GFX_ref_pal[i].g = i;
			GFX_ref_pal[i].b = i;
			GFX_ref_pal[i].a = 255;
		}
		// Use a color that fail tests if RGBA->ABGR-reversed (non-symmetric)
		// Fully saturated green to avoid fuzzy color comparisons
		GFX_ref_pal[1].r = 255;
		GFX_ref_pal[1].g = 255;
		GFX_ref_pal[1].b = 0;
		GFX_ref_pal[1].a = 255;
		GFX_ref_pal[2].r = 0;
		GFX_ref_pal[2].g = 0;
		GFX_ref_pal[2].b = 255;
		GFX_ref_pal[2].a = 255;
		gfx_palette_set_phys(GFX_ref_pal);
	}
	void tearDown() {
	}



	void openDisplay(bool gl, bool truecolor, Uint32 flags) {
		//log_info("* Requesting %s %s", gl?"GL2":"SW", truecolor?"truecolor":"");
		//flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		//flags &= ~SDL_WINDOW_HIDDEN;

		// cache
		if (display && lastDisplay.gl == gl && lastDisplay.truecolor == truecolor) {
			display->clear();
			return;
		}
		if (display) {
			display->close();
			delete display;
			display = NULL;
		}

		if (gl)
			display = new IOGfxDisplayGL2(50, 50, truecolor, flags);
		else
			display = new IOGfxDisplaySW(50, 50, truecolor, flags);
		bool opened = display->open();
		TS_ASSERT_EQUALS(opened, true);

		lastDisplay.gl = gl;
		lastDisplay.truecolor = truecolor;
	}
	void closeDisplay() {
	}

	void getColorAt(SDL_Surface* img, int x, int y, SDL_Color* c) {
		SDL_GetRGBA(((Uint32*)img->pixels)[x + y*img->w],
						img->format,
						&c->r, &c->g, &c->b, &c->a);
	}



	void ctestSplash() {
		// A first inter-texture blit before anything else
		// Tests IOGfxDisplayGL2->androidWorkAround()
		SDL_Surface* image;
		IOGfxSurface *backbuffer, *splash;

		backbuffer = display->alloc(50, 50);
		//g->flip(backbuffer); // not a single flip

		image = SDL_CreateRGBSurface(0, 40, 40, 8, 0, 0, 0, 0);
		Uint8* pixels = (Uint8*)image->pixels;
		SDL_SetPaletteColors(image->format->palette, GFX_ref_pal, 0, 256);
		// Make a bit square to cope with stretching and color interpolation
		pixels[0] = 1;
		pixels[1] = 1;
		pixels[2] = 1;
		pixels[image->pitch+0] = 1;
		pixels[image->pitch+1] = 1;
		pixels[image->pitch+2] = 1;
		pixels[image->pitch*2+0] = 1;
		pixels[image->pitch*2+1] = 1;
		pixels[image->pitch*2+0] = 1;
		splash = display->upload(image);
		//g->flip(splash); // not a single flip

		SDL_Rect dstrect = {0, 0, -1, -1};
		// without workaround, the blit is ignored/delayed
		backbuffer->blit(splash, NULL, &dstrect);

		// Check the result:
		display->flipStretch(backbuffer);

		SDL_Surface* screenshot = display->screenshot();
		TS_ASSERT(screenshot != NULL);
		if (screenshot == NULL)
			return;
		Uint8 cr, cg, cb, ca;
		int x = 1, y = 1;
		display->surfToDisplayCoords(backbuffer, x, y);
		SDL_GetRGBA(((Uint32*)screenshot->pixels)[x+y*screenshot->w],
					screenshot->format,
					&cr, &cg, &cb, &ca);
		TS_ASSERT_EQUALS(cr, 255);
		TS_ASSERT_EQUALS(cg, 255);
		TS_ASSERT_EQUALS(cb, 0);
		TS_ASSERT_EQUALS(ca, 255);
		//SDL_SaveBMP(screenshot, "1testSplash.bmp");
		SDL_FreeSurface(screenshot);

		delete splash;
		delete backbuffer;
	}



	void ctest_alloc() {
		IOGfxSurface* surf = display->alloc(300, 300);
		TS_ASSERT(surf != NULL);
		TS_ASSERT_EQUALS(surf->w, 300);
		TS_ASSERT_EQUALS(surf->h, 300);
		delete surf;
	}



	void ctest_screenshot() {
		SDL_Surface* img;
		IOGfxSurface *backbuffer, *surf;
		SDL_Rect bbbox;

		backbuffer = display->alloc(50, 50);
		bbbox = { 0,0, 50,50 };

		img = SDL_CreateRGBSurface(0, 40, 40, 8, 0, 0, 0, 0);
		Uint8* pixels = (Uint8*)img->pixels;
		SDL_SetPaletteColors(img->format->palette, GFX_ref_pal, 0, 256);
		pixels[0] = 255;
		pixels[1] = 1;
		surf = display->upload(img);
		backbuffer->blit(surf, NULL, NULL);
		display->flipRaw(backbuffer);

		// Check that pic is not vertically flipped
		SDL_Surface* screenshot = display->screenshot(&bbbox);
		TS_ASSERT(screenshot != NULL);
		if (screenshot == NULL)
			return;
		Uint8 cr, cg, cb, ca;

		SDL_GetRGBA(((Uint32*)screenshot->pixels)[0],
					screenshot->format,
					&cr, &cg, &cb, &ca);
		TS_ASSERT_EQUALS(cr, 255);
		TS_ASSERT_EQUALS(cb, 255);
		TS_ASSERT_EQUALS(cg, 255);
		TS_ASSERT_EQUALS(ca, 255);

		SDL_GetRGBA(((Uint32*)screenshot->pixels)[1],
					screenshot->format,
					&cr, &cg, &cb, &ca);
		TS_ASSERT_EQUALS(cr, 255);
		TS_ASSERT_EQUALS(cg, 255);
		TS_ASSERT_EQUALS(cb, 0);
		TS_ASSERT_EQUALS(ca, 255);

		SDL_FreeSurface(screenshot);

		delete surf;
		delete backbuffer;
	}



	void ctest_surfToDisplayCoords() {
		SDL_Surface* img;
		IOGfxSurface *surf;
		int x, y;

		img = SDL_CreateRGBSurface(0, 50, 50, 8, 0, 0, 0, 0);
		surf = display->upload(img);

		// Force display to 50x50 even if the system gave us something else
		display->onSizeChange(50, 50);

		x = 0; y = 0;
		display->surfToDisplayCoords(surf, x, y);
		TS_ASSERT_EQUALS(x, 0);
		TS_ASSERT_EQUALS(y, 0);

		x = 11; y = 12;
		display->surfToDisplayCoords(surf, x, y);
		TS_ASSERT_EQUALS(x, 11);
		TS_ASSERT_EQUALS(y, 12);

		display->onSizeChange(800, 480);

		x = 0; y = 0;
		display->surfToDisplayCoords(surf, x, y);
		TS_ASSERT_EQUALS(x, 164);
		TS_ASSERT_EQUALS(y, 4);

		x = 11; y = 12;
		display->surfToDisplayCoords(surf, x, y);
		TS_ASSERT_EQUALS(x, 270);
		TS_ASSERT_EQUALS(y, 120);

		display->onSizeChange(50, 50);

		delete surf;
	}


	void ctest_blit() {
		SDL_Surface *img, *screenshot;
		IOGfxSurface *backbuffer, *surf;
		SDL_Color cs;
		SDL_Rect bbbox;

		backbuffer = display->alloc(50, 50);
		bbbox = { 0,0, 50,50 };

		img = SDL_CreateRGBSurface(0, 5, 5, 8, 0, 0, 0, 0);
		Uint8* pixels = (Uint8*)img->pixels;
		SDL_SetPaletteColors(img->format->palette, GFX_ref_pal, 0, 256);
		SDL_SetColorKey(img, SDL_TRUE, 0);
		pixels[0] = 255;
		pixels[1] = 1;
		pixels[img->pitch] = 255;
		pixels[img->pitch+1] = 1;
		surf = display->upload(img);

		SDL_Rect dstrect = {0, 0, -1, -1};

		dstrect.x = 0; dstrect.y = 0;
		backbuffer->blit(surf, NULL, &dstrect);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&white, &cs, sizeof(SDL_Color));
		getColorAt(screenshot, 1, 0, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		dstrect.x = 20; dstrect.y = 20;
		backbuffer->blit(surf, NULL, &dstrect);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 20, 20, &cs);
		TS_ASSERT_SAME_DATA(&white, &cs, sizeof(SDL_Color));
		getColorAt(screenshot, 21, 20, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		dstrect.x = -1; dstrect.y = -1;
		backbuffer->blit(surf, NULL, &dstrect);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		dstrect.x = 49; dstrect.y = 49;
		backbuffer->blit(surf, NULL, &dstrect);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 49, 49, &cs);
		TS_ASSERT_SAME_DATA(&white, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);


		dstrect.x = 0; dstrect.y = 0;
		backbuffer->blit(surf, NULL, &dstrect);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&white, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);
		dstrect.x = -2; dstrect.y = -2;
		backbuffer->blit(surf, NULL, &dstrect);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&white, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);
		dstrect.x = -2; dstrect.y = -2;
		backbuffer->blitNoColorKey(surf, NULL, &dstrect);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&black, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		dstrect.x = 0; dstrect.y = 0;
		dstrect.w = 10; dstrect.h = 20;
		backbuffer->blitStretch(surf, NULL, &dstrect);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 7, &cs);
		TS_ASSERT_SAME_DATA(&white, &cs, sizeof(SDL_Color));
		getColorAt(screenshot, 3, 7, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		SDL_Rect srcrect = {1,1, 1,1};
		dstrect.x = 0; dstrect.y = 0;
		backbuffer->blit(surf, &srcrect, &dstrect);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		delete surf;
		delete backbuffer;
	}



	void ctest_fillRect() {
		IOGfxSurface *backbuffer;
		SDL_Surface *screenshot;
		SDL_Color cs;
		SDL_Rect bbbox;

		backbuffer = display->alloc(50, 50);
		bbbox = { 0,0, 50,50 };

		SDL_Rect dstrect = {-1, -1, -1, -1};

		backbuffer->fillRect(NULL, 255, 255, 0);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		dstrect.x = 5;  dstrect.y = 5;
		dstrect.w = 20; dstrect.h = 10;
		backbuffer->fillRect(&dstrect, 0, 0, 255);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 4, 4, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		getColorAt(screenshot, 5, 5, &cs);
		TS_ASSERT_SAME_DATA(&blue, &cs, sizeof(SDL_Color));
		getColorAt(screenshot, 24, 14, &cs);
		TS_ASSERT_SAME_DATA(&blue, &cs, sizeof(SDL_Color));
		getColorAt(screenshot, 25, 14, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		getColorAt(screenshot, 24, 15, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		delete backbuffer;
	}

	void ctest_fill_screen() {
		IOGfxSurface *backbuffer;
		SDL_Surface *screenshot;
		SDL_Color cs;
		SDL_Rect bbbox;

		backbuffer = display->alloc(50, 50);
		bbbox = { 0,0, 50,50 };

		backbuffer->fill_screen(0, GFX_ref_pal);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&black, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		backbuffer->fill_screen(1, GFX_ref_pal);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&green, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		backbuffer->fill_screen(2, GFX_ref_pal);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&blue, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		// Using a /8 value to support RGB565 (Android) screens
		SDL_Color gray16 = {16,16,16,255};
		backbuffer->fill_screen(16, GFX_ref_pal);
		display->flipRaw(backbuffer);
		screenshot = display->screenshot(&bbbox);
		getColorAt(screenshot, 0, 0, &cs);
		TS_ASSERT_SAME_DATA(&gray16, &cs, sizeof(SDL_Color));
		SDL_FreeSurface(screenshot);

		delete backbuffer;
	}



	void test01SplashGL2Truecolor() {
		openDisplay(true, true, SDL_WINDOW_HIDDEN);
		ctestSplash();
		closeDisplay();
	}
	void test_screenshotGL2Truecolor() {
		openDisplay(true, true, SDL_WINDOW_HIDDEN);
		ctest_screenshot();
		closeDisplay();
	}
	void test_allocGL2Truecolor() {
		openDisplay(true, true, SDL_WINDOW_HIDDEN);
		ctest_alloc();
		closeDisplay();
	}
	void test_surfToDisplayCoordsGL2Truecolor() {
		openDisplay(true, true, SDL_WINDOW_HIDDEN);
		ctest_surfToDisplayCoords();
		closeDisplay();
	}
	void test_blitGL2Truecolor() {
		openDisplay(true, true, SDL_WINDOW_HIDDEN);
		ctest_blit();
		closeDisplay();
	}
	void test_fill_screenGL2Truecolor() {
		openDisplay(true, true, SDL_WINDOW_HIDDEN);
		ctest_fill_screen();
		closeDisplay();
	}
	void test_fillRectGL2Truecolor() {
		openDisplay(true, true, SDL_WINDOW_HIDDEN);
		ctest_fillRect();
		closeDisplay();
	}

//	void testSplashGL2() {
//		openDisplay(true, false, SDL_WINDOW_HIDDEN);
//		ctestSplash(); // TODO
//		closeDisplay();
//	}
//	void test_blitGL2() {
//		openDisplay(true, false, SDL_WINDOW_HIDDEN);
//		ctest_blit(); // TODO
//		closeDisplay();
//	}
//	void test_fillRectGL2() {
//		openDisplay(true, false, SDL_WINDOW_HIDDEN);
//		ctest_fillRect(); // TODO
//		closeDisplay();
//	}
//	void test_fill_screenGL2() {
//		openDisplay(true, false, SDL_WINDOW_HIDDEN);
//		ctest_fill_screen(); // TODO
//		closeDisplay();
//	}

	void testSplashSWTruecolor() {
		openDisplay(false, true, 0); // can't render offscreen >(
		ctestSplash();
		closeDisplay();
	}
	void test_screenshotSWTruecolor() {
		openDisplay(false, true, 0);
		ctest_screenshot();
		closeDisplay();
	}
	void test_allocSWTruecolor() {
		openDisplay(false, true, SDL_WINDOW_HIDDEN);
		ctest_alloc();
		closeDisplay();
	}
	void test_blitSWTruecolor() {
		openDisplay(false, true, 0);
		ctest_blit();
		closeDisplay();
	}
	void test_fillRectSWTruecolor() {
		openDisplay(false, true, 0);
		ctest_fillRect();
		closeDisplay();
	}
	void test_fill_screenSWTruecolor() {
		openDisplay(false, true, 0);
		ctest_fill_screen();
		closeDisplay();
	}

	void testSplashSW() {
		openDisplay(false, false, 0);
		ctestSplash();
		closeDisplay();
	}
	void test_blitSW() {
		openDisplay(false, false, 0);
		ctest_blit();
		closeDisplay();
	}
	void test_fillRectSW() {
		openDisplay(false, false, 0);
		ctest_fillRect();
		closeDisplay();
	}
	void test_fill_screenSW() {
		openDisplay(false, false, 0);
		ctest_fill_screen();
		closeDisplay();
	}
};