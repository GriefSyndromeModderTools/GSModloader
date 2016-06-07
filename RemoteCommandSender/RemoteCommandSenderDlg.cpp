
// RemoteCommandSenderDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "RemoteCommandSender.h"
#include "RemoteCommandSenderDlg.h"
#include "afxdialogex.h"
#include <natUtil.h>
#include <natStream.h>
#include <natException.h>

#include "resource.h"

#ifdef _DEBUG
#	define new DEBUG_NEW
#endif

// CRemoteCommandSenderDlg �Ի���


CRemoteCommandSenderDlg::CRemoteCommandSenderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRemoteCommandSenderDlg::IDD, pParent), m_hPipe(NULL), m_pStream(nullptr)
	, m_StrCode(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteCommandSenderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EdtCode, m_StrCode);
}

BEGIN_MESSAGE_MAP(CRemoteCommandSenderDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BtnExe, &CRemoteCommandSenderDlg::OnBnClickedBtnexe)
	ON_BN_CLICKED(IDC_BtnClose, &CRemoteCommandSenderDlg::OnBnClickedBtnclose)
	ON_WM_DESTROY()
	ON_WM_DROPFILES()
END_MESSAGE_MAP()


// CRemoteCommandSenderDlg ��Ϣ�������

BOOL CRemoteCommandSenderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
	CString strPipeName = _T(R"(\\.\pipe\CodePipe)");

	if (argc >= 2)
	{
		strPipeName = argv[1];
	}

	if (!WaitNamedPipe(strPipeName, NMPWAIT_WAIT_FOREVER))
	{
		MessageBox(natUtil::FormatString(_T("Cannot connect to pipe. LastErr=%d"), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
		DestroyWindow();
		return TRUE;
	}

	try
	{
		m_pStream = new natFileStream(strPipeName, false, true);
	}
	catch (natException& e)
	{
		SafeRelease(m_pStream);
		MessageBox(natUtil::FormatString(_T("Failed to open pipe. In %s, description: %s, lasterr=%d"), e.GetSource(), e.GetDesc(), GetLastError()).c_str(), _T("Error"), MB_ICONERROR);
	}

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CRemoteCommandSenderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CRemoteCommandSenderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteCommandSenderDlg::OnBnClickedBtnexe()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	UpdateData();
	if (!m_pStream || !m_pStream->CanWrite())
	{
		MessageBox(_T("Pipe has not initialized."), _T("Error"), MB_ICONERROR);
	}
	
	if (m_StrCode.CompareNoCase(_T("end")) == 0)
	{
		MessageBox(_T("This command is reserved."), _T("Error"), MB_ICONERROR);
		m_StrCode = _T("");
		UpdateData(FALSE);
		return;
	}

	nLen tLen = m_StrCode.GetLength();
	m_pStream->WriteBytes(reinterpret_cast<ncData>(&tLen), sizeof(nLen));
	m_pStream->WriteBytes(ncData(natUtil::W2Cstr(static_cast<LPCTSTR>(m_StrCode)).c_str()), tLen);

	MessageBox(_T("Sended command successfully."), _T("Result"), MB_ICONINFORMATION);
}


void CRemoteCommandSenderDlg::OnBnClickedBtnclose()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	DestroyWindow();
}


void CRemoteCommandSenderDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  �ڴ˴������Ϣ����������
	UpdateData();
	if (!m_pStream || !m_pStream->CanWrite())
	{
		return;
	}

	static ncStr tExitCommand = "end";
	nLen tLen = strlen(tExitCommand);
	m_pStream->WriteBytes(reinterpret_cast<ncData>(&tLen), sizeof(nLen));
	m_pStream->WriteBytes(ncData(tExitCommand), tLen);

	SafeRelease(m_pStream);
}


void CRemoteCommandSenderDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	nTChar lpPath[MAX_PATH + 1] = _T("");
	DragQueryFile(hDropInfo, 0, lpPath, MAX_PATH + 1);

	natStream* pStream = new natFileStream(lpPath, true, false);
	nLen tLen = pStream->GetSize();
	nStr lpData = new nChar[static_cast<nuInt>(tLen + 1)];
	lpData[tLen] = 0;
	if (pStream->ReadBytes(reinterpret_cast<nData>(lpData), tLen) == tLen && pStream->GetLastErr() == NatErr_OK)
	{
		m_StrCode = lpData;
		UpdateData(FALSE);
	}
	SafeDelArr(lpData);
	pStream->Release();

	DragFinish(hDropInfo);
	
	CDialogEx::OnDropFiles(hDropInfo);
}
