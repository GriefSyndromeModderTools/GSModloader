#include "stdafx.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

static VOID IDirectInput8_CreateDevice_After(
	IDirectInput8* theDevice,
	REFGUID rguid,
	LPDIRECTINPUTDEVICE * lplpDirectInputDevice,
	LPUNKNOWN pUnkOuter)
{
	MessageBoxA(nullptr, "Success", "Inject2", 0);
}

static VOID CoCreateInstance_After(
	REFCLSID  rclsid,
	LPUNKNOWN pUnkOuter,
	DWORD     dwClsContext,
	REFIID    riid,
	LPVOID    *ppv)
{
	if (dwClsContext == 0x17)
	{
		IDirectInput8* pDInput = *reinterpret_cast<IDirectInput8**>(ppv);
		LPDWORD pvtab = *reinterpret_cast<LPDWORD*>(pDInput);
		GSOXPlugin::GetInstance().InjectVirtualTable(&pvtab[3], 16).After(IDirectInput8_CreateDevice_After);
	}
}

void GSOXPlugin::InjectData()
{
	InjectImportTable(_T("USER32.CreateWindowExA")).Before(CreateWindowExA_Before);
	InjectImportTable(_T("ole32.CoCreateInstance")).After(CoCreateInstance_After);
	ModifyData(0x25A585, 1, '!');
	ModifyCode(0x9F633, 1, 0xEB);
}
