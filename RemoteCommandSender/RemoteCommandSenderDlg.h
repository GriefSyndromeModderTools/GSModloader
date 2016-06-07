
// RemoteCommandSenderDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
struct natStream;

// CRemoteCommandSenderDlg 对话框
class CRemoteCommandSenderDlg : public CDialogEx
{
// 构造
public:
	CRemoteCommandSenderDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_REMOTECOMMANDSENDER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
