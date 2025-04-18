// proxydll.cpp
#include "stdafx.h"
#include "proxydll.h"

// global variables
#pragma data_seg (".d3d9_shared")
myIDirect3DSwapChain9*  gl_pmyIDirect3DSwapChain9;
myIDirect3DDevice9*		gl_pmyIDirect3DDevice9;
myIDirect3D9*			gl_pmyIDirect3D9;
HINSTANCE				gl_hOriginalDll;
HINSTANCE				gl_hThisInstance;
#pragma data_seg ()

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	// to avoid compiler lvl4 warnings 
    LPVOID lpDummy = lpReserved;
    lpDummy = NULL;
    
    switch (ul_reason_for_call)
	{
	    case DLL_PROCESS_ATTACH: InitInstance(hModule); break;
	    case DLL_PROCESS_DETACH: ExitInstance(); break;
        
        case DLL_THREAD_ATTACH:  break;
	    case DLL_THREAD_DETACH:  break;
	}
    return TRUE;
}

// Exported function (faking d3d9.dll's one-and-only export)
IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion)
{
	if (!gl_hOriginalDll) LoadOriginalDll(); // looking for the "right d3d9.dll"
	
	// Hooking IDirect3D Object from Original Library
	typedef IDirect3D9 *(WINAPI* D3D9_Type)(UINT SDKVersion);
	D3D9_Type D3DCreate9_fn = (D3D9_Type) GetProcAddress( gl_hOriginalDll, "Direct3DCreate9");
    
    // Debug
	if (!D3DCreate9_fn) 
    {
        OutputDebugString("PROXYDLL: Pointer to original D3DCreate9 function not received ERROR ****\r\n");
        ::ExitProcess(0); // exit the hard way
    }
	
	// Request pointer from Original Dll. 
	IDirect3D9 *pIDirect3D9_orig = D3DCreate9_fn(SDKVersion);
	
	// Create my IDirect3D8 object and store pointer to original object there.
	// note: the object will delete itself once Ref count is zero (similar to COM objects)
	gl_pmyIDirect3D9 = new myIDirect3D9(pIDirect3D9_orig);
	
	// Return pointer to hooking Object instead of "real one"
	return (gl_pmyIDirect3D9);
}

HRESULT WINAPI Direct3DCreate9Ex(UINT SDKVersion, IDirect3D9Ex** ppD3D)
{
	if (!gl_hOriginalDll) LoadOriginalDll(); // looking for the "right d3d9.dll"

	typedef HRESULT(WINAPI* Direct3DCreate9ExProc)(UINT, IDirect3D9Ex**);
	Direct3DCreate9ExProc D3DCreate9Ex_fn = (Direct3DCreate9ExProc)GetProcAddress(gl_hOriginalDll, "Direct3DCreate9Ex");

	// Debug
	if (!D3DCreate9Ex_fn)
	{
		OutputDebugString("PROXYDLL: Pointer to original D3DCreate9Ex function not received ERROR ****\r\n");
		return E_FAIL;
	}

	HRESULT hr = D3DCreate9Ex_fn(SDKVersion, ppD3D);

	if (SUCCEEDED(hr) && ppD3D)
	{
        OutputDebugString("PROXYDLL: Using D3DCreate9Ex function ****\r\n");
		//*ppD3D = new m_IDirect3D9Ex(*ppD3D, IID_IDirect3D9Ex);
	}

	return hr;
}

void InitInstance(HANDLE hModule) 
{
	OutputDebugString("PROXYDLL: InitInstance called.\r\n");
	
	// Initialisation
	gl_hOriginalDll				= NULL;
	gl_hThisInstance			= NULL;
	gl_pmyIDirect3D9			= NULL;
	gl_pmyIDirect3DDevice9		= NULL;
	gl_pmyIDirect3DSwapChain9	= NULL;
	
	// Storing Instance handle into global var
	gl_hThisInstance = (HINSTANCE)  hModule;
}

typedef HRESULT(WINAPI* Direct3DShaderValidatorCreate9Proc)();
typedef HRESULT(WINAPI* PSGPErrorProc)();
typedef HRESULT(WINAPI* PSGPSampleTextureProc)();
typedef int(WINAPI* D3DPERF_BeginEventProc)(D3DCOLOR, LPCWSTR);
typedef int(WINAPI* D3DPERF_EndEventProc)();
typedef DWORD(WINAPI* D3DPERF_GetStatusProc)();
typedef BOOL(WINAPI* D3DPERF_QueryRepeatFrameProc)();
typedef void(WINAPI* D3DPERF_SetMarkerProc)(D3DCOLOR, LPCWSTR);
typedef void(WINAPI* D3DPERF_SetOptionsProc)(DWORD);
typedef void(WINAPI* D3DPERF_SetRegionProc)(D3DCOLOR, LPCWSTR);
typedef HRESULT(WINAPI* DebugSetLevelProc)(DWORD);
typedef void(WINAPI* DebugSetMuteProc)();
typedef int(WINAPI* Direct3D9EnableMaximizedWindowedModeShimProc)(BOOL);

Direct3DShaderValidatorCreate9Proc m_pDirect3DShaderValidatorCreate9;
PSGPErrorProc m_pPSGPError;
PSGPSampleTextureProc m_pPSGPSampleTexture;
D3DPERF_BeginEventProc m_pD3DPERF_BeginEvent;
D3DPERF_EndEventProc m_pD3DPERF_EndEvent;
D3DPERF_GetStatusProc m_pD3DPERF_GetStatus;
D3DPERF_QueryRepeatFrameProc m_pD3DPERF_QueryRepeatFrame;
D3DPERF_SetMarkerProc m_pD3DPERF_SetMarker;
D3DPERF_SetOptionsProc m_pD3DPERF_SetOptions;
D3DPERF_SetRegionProc m_pD3DPERF_SetRegion;
DebugSetLevelProc m_pDebugSetLevel;
DebugSetMuteProc m_pDebugSetMute;
Direct3D9EnableMaximizedWindowedModeShimProc m_pDirect3D9EnableMaximizedWindowedModeShim;

void LoadOriginalDll(void)
{
    char buffer[MAX_PATH];
    
    // Getting path to system dir and to d3d9.dll
	//::GetSystemDirectory(buffer,MAX_PATH);

    // try to load the system's d3d9.dll, if pointer empty
    //if (!gl_hOriginalDll) gl_hOriginalDll = ::LoadLibraryEx(buffer, NULL, 0/*LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR*/);

	// Append dll name
    buffer[0] = 0;
	strcat(buffer, ".\\d3d9_sideload.dll");

	// try to load the system's d3d9.dll, if pointer empty
	if (!gl_hOriginalDll) gl_hOriginalDll = ::LoadLibraryEx(buffer, NULL, 0/*LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR*/);

	// Append dll name
    buffer[0] = 0;
	strcat(buffer,".\\d3d9.dll");
	
	// try to load the system's d3d9.dll, if pointer empty
	if (!gl_hOriginalDll) gl_hOriginalDll = ::LoadLibraryEx(buffer, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

	// Debug
	if (!gl_hOriginalDll)
	{
		OutputDebugString("PROXYDLL: Original d3d9.dll not loaded ERROR ****\r\n");
		::ExitProcess(0); // exit the hard way
	}

    // Get function addresses
    m_pDirect3DShaderValidatorCreate9 = (Direct3DShaderValidatorCreate9Proc)GetProcAddress(gl_hOriginalDll, "Direct3DShaderValidatorCreate9");
    m_pPSGPError = (PSGPErrorProc)GetProcAddress(gl_hOriginalDll, "PSGPError");
    m_pPSGPSampleTexture = (PSGPSampleTextureProc)GetProcAddress(gl_hOriginalDll, "PSGPSampleTexture");
    m_pD3DPERF_BeginEvent = (D3DPERF_BeginEventProc)GetProcAddress(gl_hOriginalDll, "D3DPERF_BeginEvent");
    m_pD3DPERF_EndEvent = (D3DPERF_EndEventProc)GetProcAddress(gl_hOriginalDll, "D3DPERF_EndEvent");
    m_pD3DPERF_GetStatus = (D3DPERF_GetStatusProc)GetProcAddress(gl_hOriginalDll, "D3DPERF_GetStatus");
    m_pD3DPERF_QueryRepeatFrame = (D3DPERF_QueryRepeatFrameProc)GetProcAddress(gl_hOriginalDll, "D3DPERF_QueryRepeatFrame");
    m_pD3DPERF_SetMarker = (D3DPERF_SetMarkerProc)GetProcAddress(gl_hOriginalDll, "D3DPERF_SetMarker");
    m_pD3DPERF_SetOptions = (D3DPERF_SetOptionsProc)GetProcAddress(gl_hOriginalDll, "D3DPERF_SetOptions");
    m_pD3DPERF_SetRegion = (D3DPERF_SetRegionProc)GetProcAddress(gl_hOriginalDll, "D3DPERF_SetRegion");
    m_pDebugSetLevel = (DebugSetLevelProc)GetProcAddress(gl_hOriginalDll, "DebugSetLevel");
    m_pDebugSetMute = (DebugSetMuteProc)GetProcAddress(gl_hOriginalDll, "DebugSetMute");
    m_pDirect3D9EnableMaximizedWindowedModeShim = (Direct3D9EnableMaximizedWindowedModeShimProc)GetProcAddress(gl_hOriginalDll, "Direct3D9EnableMaximizedWindowedModeShim");
}

void ExitInstance() 
{    
    OutputDebugString("PROXYDLL: ExitInstance called.\r\n");
	
	// Release the system's d3d9.dll
	if (gl_hOriginalDll)
	{
		::FreeLibrary(gl_hOriginalDll);
	    gl_hOriginalDll = NULL;  
	}
}

HRESULT WINAPI Direct3DShaderValidatorCreate9()
{
    if (!m_pDirect3DShaderValidatorCreate9)
    {
        return E_FAIL;
    }

    return m_pDirect3DShaderValidatorCreate9();
}

HRESULT WINAPI PSGPError()
{
    if (!m_pPSGPError)
    {
        return E_FAIL;
    }

    return m_pPSGPError();
}

HRESULT WINAPI PSGPSampleTexture()
{
    if (!m_pPSGPSampleTexture)
    {
        return E_FAIL;
    }

    return m_pPSGPSampleTexture();
}

int WINAPI D3DPERF_BeginEvent(D3DCOLOR col, LPCWSTR wszName)
{
    if (!m_pD3DPERF_BeginEvent)
    {
        return NULL;
    }

    return m_pD3DPERF_BeginEvent(col, wszName);
}

int WINAPI D3DPERF_EndEvent()
{
    if (!m_pD3DPERF_EndEvent)
    {
        return NULL;
    }

    return m_pD3DPERF_EndEvent();
}

DWORD WINAPI D3DPERF_GetStatus()
{
    if (!m_pD3DPERF_GetStatus)
    {
        return NULL;
    }

    return m_pD3DPERF_GetStatus();
}

BOOL WINAPI D3DPERF_QueryRepeatFrame()
{
    if (!m_pD3DPERF_QueryRepeatFrame)
    {
        return FALSE;
    }

    return m_pD3DPERF_QueryRepeatFrame();
}

void WINAPI D3DPERF_SetMarker(D3DCOLOR col, LPCWSTR wszName)
{
    if (!m_pD3DPERF_SetMarker)
    {
        return;
    }

    return m_pD3DPERF_SetMarker(col, wszName);
}

void WINAPI D3DPERF_SetOptions(DWORD dwOptions)
{
    if (!m_pD3DPERF_SetOptions)
    {
        return;
    }

    return m_pD3DPERF_SetOptions(dwOptions);
}

void WINAPI D3DPERF_SetRegion(D3DCOLOR col, LPCWSTR wszName)
{
    if (!m_pD3DPERF_SetRegion)
    {
        return;
    }

    return m_pD3DPERF_SetRegion(col, wszName);
}

HRESULT WINAPI DebugSetLevel(DWORD dw1)
{
    if (!m_pDebugSetLevel)
    {
        return E_FAIL;
    }

    return m_pDebugSetLevel(dw1);
}

void WINAPI DebugSetMute()
{
    if (!m_pDebugSetMute)
    {
        return;
    }

    return m_pDebugSetMute();
}

int WINAPI Direct3D9EnableMaximizedWindowedModeShim(BOOL mEnable)
{
    if (!m_pDirect3D9EnableMaximizedWindowedModeShim)
    {
        return NULL;
    }

    return m_pDirect3D9EnableMaximizedWindowedModeShim(mEnable);
}
