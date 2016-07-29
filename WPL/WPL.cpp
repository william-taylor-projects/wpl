
#include <functional>
#include <fstream>
#include "WPL.h"

using bool_lambda = std::function<bool()>;
using void_lambda = std::function<void()>;

const auto EmptyFunction {void_lambda([](){})};
const auto MajorVersion {2};
const auto MinorVersion {2};

bool removeUnconnectedRenderer(IGraphBuilder *, IBaseFilter *);
bool addFilterByCLSID(IGraphBuilder *, REFGUID, IBaseFilter **, LPCWSTR);
bool findConnectedPin(IBaseFilter *, PIN_DIRECTION, IPin **);
bool initialiseEvr(IBaseFilter *, HWND, IMFVideoDisplayControl **);

template<typename T> 
void safeRelease(T ** comPtr) 
{
    if (comPtr != nullptr && *comPtr) 
    {
        (*comPtr)->Release();
        (*comPtr) = nullptr;
    }
}

template<typename T> 
void safeDelete(T ** savePointer) 
{
    if(savePointer != nullptr && savePointer) 
    {
        delete (*savePointer);
        (*savePointer) = nullptr;
    }
}

template<typename T>
bool fileExists(T filename) 
{
    return std::ifstream(filename).good();
}

bool async(bool_lambda start, void_lambda onfail = EmptyFunction, void_lambda cleanup = EmptyFunction)
{
    auto successful {start()};

    if(!successful) onfail();
    if(cleanup) cleanup();
   
    return successful;
}

bool removeUnconnectedRenderer(IGraphBuilder * graphBuilder, IBaseFilter * baseFilter, bool * removed)
{
    IPin * pinPointer { nullptr };
    auto result { findConnectedPin(baseFilter, PINDIR_INPUT, &pinPointer) };
    safeRelease(&pinPointer);
    
    if (FAILED(result)) 
    {
        result = SUCCEEDED(graphBuilder->RemoveFilter(baseFilter));
        (*removed) = true;
    }
    else
    {
        (*removed) = false;
    }

    return result;
}

bool isPinConnected(IPin * pinPointer, bool * resultPointer)
{
    IPin * tempPinPointer { nullptr };
    auto hr { pinPointer->ConnectedTo(&tempPinPointer) };
    
    if (SUCCEEDED(hr))  
    {
        *resultPointer = true;
    } 
    else if (hr == VFW_E_NOT_CONNECTED) 
    {
        *resultPointer = true;
        hr = S_OK;
    }

    safeRelease(&tempPinPointer);
    return SUCCEEDED(hr);
}

bool isPinDirection(IPin * pinPointer, PIN_DIRECTION dir, BOOL * result)
{
    PIN_DIRECTION pinDirection;
    auto hr { pinPointer->QueryDirection(&pinDirection) };

    if (SUCCEEDED(hr))
    {
        *result = pinDirection == dir;
    }

    return SUCCEEDED(hr);
}

bool findConnectedPin(IBaseFilter * filter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    *ppPin = nullptr;
    IEnumPins * pEnum = nullptr;
    IPin * pPin = nullptr;

    auto  hr = filter->EnumPins(&pEnum);

    if (FAILED(hr))
    {
        return SUCCEEDED(hr);
    }

    auto bFound = FALSE;

    while (S_OK == pEnum->Next(1, &pPin, nullptr))
    {
        bool isConnected;
        hr = isPinConnected(pPin, &isConnected);

        if (SUCCEEDED(hr)) 
        {
            if (isConnected) 
            {
                hr = isPinDirection(pPin, PinDir, &bFound);
            }
        }

        if (FAILED(hr)) 
        {
            pPin->Release();
            break;
        }

        if (bFound) 
        {
            *ppPin = pPin;
            break;
        }

        pPin->Release();
    }

    pEnum->Release();

    if (!bFound) 
    {
        hr = VFW_E_NOT_FOUND;
    }

    return SUCCEEDED(hr);
}

bool addFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF, LPCWSTR wszName)
{
    *ppF = nullptr;

    IBaseFilter * pFilter = nullptr;

    auto hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFilter));
    
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pGraph->AddFilter(pFilter, wszName);
    
    if (FAILED(hr))
    {
        goto done;
    }

    *ppF = pFilter;
    (*ppF)->AddRef();

done:
    safeRelease(&pFilter);
    return SUCCEEDED(hr);
}

bool removeUnconnectedRenderer(IGraphBuilder *pGraph, IBaseFilter *pRenderer)
{
    IPin * pinPointer = nullptr;

    HRESULT hr = findConnectedPin(pRenderer, PINDIR_INPUT, &pinPointer);

    pinPointer->Release();

    if (FAILED(hr))
    {
        hr = pGraph->RemoveFilter(pRenderer);
    }

    return SUCCEEDED(hr);
}

bool initialiseEvr(IBaseFilter *pEVR, HWND hwnd, IMFVideoDisplayControl** ppDisplayControl)
{
    IMFGetService *pGS = nullptr;
    IMFVideoDisplayControl *pDisplay = nullptr;

    auto hr = pEVR->QueryInterface(IID_PPV_ARGS(&pGS));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = pGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplay));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDisplay->SetVideoWindow(hwnd);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDisplay->SetAspectRatioMode(MFVideoARMode_None);

    if (FAILED(hr))
    {
        goto done;
    }

    *ppDisplayControl = pDisplay;
    (*ppDisplayControl)->AddRef();

done:
    safeRelease(&pGS);
    safeRelease(&pDisplay);
    return SUCCEEDED(hr);
}

using namespace wpl;

EVR::EVR() 
    : videoDisplay(nullptr), evr(nullptr)
{
}

EVR::~EVR()
{
    safeRelease(&evr);
    safeRelease(&videoDisplay);
}

bool EVR::hasVideo() const
{
    return videoDisplay != nullptr;
}

bool EVR::addToGraph(IGraphBuilder * graph, HWND hwnd)
{
    IBaseFilter *evrFilter { nullptr };
    auto hr { addFilterByCLSID(graph, CLSID_EnhancedVideoRenderer, &evrFilter, L"EVR") };
  
    const auto task = [&]() {
        if (FAILED(hr))
            return false;

        initialiseEvr(evrFilter, hwnd, &videoDisplay);

        if (FAILED(hr))
            return false;

        this->evr = evrFilter;
        this->evr->AddRef();

        return true;
    };

    return async(task, [&](){ safeRelease(&evrFilter); });
}

bool EVR::finalizeGraph(IGraphBuilder * graph)
{
    if (evr == nullptr) 
    {
        return true;
    }

    auto removed { false };
    auto hr { removeUnconnectedRenderer(graph, evr, &removed) };

    if (removed) 
    {
        safeRelease(&evr);
        safeRelease(&videoDisplay);
    }

    return(SUCCEEDED(hr));
}

bool EVR::updateVideoWindow(HWND hwnd, const LPRECT prc)
{
    if (videoDisplay == nullptr) 
    {
        return true; 
    }

    if (prc) 
    {
        return SUCCEEDED(videoDisplay->SetVideoPosition(nullptr, prc));
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    return SUCCEEDED(videoDisplay->SetVideoPosition(nullptr, &rc));
}

bool EVR::repaint()
{
    return SUCCEEDED(videoDisplay ? videoDisplay->RepaintVideo() : S_OK);
}

VideoPlayer::VideoPlayer(HWND hwnd)
  : graphBuilder(nullptr),
    mediaControl(nullptr),
    mediaEvents(nullptr),
    mediaSeeking(nullptr),
    videoRenderer(new EVR()),
    state(PlaybackState::NoVideo),
    windowHandle(hwnd)
{
}

VideoPlayer::~VideoPlayer()
{
    safeDelete(&videoRenderer);
    releaseGraph();
}

bool VideoPlayer::openVideo(const std::string& filename)
{
    if(!fileExists(filename)) 
    {
        return false;
    }

    IBaseFilter* source {nullptr};
    auto hr { setupGraph() };

    const auto tasks = [&]() {
        const auto wstr { std::wstring(filename.begin(), filename.end()) };
        hr = SUCCEEDED(graphBuilder->AddSourceFilter(wstr.c_str(), nullptr, &source));
        return !hr ? false : renderStreams(source);
    };

    return async(tasks, [&]() { releaseGraph(); }, [&]() { safeRelease(&source); });
}

bool VideoPlayer::play()
{
    if (state != PlaybackState::Paused && state != PlaybackState::Stopped)
    {
        return false;
    }

    updateVideoWindow();

    auto hr { mediaControl->Run() };

    if (SUCCEEDED(hr))
    {
        state = PlaybackState::Playing;
    }

    return state == PlaybackState::Playing;
}

bool VideoPlayer::pause()
{
    if (state != PlaybackState::Playing) 
    {
        return false;
    }

    auto hr { mediaControl->Pause() };
    
    if (SUCCEEDED(hr)) 
    {
        state = PlaybackState::Paused;
    }

    return state == PlaybackState::Paused;
}

bool VideoPlayer::stop()
{
    if (state != PlaybackState::Playing && state != PlaybackState::Paused) 
    {
        return false;
    }

    auto hr { mediaControl->Stop() };

    if (SUCCEEDED(hr))
    {
        state = PlaybackState::Stopped;
        
        auto stopTimes = 1LL;
        auto start = 1LL;

        mediaSeeking->GetPositions(nullptr, &stopTimes);
        mediaSeeking->SetPositions(&start, 0x01 | 0x04, &stopTimes, 0x01 | 0x04);
    }
    
    return state == PlaybackState::Stopped;
}

bool VideoPlayer::hasVideo() const
{
    return videoRenderer && videoRenderer->hasVideo();
}

bool VideoPlayer::hasFinished() const
{
    auto returnValue {false};

    if(mediaEvents == nullptr) 
    {
        return returnValue;
    }

    auto param1 {0L};
    auto param2 {0L};
    auto evCode {0L};

    while (SUCCEEDED(mediaEvents->GetEvent(&evCode, &param1, &param2, 0)))
    {
        auto hr = mediaEvents->FreeEventParams(evCode, param1, param2);
        
        if (FAILED(hr))
        {
            break;
        }

        if (evCode == EC_COMPLETE) 
        {
            returnValue = true;
            break;
        }
    }

    return returnValue;
}

bool VideoPlayer::updateVideoWindow() const
{
    RECT rc;
    GetClientRect(windowHandle, &rc);
    return SUCCEEDED(videoRenderer ? videoRenderer->updateVideoWindow(windowHandle, &rc) : S_OK);
}

bool VideoPlayer::repaint() const
{
    return SUCCEEDED(videoRenderer ? videoRenderer->repaint() : S_OK);
}

HRESULT VideoPlayer::queryInterface(HRESULT prevResult, const IID& riid, void ** pvObject) const
{
    return SUCCEEDED(prevResult) ? graphBuilder->QueryInterface(riid, pvObject) : E_FAIL;
}

bool VideoPlayer::setupGraph()
{
    releaseGraph();

    auto hr { CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&graphBuilder)) };

    if (FAILED(hr))
    {
        return SUCCEEDED(hr);
    }

    hr = queryInterface(hr, IID_PPV_ARGS(&mediaControl));
    hr = queryInterface(hr, IID_PPV_ARGS(&mediaEvents));
    hr = queryInterface(hr, IID_PPV_ARGS(&mediaSeeking));
    
    if (SUCCEEDED(hr))
    {
        state = PlaybackState::Stopped;
    }

    return SUCCEEDED(hr);
}

void VideoPlayer::releaseGraph()
{
    state = PlaybackState::NoVideo;

    safeRelease(&graphBuilder);
    safeRelease(&mediaControl);
    safeRelease(&mediaSeeking);
    safeRelease(&mediaEvents);
}

PlaybackState VideoPlayer::playbackState() const 
{
    return state;
}

bool VideoPlayer::createVideoRenderer() const
{
    auto hr { E_FAIL };

    if (videoRenderer == nullptr) 
    {
        hr = E_OUTOFMEMORY;
        return false;
    }

    return SUCCEEDED(videoRenderer->addToGraph(graphBuilder, windowHandle));
}

bool VideoPlayer::renderStreams(RenderStreamsParams * params) const
{
    if (FAILED(params->hr))
    {
        return false;
    }

    params->hr = createVideoRenderer();

    if (FAILED(params->hr))
    {
        return false;
    }

    params->hr = addFilterByCLSID(graphBuilder, CLSID_DSoundRender, &params->audioRenderer, L"Audio Renderer");

    if (FAILED(params->hr))
    {
        return false;
    }

    params->hr = params->source->EnumPins(&params->pins);   

    if (FAILED(params->hr))
    {
        return false;
    }

    IPin * pin { nullptr };

    while (S_OK == params->pins->Next(1, &pin, nullptr))
    {
        auto rendered = params->filterGraph2->RenderEx(pin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, nullptr);
       
        if (SUCCEEDED(rendered))
        {
            params->renderedAnyPin = TRUE;
        }

        pin->Release();
    }

    params->hr = videoRenderer->finalizeGraph(graphBuilder);

    if (FAILED(params->hr)) 
    {
        return false;
    }

    bool removed;
    params->hr = removeUnconnectedRenderer(graphBuilder, params->audioRenderer, &removed);
    return SUCCEEDED(params->hr);
}

bool VideoPlayer::renderStreams(IBaseFilter * source)
{
    IFilterGraph2 * filterGraph2 { nullptr };
    IBaseFilter * audioRenderer { nullptr };
    IEnumPins * enumPins { nullptr };
  
    auto renderedAnyPin { false };
    auto hr { graphBuilder->QueryInterface(IID_PPV_ARGS(&filterGraph2)) };

    auto cleanup = [&]() {
        safeRelease(&enumPins);
        safeRelease(&audioRenderer);
        safeRelease(&filterGraph2);
    };

    auto task = [&]() {
        RenderStreamsParams params = {
            filterGraph2, audioRenderer, source, 
            enumPins, hr, renderedAnyPin
        };

        return renderStreams(&params);
    };

    return async(task, cleanup);
}

Version getVersion()
{
    Version v;
    v.majorVersion = MajorVersion;
    v.minorVersion = MinorVersion;
    return v;
}