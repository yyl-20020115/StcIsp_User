
// StcIsp_UserDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "StcIsp_User.h"
#include "StcIsp_UserDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

UINT CStcIspUserDlg::Download(LPVOID param) {
	if (param == nullptr) return 0;

	CStcIspUserDlg* _this = reinterpret_cast<CStcIspUserDlg*>(param);

	unsigned int current;
	int index; 
	unsigned char* ptr; 
	int left_count;
	unsigned char ret[4] = { 0 };

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
		if (_this->WriteComm(0xA0, 0, 0, 0) && _this->ReadComm(ret, 100u))
		{
			_this->SetStatusText(_T("连接目标芯垃成功 !(固件版本: %d.%d)"), ret[0], ret[1]);
			_this->SetStatusText(_T("正在擦除芯片 ... "));
			if (_this->WriteComm(0xA3, 0, 0, 0) && _this->ReadComm(ret, 5000u))
			{
				_this->SetStatusText(_T("正在下载代码 ... "));
				if (_this->Length <= 0)
				{
				LABEL_17:
					_this->WriteComm(0xA4, 0, 0, 0);
					_this->SetStatusText(_T("代码下载成功 !"));
					goto LABEL_21;
				}
				while (1)
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
						if (!_this->WriteComm(162, current, left_count, ret) 
							|| !_this->ReadComm(ret, 0x64u))
						{
							_this->SetStatusText(_T("下载失败 !"));
							goto LABEL_21;
						}
						current += left_count;

						_this->ProgressDownload.SetPos(current);

						//SendMessage(*((HWND*)this + 80), 0x402u, 100 * current / *((_DWORD*)this + 142), 0);
					}
					if (current >= _this->Length)
						goto LABEL_17;
				}
			}
			_this->SetStatusText(_T("擦除失败 !"));
		}
		else
		{
			_this->SetStatusText(_T("连接失败 !"));
		}
	LABEL_21:
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
	TCHAR v12; // al
	TCHAR v13; // cl
	TCHAR v14; // dl
	TCHAR v15; // al
	unsigned int v16; // eax
	TCHAR v17; // dl
	__int16 v18; // bx
	char* v19; // esi
	unsigned int v20; // eax
	int v21; // ebx
	TCHAR v22; // al
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
	if (!file.Open(path, CFile::shareDenyNone|CFile::typeBinary, 0))
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
			delete [] target_buffer;
			delete [] buffer;
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
	if (buffer2 !=nullptr && *buffer2 == 2)
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
					delete [] output_text_buffer;
					if (this->Source!=nullptr)
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
	delete [] buffer2;
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
	CFileDialog dialog(TRUE, _T("hex"),0, OFN_FILEMUSTEXIST,
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
			if (this->IsCodeReady = this->CheckAndLoadCodeFile(this->CodePath, this->IsCodeHex))
			{
				AfxBeginThread(Download, this);
			}
		}
		else {
			MessageBox(_T("需要先打开代码文件!"), 0, MB_ICONWARNING);

		}
	}
	else {


	}
}


void CStcIspUserDlg::OnBnClickedButtonStop()
{
	// TODO: 在此添加控件通知处理程序代码
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
	COMMTIMEOUTS CommTimeouts = { 0 };
	DCB DCB = { 0 };
	DoCloseHandle();

	CString fn;
	fn.Format(_T("\\\\.\\com%d"), port);
	this->CommHandle = CreateFile(fn, 0xC0000000, 0, 0, 3, 0, 0);
	if (this->CommHandle != INVALID_HANDLE_VALUE)
	{
		SetupComm(this->CommHandle, 0x2000, 0x2000);
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
		DCB.Parity = 0;
		DCB.StopBits = 0;
		SetCommState(this->CommHandle, &DCB);
		PurgeComm(this->CommHandle, 0x0F);
	}
	return this->CommHandle!=INVALID_HANDLE_VALUE;
}

BOOL CStcIspUserDlg::WriteComm(unsigned char function, unsigned int value, unsigned char length, const void* source)
{
	BOOL done = FALSE;
	if (this->CommHandle == INVALID_HANDLE_VALUE)
		return done;

	unsigned char sum = 0;
	DWORD NumberOfBytesWritten = 0;
	unsigned char* Buffer = new unsigned char[length + 10];
	if (Buffer != nullptr) {
		Buffer[0] = 35;
		Buffer[1] = length + 6;
		Buffer[2] = function;
		Buffer[3] = (value >> 24) & 0xff;
		Buffer[4] = (value >> 16) & 0xff;
		Buffer[5] = (value >> 8) & 0xff;
		Buffer[6] = (value & 0xff);
		Buffer[7] = length;
		if (length != 0 && source != 0)
		{
			memcpy(&Buffer[8], source, length);
		}
		Buffer[length + 8] = 36;
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
	int mpos;
	char sum;
	int v6;
	int pos;
	char bc;
	unsigned char v9;
	char v10;
	signed int v11;
	unsigned char v12;
	DWORD NumberOfBytesRead;
	ULONGLONG first_tick;
	DWORD Errors;
	COMSTAT Stat = { 0 }; 
	char Buffer[256] = { 0 };

	if (this->CommHandle!=INVALID_HANDLE_VALUE)
		return FALSE;
	mpos = 0;
	v11 = 0;
	sum = 0;
	v6 = 0;
	pos = 0;
	first_tick = GetTickCount64();
	while (TRUE)
	{
		ClearCommError(this->CommHandle, &Errors, &Stat);
		NumberOfBytesRead = Stat.cbInQue;
		if (Stat.cbInQue)
		{
			ReadFile(this->CommHandle, &Buffer[pos], Stat.cbInQue, &NumberOfBytesRead, 0);
			pos += NumberOfBytesRead;
		}
		if (mpos < pos)
		{
			bc = Buffer[mpos];
			sum += bc;
			++mpos;
			switch (v6)
			{
			case 0:
				goto LABEL_18;
			case 1:
				v10 = bc;
				v6 = 2;
				break;
			case 2:
				v9 = bc;
				v12 = 0;
				v6 = 3;
				if (!bc)
					v6 = 4;
				break;
			case 3:
				if (buffer)
					buffer[v12] = bc;
				if (++v12 >= v9)
					v6 = 4;
				break;
			case 4:
				if (bc != 36)
					goto LABEL_18;
				v6 = 5;
				break;
			case 5:
				if (sum)
				{
				LABEL_18:
					sum = bc;
					v6 = bc == 64;
				}
				else
				{
					v11 = 1;
				}
				break;
			default:
				break;
			}
		}
		if (!this->IsWorking)
			break;
		if (GetTickCount64() - first_tick >= ticks)
			break;
		if (v11)
			goto LABEL_25;
	}
	if (!v11)
		return FALSE;
LABEL_25:
	if (!v10)
		return TRUE;
	return FALSE;
}
unsigned char CStcIspUserDlg::Sum(unsigned char* buffer, int length)
{
	unsigned char result = 0;
	for(int i = 0;i<length;i++)
		result += *buffer++;
	return result;
}

void CStcIspUserDlg::SetStatusText(TCHAR* format, ...)
{
	if (format!=nullptr)
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
