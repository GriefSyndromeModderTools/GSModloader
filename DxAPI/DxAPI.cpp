// DxAPI.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <NativeLibrary.h>
#include <d3d9.h>
#include <d3dx9core.h>
#include <tchar.h>

class DxAPI
	: public NativeLibrary
{
public:
	DxAPI() = default;
	~DxAPI() = default;

	int Init() override
	{
		return 0;
	}

	void Exit() override
	{
		font->Release();
	}

	void GetSQVM(SQVM* /*vm*/) override
	{
	}

	void GetDxInterface(IDirect3D9* pDx9, IDirect3DDevice9* pDxDevice) override
	{
		m_pDx9 = pDx9;
		m_pDxDevice = pDxDevice;

		ZeroMemory(&lf, sizeof(D3DXFONT_DESCA));
		lf.Height = 24;
		lf.Width = 10;
		lf.Weight = 10;
		lf.Italic = false;
		lf.CharSet = DEFAULT_CHARSET;
		lstrcpy(lf.FaceName, _T("微软雅黑"));

		D3DXCreateFontIndirect(m_pDxDevice, &lf, &font);
	}

	bool HandlePresent() override
	{
		return true;
	}

	void PrePresent() override
	{
		TCHAR strText[] = _T("Acaly and Natsu's Modloader Test");
		RECT myrect;
		myrect.top = 150;
		myrect.left = 0;
		myrect.right = 500 + myrect.left;
		myrect.bottom = 100 + myrect.top;
		m_pDxDevice->BeginScene();

		font->DrawText(
			NULL,
			strText,
			sizeof strText - 1,
			&myrect,
			DT_TOP | DT_LEFT,
			D3DCOLOR_ARGB(255, 255, 255, 0));

		m_pDxDevice->EndScene();
	}

	static DxAPI& GetInstance()
	{
		static DxAPI Instance;
		return Instance;
	}

private:
	IDirect3D9* m_pDx9;
	IDirect3DDevice9* m_pDxDevice;

	D3DXFONT_DESCA lf;
	ID3DXFont* font;
};

int GetNativeLibraryInstance(NativeLibrary** ppLib)
{
	if (ppLib == nullptr)
	{
		return -1;
	}

	*ppLib = &DxAPI::GetInstance();

	return 0;
}