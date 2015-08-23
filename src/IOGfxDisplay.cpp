#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "IOGfxDisplay.h"

#include "SDL_image.h"

#include "log.h"
#include "freedink_xpm.h"

IOGfxDisplay::IOGfxDisplay(int w, int h, Uint32 flags)
	: w(w), h(h), flags(flags), window(NULL) {
}

IOGfxDisplay::~IOGfxDisplay() {
	close();  // non-virtual call
}

bool IOGfxDisplay::open() {
	if (!createWindow()) return false;
	logWindowInfo();

	return true;
}

void IOGfxDisplay::close() {
	if (window) SDL_DestroyWindow(window);
	window = NULL;
}

bool IOGfxDisplay::createWindow() {
	window = SDL_CreateWindow(PACKAGE_STRING,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		w, h, flags);
	if (window == NULL) {
		log_error("Unable to create %dx%d window: %s\n",
		          w, h, SDL_GetError());
		return false;
	}

	/* Window configuration */
	{
		SDL_Surface *icon = NULL;
		if ((icon = IMG_ReadXPMFromArray(freedink_xpm)) == NULL) {
			log_error("Error loading icon: %s", IMG_GetError());
		} else {
			SDL_SetWindowIcon(window, icon);
			SDL_FreeSurface(icon);
		}
	}
	return true;
}

void IOGfxDisplay::logWindowInfo() {
	log_info("Video driver: %s", SDL_GetCurrentVideoDriver());
	log_info("Video fall-back surface (unused): %s",
	         SDL_GetPixelFormatName(SDL_GetWindowPixelFormat(window)));
	if (0) {
		// Segfaults on quit:
		//SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "opengl");
		SDL_Surface* window_surface = SDL_GetWindowSurface(window);
		log_info("Video fall-back surface (unused): %s",
				SDL_GetPixelFormatName(window_surface->format->format));
	}
}

void IOGfxDisplay::toggleFullScreen() {
	SDL_SetWindowFullscreen(window,
			(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)
			? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
	// Note: software renderer is buggy in SDL 2.0.2: it doesn't resize the surface
}

void IOGfxDisplay::toggleScreenKeyboard() {
	if (SDL_HasScreenKeyboardSupport()) {
		if (!SDL_IsScreenKeyboardShown(window))
			SDL_StartTextInput();
		else
			SDL_StopTextInput();
	}
}