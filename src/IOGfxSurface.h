#ifndef IOGFXSURFACE_H
#define IOGFXSURFACE_H

#include "SDL.h"
#include "rect.h"

class IOGfxSurface {
public:
	virtual ~IOGfxSurface();
	virtual void fill(int num, SDL_Color* palette) = 0;
	virtual void vlineRGB(Sint16 x, Sint16 y1, Sint16 y2, Uint8 r, Uint8 g, Uint8 b) = 0;
	virtual void hlineRGB(Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b) = 0;
	virtual void drawBox(rect box, int color) = 0;
};

#endif
