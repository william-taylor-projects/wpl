
#pragma once

#ifdef WIN32
    #ifdef WPL_API_EXPORT
        #define WPL_API __declspec(dllexport)
    #else
        #define WPL_API __declspec(dllimport)
    #endif

#include <windows.h>
#include <dshow.h>
#include <string>
#include <Evr.h>

#pragma comment(lib, "strmiids.lib")

namespace wpl {
    struct Version {
        unsigned int majorVersion;
        unsigned int minorVersion;
    };

    enum class PlaybackState { NoVideo, Playing, Paused, Stopped };

    class VideoRenderer 
    {
    public:
        virtual ~VideoRenderer() {};
        virtual bool addToGraph(IGraphBuilder * graph, HWND hwnd) = 0;
        virtual bool finalizeGraph(IGraphBuilder * graph) = 0;
        virtual bool updateVideoWindow(HWND hwnd, const LPRECT prc) = 0;
        virtual bool hasVideo() const = 0;
        virtual bool repaint() = 0;
    };

    class EVR : public VideoRenderer
    {
        IMFVideoDisplayControl * videoDisplay;
        IBaseFilter * evr;
    public:
        EVR();
        ~EVR();

        bool addToGraph(IGraphBuilder * graph, HWND hwnd) override;
        bool finalizeGraph(IGraphBuilder * graph) override;
        bool updateVideoWindow(HWND hwnd, const LPRECT prc) override;
        bool hasVideo() const override;
        bool repaint() override;
    };


    class WPL_API VideoPlayer {
        IGraphBuilder * graphBuilder;
        IMediaControl * mediaControl;
        IMediaEventEx * mediaEvents;
        IMediaSeeking * mediaSeeking;
        VideoRenderer * videoRenderer;
        PlaybackState state;
        HWND windowHandle;
    public:
        explicit VideoPlayer(HWND hwnd = nullptr);
        ~VideoPlayer();

        PlaybackState playbackState() const;

        bool openVideo(const std::string& filename);
        bool updateVideoWindow() const;
        bool repaint() const;
        bool pause();
        bool play();
        bool stop();

        bool hasFinished() const;
        bool hasVideo() const;
    private:
        bool setupGraph();
        bool createVideoRenderer();
        bool renderStreams(IBaseFilter *pSource);

        void releaseGraph();
    };
}

#endif