
#include "CppUnitTest.h"
#include "Tests.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WPLTests
{
    TEST_CLASS(StateTests)
    {
    public:
        TEST_METHOD(PlayTest)
        {
            SDL_Init(SDL_INIT_VIDEO);
            SDL_Window * window = SDL_CreateWindow("Playback Demo", 100, 100, 800, 500, SDL_WINDOW_SHOWN);

            SDL_SysWMinfo wmInfo;
            SDL_VERSION(&wmInfo.version);
            SDL_GetWindowWMInfo(window, &wmInfo);

            wpl::VideoPlayer videoPlayer(wmInfo.info.win.window);

            Assert::IsTrue(videoPlayer.openVideo("demo.wmv"), L"Error didnt load file");
            Assert::IsTrue(videoPlayer.play(), L"Error couldnt play file");

            SDL_DestroyWindow(window);
            SDL_Quit();
        }

        TEST_METHOD(PauseTest)
        {
            SDL_Init(SDL_INIT_VIDEO);
            SDL_Window * window = SDL_CreateWindow("", 100, 100, 800, 500, SDL_WINDOW_SHOWN);

            SDL_SysWMinfo wmInfo;
            SDL_VERSION(&wmInfo.version);
            SDL_GetWindowWMInfo(window, &wmInfo);

            wpl::VideoPlayer videoPlayer(wmInfo.info.win.window);

            Assert::IsTrue(videoPlayer.openVideo("demo.wmv"), L"Error didnt load file");
            Assert::IsTrue(videoPlayer.play(), L"Error couldnt play file");
            Assert::IsTrue(videoPlayer.pause(), L"Error couldnt pause file");
            
            SDL_DestroyWindow(window);
            SDL_Quit();
        }

        TEST_METHOD(Stoptest)
        {
            SDL_Init(SDL_INIT_VIDEO);
            SDL_Window * window = SDL_CreateWindow("", 100, 100, 800, 500, SDL_WINDOW_SHOWN);

            SDL_SysWMinfo wmInfo;
            SDL_VERSION(&wmInfo.version);
            SDL_GetWindowWMInfo(window, &wmInfo);

            wpl::VideoPlayer videoPlayer(wmInfo.info.win.window);

            Assert::IsTrue(videoPlayer.openVideo("demo.wmv"), L"Error didnt load file");
            Assert::IsTrue(videoPlayer.play(), L"Error couldnt play file");
            Assert::IsTrue(videoPlayer.stop(), L"Error couldnt stop file");

            SDL_DestroyWindow(window);
            SDL_Quit();
        }

        TEST_METHOD(FinishedTest)
        {
            SDL_Init(SDL_INIT_VIDEO);
            SDL_Window * window = SDL_CreateWindow("", 100, 100, 800, 500, SDL_WINDOW_SHOWN);

            SDL_SysWMinfo wmInfo;
            SDL_VERSION(&wmInfo.version);
            SDL_GetWindowWMInfo(window, &wmInfo);

            wpl::VideoPlayer videoPlayer(wmInfo.info.win.window);

            Assert::IsTrue(videoPlayer.openVideo("demo.wmv"), L"Error didnt load file");
            Assert::IsTrue(videoPlayer.play(), L"Error couldnt play file");

            SDL_AddTimer(PLAYBACK_TIMEOUT, timeout, nullptr);

            auto quit = false;

            while (!quit) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        Assert::Fail();
                        quit = true;
                    }
                }

                if (videoPlayer.hasFinished())
                    quit = true;
            }

            SDL_DestroyWindow(window);
            SDL_Quit();
        }
    };
}