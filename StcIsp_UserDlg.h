
// StcIsp_UserDlg.h: 头文件
//

#pragma once
#include "CColorEdit.h"

#define PAGE_SIZE 128

// CStcIspUserDlg 对话框
class CStcIspUserDlg : public CDialogEx
{
public:
	static UINT DoDownload(LPVOID param);
// 构造
public:
	CStcIspUserDlg(CWnd* pParent = nullptr);	// 标准构造函数
	~CStcIspUserDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_STCISP_USER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonOpenFile();
	afx_msg void OnBnClickedButtonDownload();
	afx_msg void OnBnClickedButtonStop();
	CColorEdit HexEdit;
	CComboBox ComboPorts;
	CStatic StatusText;
	CProgressCtrl ProgressDownload;
	CButton StopButton;
	CButton DownloadButton;
	CButton OpenButton;
protected:
	BOOL IsCodeReady;
	BOOL IsCodeHex;
	CString CodePath;
	HANDLE CommHandle;
	BOOL IsWorking;
	unsigned char* CodeBuffer;
	unsigned long long CodeLength;
	HANDLE QuitEvent;
	CWinThread* WorkingThread;
protected:

	BOOL CheckAndLoadCodeFile(const CString& path, BOOL IsHex);

	BOOL DoCloseHandle();

	BOOL OpenComm(int port);
	
	BOOL WriteComm(unsigned char function, unsigned int value, unsigned char length, unsigned char buffer[PAGE_SIZE]);

	BOOL ReadComm(unsigned char buffer[PAGE_SIZE], ULONGLONG ticks) const;

	unsigned char Sum(unsigned char* buffer, int length);

	void SetStatusText(const TCHAR* format = nullptr, ...);
public:
	afx_msg void OnClose();
};
