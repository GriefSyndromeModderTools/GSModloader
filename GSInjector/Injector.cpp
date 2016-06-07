#include "stdafx.h"

void* my_memcpy(void * _Dst, const void * _Src, size_t _Size)
{
	return memcpy(_Dst, _Src, _Size);
}

DWORD InjectedFunction::Call(LPVOID lpStackTop, LPDWORD lpReturnValue, DWORD dwMaxArgSize)
{
	if (lpStdcallFunction)
	{
		typedef DWORD(INJECT_STDCALL *StdcallNoArg)();
		StdcallNoArg pf = static_cast<StdcallNoArg>(lpStdcallFunction);
		if (szArgSize <= dwMaxArgSize)
		{
			DWORD returnValue;
			DWORD mszArgSize = szArgSize;

			__asm {
				sub esp, mszArgSize;
				lea eax, [esp];

				push mszArgSize;
				push lpStackTop;
				push eax;
				call my_memcpy;
				add esp, 12;

				mov eax, pf;
				call eax;
				mov returnValue, eax;
			}
			return returnValue;
		}
		else if (szArgSize == dwMaxArgSize + 4)
		{
			DWORD returnValue = *lpReturnValue;
			DWORD mszArgSize = szArgSize;
			__asm {
				push returnValue;
				sub esp, dwMaxArgSize;
				lea eax, [esp];

				push dwMaxArgSize;
				push lpStackTop;
				push eax;
				call my_memcpy;
				add esp, 12;

				mov eax, pf;
				call eax;
				mov returnValue, eax;
			}
			return returnValue;
		}
		//Error
		return 0;
	}
	else if (lpCdeclFunction)
	{
		typedef DWORD(INJECT_CDECL *CdeclNoArg)();
		CdeclNoArg pf = static_cast<CdeclNoArg>(lpCdeclFunction);
		if (szArgSize <= dwMaxArgSize)
		{
			DWORD returnValue;
			DWORD mszArgSize = szArgSize;
			LPDWORD lpModifiedReturned;
			__asm {
				push ecx;
				push edx;

				sub esp, mszArgSize;
				lea eax, [esp];
				mov lpModifiedReturned, eax;

				push mszArgSize;
				push lpStackTop;
				push eax;
				call my_memcpy;
				add esp, 12;

				mov eax, pf;
				call eax;

				push mszArgSize;
				push lpModifiedReturned;
				push lpStackTop;
				call my_memcpy;
				add esp, 12;

				mov returnValue, eax;
				add esp, mszArgSize;

				pop edx;
				pop ecx;
			}
			return returnValue;
		}
		else if (szArgSize == dwMaxArgSize + 4)
		{
			DWORD returnValue = *lpReturnValue;
			DWORD mszArgSize = szArgSize;
			LPDWORD lpModifiedReturned;
			__asm {
				push ecx;
				push edx;

				push returnValue;
				sub esp, dwMaxArgSize;
				lea eax, [esp];
				mov lpModifiedReturned, eax;

				push dwMaxArgSize;
				push lpStackTop;
				push eax;
				call my_memcpy;
				add esp, 12;

				mov eax, pf;
				call eax;

				push dwMaxArgSize;
				push lpModifiedReturned;
				push lpStackTop;
				call my_memcpy;
				add esp, 12;

				add esp, dwMaxArgSize;
				pop returnValue;

				pop edx;
				pop ecx
			}
			return returnValue;
		}
		//Error
		return 0;
	}
	return 0;
}

void Injector::Run(LPVOID lpStackTop)
{
	DWORD returnValue = 0;
	for (auto&& f : info->injectedBefore)
	{
		f.Call(lpStackTop, &returnValue, info->dwMaxArgSize);
	}
	returnValue = info->replaced.Call(lpStackTop, &returnValue, info->dwMaxArgSize);
	__asm {
		mov returnValue, eax
	}
	{
		for (auto&& f : info->injectedAfter)
		{
			returnValue = f.Call(lpStackTop, &returnValue, info->dwMaxArgSize);
		}
	}
	__asm {
		mov eax, returnValue
	}
}
