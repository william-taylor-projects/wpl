
#include "../WPL/WPL.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#pragma comment(lib, "SDL2/SDL2main.lib")
#pragma comment(lib, "SDL2/SDL2.lib")
#pragma comment(lib, "WPL.lib")

using namespace std;
using namespace wpl;

int main(int argc, char * argv[])
{
	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window * window = SDL_CreateWindow("Playback Demo", 100, 100, 800, 500, flags);
	SDL_bool exit = SDL_FALSE;

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);

	VideoPlayer videoPlayer(wmInfo.info.win.window);
	videoPlayer.openVideo(L"demo.wmv");
	videoPlayer.play();

	while(!exit) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				exit = SDL_TRUE;
				break;
			}

			if(event.key.type == SDL_KEYUP) {
				switch (event.key.keysym.sym) {
					case SDLK_RIGHT: videoPlayer.play(); break;
					case SDLK_LEFT: videoPlayer.pause(); break;
					case SDLK_DOWN: videoPlayer.stop(); break;

					default: break;
				}
			}
		}

		videoPlayer.updateVideoWindow();
		videoPlayer.repaint();
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
