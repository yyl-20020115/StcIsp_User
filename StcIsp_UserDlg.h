#pragma once
#include "CColorEdit.h"

constexpr int PAGE_SIZE = 0x80;
constexpr int MEMORY_SIZE = 0x10000;

#define DEFAULT_LEADING_ENABLE TRUE
constexpr unsigned char DEFAULT_LEADING_SYMBOL = 0x7f;
//USE DOUBLE LENGTH OF THE MCU
constexpr unsigned char DEFAULT_LEADING_SIZE = 0x80;
constexpr unsigned long DEFAULT_LEADING_DELAY = 100000;
constexpr unsigned long DEFAULT_OPERATING_DELAY = 100000;

enum class DataSource : int
{
	None = -1,
	MCU = 0,
	FILE = 1,
};
class CStcIspUserDlg : public CDialogEx
{
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
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedCheckAutotrace();
	afx_msg void OnBnClickedCheckAutodownload();
	afx_msg void OnCbnSelchangeComboComports();
	afx_msg void OnBnClickedButtonUpload();
	afx_msg void OnBnClickedButtonSaveFile();
protected:
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

	CString DownloadCodePath;
	CString UploadCodePath;
	CString LastWriteTime;

	DataSource Source;
	CWinThread* DownloadWorkerThread;
	CWinThread* UploadWorkerThread;
	HANDLE CommHandle;
	HANDLE QuitEvent;
	BOOL IsWorking;
	BOOL FileChanged;
	BOOL UseLeadings;
	unsigned char LeadingSymbol;
	unsigned int LeadingSize;
	unsigned char* CodeBuffer;
	unsigned long long CodeLength;
protected:
	static UINT DoDownload(LPVOID param);
	static UINT DoUpload(LPVOID param);
protected:
	BOOL SetAlwaysOnTop(BOOL aot);
	BOOL CheckAndLoadCodeFile(const CString& path, BOOL ShowMessage = TRUE);
	BOOL OpenCommPort(int port);	
	BOOL CloseCommPort();
	BOOL SendLeadings(unsigned char symbol, unsigned int size, int delay_us = DEFAULT_LEADING_DELAY) const;
	BOOL SendCommand(unsigned char function, unsigned int address = 0, unsigned char size = 0, unsigned char* buffer = nullptr);
	BOOL GetResponse(unsigned char* buffer, ULONGLONG max_delay_ms = DEFAULT_OPERATING_DELAY, unsigned char* payload_length_ptr = nullptr) const;
	unsigned char CalculateSum(unsigned char* buffer, int length);
	void AppendStatusText(const TCHAR* format = nullptr, ...);
	BOOL CheckAndUpdateCodeDisplay(unsigned char* code_buffer, unsigned int code_length);
	void UpdateCodeDisplay(unsigned char* code_buffer, unsigned int code_length,DataSource Source = DataSource::FILE);

	void UpdateCommPortsList();

};
