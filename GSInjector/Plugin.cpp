#include "stdafx.h"

// assembly->C
static UCHAR InjectEntryTemplate[] = {
	0x53,                               //push ebx
	0x8D, 0x5C, 0x24, 0x08,             //lea ebx, [esp+8] //skip saved ebx and ret addr
	0x51,                               //push ecx
	0x56,                               //push esi
	0xBE, 0x00, 0x00, 0x00, 0x00,       //mov esi, 0x00000000(+8)
	0xB8, 0x00, 0x00, 0x00, 0x00,       //mov eax, 0x00000000(+D)
	0x53,                               //push ebx
	0x56,                               //push esi
	0xFF, 0xD0,                         //call eax
	0x5E,                               //pop esi
	0x59,                               //pop ecx
	0x5B,                               //pop ebx
	0xC2, 0x00, 0x00,                   //ret 0
};

// C->C++
static void INJECT_STDCALL InjectHandler(InjectorInformation* pInfo, LPVOID stacktop)
{
	Injector(pInfo).Run(stacktop);
}

//-------------------------------------------------------------------------

static InjectorInformation* AllocInformation(LPVOID id)
{
	return new InjectorInformation();
}

static DWORD oldProtect;

static UCHAR* AllocCode(SIZE_T dwSize)
{
	UCHAR* ret = static_cast<UCHAR*>(VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
	return ret;
}

static void FlushCode(UCHAR* pCode, SIZE_T dwSize)
{
	FlushInstructionCache(GetModuleHandle(NULL), pCode, dwSize);
}

static InjectorInformation* InjectStdCallFunctionPointer(LPVOID lpAddr, DWORD dwArgSize)
{
	InjectorInformation* info = AllocInformation(lpAddr);
	UCHAR* pInjectImportTableEntry = AllocCode(sizeof(InjectEntryTemplate));

	memcpy(pInjectImportTableEntry, InjectEntryTemplate, sizeof(InjectEntryTemplate));
	memcpy(pInjectImportTableEntry + 8, &info, 4);
	LPVOID pHandler = InjectHandler;
	memcpy(pInjectImportTableEntry + 13, &pHandler, 4);
	WORD bPopStack = (WORD) dwArgSize;
	memcpy(pInjectImportTableEntry + 25, &bPopStack, 2);

	FlushCode(pInjectImportTableEntry, sizeof(InjectEntryTemplate));

	LPVOID* lpFuncPointer = (LPVOID*) lpAddr;
	DWORD oldProtect;
	VirtualProtect(lpFuncPointer, 4, PAGE_READWRITE, &oldProtect);
	info->original = *lpFuncPointer;
	*lpFuncPointer = pInjectImportTableEntry;
	VirtualProtect(lpFuncPointer, 4, oldProtect, &oldProtect);

	info->replaced.lpStdcallFunction = info->original;
	info->replaced.szArgSize = dwArgSize;
	info->dwMaxArgSize = dwArgSize;

	return info;
}

//-------------------------------------------------------------------------

void Plugin::ModifyData(DWORD dwOffset, DWORD dwSize, ...)
{
	DWORD oldProtect;
	UCHAR* ptr = (UCHAR*)GetModuleHandle(NULL);
	ptr += dwOffset;
	VirtualProtect(ptr, dwSize, PAGE_READWRITE, &oldProtect);
	memcpy(ptr, (char*)((&dwSize) + 1), dwSize);
	VirtualProtect(ptr, dwSize, oldProtect, &oldProtect);
}

void Plugin::ModifyCode(DWORD dwOffset, DWORD dwSize, ...)
{
	DWORD oldProtect;
	UCHAR* ptr = (UCHAR*)GetModuleHandle(NULL);
	ptr += dwOffset;
	VirtualProtect(ptr, dwSize, PAGE_READWRITE, &oldProtect);
	memcpy(ptr, (char*)((&dwSize) + 1), dwSize);
	VirtualProtect(ptr, dwSize, oldProtect, &oldProtect);
	FlushInstructionCache(GetModuleHandle(NULL), ptr, dwSize);
}

//-------------------------------------------------------------------------

struct ImportTableEntryInfo
{
	LPVOID lpAddress;
	SIZE_T szArgSize;
};

struct {
	LPCTSTR key;
	DWORD offset;
	DWORD argsize;
} GSImportTable[] = {

	{ _T("USER32.CreateWindowExA"), 0x20E258, 0x30 },
	{ _T("ole32.CoCreateInstance"), 0x20E37C, 0x14 },
	{ _T("kernel32.CreateFileA"), 0x20E0B0, 0x1C },
	{ _T("kernel32.ReadFile"), 0x20E09C, 0x14 },
	{ _T("kernel32.SetFilePointer"), 0x20E0A4, 0x10 },
	{ _T("kernel32.CloseHandle"), 0x20E0BC, 0x04 },
	{ _T("d3d9.Direct3DCreate9"), 0x20E2D0, 0x04 },

};


static ImportTableEntryInfo GetImportTableEntry(LPCTSTR lpFunctionId)
{
	PCHAR lpBase = (char*) GetModuleHandle(NULL);
	for (auto entry : GSImportTable)
	{
		if (_tcscmp(entry.key, lpFunctionId) == 0)
		{
			return { (LPVOID)(lpBase + entry.offset), entry.argsize };
		}
	}
	return { NULL, 0 };
}

Injector Plugin::InjectImportTable(LPCTSTR lpFunctionId)
{
	ImportTableEntryInfo entry = GetImportTableEntry(lpFunctionId);
	if (!entry.lpAddress)
	{
		Error(_T("Import table entry not found."));
	}
	InjectorInformation* info = InjectStdCallFunctionPointer(entry.lpAddress, entry.szArgSize);
	return Injector(info);
}

Injector Plugin::InjectVirtualTable(LPVOID lpObject, DWORD dwIndex, DWORD dwArgSize)
{
	LPDWORD lpVTable = *(LPDWORD*)lpObject;
	LPVOID lpTableEntry = &lpVTable[dwIndex];
	InjectorInformation* info = InjectStdCallFunctionPointer(lpTableEntry, dwArgSize);
	return Injector(info);
}

void Plugin::InjectCode(DWORD dwDestOffset, DWORD dwDestSize, LPCVOID lpCode, DWORD dwCodeSize)
{
	if (dwDestSize < 6)
	{
		Error(_T("Code injection must have a destination of at least 6 bytes length."));
	}
	PCHAR lpBase = (char*) GetModuleHandle(NULL);
	LPVOID lpDest = (LPVOID)(lpBase + dwDestOffset);

	DWORD dwNewCodePureSize = dwCodeSize + dwDestSize + 6;
	PCHAR lpNewCode = (PCHAR) AllocCode(dwNewCodePureSize + 4 + 4);

	PCHAR lpDest_back = (PCHAR)lpDest + dwDestSize;
	LPVOID lpJmpForward = lpNewCode + dwNewCodePureSize;
	LPVOID lpJmpBackward = lpNewCode + dwNewCodePureSize + 4;

	PCHAR pWrite = (PCHAR)lpNewCode;

	memcpy(pWrite, lpCode, dwCodeSize);
	pWrite += dwCodeSize;

	memcpy(pWrite, lpDest, dwDestSize);
	pWrite += dwDestSize;

	BYTE code_jmp_back[6] = { 0xFF, 0x25 };
	memcpy(code_jmp_back + 2, &lpJmpBackward, 4);
	memcpy(pWrite, code_jmp_back, 6);
	pWrite += 6;

	memcpy(pWrite, &lpNewCode, 4);
	memcpy(pWrite + 4, &lpDest_back, 4);

	DWORD old_protect;
	VirtualProtect(lpDest, dwDestSize, PAGE_EXECUTE_READWRITE, &old_protect);
	BYTE code_jmp_forward[6] = { 0xFF, 0x25 };
	memcpy(code_jmp_forward + 2, &lpJmpForward, 4);
	memcpy(lpDest, code_jmp_forward, 6);
	VirtualProtect(lpDest, dwDestSize, old_protect, &old_protect);
}

void Plugin::InjectFunction(DWORD dwOffset, LPVOID lpNewFunc)
{
	PCHAR lpBase = (char*)GetModuleHandle(NULL);
	LPVOID lpDest = (LPVOID)(lpBase + dwOffset);
	LPDWORD lpIndirect = new DWORD((DWORD) lpNewFunc);
	BYTE code[6] = { 0xFF, 0x25 };
	*(LPDWORD*)(code + 2) = lpIndirect;

	DWORD old_protect;
	VirtualProtect(lpDest, 6, PAGE_EXECUTE_READWRITE, &old_protect);
	memcpy(lpDest, code, 6);
	VirtualProtect(lpDest, 6, old_protect, &old_protect);
}