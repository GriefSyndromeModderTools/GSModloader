// DxAPI.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <NativeLibrary.h>
#include <d3d9.h>
#include <d3dx9core.h>
#include <tchar.h>
#include <string>
#include <natRefObj.h>
#include <vector>
#include <sqrat.h>
#include <memory>

namespace
{
	int sqCreateFont(nuInt Height, nuInt Width, nuInt Weight, nuInt MipLevels, nBool Italic, nuInt CharSet, nuInt OutputPrecision, nuInt Quality, nuInt PitchAndFamily, std::string const& pFaceName);
	int sqDrawText(nuInt font, std::string const& str, nuInt top, nuInt left, nuInt width, nuInt height, nuInt format, nuInt color);
}

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
	}

	void GetSQVM(nUnsafePtr<SQVM> vm) override
	{
		Sqrat::DefaultVM::Set(vm);

		Sqrat::Table modloadertable(Sqrat::RootTable().GetSlot(_SC("modloader")).Cast<Sqrat::Table>());
		modloadertable.Func(_SC("CreateFont"), sqCreateFont);
		modloadertable.Func(_SC("DrawText"), sqDrawText);
	}

	void GetDxInterface(nUnsafePtr<IDirect3D9> pDx9, nUnsafePtr<IDirect3DDevice9> pDxDevice) override
	{
		m_pDx9 = pDx9;
		m_pDxDevice = pDxDevice;
	}

	bool HandlePresent() override
	{
		return true;
	}

	void PrePresent() override
	{
		m_pDxDevice->BeginScene();

		/* codes here */

		m_pDxDevice->EndScene();
	}

	int DxCreateFont(nuInt Height, nuInt Width, nuInt Weight, nuInt MipLevels, nBool Italic, nuInt CharSet, nuInt OutputPrecision, nuInt Quality, nuInt PitchAndFamily, std::string const& pFaceName)
	{
		natRefPointer<ID3DXFont> font;
		D3DXCreateFont(m_pDxDevice, Height, Width, Weight, MipLevels, Italic, CharSet, OutputPrecision, Quality, PitchAndFamily, pFaceName.c_str(), &font);
		m_fonts.emplace_back(std::move(font));
		return m_fonts.size() - 1;
	}

	int DxDrawString(nuInt font, std::string const& str, nuInt top, nuInt left, nuInt width, nuInt height, nuInt format, nuInt color)
	{
		if (font >= m_fonts.size())
		{
			return -1;
		}

		RECT rect{ top, left, left + width, top + height };
		return m_fonts[font]->DrawText(NULL, str.c_str(), str.size(), std::addressof(rect), format, color);
	}

	static DxAPI& GetInstance()
	{
		static DxAPI Instance;
		return Instance;
	}

private:
	nUnsafePtr<IDirect3D9> m_pDx9;
	nUnsafePtr<IDirect3DDevice9> m_pDxDevice;

	std::vector<natRefPointer<ID3DXFont>> m_fonts;
};

namespace
{
	int sqCreateFont(nuInt Height, nuInt Width, nuInt Weight, nuInt MipLevels, nBool Italic, nuInt CharSet, nuInt OutputPrecision, nuInt Quality, nuInt PitchAndFamily, std::string const& pFaceName)
	{
		return DxAPI::GetInstance().DxCreateFont(Height, Width, Weight, MipLevels, Italic, CharSet, OutputPrecision, Quality, PitchAndFamily, pFaceName);
	}

	int sqDrawText(nuInt font, std::string const& str, nuInt top, nuInt left, nuInt width, nuInt height, nuInt format, nuInt color)
	{
		return DxAPI::GetInstance().DxDrawString(font, str, top, left, width, height, format, color);
	}
}

int GetNativeLibraryInstance(nUnsafePtr<nUnsafePtr<NativeLibrary>> ppLib)
{
	if (ppLib == nullptr)
	{
		return -1;
	}
	
	*ppLib = std::addressof(DxAPI::GetInstance());

	return 0;
}