#include "stdafx.h"
#include "NMLPlugin.h"

//use fake file handle (-2, -3, ...)
//this may lead to invalid call to CloseHandle (which is not an error 2333)
static int next_handle = -2;
static HANDLE MakeFreeHandle()
{
	int ret = next_handle--;
	return (HANDLE)ret;
}

void INJECT_CDECL CreateFileA_After(
	LPCTSTR               lpFileName,
	DWORD                 /*dwDesiredAccess*/,
	DWORD                 /*dwShareMode*/,
	LPSECURITY_ATTRIBUTES /*lpSecurityAttributes*/,
	DWORD                 /*dwCreationDisposition*/,
	DWORD                 /*dwFlagsAndAttributes*/,
	HANDLE                /*hTemplateFile*/,
	HANDLE                returnedValue)
{
	if (strcmp(lpFileName, szFileNotExist) == 0)
	{
		NMLPlugin::InjectedFileInformation f;
		f.client = MakeFreeHandle();
		f.offset = 0;
		plugin.injectedFiles[f.client] = f;
		returnedValue = f.client;
	}
}

void INJECT_CDECL ReadFile_After(
	HANDLE       hFile,
	LPVOID       lpBuffer,
	DWORD        nNumberOfBytesToRead,
	LPDWORD      lpNumberOfBytesRead,
	LPOVERLAPPED /*lpOverlapped*/,
	BOOL         returnedValue)
{
	if (plugin.injectedFiles.find(hFile) != plugin.injectedFiles.end())
	{
		NMLPlugin::InjectedFileInformation& f = plugin.injectedFiles[hFile];
		if (f.offset > plugin.memoryFileSize)
		{
			returnedValue = FALSE;
			return;
		}
		memcpy(lpBuffer, plugin.memoryFile + f.offset, nNumberOfBytesToRead);
		if (lpNumberOfBytesRead)
		{
			*lpNumberOfBytesRead = nNumberOfBytesToRead;
		}

		f.offset += nNumberOfBytesToRead;
		returnedValue = TRUE;
	}
}

void INJECT_CDECL SetFilePointer_After(
	HANDLE hFile,
	LONG   lDistanceToMove,
	PLONG  lpDistanceToMoveHigh,
	DWORD  dwMoveMethod,
	DWORD  returnedValue)
{
	if (plugin.injectedFiles.find(hFile) != plugin.injectedFiles.end())
	{
		NMLPlugin::InjectedFileInformation& f = plugin.injectedFiles[hFile];
		if (lpDistanceToMoveHigh)
		{
			returnedValue = INVALID_SET_FILE_POINTER;
			return;
		}
		switch (dwMoveMethod)
		{
		case FILE_BEGIN:
			f.offset = lDistanceToMove;
			break;
		case FILE_CURRENT:
			f.offset += lDistanceToMove;
			break;
		case FILE_END:
			f.offset = plugin.memoryFileSize;
			break;
		}
		if (f.offset > plugin.memoryFileSize)
		{
			returnedValue = INVALID_SET_FILE_POINTER;
			return;
		}
		returnedValue = f.offset;
	}
}
