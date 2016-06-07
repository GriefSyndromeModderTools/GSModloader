#include "stdafx.h"
#include "NMLPlugin.h"
#include "NativeLibrary.h"
#include <d3d9.h>

IDirect3D9* g_pDx9;
IDirect3DDevice9* g_pDxDevice;
IDirect3DSwapChain9* g_pDxSwapChain;

void INJECT_CDECL CreateDevice_After(
	LPDIRECT3D9 pDx9,
	UINT Adapter,
	D3DDEVTYPE DeviceType,
	HWND hFocusWindow,
	DWORD BehaviorFlags,
	D3DPRESENT_PARAMETERS * pPresentationParameters,
	IDirect3DDevice9 ** ppReturnedDeviceInterface,
	HRESULT returnedValue);

void INJECT_CDECL Present_Before(
	LPDIRECT3DSWAPCHAIN9 pDxSwapChain,
	const RECT* pSourceRect,
	const RECT* pDestRect,
	HWND hDestWindowOverride,
	const RGNDATA* pDirtyRegion,
	DWORD dwFlags);

void INJECT_CDECL Direct3DCreate9_After(UINT /*SDKVersion*/, IDirect3D9* returnedValue)
{
	if (returnedValue != nullptr)
	{
		g_pDx9 = returnedValue;
		plugin.InjectVirtualTable(g_pDx9, 16ul, 28ul).After(CreateDevice_After);
	}
}

void INJECT_CDECL CreateDevice_After(
	LPDIRECT3D9 pDx9,
	UINT /*Adapter*/,
	D3DDEVTYPE /*DeviceType*/,
	HWND /*hFocusWindow*/,
	DWORD /*BehaviorFlags*/,
	D3DPRESENT_PARAMETERS * /*pPresentationParameters*/,
	IDirect3DDevice9 ** ppReturnedDeviceInterface,
	HRESULT returnedValue)
{
	if (pDx9 == g_pDx9 && returnedValue == D3D_OK)
	{
		g_pDxDevice = *ppReturnedDeviceInterface;
		if (g_pDxDevice->GetSwapChain(0u, &g_pDxSwapChain) != D3D_OK)
		{
			plugin.Error(_T("Cannot get SwapChain, Present will not be injected"));
			return;
		}

		plugin.InjectVirtualTable(g_pDxSwapChain, 3ul, 24ul).Before(Present_Before);
	}
}

void INJECT_CDECL Present_Before(LPDIRECT3DSWAPCHAIN9 pDxSwapChain, const RECT* /*pSourceRect*/, const RECT* /*pDestRect*/, HWND /*hDestWindowOverride*/, const RGNDATA* /*pDirtyRegion*/, DWORD /*dwFlags*/)
{
	if (pDxSwapChain != g_pDxSwapChain || plugin.NativeLibraries.empty())
	{
		return;
	}

	for (auto pNativeLib : plugin.NativeLibraries)
	{
		if (pNativeLib->HandlePresent())
		{
			pNativeLib->PrePresent();
		}
	}
}
