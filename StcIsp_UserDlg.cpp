#include "pch.h"
#include "framework.h"
#include "StcIsp_User.h"
#include "StcIsp_UserDlg.h"
#include "afxdialogex.h"
#include "MD5Checksum.h"
#include <vector>
#include <chrono>
#include <algorithm>
#define CMD_HEAD_SIGN '#'
#define REPLY_HEAD_SIGN '@'
#define TAIL_SIGN '$'
#define BLOCK_SIZE 4096

#define DFU_CMD_CONNECT         0xa0
#define DFU_CMD_READ            0xa1
#define DFU_CMD_PROGRAM         0xa2
#define DFU_CMD_ERASE           0xa3
#define DFU_CMD_REBOOT          0xa4

#define STATUS_OK               0x00
#define STATUS_ERRORCMD         0x01
#define STATUS_OUTOFRANGE       0x02
#define STATUS_PROGRAMERR       0x03
#define STATUS_ERRORWRAP        0xff

#define REFRESH_AUTOTRACE_TIMER_ID			0x100
#define REFRESH_AUTODOWNLOAD_TIMER_ID		0x200
#define REFRESH_TIMER_INTERVAL	500
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
static CString GetExtension(const CString& Path) {
	int p = Path.ReverseFind(_T('.'));
	return p >= 0 ? Path.Mid(p) : _T("");
}
static CString GetFileMD5(const CString& fileName)
{
	CString MD5Return;
	CFileFind  finder;

	if (!finder.FindFile(fileName))
	{
		return _T("");
	}

	CFile file(fileName, CFile::typeBinary | CFile::modeRead | CFile::shareDenyNone);

	MD5Return = CMD5Checksum::GetMD5(file, TRUE);

	file.Close();

	return MD5Return;

}
static void GetSerialPorts(std::vector<int>& ports, DWORD maxlen = 1ULL << 20)
{
	//Make sure we clear out any elements which may already be in the array
	ports.clear();
	//Use QueryDosDevice to look for all devices of the form COMx. This is a better
	//solution as it means that no ports have to be opened at all.
	TCHAR* szDevices = new TCHAR[maxlen];
	if (szDevices != nullptr) {
		memset(szDevices, 0, maxlen * sizeof(TCHAR));

		DWORD dwChars = QueryDosDevice(NULL, szDevices, maxlen);
		if (dwChars)
		{
			int i = 0;

			for (;;)
			{
				//Get the current device name
				TCHAR* pszCurrentDevice = &szDevices[i];

				//If it looks like "COMX" then
				//add it to the array which will be returned
				size_t nLen = _tcslen(pszCurrentDevice);
				if (nLen > 3 && _tcsnicmp(pszCurrentDevice, _T("COM"), 3) == 0)
				{
					//Work out the port number
					int nPort = _ttoi(&pszCurrentDevice[3]);
					ports.push_back(nPort);
				}

				// Go to next NULL character
				while (szDevices[i] != _T('\0'))
					i++;

				// Bump pointer to the next string
				i++;

				// The list is double-NULL terminated, so if the character is
				// now NULL, we're at the end
				if (szDevices[i] == _T('\0'))
					break;
			}
		}
		delete[] szDevices;
	}
	std::sort(ports.begin(), ports.end());
}

UINT CStcIspUserDlg::DoUpload(LPVOID param) {
	if (param == nullptr) return 0;
	CStcIspUserDlg* _this = reinterpret_cast<CStcIspUserDlg*>(param);
	int index = _this->ComboPorts.GetCurSel();
	if (index < 0) return 0;
	_this->IsWorking = TRUE;
	int page_size = 0;
	unsigned int address = 0;
	unsigned char payload_length = 0;
	unsigned char* ptr = nullptr;
	unsigned char buffer[PAGE_SIZE] = { 0 };
	DWORD WaitResult = 0;
	_this->AppendStatusText();
	_this->ComboPorts.EnableWindow(FALSE);
	_this->OpenButton.EnableWindow(FALSE);
	_this->DownloadButton.EnableWindow(FALSE);
	_this->UploadButton.EnableWindow(FALSE);
	_this->StopButton.EnableWindow(TRUE);

	_this->ProgressDownload.SetRange32(0, MEMORY_SIZE);
	_this->ProgressDownload.SetPos(0);

	int com_number = (int)_this->ComboPorts.GetItemData(index);
	if (!_this->OpenComPort(com_number))
	{
		AfxMessageBox(_T("端口打开失败 !"), 0, 0);
	}
	else
	{
		_this->AppendStatusText(_T("连接目标芯片 ..."));
		if (!_this->SendCommand(DFU_CMD_CONNECT, 0, 0, 0) || !_this->GetResponse(buffer, 100))
		{
			_this->AppendStatusText(_T("连接失败 !"));
		}
		else
		{
			unsigned char* code_buffer = new unsigned char[MEMORY_SIZE];
			memset(code_buffer, 0xff, MEMORY_SIZE);
			_this->AppendStatusText(_T("连接目标芯垃成功 !(固件版本: %d.%d)"), buffer[0], buffer[1]);
			_this->AppendStatusText(_T("正在上传代码 ... "));
			while ((WaitResult = WaitForSingleObject(_this->QuitEvent, 0)) != WAIT_OBJECT_0)
			{
				ptr = code_buffer + address;
				page_size = PAGE_SIZE;
				memcpy(ptr, buffer, page_size);
				memset(buffer, 0xff, PAGE_SIZE);
				//read PAGE_SIZE bytes from address
				if (!_this->SendCommand(DFU_CMD_READ, address, PAGE_SIZE, 0)
					|| !_this->GetResponse(buffer, 100, &payload_length))
				{
					_this->AppendStatusText(_T("上传失败 !"));
					_this->DoCloseHandle();
					break;
				}
				if (payload_length != PAGE_SIZE) {
					payload_length = payload_length;
				}
				memcpy(ptr, buffer, payload_length);
				address += payload_length;
				_this->ProgressDownload.SetPos(address);

				if (address >= MEMORY_SIZE)
				{
					_this->DoCloseHandle();
					_this->AppendStatusText(_T("代码上传成功 !"));
					break;
				}
			}

			_this->UpdateCodeDisplay(code_buffer, MEMORY_SIZE);
		}
	}

	::InterlockedExchangePointer((void**)&_this->UploadWorkerThread, nullptr);
	if (WaitResult == WAIT_OBJECT_0) {
		_this->AppendStatusText(_T("代码上传被终止 !"));
	}
	_this->OpenButton.EnableWindow(TRUE);
	_this->StopButton.EnableWindow(FALSE);
	_this->DownloadButton.EnableWindow(_this->CodeBuffer != nullptr);
	_this->UploadButton.EnableWindow(TRUE);
	_this->SaveButton.EnableWindow(_this->CodeBuffer != nullptr);
	_this->ComboPorts.EnableWindow(TRUE);

	_this->IsWorking = FALSE;
	return 0;
}
UINT CStcIspUserDlg::DoDownload(LPVOID param) {
	if (param == nullptr) return 0;
	CStcIspUserDlg* _this = reinterpret_cast<CStcIspUserDlg*>(param);
	if (_this->CodeLength == 0 || _this->CodeBuffer == nullptr) return 0;
	int index = _this->ComboPorts.GetCurSel();
	if (index < 0) return 0;
	_this->IsWorking = TRUE;
	unsigned char page_size = 0;
	unsigned int address = 0;
	unsigned char* ptr = nullptr;
	unsigned char buffer[PAGE_SIZE] = { 0 };
	DWORD WaitResult = 0;
	_this->AppendStatusText();
	_this->ComboPorts.EnableWindow(FALSE);
	_this->OpenButton.EnableWindow(FALSE);
	_this->DownloadButton.EnableWindow(FALSE);
	_this->UploadButton.EnableWindow(FALSE);
	_this->StopButton.EnableWindow(TRUE);

	_this->ProgressDownload.SetRange32(0, (int)_this->CodeLength);
	_this->ProgressDownload.SetPos(0);

	int com_number = (int)_this->ComboPorts.GetItemData(index);
	if (!_this->OpenComPort(com_number))
	{
		AfxMessageBox(_T("端口打开失败 !"), 0, 0);
	}
	else
	{
		_this->AppendStatusText(_T("连接目标芯片 ..."));
		if (!_this->SendCommand(DFU_CMD_CONNECT, 0, 0, 0) || !_this->GetResponse(buffer, 100))
		{
			_this->AppendStatusText(_T("连接失败 !"));
		}
		else
		{
			_this->AppendStatusText(_T("连接目标芯垃成功 !(固件版本: %d.%d)"), buffer[0], buffer[1]);
			_this->AppendStatusText(_T("正在擦除芯片 ... "));
			if (!_this->SendCommand(DFU_CMD_ERASE, 0, 0, 0) || !_this->GetResponse(buffer, 5000))
			{
				_this->AppendStatusText(_T("擦除失败 !"));
			}
			else
			{
				_this->AppendStatusText(_T("正在下载代码 ... "));
				while ((WaitResult = WaitForSingleObject(_this->QuitEvent, 0)) != WAIT_OBJECT_0)
				{
					ptr = _this->CodeBuffer + address;
					if (*ptr == 0xff) //skip 0xff bytes
					{
						++address;
					}
					else
					{
						page_size = PAGE_SIZE;
						if (address < MEMORY_SIZE && address + PAGE_SIZE > MEMORY_SIZE)
						{
							page_size = MEMORY_SIZE - address;
						}
						memcpy(buffer, ptr, page_size);
						if (!_this->SendCommand(DFU_CMD_PROGRAM, address, page_size, buffer)
							|| !_this->GetResponse(buffer, 100))
						{
							_this->AppendStatusText(_T("下载失败 !"));
							_this->DoCloseHandle();
							break;
						}
						address += page_size;

						_this->ProgressDownload.SetPos(address);
					}
					if (address >= _this->CodeLength)
					{
						_this->SendCommand(DFU_CMD_REBOOT, 0, 0, 0);
						_this->DoCloseHandle();
						_this->AppendStatusText(_T("代码下载成功 !"));
						break;
					}
				}
			}
		}
	}

	::InterlockedExchangePointer((void**)&_this->DownloadWorkerThread, nullptr);
	if (WaitResult == WAIT_OBJECT_0) {
		_this->AppendStatusText(_T("代码下载被终止 !"));
	}
	_this->OpenButton.EnableWindow(TRUE);
	_this->StopButton.EnableWindow(FALSE);
	_this->DownloadButton.EnableWindow(_this->CodeBuffer != nullptr);
	_this->SaveButton.EnableWindow(_this->CodeBuffer != nullptr);
	_this->UploadButton.EnableWindow(_this->CodeBuffer != nullptr);
	_this->ComboPorts.EnableWindow(TRUE);
	_this->IsWorking = FALSE;
	return 0;
}

BOOL CStcIspUserDlg::CheckAndLoadCodeFile(const CString& path, BOOL ShowMessage)
{
	if (path.IsEmpty()) return FALSE;
	CString ext = GetExtension(path);
	ext.MakeLower();
	BOOL IsHex = ext == _T(".hex");
	unsigned int input_text_length; // esi
	char* input_text_buffer; // ebp
	char* token; // esi
	char ch; // dl
	unsigned int length;
	unsigned int address;
	unsigned int code_length = 0;
	unsigned char* code_buffer;
	unsigned char type;
	unsigned char check_sum;
	unsigned char real_sum;
	CFile file;
	if (!file.Open(path, CFile::shareDenyNone | CFile::typeBinary, 0))
	{
		if (ShowMessage)
			AfxMessageBox(_T("打开文件失败 !"), 0, 0);
		return FALSE;
	}
	input_text_length = (unsigned int)file.GetLength();
	input_text_buffer = new char[input_text_length + 1];
	file.Read(input_text_buffer, (UINT)input_text_length);
	input_text_buffer[input_text_length] = 0;

	code_buffer = new unsigned char[MEMORY_SIZE];
	memset(code_buffer, 0xff, MEMORY_SIZE);

	if (IsHex) {
		token = strtok(input_text_buffer, "\r\n");
		if (token != 0)
		{
			char local[5] = { 0 };
			do
			{
				ch = *token;
				//长度 地址 类型 数据 校验和
				//:BBAAAATTHHHHHHHH......HHCC
				//: START
				//BB: Length of HH bytes
				//AAAA: Address of data
				//TT: Type, 00:Record;01:HEX End with checksum,02:Ext Address;03:Start Segment;04:Ext Linear;05:Start Linear(entry point)
				//HH: Hex data
				//CC: Check sum
				if (ch == ':')
				{
					real_sum = 0;
					local[0] = *(++token);
					local[1] = *(++token);
					length = strtoul(local, 0, 16);
					real_sum += (char)length;
					memset(local, 0, sizeof(local));
					local[0] = *(++token);
					local[1] = *(++token);
					local[2] = *(++token);
					local[3] = *(++token);
					address = strtoul(local, 0, 16);
					real_sum += (char)((address >> 8) & 0xff);
					real_sum += (char)(address & 0xff);
					memset(local, 0, sizeof(local));
					local[0] = *(++token);
					local[1] = *(++token);
					type = (unsigned char)strtoul(local, 0, 16);
					real_sum += type;
					//only process type=0 (data) because the input hex file is simple
					if (type == 0 && length > 0)
					{
						memset(local, 0, sizeof(local));
						for (unsigned int i = 0; i < length; i++)
						{
							local[0] = *(++token);
							local[1] = *(++token);
							unsigned char value = (unsigned char)strtoul(local, 0, 16);
							*((unsigned char*)code_buffer + (address++)) = value;
							real_sum += value;
						}
						if (address > code_length) {
							code_length = address;
						}

						memset(local, 0, sizeof(local));
						local[0] = *(++token);
						local[1] = *(++token);

						check_sum = (unsigned char)strtoul(local, 0, 16);
						//check the checksum
						unsigned char sum = real_sum + check_sum;
						if (sum != 0)
						{
							delete[] code_buffer;
							delete[] input_text_buffer;
							if (ShowMessage)
								AfxMessageBox(_T("HEX文件数据错误 !"), 0, 0);
							return FALSE;
						}
					}
				}
				token = strtok(0, "\r\n");
			} while (token != 0);
		}
	}
	else {
		code_length = input_text_length > MEMORY_SIZE ? MEMORY_SIZE : input_text_length;
		memcpy(code_buffer, input_text_buffer, code_length);
	}
	delete[] input_text_buffer;
	if (this->CheckAndUpdateCodeDisplay(code_buffer, code_length)) {
		this->DownloadCodePath = path;
		this->AppendStatusText(path);
		return TRUE;
	}
	delete[] code_buffer;
	if (ShowMessage)
		AfxMessageBox(_T("代码文件不规范, 无法加载 !"), 0, 0);
	return FALSE;
}

BOOL CStcIspUserDlg::CheckAndUpdateCodeDisplay(unsigned char* code_buffer, unsigned int code_length)
{
	//here is for binary
	if (code_buffer != nullptr && code_buffer[0] == 0x02)
	{
		unsigned int code_length_at_header = (((unsigned short)code_buffer[1]) << 8) | (code_buffer[2]);
		if (code_length_at_header >= (BLOCK_SIZE + 3))
		{
			int index = 3;
			int cs = 0;
			int fills = 0;
			while (code_buffer[index++] == 0xff) cs++;
			if (cs == 0) {
				fills = BLOCK_SIZE;
				index = 3;
				while (code_buffer[index++] == 0) cs++;
			}
			if (cs >= BLOCK_SIZE && index >= BLOCK_SIZE + 3)
			{
				//relocate first 3 bytes
				code_buffer[BLOCK_SIZE + 0] = code_buffer[0];
				code_buffer[BLOCK_SIZE + 1] = code_buffer[1];
				code_buffer[BLOCK_SIZE + 2] = code_buffer[2];
				//make original 3 byte 0xff
				if (fills > 0) {
					memset(code_buffer, 0xff, fills);
				}
				code_buffer[0] = 0xff;
				code_buffer[1] = 0xff;
				code_buffer[2] = 0xff;
				this->UpdateCodeDisplay(code_buffer, code_length);

				return TRUE;
			}
		}
	}

	return FALSE;
}

void CStcIspUserDlg::UpdateCodeDisplay(unsigned char* code_buffer, unsigned int code_length)
{
	if (code_buffer != nullptr && code_length > 0)
	{
		CString display_text_buffer;
		unsigned int address = 0;
		do
		{
			if (0 == (address % 16)) {
				CString addressHex;
				addressHex.Format(_T("%04X "), address);
				display_text_buffer += addressHex;
			}
			{
				CString dataHex;
				dataHex.Format(_T("%02X "), (unsigned char)code_buffer[address]);
				display_text_buffer += dataHex;
			}
			if (address % 16 == 15) {
				display_text_buffer += _T("\r\n");
			}
			++address;
		} while (address < code_length);
		this->HexEdit.SetWindowText(display_text_buffer);
		if (this->CodeBuffer != nullptr)
		{
			delete[] this->CodeBuffer;
			this->CodeBuffer = 0;
		}
		this->CodeBuffer = code_buffer;
		this->CodeLength = code_length;
	}
}

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CStcIspUserDlg::CStcIspUserDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_STCISP_USER_DIALOG, pParent)
	, DownloadCodePath()
	, UploadCodePath()
	, LastMD5()
	, CommHandle(INVALID_HANDLE_VALUE)
	, IsWorking(FALSE)
	, FileChanged(FALSE)
	, CodeBuffer(nullptr)
	, CodeLength(0)
	, QuitEvent(INVALID_HANDLE_VALUE)
	, DoneEvent(INVALID_HANDLE_VALUE)
	, DownloadWorkerThread(nullptr)
	, UploadWorkerThread(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	this->QuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->DoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CStcIspUserDlg::~CStcIspUserDlg()
{
	if (this->CodeBuffer != nullptr) {
		delete[] this->CodeBuffer;
		this->CodeBuffer = nullptr;
	}
	this->CodeLength = 0;
	if (this->QuitEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(this->QuitEvent);
		this->QuitEvent = INVALID_HANDLE_VALUE;
	}
	if (this->DoneEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(this->DoneEvent);
		this->DoneEvent = INVALID_HANDLE_VALUE;
	}
}

void CStcIspUserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_COMPORTS, ComboPorts);
	DDX_Control(pDX, IDC_PROGRESS_DOWNLOAD, ProgressDownload);
	DDX_Control(pDX, IDC_BUTTON_STOP, StopButton);
	DDX_Control(pDX, IDC_BUTTON_DOWNLOAD, DownloadButton);
	DDX_Control(pDX, IDC_BUTTON_OPEN_FILE, OpenButton);
	DDX_Control(pDX, IDC_EDIT_HEX, HexEdit);
	DDX_Control(pDX, IDC_CHECK_AUTOTRACE, AutoTraceCheckBox);
	DDX_Control(pDX, IDC_CHECK_AUTODOWNLOAD, AutoDownloadCheckBox);
	DDX_Control(pDX, IDC_EDIT_STATUS, StatusEdit);
	DDX_Control(pDX, IDC_BUTTON_SAVE_FILE, SaveButton);
	DDX_Control(pDX, IDC_BUTTON_UPLOAD, UploadButton);
}

BEGIN_MESSAGE_MAP(CStcIspUserDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	ON_BN_CLICKED(IDCANCEL, &CStcIspUserDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_OPEN_FILE, &CStcIspUserDlg::OnBnClickedButtonOpenFile)
	ON_BN_CLICKED(IDC_BUTTON_DOWNLOAD, &CStcIspUserDlg::OnBnClickedButtonDownload)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CStcIspUserDlg::OnBnClickedButtonStop)
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK_AUTOTRACE, &CStcIspUserDlg::OnBnClickedCheckAutotrace)
	ON_BN_CLICKED(IDC_CHECK_AUTODOWNLOAD, &CStcIspUserDlg::OnBnClickedCheckAutodownload)
	ON_CBN_SELCHANGE(IDC_COMBO_COMPORTS, &CStcIspUserDlg::OnCbnSelchangeComboComports)
	ON_BN_CLICKED(IDC_BUTTON_UPLOAD, &CStcIspUserDlg::OnBnClickedButtonUpload)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_FILE, &CStcIspUserDlg::OnBnClickedButtonSaveFile)
END_MESSAGE_MAP()

BOOL CStcIspUserDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);
	this->OpenButton.EnableWindow(TRUE);
	this->UploadButton.EnableWindow(TRUE);
	this->DownloadButton.EnableWindow(FALSE);
	this->StopButton.EnableWindow(FALSE);
	this->SaveButton.EnableWindow(FALSE);

	this->HexEdit.SetBackColor(RGB(0xff, 0xff, 0xff));
	std::vector<int> ports;
	GetSerialPorts(ports);
	for (int i = 0; i < ports.size(); i++) {
		CString com_name;
		int com_number = ports[i];
		com_name.Format(_T("COM%d"), com_number);
		int index = this->ComboPorts.AddString(com_name);
		if (index >= 0) {
			this->ComboPorts.SetItemData(index, com_number);
		}
	}
	if (this->ComboPorts.GetCount() > 0) {
		int index = 0;
		CString text = theApp.GetProfileString(_T("Config"), _T("COMPort"), _T(""));
		if (!text.IsEmpty())
		{
			index = this->ComboPorts.FindStringExact(-1, text);
		}
		this->ComboPorts.SetCurSel(index >= 0 ? index : 0);
	}

	this->DownloadCodePath = theApp.GetProfileString(_T("Config"), _T("DownloadPath"), _T(""));
	this->UploadCodePath = theApp.GetProfileString(_T("Config"), _T("UploadPath"), _T(""));

	if (this->AutoTraceCheckBox.GetCheck() == BST_CHECKED) {
		this->SetTimer(REFRESH_AUTOTRACE_TIMER_ID, REFRESH_TIMER_INTERVAL, NULL);
	}
	if (this->AutoDownloadCheckBox.GetCheck() == BST_CHECKED) {
		this->SetTimer(REFRESH_AUTODOWNLOAD_TIMER_ID, REFRESH_TIMER_INTERVAL, NULL);
	}

	return TRUE;
}

void CStcIspUserDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CStcIspUserDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CStcIspUserDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CStcIspUserDlg::OnBnClickedCancel()
{
	this->OnBnClickedButtonStop();
	CDialogEx::OnCancel();
}


void CStcIspUserDlg::OnBnClickedButtonOpenFile()
{
	CFileDialog dialog(TRUE, _T("hex"),
		this->DownloadCodePath.IsEmpty() ? NULL : this->DownloadCodePath,
		OFN_FILEMUSTEXIST,
		_T("代码文件 (*.bin; *.hex)|*.bin; *.hex|所有文件 (*.*)|*.*||"));
	if (dialog.DoModal()) {
		CString path = dialog.GetPathName();
		if (CheckAndLoadCodeFile(path)) {
			this->DownloadButton.EnableWindow(TRUE);
			this->SaveButton.EnableWindow(TRUE);
			theApp.WriteProfileString(_T("Config"), _T("DownloadPath"), this->DownloadCodePath);
		}
	}
}

void CStcIspUserDlg::OnBnClickedButtonDownload()
{
	if (this->DownloadWorkerThread == nullptr) {
		this->UploadButton.EnableWindow(FALSE);
		::InterlockedExchangePointer(
			(void**)&this->DownloadWorkerThread, AfxBeginThread(DoDownload, this));
	}
}
void CStcIspUserDlg::OnBnClickedButtonUpload()
{
	if (this->UploadWorkerThread == nullptr) {
		this->DownloadButton.EnableWindow(FALSE);
		::InterlockedExchangePointer(
			(void**)&this->UploadWorkerThread, AfxBeginThread(DoUpload, this));
	}
}


void CStcIspUserDlg::OnClose()
{
	this->OnBnClickedButtonStop();
	CDialogEx::OnClose();
}

void CStcIspUserDlg::OnBnClickedButtonStop()
{
	this->StopButton.EnableWindow(FALSE);

	if (this->DownloadWorkerThread != nullptr
		|| this->UploadWorkerThread != nullptr) {
		SetEvent(this->QuitEvent);
	}
}


BOOL CStcIspUserDlg::DoCloseHandle()
{
	BOOL ret = FALSE;
	if (this->CommHandle != INVALID_HANDLE_VALUE) {
		ret = CloseHandle(this->CommHandle);
		this->CommHandle = INVALID_HANDLE_VALUE;
	}
	return ret;
}

BOOL CStcIspUserDlg::OpenComPort(int port)
{
	DoCloseHandle();
	COMMTIMEOUTS CommTimeouts = { 0 };
	DCB DCB = { 0 };
	CString fn;
	fn.Format(_T("\\\\.\\COM%d"), port);
	this->CommHandle = CreateFile(fn, GENERIC_READ | GENERIC_WRITE, 0, 0, 3, 0, 0);
	if (this->CommHandle != INVALID_HANDLE_VALUE)
	{
		SetupComm(this->CommHandle, 8192, 8192);
		CommTimeouts.ReadIntervalTimeout = 0;
		CommTimeouts.ReadTotalTimeoutMultiplier = 10;
		CommTimeouts.ReadTotalTimeoutConstant = 0;
		CommTimeouts.WriteTotalTimeoutMultiplier = 0;
		CommTimeouts.WriteTotalTimeoutConstant = 5000;
		SetCommTimeouts(this->CommHandle, &CommTimeouts);
		GetCommState(this->CommHandle, &DCB);
		DCB.BaudRate = 115200;
		DCB.fBinary = 1;
		DCB.ByteSize = 8;
		DCB.Parity = 0; //NONE
		DCB.StopBits = 0; //1,1.5,2
		SetCommState(this->CommHandle, &DCB);
		PurgeComm(this->CommHandle, 0x0F);
	}
	return this->CommHandle != INVALID_HANDLE_VALUE;
}

BOOL CStcIspUserDlg::SendCommand(unsigned char cmd, unsigned int address, unsigned char size, unsigned char output[PAGE_SIZE])
{
	if (this->CommHandle == INVALID_HANDLE_VALUE) return FALSE;
	BOOL done = FALSE;
	unsigned char sum = 0;
	DWORD NumberOfBytesWritten = 0;
	unsigned char* frame_buffer = new unsigned char[size + 10];
	if (frame_buffer != nullptr) {
		memset(frame_buffer, 0, size + 10);
		frame_buffer[0] = CMD_HEAD_SIGN;
		frame_buffer[1] = size + 6;
		frame_buffer[2] = cmd;
		frame_buffer[3] = HIBYTE((address >> 16)&0xffff); //ignored by receiver
		frame_buffer[4] = LOBYTE((address >> 16)&0xffff); //ignored by receiver
		frame_buffer[5] = HIBYTE((address & 0xffff));
		frame_buffer[6] = LOBYTE((address & 0xffff));
		frame_buffer[7] = size;
		if (size != 0 && output != 0)
		{
			memcpy(frame_buffer + 8, output, size);
		}
		frame_buffer[size + 8] = TAIL_SIGN;
		sum = Sum(frame_buffer, size + 9);
		frame_buffer[size + 9] = -sum;
		done = WriteFile(
			this->CommHandle,
			frame_buffer,
			size + 10,
			&NumberOfBytesWritten, 0);

		delete[] frame_buffer;
	}
	return done;
}

BOOL CStcIspUserDlg::GetResponse(unsigned char input[PAGE_SIZE], ULONGLONG max_delay_ms, unsigned char* payload_length_ptr) const
{
	if (this->CommHandle == INVALID_HANDLE_VALUE) return FALSE;

	int stage;
	int buffer_pos;
	int payload_pos;
	int payload_count;
	int completed;
	unsigned char sum;
	unsigned char current;
	unsigned char payload_length;
	unsigned char status;
	DWORD NumberOfBytesRead;
	DWORD Errors;
	COMSTAT Stat = { 0 };
	char frame_buffer[256] = { 0 };

	payload_pos = 0;
	completed = 0;
	sum = 0;
	stage = 0;
	buffer_pos = 0;

	ULONGLONG start = GetTickCount64();

	while (TRUE)
	{
		ClearCommError(this->CommHandle, &Errors, &Stat);
		DWORD to_read_bytes = Stat.cbInQue;
		if (to_read_bytes > sizeof(input)) {
			to_read_bytes = sizeof(input);
		}
		if (to_read_bytes > 0)
		{
			if (!ReadFile(this->CommHandle,
				frame_buffer + buffer_pos, to_read_bytes,
				&NumberOfBytesRead, 0))
				return FALSE;
			buffer_pos += NumberOfBytesRead;
		}
		if (payload_pos < buffer_pos)
		{
			current = frame_buffer[payload_pos];
			sum += current;
			++payload_pos;
			switch (stage)
			{
			case 0:
			{
				sum = current;
				stage = (current == REPLY_HEAD_SIGN);
				break;
			}
			case 1:
				status = current;
				stage = 2;
				break;
			case 2:
				payload_length = current;
				if (payload_length_ptr != nullptr) {
					*payload_length_ptr = payload_length;
				}
				payload_count = 0;
				stage = 3;
				if (current == 0) stage = 4;
				break;
			case 3:
				if (input != nullptr) {
					input[payload_count] = current;
				}
				if (++payload_count >= payload_length)
					stage = 4;
				break;
			case 4:
				if (current != TAIL_SIGN)
				{
					sum = current;
					stage = (current == REPLY_HEAD_SIGN);
				}
				stage = 5;
				break;
			case 5:
				if (sum != 0)
				{
					sum = current;
					stage = (current == REPLY_HEAD_SIGN);
				}
				else //complted
				{
					completed = TRUE;
				}
				break;
			default:
				break;
			}
		}

		if (!this->IsWorking)
			break;

		ULONGLONG tick = GetTickCount64();

		if (tick - start >= max_delay_ms)
			break;
		if (completed)
			return status == STATUS_OK;
	}
	return completed && status == STATUS_OK;
}
unsigned char CStcIspUserDlg::Sum(unsigned char* buffer, int length)
{
	unsigned char result = 0;
	for (int i = 0; i < length; i++)
		result += *buffer++;
	return result;
}

void CStcIspUserDlg::AppendStatusText(const TCHAR* format, ...)
{
	if (format != nullptr)
	{
		va_list Args;
		va_start(Args, format);
		CString Text;
		Text.FormatV(format, Args);

		CString Lines;
		this->StatusEdit.GetWindowText(Lines);
		if (!Lines.IsEmpty()) {
			Lines += _T("\r\n");
		}
		Lines += Text;
		this->StatusEdit.SetWindowText(Lines);
	}
	else
	{
		this->StatusEdit.SetWindowText(_T(""));
	}
}

void CStcIspUserDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent) {
	case REFRESH_AUTOTRACE_TIMER_ID:
		if (!this->IsWorking &&
			(this->AutoTraceCheckBox.GetCheck() == BST_CHECKED)) {
			CString MD5 = GetFileMD5(this->DownloadCodePath);
			if (MD5 != this->LastMD5) {
				this->FileChanged = CheckAndLoadCodeFile(
					this->DownloadCodePath,
					FALSE);
				if (this->FileChanged) {
					this->LastMD5 = MD5;
					this->DownloadButton.EnableWindow(TRUE);
				}
			}
			else {
				this->FileChanged = FALSE;
			}
		}
		break;
	case REFRESH_AUTODOWNLOAD_TIMER_ID:
		if (!this->IsWorking && this->AutoDownloadCheckBox.GetCheck() == BST_CHECKED) {
			if (this->FileChanged) {
				this->FileChanged = FALSE;
				this->OnBnClickedButtonDownload();
			}
		}
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CStcIspUserDlg::OnBnClickedCheckAutotrace()
{
	if (this->AutoTraceCheckBox.GetCheck() == BST_CHECKED) {
		this->SetTimer(REFRESH_AUTOTRACE_TIMER_ID, REFRESH_TIMER_INTERVAL, NULL);
	}
	else {
		this->KillTimer(REFRESH_AUTOTRACE_TIMER_ID);
	}
}


void CStcIspUserDlg::OnBnClickedCheckAutodownload()
{
	if (this->AutoDownloadCheckBox.GetCheck() == BST_CHECKED) {
		this->SetTimer(REFRESH_AUTODOWNLOAD_TIMER_ID, REFRESH_TIMER_INTERVAL, NULL);
		this->AutoTraceCheckBox.SetCheck(BST_CHECKED);
		this->OnBnClickedCheckAutotrace();
	}
	else {
		this->KillTimer(REFRESH_AUTODOWNLOAD_TIMER_ID);
	}
}

void CStcIspUserDlg::OnCbnSelchangeComboComports()
{
	int index = this->ComboPorts.GetCurSel();
	CString text;
	if (index >= 0) {
		this->ComboPorts.GetLBText(index, text);
	}
	if (theApp.WriteProfileString(_T("Config"), _T("COMPort"), text)) {
		text.Empty();
	}
}

void CStcIspUserDlg::OnBnClickedButtonSaveFile()
{
	if (this->CodeBuffer == nullptr || this->CodeLength == 0) {
		AfxMessageBox(_T("尚无可用保存的数据 !"));
	}
	else {
		CFileDialog dialog(FALSE, _T("bin"),
			this->UploadCodePath.IsEmpty() ? NULL : this->UploadCodePath,
			OFN_OVERWRITEPROMPT,
			_T("代码文件 (*.bin)|*.bin|所有文件 (*.*)|*.*||"));
		if (dialog.DoModal()) {
			this->UploadCodePath = dialog.GetPathName();
			CFile file;
			if (file.Open(this->UploadCodePath, CFile::typeBinary | CFile::modeWrite | CFile::shareDenyWrite))
			{
				file.Write(this->CodeBuffer, (UINT)this->CodeLength);
				file.Close();
				theApp.WriteProfileString(_T("Config"), _T("UploadPath"), this->UploadCodePath);
			}
		}
	}
}
