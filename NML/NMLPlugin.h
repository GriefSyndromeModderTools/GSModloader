#pragma once

#include <tchar.h>

#include "../GSInjector/GSInjector.h"
#include <map>


extern LPCTSTR szFileNotExist;
struct NativeLibrary;

class NMLPlugin : public Plugin
{
public:
	virtual void InjectData() override;
	virtual void OnPluginLoad() override;
	virtual void OnPluginUnload() override;

	struct InjectedFileInformation
	{
		HANDLE client;
		LONG offset;
	};
	std::map<HANDLE, InjectedFileInformation> injectedFiles;
	char* memoryFile;
	int memoryFileSize;
	std::vector<NativeLibrary*> NativeLibraries;

	static std::map<std::string, std::vector<std::vector<char>>> tmpScript;
};

extern NMLPlugin plugin;
