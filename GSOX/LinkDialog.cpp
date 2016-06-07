#include "stdafx.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static BOOL CALLBACK DialogProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam){
	switch (iMessage){
	case WM_COMMAND:
		switch (LOWORD(wParam)){
		case IDC_BUTTON_SERVER:
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT_LOG), _T("HOST"));
			break;
		case IDC_BUTTON_CLIENT:
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT_LOG), _T("CLIENT"));
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		//exit(0);
		return TRUE;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return TRUE;
	}
	return FALSE;
}

static void ShowLinkDialog(HINSTANCE hInstance)
{
	HWND hDialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_LINKDIALOG), NULL, DialogProc);
	HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HOMURA));
	SendMessage(hDialog, WM_SETICON, WPARAM(ICON_SMALL), LPARAM(hIcon));
	ShowWindow(hDialog, SW_SHOW);

	HWND hWnd = CreateWindow(_T("BUTTON"), _T(""),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		500, 100,
		NULL, NULL, hInstance, NULL);
	RECT pos;
	GetWindowRect(hWnd, &pos);
	SetWindowPos(hDialog, HWND_NOTOPMOST, pos.left, pos.top, CW_USEDEFAULT, CW_USEDEFAULT, SWP_NOSIZE);
	DestroyWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void INJECT_CDECL CreateWindowExA_Before(
	DWORD dwExStyle,
	LPCSTR lpClassName,
	LPCSTR lpWindowName)
{
	lpWindowName = "GriefSyndrome Online X";
	ShowLinkDialog(GSOXPlugin::GetInstance().hInstanceDll);
}
