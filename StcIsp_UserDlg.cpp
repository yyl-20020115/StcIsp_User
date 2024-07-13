#include "pch.h"
#include "framework.h"
#include "StcIsp_User.h"
#include "StcIsp_UserDlg.h"
#include "afxdialogex.h"
#define HEAD_SIGN 0x23
#define TAIL_SIGN 0x24
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


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

UINT CStcIspUserDlg::DoDownload(LPVOID param) {
	if (param == nullptr) return 0;
	CStcIspUserDlg* _this = reinterpret_cast<CStcIspUserDlg*>(param);
	if (_this->CodeLength == 0 || _this->CodeBuffer == nullptr) return 0;
	int index = _this->ComboPorts.GetCurSel();
	if (index < 0) return 0;

	int page_size = 0;
	unsigned int address = 0;
	unsigned char* ptr = nullptr;
	unsigned char buffer[PAGE_SIZE] = { 0 };

	_this->SetStatusText();
	_this->OpenButton.EnableWindow(FALSE);
	_this->DownloadButton.EnableWindow(FALSE);
	_this->StopButton.EnableWindow(TRUE);

	_this->ProgressDownload.SetRange32(0, (int)_this->CodeLength);
	if (!_this->OpenComm(index + 1))
	{
		AfxMessageBox(_T("端口打开失败 !"), 0, 0);
	}
	else
	{
		_this->SetStatusText(_T("连接目标芯片 ..."));
		if (!_this->WriteComm(DFU_CMD_CONNECT, 0, 0, 0) || !_this->ReadComm(buffer, 100))
		{
			_this->SetStatusText(_T("连接失败 !"));
		}
		else
		{
			_this->SetStatusText(_T("连接目标芯垃成功 !(固件版本: %d.%d)"), buffer[0], buffer[1]);
			_this->SetStatusText(_T("正在擦除芯片 ... "));
			if (!_this->WriteComm(DFU_CMD_ERASE, 0, 0, 0) || !_this->ReadComm(buffer, 5000))
			{
				_this->SetStatusText(_T("擦除失败 !"));
			}
			else
			{
				_this->SetStatusText(_T("正在下载代码 ... "));
				while (WaitForSingleObject(_this->QuitEvent,1)!=WAIT_OBJECT_0)
				{
					ptr = _this->CodeBuffer + address;
					if (*ptr == 0xff) //skip 0xff bytes
					{
						++address;
					}
					else
					{
						page_size = PAGE_SIZE;
						if (address < 0x10000 && address + PAGE_SIZE > 0x10000)
						{
							page_size = 0x10000 - address;
						}
						memcpy(buffer, ptr, page_size);
						if (!_this->WriteComm(DFU_CMD_PROGRAM, address, page_size, buffer)
							|| !_this->ReadComm(buffer, 100))
						{
							_this->SetStatusText(_T("下载失败 !"));
							_this->DoCloseHandle();
							break;
						}
						address += page_size;

						_this->ProgressDownload.SetPos(address);
					}
					if (address >= _this->CodeLength)
					{
						_this->WriteComm(DFU_CMD_REBOOT, 0, 0, 0);
						_this->DoCloseHandle();
						_this->SetStatusText(_T("代码下载成功 !"));
					}
				}
			}
		}
	}

	_this->OpenButton.EnableWindow(TRUE);
	_this->DownloadButton.EnableWindow(_this->CodeBuffer != nullptr);
	_this->StopButton.EnableWindow(FALSE);
	_this->IsWorking = FALSE;

	return 0;
}
BOOL CStcIspUserDlg::CheckAndLoadCodeFile(const CString& path, BOOL IsHex)
{
	unsigned int input_text_length; // esi
	char* input_text_buffer; // ebp
	char* token; // esi
	char ch; // dl
	unsigned int length;
	int index;
	unsigned short binary_length;
	unsigned int address;
	unsigned int binary_input_length = 0;
	unsigned char* code_buffer;
	unsigned char type;
	unsigned char check_sum;
	unsigned char real_sum;
	CFile file;
	if (!file.Open(path, CFile::shareDenyNone | CFile::typeBinary, 0))
	{
		AfxMessageBox(_T("打开文件失败 !"), 0, 0);
		return FALSE;
	}
	input_text_length = (unsigned int)file.GetLength();
	input_text_buffer = new char[input_text_length + 1];
	file.Read(input_text_buffer, (UINT)input_text_length);
	input_text_buffer[input_text_length] = 0;

	code_buffer = new unsigned char[0x10000];
	memset(code_buffer, 0xff, 0x10000);

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
					real_sum += (char)(address& 0xff);
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
						if (address > binary_input_length) {
							binary_input_length = address;
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
		binary_input_length = input_text_length > 0x10000 ? 0x10000 : input_text_length;
		memcpy(code_buffer, input_text_buffer, binary_input_length);
	}
	delete[] input_text_buffer;
	//here is for binary
	if (code_buffer != nullptr && code_buffer[0] == 0x02)
	{
		binary_length = (((unsigned short)code_buffer[1]) << 8) | (code_buffer[2]);
		if (binary_length >= (BLOCK_SIZE + 3))
		{
			index = 3;
			int cs = 0;
			int fills = 0;
			while (code_buffer[index++] == 0xff) cs++;
			if (cs == 0) {
				fills = BLOCK_SIZE;
				index = 3;
				while (code_buffer[index++] == 0) cs++;
			}
			if (cs>= BLOCK_SIZE && index >= BLOCK_SIZE + 3)
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
				CString display_text_buffer;
				if (binary_input_length > 0)
				{
					address = 0;
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
					} while (address < binary_input_length);
				}
				this->HexEdit.SetWindowText(display_text_buffer);

				if (this->CodeBuffer != nullptr)
				{
					delete[] this->CodeBuffer;
					this->CodeBuffer = 0;
				}
				this->CodeLength = binary_input_length;
				this->CodeBuffer = code_buffer;
				this->CodePath = path;
				this->IsCodeHex = IsHex;
				this->SetStatusText(path);
				return TRUE;
			}
		}
	}
	delete[] code_buffer;
	AfxMessageBox(_T("代码文件不规范, 无法加载 !"), 0, 0);
	return FALSE;
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


// CStcIspUserDlg 对话框

CStcIspUserDlg::CStcIspUserDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_STCISP_USER_DIALOG, pParent)
	, IsCodeReady(FALSE)
	, IsCodeHex(FALSE)
	, CodePath()
	, CommHandle(INVALID_HANDLE_VALUE)
	, IsWorking(FALSE)
	, CodeBuffer(nullptr)
	, CodeLength(0)
	, QuitEvent(INVALID_HANDLE_VALUE)
	, WorkingThread(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	this->QuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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
}

void CStcIspUserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_COMPORTS, ComboPorts);
	DDX_Control(pDX, IDC_STATIC_STATUS, StatusText);
	DDX_Control(pDX, IDC_PROGRESS_DOWNLOAD, ProgressDownload);
	DDX_Control(pDX, IDC_BUTTON_STOP, StopButton);
	DDX_Control(pDX, IDC_BUTTON_DOWNLOAD, DownloadButton);
	DDX_Control(pDX, IDC_BUTTON_OPEN_FILE, OpenButton);
	DDX_Control(pDX, IDC_EDIT_HEX, HexEdit);
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
	this->DownloadButton.EnableWindow(FALSE);
	this->StopButton.EnableWindow(FALSE);
	this->HexEdit.SetBackColor(RGB(0xff, 0xff, 0xff));
	for (int i = 1; i < 256; i++) {
		CString com_name;
		com_name.Format(_T("COM%d"), i);
		this->ComboPorts.InsertString(i - 1, com_name);
	}
	if (this->ComboPorts.GetCount() > 0) {
		this->ComboPorts.SetCurSel(0);
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
	CFileDialog dialog(TRUE, _T("hex"), 0, OFN_FILEMUSTEXIST,
		_T("代码文件 (*.bin; *.hex)|*.bin; *.hex|所有文件 (*.*)|*.*||"));
	if (dialog.DoModal()) {
		CString ext = dialog.GetFileExt();
		ext.MakeLower();
		CString path = dialog.GetPathName();
		this->DownloadButton.EnableWindow(
			CheckAndLoadCodeFile(path, ext == _T("hex"))
		);
	}
}

void CStcIspUserDlg::OnBnClickedButtonDownload()
{
	if (this->WorkingThread != nullptr) {
		this->WorkingThread = AfxBeginThread(DoDownload, this);
	}
}

void CStcIspUserDlg::OnClose()
{
	if (this->WorkingThread != nullptr) {
		SetEvent(this->QuitEvent);
		WaitForSingleObject(this->WorkingThread->m_hThread, INFINITE);
		delete this->WorkingThread;
		this->WorkingThread = nullptr;
	}
	CDialogEx::OnClose();
}

void CStcIspUserDlg::OnBnClickedButtonStop()
{
	this->StopButton.EnableWindow(FALSE);
	if (this->WorkingThread != nullptr) {
		SetEvent(this->QuitEvent);
		WaitForSingleObject(this->WorkingThread->m_hThread, INFINITE);
		delete this->WorkingThread;
		this->WorkingThread = nullptr;
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

BOOL CStcIspUserDlg::OpenComm(int port)
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

BOOL CStcIspUserDlg::WriteComm(unsigned char function, unsigned int value, unsigned char length, unsigned char output[PAGE_SIZE])
{
	if (this->CommHandle == INVALID_HANDLE_VALUE) return FALSE;
	BOOL done = FALSE;
	unsigned short high = (unsigned short)(value >> 16);
	unsigned short _low = (unsigned short)(value & 0xffff);
	unsigned char sum = 0;
	DWORD NumberOfBytesWritten = 0;
	unsigned char* frame_buffer = new unsigned char[length + 10];
	if (frame_buffer != nullptr) {
		memset(frame_buffer, 0, length + 10);
		frame_buffer[0] = HEAD_SIGN;
		frame_buffer[1] = length + 6;
		frame_buffer[2] = function;
		frame_buffer[3] = HIBYTE(high);
		frame_buffer[4] = LOBYTE(high);
		frame_buffer[5] = HIBYTE(_low);
		frame_buffer[6] = LOBYTE(_low);
		frame_buffer[7] = length;
		if (length != 0 && output != 0)
		{
			memcpy(&frame_buffer[8], output, length);
		}
		frame_buffer[length + 8] = TAIL_SIGN;
		sum = Sum(frame_buffer, length + 9);
		frame_buffer[length + 9] = -sum;
		done = WriteFile(
			this->CommHandle,
			frame_buffer,
			length + 10,
			&NumberOfBytesWritten, 0);

		delete[] frame_buffer;
	}
	return done;
}

BOOL CStcIspUserDlg::ReadComm(unsigned char input[PAGE_SIZE], ULONGLONG ticks) const
{
	if (this->CommHandle == INVALID_HANDLE_VALUE)
		return FALSE;

	int stage;
	int pos;
	int mpos;
	int npos;
	int sign;
	unsigned char sum;
	unsigned char bc;
	unsigned char tag;
	unsigned char rbc;
	DWORD NumberOfBytesRead;
	DWORD Errors;
	COMSTAT Stat = { 0 };
	char frame_buffer[256] = { 0 };

	mpos = 0;
	sign = 0;
	sum = 0;
	stage = 0;
	pos = 0;
	ULONGLONG tick = GetTickCount64();
	while (TRUE)
	{
		ClearCommError(this->CommHandle, &Errors, &Stat);
		DWORD to_read_bytes = Stat.cbInQue;
		if (to_read_bytes > sizeof(input)) {
			to_read_bytes = sizeof(input);
		}
		Stat.cbInQue -= sizeof(input);
		if (to_read_bytes > 0)
		{
			if (!ReadFile(this->CommHandle,
				frame_buffer + pos, to_read_bytes,
				&NumberOfBytesRead, 0))
				return FALSE;
			pos += NumberOfBytesRead;
		}
		if (mpos < pos)
		{
			bc = frame_buffer[mpos];
			sum += bc;
			++mpos;
			switch (stage)
			{
			case 0:
			{
				sum = bc;
				stage = bc == 64;
				break;
			}
			case 1:
				rbc = bc;
				stage = 2;
				break;
			case 2:
				tag = bc;
				npos = 0;
				stage = 3;
				if (bc == 0) stage = 4;
				break;
			case 3:
				if (input != nullptr)
					input[npos] = bc;
				if (++npos >= tag)
					stage = 4;
				break;
			case 4:
				if (bc != TAIL_SIGN)
				{
					sum = bc;
					stage = bc == 64;
				}
				stage = 5;
				break;
			case 5:
				if (sum != 0)
				{
					sum = bc;
					stage = bc == 64;
				}
				else
				{
					sign = 1;
				}
				break;
			default:
				break;
			}
		}
		if (!this->IsWorking)
			break;
		if (GetTickCount64() - tick >= ticks)
			break;
		if (sign != 0)
			return rbc == 0;
	}
	return sign != 0 && rbc == 0;
}
unsigned char CStcIspUserDlg::Sum(unsigned char* buffer, int length)
{
	unsigned char result = 0;
	for (int i = 0; i < length; i++)
		result += *buffer++;
	return result;
}

void CStcIspUserDlg::SetStatusText(const TCHAR* format, ...)
{
	if (format != nullptr)
	{
		va_list Args;
		va_start(Args, format);
		CString Text;
		Text.FormatV(format, Args);
		this->StatusText.SetWindowText(Text);
	}
	else
	{
		this->StatusText.SetWindowText(_T(""));
	}
}


