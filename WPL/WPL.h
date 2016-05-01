
#pragma once

#ifdef WIN32
	#ifdef WPL_API_EXPORT
		#define WPL_API __declspec(dllexport)
	#else
		#define WPL_API __declspec(dllimport)
	#endif

#include <windows.h>
#include <dshow.h>
#include <Evr.h>

#pragma comment(lib, "strmiids.lib")

namespace wpl {
	using uint = unsigned int;

	struct Version {
		uint majorVersion;
		uint minorVersion;
	};

	enum class PlaybackState { NoVideo, Playing, Paused, Stopped };

	class VideoRenderer 
	{
	public:
		virtual ~VideoRenderer() {};
		virtual BOOL hasVideo() const = 0;
		virtual HRESULT addToGraph(IGraphBuilder *pGraph, HWND hwnd) = 0;
		virtual HRESULT finalizeGraph(IGraphBuilder *pGraph) = 0;
		virtual HRESULT updateVideoWindow(HWND hwnd, const LPRECT prc) = 0;
		virtual HRESULT repaint() = 0;
	};

	class EVR : public VideoRenderer
	{
		IMFVideoDisplayControl * videoDisplay;
		IBaseFilter * evr;
	public:
		EVR();
		~EVR();

		HRESULT addToGraph(IGraphBuilder *pGraph, HWND hwnd) override;
		HRESULT finalizeGraph(IGraphBuilder *pGraph) override;
		HRESULT updateVideoWindow(HWND hwnd, const LPRECT prc) override;
		HRESULT repaint() override;

		BOOL hasVideo() const override;
	};


	class WPL_API VideoPlayer {
		IGraphBuilder * m_pGraph;
		IMediaControl * m_pControl;
		IMediaEventEx * m_pEvent;
		VideoRenderer * videoRenderer;
		PlaybackState state;
		HWND windowHandle;
	public:
		explicit VideoPlayer(HWND hwnd = nullptr);
		~VideoPlayer();

		PlaybackState playbackState() const;

		HRESULT openVideo(PCWSTR pszFileName);
		HRESULT updateVideoWindow() const;
		HRESULT repaint() const;
		HRESULT pause();
		HRESULT play();
		HRESULT stop();

		BOOL hasVideo() const;
	private:
		HRESULT setupGraph();
		HRESULT createVideoRenderer();
		HRESULT renderStreams(IBaseFilter *pSource);

		void releaseGraph();
	};
}

#endif