
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
#include <Mfidl.h>
#include <d3d9.h>
#include <Vmr9.h>
#include <Evr.h>
#include <new>

#pragma comment(lib, "strmiids.lib")

enum PlaybackState
{
	STATE_NO_GRAPH,
	STATE_RUNNING,
	STATE_PAUSED,
	STATE_STOPPED,
};

struct WPL_Video {
	IMFVideoDisplayControl * videoDisplayControl;
	IGraphBuilder * graphBuilder;
	IMediaControl * mediaControl;
	IMediaSeeking * mediaSeeking;
	IMediaEventEx * mediaEvents;
	IBaseFilter * evrBaseFilter;
	PlaybackState playbackState;
	std::wstring filename;
	HWND wndHwnd;
	BOOL show;
};

enum WPL_ERROR {
	FILE_NOT_FOUND = 0
};

struct WPL_Version {
	unsigned int majorVersion;
	unsigned int minorVersion;
};

WPL_API WPL_Video * WPL_OpenVideo(std::string filename);
WPL_API WPL_Version WPL_GetVersion();
WPL_API void WPL_PauseVideo(WPL_Video* video);
WPL_API void WPL_ExitVideo(WPL_Video** video);
WPL_API void WPL_StopVideo(WPL_Video* video);
WPL_API void WPL_PlayVideo(WPL_Video* video);
WPL_API void WPL_ShowVideo(WPL_Video* video);
WPL_API std::string WPL_GetError(WPL_Video* video);

template <class T> void SafeRelease(T **ppT) {
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}


class CVideoRenderer;

const UINT WM_GRAPH_EVENT = WM_APP + 1;

typedef void (CALLBACK *GraphEventFN)(HWND hwnd, long eventCode, LONG_PTR param1, LONG_PTR param2);

class DShowPlayer
{
public:
	DShowPlayer(HWND hwnd);
	~DShowPlayer();

	PlaybackState State() const { return m_state; }

	HRESULT OpenFile(PCWSTR pszFileName);

	HRESULT Play();
	HRESULT Pause();
	HRESULT Stop();

	BOOL    HasVideo() const;
	HRESULT UpdateVideoWindow(const LPRECT prc);
	HRESULT Repaint(HDC hdc);
	HRESULT DisplayModeChanged();

	HRESULT HandleGraphEvent(GraphEventFN pfnOnGraphEvent);

private:
	HRESULT InitializeGraph();
	void    TearDownGraph();
	HRESULT CreateVideoRenderer();
	HRESULT RenderStreams(IBaseFilter *pSource);

	PlaybackState   m_state;

	HWND m_hwnd; // Video window. This window also receives graph events.

	IGraphBuilder   *m_pGraph;
	IMediaControl   *m_pControl;
	IMediaEventEx   *m_pEvent;
	CVideoRenderer  *m_pVideo;
};


HRESULT RemoveUnconnectedRenderer(IGraphBuilder *pGraph, IBaseFilter *pRenderer, BOOL *pbRemoved);
HRESULT AddFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF, LPCWSTR wszName);

// Abstract class to manage the video renderer filter.
// Specific implementations handle the VMR-7, VMR-9, or EVR filter.
class CVideoRenderer
{
public:
	virtual ~CVideoRenderer() {};
	virtual BOOL    HasVideo() const = 0;
	virtual HRESULT AddToGraph(IGraphBuilder *pGraph, HWND hwnd) = 0;
	virtual HRESULT FinalizeGraph(IGraphBuilder *pGraph) = 0;
	virtual HRESULT UpdateVideoWindow(HWND hwnd, const LPRECT prc) = 0;
	virtual HRESULT Repaint(HWND hwnd, HDC hdc) = 0;
	virtual HRESULT DisplayModeChanged() = 0;
};

// Manages the VMR-7 video renderer filter.

class CVMR7 : public CVideoRenderer
{
	IVMRWindowlessControl   *m_pWindowless;
public:
	CVMR7();
	~CVMR7();
	BOOL    HasVideo() const;
	HRESULT AddToGraph(IGraphBuilder *pGraph, HWND hwnd);
	HRESULT FinalizeGraph(IGraphBuilder *pGraph);
	HRESULT UpdateVideoWindow(HWND hwnd, const LPRECT prc);
	HRESULT Repaint(HWND hwnd, HDC hdc);
	HRESULT DisplayModeChanged();
};


// Manages the VMR-9 video renderer filter.

class CVMR9 : public CVideoRenderer
{
	IVMRWindowlessControl9 *m_pWindowless;
public:
	CVMR9();
	~CVMR9();
	BOOL    HasVideo() const;
	HRESULT AddToGraph(IGraphBuilder *pGraph, HWND hwnd);
	HRESULT FinalizeGraph(IGraphBuilder *pGraph);
	HRESULT UpdateVideoWindow(HWND hwnd, const LPRECT prc);
	HRESULT Repaint(HWND hwnd, HDC hdc);
	HRESULT DisplayModeChanged();
};


// Manages the EVR video renderer filter.
class CEVR : public CVideoRenderer
{
	IBaseFilter            *m_pEVR;
	IMFVideoDisplayControl *m_pVideoDisplay;
public:
	CEVR();
	~CEVR();
	BOOL    HasVideo() const;
	HRESULT AddToGraph(IGraphBuilder *pGraph, HWND hwnd);
	HRESULT FinalizeGraph(IGraphBuilder *pGraph);
	HRESULT UpdateVideoWindow(HWND hwnd, const LPRECT prc);
	HRESULT Repaint(HWND hwnd, HDC hdc);
	HRESULT DisplayModeChanged();
};


#endif