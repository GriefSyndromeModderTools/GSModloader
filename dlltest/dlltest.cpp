// dlltest.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "dlltest.h"
#include <sqrat.h>
#include <natUtil.h>
#include <tchar.h>

int LaunchCommandSender(nTString const& strClientPath, nTString const& strPipeName)
{
	HANDLE hPipe;
	HANDLE hEvent;

	OVERLAPPED Ovlpd;

	hPipe = CreateNamedPipe(strPipeName.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, 0, 1, 1024, 1024, 0, NULL);
	if (INVALID_HANDLE_VALUE == hPipe)
	{
		MessageBox(NULL, natUtil::FormatString(_T("Cannot create named pipe. LastErr=%d"), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
		return -1;
	}

	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!hEvent)
	{
		CloseHandle(hPipe);
		MessageBox(NULL, natUtil::FormatString(_T("Cannot create event. LastErr=%d"), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
		return -1;
	}

	memset(&Ovlpd, 0, sizeof(OVERLAPPED));

	Ovlpd.hEvent = hEvent;

	STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	PROCESS_INFORMATION pi;
	nTChar tpCommandLine[MAX_PATH] = _T("");
	strcpy_s(tpCommandLine, strPipeName.c_str());
	if (!CreateProcess(strClientPath.c_str(), tpCommandLine, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		CloseHandle(hPipe);
		CloseHandle(hEvent);

		MessageBox(NULL, natUtil::FormatString(_T("CreateProcess failed. LastErr=%d"), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
		return -1;
	}

	if (!ConnectNamedPipe(hPipe, &Ovlpd))
	{
		DWORD tLastErr = GetLastError();
		if (ERROR_IO_PENDING != tLastErr)
		{
			CloseHandle(hPipe);
			CloseHandle(hEvent);

			MessageBox(NULL, natUtil::FormatString(_T("Cannot create event. LastErr=%d"), tLastErr).c_str(), _T("Error"), MB_ICONERROR);
			return -1;
		}
	}

	if (WAIT_FAILED == WaitForSingleObject(hEvent, INFINITE))
	{
		CloseHandle(hPipe);
		CloseHandle(hEvent);

		MessageBox(NULL, natUtil::FormatString(_T("Waiting for event failed. LastErr=%d"), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
		return -1;
	}

	CloseHandle(hEvent);

	while (true)
	{
		nLen tLen = 0ul;
		DWORD dwRead = 0ul;

		if (!ReadFile(hPipe, &tLen, sizeof(nLen), &dwRead, NULL) || dwRead != sizeof(nLen))
		{
			CloseHandle(hPipe);

			MessageBox(NULL, natUtil::FormatString(_T("Cannot get code length. LastErr=%d"), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
			return NatErr_InternalErr;
		}

		std::vector<nChar> lpCode(static_cast<nuInt>(tLen + 1));
		dwRead = 0ul;

		if (!ReadFile(hPipe, lpCode.data(), static_cast<nuInt>(tLen)* sizeof(nChar), &dwRead, NULL) || dwRead != tLen)
		{
			CloseHandle(hPipe);

			MessageBox(NULL, natUtil::FormatString(_T("Cannot get code. LastErr=%d"), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
			return NatErr_InternalErr;
		}

		if (lstrcmpi(lpCode.data(), _T("end")) == 0)
		{
			break;
		}

		Sqrat::Script tScript;
		Sqrat::string tError;
		if (!tScript.CompileString(lpCode.data(), tError))
		{
			CloseHandle(hPipe);

			MessageBox(NULL, natUtil::FormatString(_T("Compile script failed. ErrMsg: %s"), tError.c_str()).c_str(), _T("Error"), MB_ICONERROR);
			return NatErr_InternalErr;
		}

		if (!tScript.Run(tError))
		{
			CloseHandle(hPipe);

			MessageBox(NULL, natUtil::FormatString(_T("Compile script failed. ErrMsg: %s"), tError.c_str()).c_str(), _T("Error"), MB_ICONERROR);
			return NatErr_InternalErr;
		}
	}

	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	return NatErr_OK;
}

class RemoteCommand
	: public NativeLibrary
{
public:
	RemoteCommand() = default;
	~RemoteCommand() = default;

	int Init() override
	{
		return 0;
	}

	void Exit() override
	{
	}

	void GetSQVM(SQVM* hSQvm) override
	{
		Sqrat::DefaultVM::Set(static_cast<HSQUIRRELVM>(hSQvm));

		Sqrat::Table tModloader = Sqrat::RootTable().GetSlot(_SC("modloader")).Cast<Sqrat::Table>();
		if (!tModloader.IsNull())
		{
			tModloader.Func(_SC("LaunchCommandSender"), LaunchCommandSender);
		}
	}

	void GetDxInterface(IDirect3D9* /*pDx9*/, IDirect3DDevice9* /*pDxDevice*/) override
	{
	}

	bool HandlePresent() override
	{
		return false;
	}

	void PrePresent() override
	{
	}

	static RemoteCommand& GetInstance()
	{
		static RemoteCommand Instance;
		return Instance;
	}
};

int GetNativeLibraryInstance(NativeLibrary** ppNativeLib)
{
	if (ppNativeLib == nullptr)
	{
		return -1;
	}

	*ppNativeLib = &RemoteCommand::GetInstance();

	return 0;
}