
#include "../WPL/WPL.h"
#include "SDL2/SDL.h"
#include <iostream>

#pragma comment(lib, "SDL2/SDL2main.lib")
#pragma comment(lib, "SDL2/SDL2.lib")
#pragma comment(lib, "WPL.lib")

using namespace std;

int main(int argc, char * argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	auto window = SDL_CreateWindow("Demo", 100, 100, 800, 500, SDL_WINDOW_SHOWN);
	auto video = WPL_OpenVideo("demo.wmv");
	auto exit = SDL_FALSE;

	if (video == nullptr) {
		cout << WPL_GetError(video) << endl;
	}

	while(!exit) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				exit = SDL_TRUE;
				break;
			}

			if(event.key.type == SDL_KEYUP) {
				switch (event.key.keysym.sym) {
					case SDLK_RIGHT: WPL_PlayVideo(video); break;
					case SDLK_LEFT: WPL_PauseVideo(video); break;
					case SDLK_DOWN: WPL_StopVideo(video); break;

					default: break;
				}
			}
		}

		WPL_ShowVideo(video);
	}

	WPL_ExitVideo(&video);

	SDL_DestroyWindow(window);
	SDL_Quit();
	return(0);
}
