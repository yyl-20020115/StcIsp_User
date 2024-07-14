#pragma once
#include "CColorEdit.h"

#define PAGE_SIZE	0x80
#define MEMORY_SIZE 0x10000

#define DEFAULT_LEADING_ENABLE TRUE
#define DEFAULT_LEADING_SYMBOL 0x7f
#define DEFAULT_LEADING_SIZE 0x40
#define DEFAULT_LEADING_DELAY 50000

enum class CodeType : int
{
	None = -1,
	DownloadingCode = 0,
	UploadedCode = 1,
};
class CStcIspUserDlg : public CDialogEx
{
public:
	static UINT DoDownload(LPVOID param);
	static UINT DoUpload(LPVOID param);
public:
	CStcIspUserDlg(CWnd* pParent = nullptr);	// 标准构造函数
	~CStcIspUserDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_STCISP_USER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
protected:
	HICON m_hIcon;
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
	CColorEdit StatusEdit;
	CComboBox ComboPorts;
	CButton StopButton;
	CButton DownloadButton;
	CButton OpenButton;
	CButton SaveButton;
	CButton UploadButton;
	CButton AutoTraceCheckBox;
	CButton AutoDownloadCheckBox;
	CProgressCtrl ProgressDownload;
protected:
	CString DownloadCodePath;
	CString UploadCodePath;
	CString LastMD5;
	HANDLE CommHandle;
	BOOL IsWorking;
	BOOL FileChanged;
	BOOL UseLeadings;
	unsigned char LeadingSymbol;
	unsigned int LeadingSize;
	unsigned char* CodeBuffer;
	CodeType _CodeType;
	unsigned long long CodeLength;
	HANDLE QuitEvent;
	CWinThread* DownloadWorkerThread;
	CWinThread* UploadWorkerThread;
protected:
	BOOL CheckAndLoadCodeFile(const CString& path, BOOL ShowMessage = TRUE);
	BOOL DoCloseHandle();
	BOOL OpenComPort(int port);	
	BOOL SendLeandings(unsigned char symbol, unsigned int size, int delay_us = DEFAULT_LEADING_DELAY);
	BOOL SendCommand(unsigned char function, unsigned int address = 0, unsigned char size = 0, unsigned char* buffer = nullptr);
	BOOL GetResponse(unsigned char* buffer, ULONGLONG max_delay_ms = 0, unsigned char* payload_length_ptr = nullptr) const;
	unsigned char Sum(unsigned char* buffer, int length);
	void AppendStatusText(const TCHAR* format = nullptr, ...);
	BOOL CheckAndUpdateCodeDisplay(unsigned char* code_buffer, unsigned int code_length);
	void UpdateCodeDisplay(unsigned char* code_buffer, unsigned int code_length);
public:
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedCheckAutotrace();
	afx_msg void OnBnClickedCheckAutodownload();
	afx_msg void OnCbnSelchangeComboComports();
	afx_msg void OnBnClickedButtonUpload();
	afx_msg void OnBnClickedButtonSaveFile();
};
