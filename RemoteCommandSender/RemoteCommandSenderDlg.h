
// RemoteCommandSenderDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
struct natStream;

// CRemoteCommandSenderDlg �Ի���
class CRemoteCommandSenderDlg : public CDialogEx
{
// ����
public:
	CRemoteCommandSenderDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_REMOTECOMMANDSENDER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	CHandle m_hPipe;
	natStream* m_pStream;
public:
	afx_msg void OnBnClickedBtnexe();
	afx_msg void OnBnClickedBtnclose();
	afx_msg void OnDestroy();
	CString m_StrCode;
	afx_msg void OnDropFiles(HDROP hDropInfo);
};
