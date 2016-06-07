#include "stdafx.h"
#include "GSLoader.h"

BOOL CreateProcessWithDll(LPCTSTR lpExeFile, LPCTSTR lpDllName);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	CreateProcessWithDll(_T("griefsyndrome.exe"), _T("NML.dll"));

	LPVOID* lpModuleBase = (LPVOID*)GetModuleHandle(NULL);
	return 0;
}

BOOL CreateProcessWithDll(LPCTSTR lpExeFile, LPCTSTR lpDllName)
{
	STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	PROCESS_INFORMATION pi;
	BOOL r = CreateProcess(lpExeFile, NULL, NULL, NULL, TRUE,
		NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED,
		NULL, NULL, &si, &pi);
	Sleep(250);

	SIZE_T szMemSize = _tcslen(lpDllName) * sizeof(TCHAR);
	LPVOID remoteAddr = VirtualAllocEx(pi.hProcess, NULL, szMemSize + 8, MEM_COMMIT, PAGE_READWRITE);
	WriteProcessMemory(pi.hProcess, remoteAddr, lpDllName, szMemSize, NULL);

	HANDLE hThread = CreateRemoteThread(pi.hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(_T("Kernel32")),
#ifdef UNICODE
		_T("LoadLibraryW")
#else
		_T("LoadLibraryA")
#endif
		),
		remoteAddr, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);

	ResumeThread(pi.hThread);
	return TRUE;
}
