// dlltest.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "dlltest.h"
#include <sqrat.h>
#include <natUtil.h>
#include <natNamedPipe.h>
#include <tchar.h>

namespace
{
	int LaunchCommandSender(nTString const& strClientPath, nTString const& strPipeName)
	{
		natNamedPipeServerStream stream(strPipeName.c_str(), PipeDirection::In, 1);

		STARTUPINFO si;
		memset(&si, 0, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);

		PROCESS_INFORMATION pi;
		nTChar tpCommandLine[MAX_PATH] = _T("");
		strcpy_s(tpCommandLine, strPipeName.c_str());
		if (!CreateProcess(strClientPath.c_str(), tpCommandLine, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
		{
			MessageBox(NULL, natUtil::FormatString(_T("CreateProcess failed. LastErr=%d"), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
			return -1;
		}

		stream.WaitForConnection();

		while (true)
		{
			nLen tLen = 0ul;

			stream.ReadBytes(reinterpret_cast<nData>(&tLen), sizeof(nLen));
			if (NATFAIL(stream.GetLastErr()))
			{
				MessageBox(NULL, natUtil::FormatString(_T("Cannot get code length. LastErr=%d"), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
				return NatErr_InternalErr;
			}

			std::vector<nChar> lpCode(static_cast<nuInt>(tLen + 1));

			stream.ReadBytes(reinterpret_cast<nData>(lpCode.data()), tLen);
			if (NATFAIL(stream.GetLastErr()))
			{
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
				MessageBox(NULL, natUtil::FormatString(_T("Compile script failed. ErrMsg: %s"), tError.c_str()).c_str(), _T("Error"), MB_ICONERROR);
				return NatErr_InternalErr;
			}

			if (!tScript.Run(tError))
			{
				MessageBox(NULL, natUtil::FormatString(_T("Compile script failed. ErrMsg: %s"), tError.c_str()).c_str(), _T("Error"), MB_ICONERROR);
				return NatErr_InternalErr;
			}
		}

		return NatErr_OK;
	}
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