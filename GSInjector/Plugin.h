#pragma once

class Plugin
{
public:
	Plugin(LPCTSTR id)
	{
	}

	virtual ~Plugin()
	{
	}
public:
	void Load(HINSTANCE _hInstanceDll)
	{
		this->hInstanceDll = _hInstanceDll;
		this->OnPluginLoad();
		this->InjectData();
	}

	static void Error(LPCTSTR msg)
	{
		MessageBox(NULL, msg, _T("GSInjector"), 0);
		exit(1);
	}

	void Unload()
	{
		this->OnPluginUnload();
	}

protected:
	virtual void OnPluginLoad() {}
	virtual void OnPluginUnload() {}
	virtual void InjectData() = 0;

public:
	//Inject a stdcall function by overriding its pointer in import table.
	Injector InjectImportTable(LPCTSTR lpFunctionId);

	Injector InjectVirtualTable(LPVOID lpObject, DWORD dwIndex, DWORD dwArgSize);

	void InjectCode(DWORD dwOffset, DWORD dwSize, LPCVOID lpCode, DWORD dwCodeSize);
	void InjectFunction(DWORD dwOffset, LPVOID lpNewFunc);

public:
	void ModifyData(DWORD dwOffset, DWORD dwSize, ...);
	void ModifyCode(DWORD dwOffset, DWORD dwSize, ...);

public:
	HINSTANCE hInstanceDll = NULL;
};
