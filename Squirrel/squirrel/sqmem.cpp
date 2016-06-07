/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <Windows.h>
///	@warning	edited
void *sq_vm_malloc(SQUnsignedInteger size)
{
	auto gs_malloc = reinterpret_cast<void*(__cdecl *)(size_t)>(reinterpret_cast<DWORD>(GetModuleHandle(NULL)) + 0x1AA447);
	return gs_malloc(size);
	//return malloc(size);
}

///	@warning	edited
void *sq_vm_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size)
{
	auto gs_realloc = reinterpret_cast<void*(__cdecl *)(void*, size_t)>(reinterpret_cast<DWORD>(GetModuleHandle(NULL)) + 0x1AF762);
	return gs_realloc(p, size);
	//return realloc(p, size);
}

///	@warning	edited
void sq_vm_free(void *p, SQUnsignedInteger size)
{
	auto gs_free = reinterpret_cast<void(__cdecl *)(void *)>(reinterpret_cast<DWORD>(GetModuleHandle(NULL)) + 0x1AB069);
	gs_free(p);
	//free(p);
}
