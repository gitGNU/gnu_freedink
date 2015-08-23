/**
 * Test OpenGL screen

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

#include "IOGfxDisplayGL2.h"
#include "IOGfxGLFuncs.h"

class TestIOGraphics : public CxxTest::TestSuite {
public:
	void setUp() {
		TS_ASSERT_EQUALS(SDL_InitSubSystem(SDL_INIT_VIDEO), 0);
	}
	void tearDown() {
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}

	void test_graphics() {
		IOGfxDisplay* g = new IOGfxDisplayGL2(800, 600, SDL_WINDOW_HIDDEN);
		TS_ASSERT_EQUALS(g->open(), true);

		g->clear();
		SDL_GL_SwapWindow(g->window);

		SDL_Surface* screenshot = dynamic_cast<IOGfxDisplayGL2*>(g)->screenshot();
		Uint8 cr, cg, cb, ca;
		SDL_GetRGBA(((Uint32*)screenshot->pixels)[0],
					screenshot->format,
					&cr, &cg, &cb, &ca);
		TS_ASSERT_EQUALS(cr, 0);
		TS_ASSERT_EQUALS(cb, 255);
		TS_ASSERT_EQUALS(cg, 0);
		TS_ASSERT_EQUALS(ca, 255);
		
		SDL_FreeSurface(screenshot);
		g->close();
	}
};