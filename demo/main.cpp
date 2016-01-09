
#include <iostream>
#include <SDL.h>
#include "WPL.h"

#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "WPL.lib")

using namespace std;

int main(int argc, char * argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_bool exit = SDL_FALSE;	
	SDL_Window * window = SDL_CreateWindow("demo",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800, 500,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
	);

	SDL_GL_CreateContext(window);

	WPL_Video * video = WPL_OpenVideo("demo.wmv");

	if (video == NULL)
	{
		cout << WPL_GetError(video) << endl;
	}

	while(!exit)
	{
		SDL_Event event;

		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_QUIT)
			{
				exit = SDL_TRUE;
				break;
			}

			if(event.key.type == SDL_KEYUP)
			{
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
