// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <Windows.h>
#include <io.h>
#include <tchar.h>
#include <string>
#include <sstream>
#include <map>
#include <natEvent.h>
#include <natUtil.h>
#include "natLog.h"

#include "extern\unzip.h"

#include "resource.h"
#include <GSInjector.h>
#include "GSPack.h"
#include <sqrat.h>

#include "NativeLibrary.h"

#include <rapidjson\reader.h>

//injected functions
LPCTSTR szFileNotExist = _T("@@@@@");

void INJECT_CDECL CreateFileA_After(
	LPCTSTR               lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile,
	HANDLE                returnedValue);

void INJECT_CDECL ReadFile_After(
	HANDLE       hFile,
	LPVOID       lpBuffer,
	DWORD        nNumberOfBytesToRead,
	LPDWORD      lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped,
	BOOL         returnedValue);

void INJECT_CDECL SetFilePointer_After(
	HANDLE hFile,
	LONG   lDistanceToMove,
	PLONG  lpDistanceToMoveHigh,
	DWORD  dwMoveMethod,
	DWORD  returnedValue);

void INJECT_CDECL Direct3DCreate9_After(
	UINT SDKVersion,
	IDirect3D9* returnedValue);

static void INJECT_STDCALL InjectedGameWndProc(HWND /*hwnd*/, UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/);
static void INJECT_STDCALL InjectSquirrel(LPVOID hSQVM);
static bool CreateGSPackFileList(std::unordered_map<std::string, std::unordered_map<std::string, std::vector<char>>>& out);

static std::unordered_map<std::string, std::unordered_map<std::string, std::vector<char>>> tmpScript;
static std::unordered_map<std::string, std::vector<char>> ModInfoJson;
static std::unordered_map<std::string, std::string> ModID_OriginNameMap;

struct Msgdata
{
	HWND hWnd;		///< @brief	窗口句柄
	UINT uMsg;		///< @brief	信息类型
	WPARAM wParam;	///< @brief	wParam
	LPARAM lParam;	///< @brief	lParam
};

class SQEventBus final
{
public:
	static SQEventBus& GetInstance()
	{
		static SQEventBus s_Instance;
		return s_Instance;
	}

	template <typename EventClass>
	std::enable_if_t<std::is_base_of<natEventBase, EventClass>::value, void> RegisterEvent()
	{
		m_EventHandlerMap.try_emplace(typeid(EventClass));
	}

	template <typename EventClass, typename EventListener>
	nuInt RegisterEventListener(EventListener listener, int priority = Priority::Normal)
	{
		auto iter = m_EventHandlerMap.find(typeid(EventClass));
		if (iter == m_EventHandlerMap.end())
		{
			throw natException(_T(__FUNCTION__), _T("Unregistered event."));
		}

		auto&& listeners = iter->second[priority];
		auto ret = listeners.empty() ? 0u : listeners.rbegin()->first + 1u;
		listeners[ret] = listener;
		return ret;
	}

	template <typename EventClass>
	void UnregisterEventListener(int priority, nuInt Handle)
	{
		auto iter = m_EventHandlerMap.find(typeid(EventClass));
		if (iter == m_EventHandlerMap.end())
		{
			throw natException(_T(__FUNCTION__), _T("Unregistered event."));
		}

		auto listeneriter = iter->second.find(priority);
		if (listeneriter == iter->second.end())
		{
			return;
		}

		listeneriter->second.erase(Handle);
	}

	template <typename EventClass>
	bool Post(EventClass& event)
	{
		auto iter = m_EventHandlerMap.find(typeid(EventClass));
		if (iter == m_EventHandlerMap.end())
		{
			throw natException(_T(__FUNCTION__), _T("Unregistered event."));
		}

		for (auto& listeners : iter->second)
		{
			for (auto& listener : listeners.second)
			{
				listener.second(static_cast<natEventBase&>(event));
			}
		}

		return event.IsCanceled();
	}

private:
	std::unordered_map<std::type_index, std::map<int, std::map<nuInt, Sqrat::Function>>> m_EventHandlerMap;

	SQEventBus() = default;
	~SQEventBus() = default;
};

class WndMsgEvent final
	: public natEventBase
{
public:
	explicit WndMsgEvent(Msgdata WndMsg)
		: m_WndMsg(WndMsg)
	{
	}

	bool CanCancel() const noexcept override
	{
		return false;
	}

	Msgdata m_WndMsg;
};

nuInt sqRegister(Sqrat::Function tFunc, nuInt uPriority)
{
	return SQEventBus::GetInstance().RegisterEventListener<WndMsgEvent>(tFunc, uPriority);
}

bool sqLoadLibrary(const SQChar* lpDllName);

// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppMemberFunctionMayBeConst
struct JsonReaderHandler
{
	Sqrat::Table GetTable() const
	{
		return LastTable.front();
	}

	template <typename T>
	bool Push(T const& value)
	{
		if (LastContainerIsArray.empty())
		{
			return false;
		}

		if (LastContainerIsArray.back())
		{
			if (LastArray.empty())
			{
				return false;
			}

			LastArray.back().Append(value);
		}
		else
		{
			if (LastTable.empty() || LastKey.empty())
			{
				return false;
			}

			LastTable.back().SetValue(LastKey.back().c_str(), value);
			LastKey.pop_back();
		}

		return true;
	}

	bool Null()
	{
		return Push(Sqrat::Object());
	}
	bool Bool(bool value)
	{
		return Push(value);
	}
	bool Int(int value)
	{
		return Push(value);
	}
	bool Uint(unsigned value)
	{
		return Push(value);
	}
	bool Int64(int64_t value)
	{
		return Push(value);
	}
	bool Uint64(uint64_t value)
	{
		return Push(value);
	}
	bool Double(double value)
	{
		return Push(value);
	}
	bool String(const TCHAR* str, size_t len)
	{
		return Push(Sqrat::string(str, len));
	}
	bool StartObject()
	{
		Sqrat::Table tmpTable(Sqrat::DefaultVM::Get());
		if (!LastContainerIsArray.empty())
		{
			if (LastContainerIsArray.back())
			{
				if (LastArray.empty())
				{
					return false;
				}

				LastArray.back().Append(tmpTable);
			}
			else
			{
				if (!LastTable.empty())
				{
					if (LastKey.empty())
					{
						return false;
					}

					LastTable.back().SetValue(LastKey.back().c_str(), tmpTable);
					LastKey.pop_back();
				}
			}
		}

		LastTable.emplace_back(tmpTable);
		LastContainerIsArray.push_back(false);

		return true;
	}
	bool Key(const TCHAR* str, size_t len, bool /*copy*/)
	{
		LastKey.emplace_back(str, len);

		return true;
	}
	bool EndObject(size_t size)
	{
		if (LastTable.empty())
		{
			return false;
		}

		bool SizeCorrect = LastTable.back().GetSize() == size;

		if (LastTable.size() > 1u)
		{
			LastTable.pop_back();
		}

		LastContainerIsArray.pop_back();
		return SizeCorrect;
	}
	bool StartArray()
	{
		if (LastKey.empty() || LastKey.back().empty() || LastContainerIsArray.empty())
		{
			return false;
		}

		Sqrat::Array tmpArray(Sqrat::DefaultVM::Get());

		if (LastContainerIsArray.back())
		{
			if (LastArray.empty())
			{
				return false;
			}

			LastArray.back().Append(tmpArray);
		}
		else
		{
			if (LastTable.empty())
			{
				return false;
			}

			LastTable.back().SetValue(LastKey.back().c_str(), tmpArray);
			LastKey.pop_back();
		}

		LastArray.emplace_back(tmpArray);
		LastContainerIsArray.push_back(true);

		return true;
	}
	bool EndArray(size_t size)
	{
		if (LastArray.empty())
		{
			return false;
		}

		bool SizeCorrect = LastArray.back().GetSize() == size;

		LastArray.pop_back();
		LastContainerIsArray.pop_back();
		return SizeCorrect;
	}

private:
	std::vector<bool> LastContainerIsArray;
	std::vector<std::string> LastKey;
	std::vector<Sqrat::Array> LastArray;
	std::vector<Sqrat::Table> LastTable;
};
// ReSharper restore CppMemberFunctionMayBeConst
// ReSharper restore CppMemberFunctionMayBeStatic

Sqrat::Table sqParseJson(const SQChar* pJsonContent)
{
	Sqrat::Table JsonTable;
	JsonReaderHandler myHandler;
	rapidjson::Reader tmpReader;
	rapidjson::StringStream tmpSS(pJsonContent);

	if (tmpReader.Parse(tmpSS, myHandler))
	{
		JsonTable = myHandler.GetTable();
	}

	return JsonTable;
}

class NMLPlugin : public Plugin
{
public:
	NMLPlugin() : Plugin(_T("NML")), memoryFile(nullptr), memoryFileSize(0)
	{
	}

	void InjectData() override
	{
		{
			//inject file read
			InjectImportTable(_T("kernel32.CreateFileA")).After(CreateFileA_After);
			InjectImportTable(_T("kernel32.ReadFile")).After(ReadFile_After);
			InjectImportTable(_T("kernel32.SetFilePointer")).After(SetFilePointer_After);
			InjectImportTable(_T("d3d9.Direct3DCreate9")).After(Direct3DCreate9_After);
		}
		{
			//inject game window message process function
			LPVOID lpAddrFunc = &InjectedGameWndProc;
			BYTE new_code[] = {
				0x8B, 0x45, 0x14,				//mov eax,[ebp+0x14]
				0x50,							//push eax
				0x8B, 0x45, 0x10,				//mov eax,[ebp+0x10]
				0x50,							//push eax
				0x8B, 0x45, 0x0C,				//mov eax,[ebp+0x0C]
				0x50,							//push eax
				0x8B, 0x45, 0x08,				//mov eax,[ebp+0x08]
				0x50,							//push eax
				0xB8, 0x00, 0x00, 0x00, 0x00,	//mov eax, 0x00000000
				0xFF, 0xD0,						//call eax
			}; //no need to save eax here
			memcpy(new_code + 17, &lpAddrFunc, 4);
			InjectCode(0x9FA93, 7, new_code, sizeof(new_code));
		}
		{
			//inject data pack

			//read a file that not exist, so that the system will
			//not handle it, in which case we can handle it saftly.
			LPCSTR szDataFile = szFileNotExist;

			PCHAR lpLoadFunc = ((char*)GetModuleHandle(NULL) + 0xD0400);
			BYTE new_code[] = {
				0x50, //push eax
				0xB9, 0x00, 0x00, 0x00, 0x00, //mov ecx, 0x00000000
				0xB8, 0x00, 0x00, 0x00, 0x00, //mov eax, 0x00000000
				0xFF, 0xD0, //call eax
				0x58, //pop eax
			};
			*(LPCVOID*)(new_code + 2) = szDataFile;
			*(LPCVOID*)(new_code + 7) = lpLoadFunc;
			InjectCode(0x9F9F1, 7, new_code, sizeof(new_code));
		}
		{
			LPVOID lpAddrFunc = &InjectSquirrel;
			BYTE new_codetest[] = {
				0x50,								//push eax //save eax
				0x50,								//push eax //push argument
				0xB8, 0x00, 0x00, 0x00, 0x00,		//mov eax, 0x00000000
				0xFF, 0xD0,							//call eax
				0x58,								//pop eax  //restore eax
			};
			memcpy(new_codetest + 3, &lpAddrFunc, 4);

			InjectCode(0xB69AA, 6, new_codetest, sizeof(new_codetest));
		}
	}

	void OnPluginLoad() override
	{
		SQEventBus::GetInstance().RegisterEvent<WndMsgEvent>();

		GSPack newPack;
		if (_access("gs03.dat", 0) == -1)
		{
			HRSRC hRsrc = FindResource(hInstanceDll, MAKEINTRESOURCE(IDR_GSPACK1), "GSPACK");
			if (hRsrc != NULL)
			{
				DWORD dwSize = SizeofResource(hInstanceDll, hRsrc);
				if (dwSize != 0ul)
				{
					HGLOBAL hGlobal = LoadResource(hInstanceDll, hRsrc);
					if (hGlobal != NULL)
					{
						char *pBuffer = static_cast<char *>(LockResource(hGlobal));
						if (pBuffer != nullptr)
						{
							std::vector<char> tmpBuffer(pBuffer, pBuffer + dwSize);
							newPack.OpenPackage(tmpBuffer);
						}
					}
				}
			}
		}

		std::unordered_map<std::string, std::unordered_map<std::string, std::vector<char>>> FileList;
		if (CreateGSPackFileList(FileList))
		{
			for (auto& itea : FileList)
			{
				for (auto& itea2 : itea.second)
				{
					if (_stricmp(itea2.first.substr(0u, sizeof _T("assets/") - 1).c_str(), _T("assets/")) == 0)
					{
						std::vector<char>& bugfix = itea2.second;
						newPack.AddFile(natUtil::FormatString(_T("mods/%s/resource/%s"), itea.first.c_str(), itea2.first.substr(sizeof _T("assets/") - 1).c_str()), bugfix);
					}
					else
					{
						if (ModInfoJson[itea.first].empty() && _stricmp(itea2.first.c_str(), _T("modinfo.json")) == 0)
						{
							ModInfoJson[itea.first] = move(itea2.second);
							continue;
						}

						std::string ext = itea2.first.substr(itea2.first.find_last_of(_T('.')) + 1u);
						if (_stricmp(ext.c_str(), _T("nut")) == 0 || _stricmp(ext.c_str(), _T("cv4")) == 0)
						{
							tmpScript[itea.first][itea2.first] = move(itea2.second);
						}
					}
				}
			}
		}

		//this is called before data injection
		//make the memory file here
		std::vector<char> tmpPack;
		newPack.SavePackage(tmpPack);
		memoryFileSize = tmpPack.size();
		memoryFile = new char[memoryFileSize];
		memcpy_s(memoryFile, memoryFileSize, &tmpPack[0], tmpPack.size());
	}

	void OnPluginUnload() override
	{
		for (auto pNativeLib : NativeLibraries)
		{
			pNativeLib->Exit();
		}

		delete[] memoryFile;
		memoryFile = nullptr;
	}

	struct InjectedFileInformation
	{
		HANDLE client;
		LONG offset;
	};
	std::map<HANDLE, InjectedFileInformation> injectedFiles;
	char* memoryFile;
	int memoryFileSize;

	std::vector<NativeLibrary*> NativeLibraries;
};

static void CompileErrorHandler(HSQUIRRELVM, const SQChar *desc, const SQChar *source, SQInteger line, SQInteger column)
{
	std::stringstream ss;
	ss << "Compile Error: " << desc << "\nIn source \"" << source << "\", line " << line << ", column " << column << ".";
	Plugin::Error(ss.str().c_str());
}

static nuInt ReadModPack()
{
	HSQUIRRELVM vm = Sqrat::DefaultVM::Get();

	Sqrat::Table tmpmodtable(vm);
	for (auto& itea : tmpScript)
	{
		Sqrat::Table modtable(vm);

		Sqrat::Table modInfoTable;
		auto& tmpJsonContent = ModInfoJson[itea.first];
		if (!tmpJsonContent.empty())
		{
			if (tmpJsonContent.back() != 0)
			{
				tmpJsonContent.push_back(0);
			}

			modInfoTable = sqParseJson(tmpJsonContent.data());
		}

		modtable.Bind(_SC("ModInfo"), modInfoTable);
		std::string modName = itea.first;
		if (!modInfoTable.IsNull() && modInfoTable.HasKey(_SC("modid")))
		{
			modName = modInfoTable.GetSlot(_SC("modid")).Cast<Sqrat::string>();
			ModID_OriginNameMap[modName] = itea.first;
		}
		tmpmodtable.Bind(modName.c_str(), modtable);

		for (auto& itea2 : itea.second)
		{
			Sqrat::Script script;
			std::string str(itea2.second.begin(), itea2.second.end());
			std::string errorinfo;

			if (ModID_OriginNameMap.find(itea.first) != ModID_OriginNameMap.end())
			{
				script.CompileString(str, errorinfo, itea.first + "(modid=" + modName + "):" + itea2.first);
			}
			else
			{
				script.CompileString(str, errorinfo, itea.first + ":" + itea2.first);
			}
			
			if (!errorinfo.empty())
			{
				natLog::GetInstance().LogWarn(natUtil::FormatString(_T(R"(Script "%s" in modpack "%s" failed to compile, errinfo: %s.)"), itea2.first.c_str(), itea.first.c_str(), errorinfo.c_str()).c_str());
			}

			if (!sq_isnull(script.GetObject())) {
				SQInteger top = sq_gettop(vm);
				sq_pushobject(vm, script.GetObject());
				sq_pushobject(vm, modtable.GetObject());
				if (SQ_FAILED(sq_call(vm, 1, false, true)))
				{
					auto Errinfo = Sqrat::LastErrorString(vm);
					natLog::GetInstance().LogWarn(natUtil::FormatString(_T(R"(Script "%s" in modpack "%s" failed to execute, errinfo: %s.)"), itea2.first.c_str(), itea.first.c_str(), Errinfo.c_str()).c_str());
				}

				sq_settop(vm, top);
			}
		}
	}

	Sqrat::Object tablemodloader = Sqrat::RootTable().GetSlot(_SC("modloader"));
	if (!tablemodloader.IsNull())
	{
		tablemodloader.Cast<Sqrat::Table>().Bind(_SC("modpacks"), tmpmodtable);
	}
	else
	{
		natLog::GetInstance().LogErr(_T("Failed to find modloader table."));
	}

	return tmpScript.size();
}
static Sqrat::Array GetModPackList()
{
	Sqrat::Array ret(Sqrat::DefaultVM::Get(), tmpScript.size());
	Sqrat::Table ModPackTable = Sqrat::RootTable().GetSlot(_SC("modloader")).GetSlot(_SC("modpacks"));

	Sqrat::Object::iterator itea;
	
	while (ModPackTable.Next(itea))
	{
		ret.Append(itea.getName());
	}

	return ret;
}
Sqrat::string GetResourceLocation(const SQChar* ModID, const SQChar* Path)
{
	auto itea = ModID_OriginNameMap.find(ModID);
	return natUtil::FormatString(_T("mods/%s/resource/%s"), (itea == ModID_OriginNameMap.end() ? ModID : itea->second.c_str()), Path);
}

static void sqLogMsg(std::string const& str)
{
	natLog::GetInstance().LogMsg(str.c_str());
}
static void sqLogWarn(std::string const& str)
{
	natLog::GetInstance().LogWarn(str.c_str());
}
static void sqLogErr(std::string const& str)
{
	natLog::GetInstance().LogErr(str.c_str());
}

static void INJECT_STDCALL InjectSquirrel(LPVOID hSQVM)
{
	HSQUIRRELVM vm = (HSQUIRRELVM)hSQVM;
	Sqrat::DefaultVM::Set(vm);
	sq_setcompilererrorhandler(vm, CompileErrorHandler);
	Sqrat::RootTable rootTable(vm);

	Sqrat::Table modloader(vm);

	rootTable.Bind(_SC("modloader"), modloader);

	Sqrat::Class<Msgdata, Sqrat::NoConstructor<Msgdata>> sqMsgdata(vm, _SC("WndMsgdata"));
	sqMsgdata.Var(_SC("hWnd"), &Msgdata::hWnd);
	sqMsgdata.Var(_SC("uMsg"), &Msgdata::uMsg);
	sqMsgdata.Var(_SC("wParam"), &Msgdata::wParam);
	sqMsgdata.Var(_SC("lParam"), &Msgdata::lParam);
	rootTable.Bind(_SC("WndMsgdata"), sqMsgdata);

	Sqrat::Class<WndMsgEvent, Sqrat::NoConstructor<WndMsgEvent>> sqWndMsgEvent(vm, _SC("WndMsgEvent"));
	sqWndMsgEvent.Var(_SC("WndMsg"), &WndMsgEvent::m_WndMsg);
	rootTable.Bind(_SC("WndMsgEvent"), sqWndMsgEvent);

	rootTable.Func(_SC("LoadLibrary"), sqLoadLibrary);
	rootTable.Func(_SC("ReadModPack"), ReadModPack);
	rootTable.Func(_SC("GetModPackList"), GetModPackList);

	modloader.Func(_SC("GetResourceLocation"), GetResourceLocation);

	rootTable.Func(_SC("ParseJson"), sqParseJson);

	rootTable.Func(_SC("RegisterWndMsgHandler"), sqRegister);

	rootTable.Func(_SC("LogMsg"), sqLogMsg);
	rootTable.Func(_SC("LogWarn"), sqLogWarn);
	rootTable.Func(_SC("LogErr"), sqLogErr);
}
static void INJECT_STDCALL InjectedGameWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WndMsgEvent event(Msgdata { hwnd, uMsg, wParam, lParam });
	SQEventBus::GetInstance().Post(event);

	if (uMsg == WM_DESTROY)
	{
		Sqrat::RootTable().GetFunction(_SC("ExitGame")).Execute();
	}
}

static bool GetZipFile(LPCTSTR filename, std::unordered_map<std::string, std::vector<char>>& out)
{
	out.clear();
	unz_file_info64 FileInfo;
	unzFile zFile = unzOpen64(filename);

	if (!zFile)
	{
		return false;
	}

	unz_global_info64 gi;

	if (unzGetGlobalInfo64(zFile, &gi) != UNZ_OK)
	{
		unzClose(zFile);
		return false;
	}

	char file[256];
	char ext[256];
	char com[1024];
	char data[1024];
	int size;

	for (ZPOS64_T i = 0ull; i < gi.number_entry; ++i)
	{
		memset(file, 0, sizeof(file));
		memset(ext, 0, sizeof(ext));
		memset(com, 0, sizeof(com));
		if (unzGetCurrentFileInfo64(zFile, &FileInfo, file, sizeof(file), ext, sizeof(ext), com, sizeof(com)) != UNZ_OK)
		{
			unzClose(zFile);
			return false;
		}

		if (!(FileInfo.external_fa & FILE_ATTRIBUTE_DIRECTORY))
		{
			unzOpenCurrentFile(zFile);
			auto& filedata = out[file];

			while (true)
			{
				size = unzReadCurrentFile(zFile, data, sizeof(data));
				if (size <= 0)
				{
					break;
				}

				filedata.insert(filedata.end(), data, data + size);
			}

			unzCloseCurrentFile(zFile);
		}

		if (i < gi.number_entry - 1 && unzGoToNextFile(zFile) != UNZ_OK)
		{
			unzClose(zFile);
			return false;
		}
	}

	unzClose(zFile);
	return true;
}

static bool CreateGSPackFileList(std::unordered_map<std::string, std::unordered_map<std::string, std::vector<char>>>& out)
{
	out.clear();
	WIN32_FIND_DATA fd;
	HANDLE hzFile = FindFirstFile(_T("mods\\*.zip"), &fd);
	do
	{
		std::string DirName(fd.cFileName);
		auto& ZipFileList = out[DirName.substr(0u, DirName.find_last_of(_T('.')))];
		if (!GetZipFile((std::string(_T("mods\\")) + fd.cFileName).c_str(), ZipFileList))
		{
			FindClose(hzFile);
			return false;
		}
	} while (FindNextFile(hzFile, &fd) == TRUE);
	FindClose(hzFile);

	return true;
}

NMLPlugin plugin;

extern IDirect3D9* g_pDx9;
extern IDirect3DDevice9* g_pDxDevice;

bool sqLoadLibrary(const SQChar* lpDllName)
{
	HSQUIRRELVM sqvm = Sqrat::DefaultVM::Get();
	if (_access(lpDllName, 0) == -1)
	{
		std::stringstream ss;
		ss << "There isn't any DLL named \"" << lpDllName << "\"";
		sq_throwerror(sqvm, ss.str().c_str());
		return false;
	}

	HMODULE hDll = LoadLibrary(lpDllName);
	DWORD tlasterr;
	if ((tlasterr = GetLastError()) != ERROR_SUCCESS || !hDll)
	{
		std::stringstream ss;
		ss << "LoadLibrary failed, Dllname: \"" << lpDllName << "\", Lasterrno: " << tlasterr;
		MessageBox(NULL, ss.str().c_str(), _T("Error"), MB_ICONERROR);
		sq_throwerror(sqvm, ss.str().c_str());
		return false;
	}

	typedef int(*GetNativeLibraryInstanceProc)(NativeLibrary**);
	GetNativeLibraryInstanceProc pInitproc;
	if ((tlasterr = GetLastError()) == ERROR_SUCCESS && (pInitproc = reinterpret_cast<GetNativeLibraryInstanceProc>(GetProcAddress(hDll, "GetNativeLibraryInstance"))) != nullptr)
	{
		NativeLibrary* pLib;
		if (pInitproc(&pLib) != 0)
		{
			std::stringstream ss;
			ss << "GetNativeLibraryInstance failed, Dllname: \"" << lpDllName << "\"";
			MessageBox(NULL, ss.str().c_str(), _T("Error"), MB_ICONERROR);
			sq_throwerror(sqvm, ss.str().c_str());
			return false;
		}

		if (pLib->Init() != 0)
		{
			std::stringstream ss;
			ss << "NativeLibrary init failed, Dllname: \"" << lpDllName << "\"";
			MessageBox(NULL, ss.str().c_str(), _T("Error"), MB_ICONERROR);
			sq_throwerror(sqvm, ss.str().c_str());
			return false;
		}

		pLib->GetSQVM(sqvm);
		pLib->GetDxInterface(g_pDx9, g_pDxDevice);

		plugin.NativeLibraries.push_back(pLib);
	}
	else
	{
		std::stringstream ss;
		ss << "Cannot GetProcAddress of GetNativeLibraryInstance, Dllname: \"" << lpDllName << "\", Lasterrno: " << tlasterr;
		MessageBox(NULL, ss.str().c_str(), _T("Error"), MB_ICONERROR);
	}

	return true;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID /*lpReserved*/
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		plugin.Load(hModule);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		plugin.Unload();
		break;
	}
	return TRUE;
}
