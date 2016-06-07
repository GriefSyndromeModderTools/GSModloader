#pragma once

#ifdef ML_EXPORTS
#	define ML_API __declspec(dllexport)
#else
#	define ML_API __declspec(dllimport)
#endif

struct SQVM;
struct IDirect3DDevice9;
struct IDirect3D9;

struct NativeLibrary
{
	virtual ~NativeLibrary() = default;

	virtual int Init() = 0;
	virtual void Exit() = 0;

	virtual void GetSQVM(SQVM* vm) = 0;
	virtual void GetDxInterface(IDirect3D9* pDx9, IDirect3DDevice9* pDxDevice) = 0;
	virtual bool HandlePresent() = 0;
	virtual void PrePresent() = 0;
};

extern "C"
{
	ML_API int GetNativeLibraryInstance(NativeLibrary**);
}