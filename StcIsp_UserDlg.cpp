
// StcIsp_UserDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "StcIsp_User.h"
#include "StcIsp_UserDlg.h"
#include "afxdialogex.h"
#define HEAD_SIGN 0x23
#define TAIL_SIGN 0x24

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

	int index = 0;
	int left_count = 0;
	unsigned int current = 0;
	unsigned char* ptr;
	unsigned char ret[128] = { 0 };

	current = 0;
	_this->SetStatusText();
	_this->OpenButton.EnableWindow(FALSE);
	_this->DownloadButton.EnableWindow(FALSE);
	_this->StopButton.EnableWindow(TRUE);
	_this->ProgressDownload.SetRange32(0, (int)_this->Length);
	index = _this->ComboPorts.GetCurSel();

	if (_this->OpenComm(index + 1))
	{
		_this->SetStatusText(_T("连接目标芯片 ..."));
		if (_this->WriteComm(DFU_CMD_CONNECT, 0, 0, 0) && _this->ReadComm(ret, 100))
		{
			_this->SetStatusText(_T("连接目标芯垃成功 !(固件版本: %d.%d)"), ret[0], ret[1]);
			_this->SetStatusText(_T("正在擦除芯片 ... "));
			if (_this->WriteComm(DFU_CMD_ERASE, 0, 0, 0) && _this->ReadComm(ret, 5000))
			{
				_this->SetStatusText(_T("正在下载代码 ... "));
				if (_this->Length <= 0)
				{
				DO_REBOOT:
					_this->WriteComm(DFU_CMD_REBOOT, 0, 0, 0);
					_this->SetStatusText(_T("代码下载成功 !"));
					goto DO_CLOSE;
				}
				while (TRUE)
				{
					ptr = _this->Source + current;
					if (*ptr == -1)
					{
						++current;
					}
					else
					{
						left_count = 128;
						if (current < 0x10000 && current + 128 > 0x10000)
						{
							left_count = 0x10000 - current;
						}
						memcpy(ret, ptr, left_count);
						if (!_this->WriteComm(DFU_CMD_PROGRAM,
							current, left_count, ret)
							|| !_this->ReadComm(ret, 100))
						{
							_this->SetStatusText(_T("下载失败 !"));
							goto DO_CLOSE;
						}
						current += left_count;

						_this->ProgressDownload.SetPos(current);

						//SendMessage(*((HWND*)this + 80), 0x402u, 100 * current / *((_DWORD*)this + 142), 0);
					}
					if (current >= _this->Length)
						goto DO_REBOOT;
				}
			}
			_this->SetStatusText(_T("擦除失败 !"));
		}
		else
		{
			_this->SetStatusText(_T("连接失败 !"));
		}
	DO_CLOSE:
		_this->DoCloseHandle();
	}
	else
	{
		AfxMessageBox(_T("端口打开失败 !"), 0, 0);
	}

	_this->OpenButton.EnableWindow(TRUE);
	_this->DownloadButton.EnableWindow(TRUE);
	_this->StopButton.EnableWindow(FALSE);
	_this->IsWorking = FALSE;

	return 0;
}
BOOL CStcIspUserDlg::CheckAndLoadCodeFile(const CString& path, BOOL IsHex)
{
	unsigned long long length; // esi
	char* buffer; // ebp
	char* token; // esi
	char ch; // dl
	char* pch_next; // esi
	char v10; // al
	char* v11; // esi
	char v12; // al
	char v13; // cl
	char v14; // dl
	char v15; // al
	unsigned int v16; // eax
	char v17; // dl
	__int16 v18; // bx
	char* v19; // esi
	unsigned int v20; // eax
	int v21; // ebx
	char v22; // al
	char* v23; // esi
	int v24; // al
	bool v25; // zf
	unsigned __int16 sign_value; // cx
	signed int index; // eax
	char v28; // al
	char v29; // cl
	unsigned char* buffer2 = nullptr;
	TCHAR* output_text_buffer; // esi
	unsigned int address; // edi
	unsigned long long length2; // [esp+10h] [ebp-38h]
	signed int v35; // [esp+18h] [ebp-30h]
	void* target_buffer; // [esp+1Ch] [ebp-2Ch]
	TCHAR* pout; // [esp+1Ch] [ebp-2Ch]
	unsigned int v38; // [esp+20h] [ebp-28h]
	char Str; // [esp+24h] [ebp-24h]
	TCHAR v40; // [esp+25h] [ebp-23h]
	TCHAR v41; // [esp+26h] [ebp-22h]
	TCHAR v42; // [esp+27h] [ebp-21h]
	CFile file;
	if (!file.Open(path, CFile::shareDenyNone | CFile::typeBinary, 0))
	{
		AfxMessageBox(_T("打开文件失败 !"), 0, 0);
		return FALSE;
	}
	length = file.GetLength();
	length2 = length;
	buffer = new char[length + 1];
	file.Read(buffer, (UINT)length);
	buffer[length] = 0;
	if (!IsHex)
		goto for_binary;
	target_buffer = new unsigned char[0x10000];
	memset(target_buffer, 0xFF, 0x10000u);
	length2 = 0;
	token = strtok(buffer, "\r\n");
	if (!token)
		goto free_target_buffer;
	while (TRUE)
	{
		ch = *token;
		pch_next = token + 1;
		if (ch == ':')
			break;
	LABEL_14:
		token = strtok(0, "\r\n");
		if (!token)
			goto free_target_buffer;
	}
	v10 = *pch_next;
	v11 = pch_next + 1;
	Str = v10;
	v40 = *v11++;
	v41 = 0;
	v38 = strtoul(&Str, 0, 16);
	v12 = *v11++;
	Str = v12;
	v13 = *v11++;
	v40 = v13;
	v14 = *v11++;
	v41 = v14;
	v15 = *v11++;
	v42 = v15;
	v16 = strtoul(&Str, 0, 16);
	v17 = *v11++;
	v18 = v16;
	Str = v17;
	v35 = v16;
	v40 = *v11;
	v19 = v11 + 1;
	v41 = 0;
	v20 = strtoul(&Str, 0, 16);
	v21 = v38 + HIBYTE(v18) + v20 + (unsigned __int8)v18;
	if (!v20)
	{
		v41 = 0;
		if (v38)
		{
			do
			{
				v22 = *v19;
				v23 = v19 + 1;
				Str = v22;
				v40 = *v23;
				v19 = v23 + 1;
				v24 = strtoul(&Str, 0, 16);
				*((unsigned char*)target_buffer + v35) = v24;
				//LOBYTE(v21) = v24 + v21;
				v21 += v24;
				v25 = v38 == 1;
				++v35;
				--v38;
			} while (!v25);
		}
		Str = *v19;
		v40 = v19[1];
		if ((unsigned __int8)strtoul(&Str, 0, 16) + (unsigned char)v21)
		{
			delete[] target_buffer;
			delete[] buffer;
			AfxMessageBox(_T("HEX文件数据错误 !"), 0, 0);
			return FALSE;
		}
		if (v35 > length2)
			length2 = v35;
		goto LABEL_13;
	}
	if (v20 != 1)
	{
	LABEL_13:
		goto LABEL_14;
	}
free_target_buffer:
	delete[] buffer;
	buffer2 = (unsigned char*)target_buffer;
	length = length2;
for_binary:
	if (buffer2 != nullptr && *buffer2 == 2)
	{
		sign_value = (((unsigned short)buffer2[1]) << 8) | (buffer2[2]);
		if (sign_value > 0x1003u)
		{
			index = 3;
			while (buffer2[index] == -1)
			{
				if (++index >= 4099)
				{
					v28 = buffer2[1];
					v29 = buffer2[2];
					buffer2[4096] = *buffer2;
					buffer2[4097] = v28;
					buffer2[4098] = v29;
					*buffer2 = -1;
					buffer2[1] = -1;
					buffer2[2] = -1;
					output_text_buffer = new TCHAR[4 * length];
					address = 0;
					pout = output_text_buffer;
					if (length2 > 0)
					{
						do
						{
							if (!(address % 16))
								output_text_buffer += _sntprintf(output_text_buffer, 4 * length, _T("%04X "), address);
							output_text_buffer += _sntprintf(output_text_buffer, 4 * length, _T("%02X"), (unsigned __int8)buffer[address]);
							if (address % 16 == 15)
								output_text_buffer += _sntprintf(output_text_buffer, 4 * length, _T("\r\n"));
							++address;
						} while (address < length2);
						output_text_buffer = pout;
					}
					this->HexEdit.SetWindowText(output_text_buffer);
					delete[] output_text_buffer;
					if (this->Source != nullptr)
					{
						delete[] this->Source;
						this->Source = 0;
					}
					this->Length = length2;
					this->Source = buffer2;
					this->CodePath = path;
					//v32 = (char*)*((_DWORD*)this + 140);
					this->IsCodeHex = IsHex;
					//this->SetStatusText(v32);
					return TRUE;
				}
			}
		}
	}
	delete[] buffer2;
	AfxMessageBox(_T("代码文件不规范 !"), 0, 0);
	return FALSE;
}


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

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
	, Source(nullptr)
	, Length(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
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
END_MESSAGE_MAP()


// CStcIspUserDlg 消息处理程序

BOOL CStcIspUserDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	this->StopButton.EnableWindow(FALSE);
	for (int i = 1; i < 256; i++) {
		CString com_name;
		com_name.Format(_T("COM%d"), i);
		this->ComboPorts.InsertString(i - 1, com_name);
	}
	if (this->ComboPorts.GetCount() > 0) {
		this->ComboPorts.SetCurSel(0);
	}
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CStcIspUserDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CStcIspUserDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CStcIspUserDlg::OnBnClickedCancel()
{
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
		CheckAndLoadCodeFile(path, ext == _T("hex"));
	}
}

void CStcIspUserDlg::OnBnClickedButtonDownload()
{
	if (!this->IsCodeReady) {
		if (this->CodePath.GetLength() > 0) {
			if (this->IsCodeReady 
				= this->CheckAndLoadCodeFile(
					this->CodePath, this->IsCodeHex))
				AfxBeginThread(DoDownload, this);
		}
		else {
			MessageBox(_T("需要先打开代码文件!"), 0, MB_ICONWARNING);
		}
	}
	else {
		AfxBeginThread(DoDownload, this);
	}
}


void CStcIspUserDlg::OnBnClickedButtonStop()
{
	this->IsWorking = FALSE;
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
	this->CommHandle = CreateFile(fn, GENERIC_READ|GENERIC_WRITE, 0, 0, 3, 0, 0);
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

BOOL CStcIspUserDlg::WriteComm(unsigned char function, unsigned int value, unsigned char length, const void* source)
{
	if (this->CommHandle == INVALID_HANDLE_VALUE) return FALSE;
	BOOL done = FALSE;
	unsigned short high = (unsigned short)(value >> 16);
	unsigned short low = (unsigned short)(value & 0xffff);
	unsigned char sum = 0;
	DWORD NumberOfBytesWritten = 0;
	unsigned char* Buffer = new unsigned char[length + 10];
	if (Buffer != nullptr) {
		memset(Buffer, 0, length + 10);
		Buffer[0] = HEAD_SIGN;
		Buffer[1] = length + 6;
		Buffer[2] = function;
		Buffer[3] = HIBYTE(high);
		Buffer[4] = LOBYTE(high);
		Buffer[5] = HIBYTE(low);
		Buffer[6] = LOBYTE(low);
		Buffer[7] = length;
		if (length != 0 && source != 0)
		{
			memcpy(&Buffer[8], source, length);
		}
		Buffer[length + 8] = TAIL_SIGN;
		sum = Sum(Buffer, length + 9);
		Buffer[length + 9] = -sum;
		done = WriteFile(
			this->CommHandle,
			Buffer,
			length + 10,
			&NumberOfBytesWritten, 0);

		delete[] Buffer;
	}
	return done;
}

BOOL CStcIspUserDlg::ReadComm(unsigned char* buffer, ULONGLONG ticks) const
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
	char Buffer[256] = { 0 };

	mpos = 0;
	sign = 0;
	sum = 0;
	stage = 0;
	pos = 0;
	ULONGLONG tick = GetTickCount64();
	while (TRUE)
	{
		ClearCommError(this->CommHandle, &Errors, &Stat);
		if (Stat.cbInQue > 0)
		{
			if (!ReadFile(this->CommHandle,
				Buffer + pos, Stat.cbInQue,
				&NumberOfBytesRead, 0))
				return FALSE;
			pos += NumberOfBytesRead;
		}
		if (mpos < pos)
		{
			bc = Buffer[mpos];
			sum += bc;
			++mpos;
			switch (stage)
			{
			case 0:
				goto DoPrepare;
			case 1:
				rbc = bc;
				stage = 2;
				break;
			case 2:
				tag = bc;
				npos = 0;
				stage = 3;
				if (!bc)
					stage = 4;
				break;
			case 3:
				if (buffer != nullptr)
					buffer[npos] = bc;
				if (++npos >= tag)
					stage = 4;
				break;
			case 4:
				if (bc != TAIL_SIGN)
					goto DoPrepare;
				stage = 5;
				break;
			case 5:
				if (sum != 0)
				{
				DoPrepare:
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
		if (sign)
			goto BeforeReturn;
	}
	if (sign == 0)
		return FALSE;
BeforeReturn:
	if (rbc == 0)
		return TRUE;
	return FALSE;
}
unsigned char CStcIspUserDlg::Sum(unsigned char* buffer, int length)
{
	unsigned char result = 0;
	for (int i = 0; i < length; i++)
		result += *buffer++;
	return result;
}

void CStcIspUserDlg::SetStatusText(TCHAR* format, ...)
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
