
// CameraDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Camera.h"
#include "CameraDlg.h"
#include "afxdialogex.h"

#include <opencv2/opencv.hpp>
using namespace cv;

#include <uc480.h>
//#include <uc480_tools.h>
//#include <uc480CaptureInterface.h>
#include <uc480_deprecated.h>

#define IMG_WIDTH 1280
#define IMG_HEIGHT 1024

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



UINT __cdecl CameraThread(LPVOID pParam);
extern char temp[1280 * 1024] = { "0" };
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCameraDlg 对话框



CCameraDlg::CCameraDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(CCameraDlg::IDD, pParent)
, m_exposure(5)
, m_gain(5)
, slider_exposure(0)
, slider_gain(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_open = true;
	m_live = false;

	m_project = false;

	//image size : 1280x1024
	//char* data = (char*)malloc(sizeof(char*)* 1280 * 1024);
}

void CCameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_EXPOSURE, m_exposure);
	DDX_Text(pDX, IDC_EDIT_GAIN, m_gain);
	DDX_Slider(pDX, IDC_SLIDER_EXPOSURE, slider_exposure);
	DDX_Slider(pDX, IDC_SLIDER_GAIN, slider_gain);
}

BEGIN_MESSAGE_MAP(CCameraDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CCameraDlg::OnBnClickedButtonOpen)
	ON_EN_CHANGE(IDC_EDIT_EXPOSURE, &CCameraDlg::OnChangeEditExposure)
	ON_EN_CHANGE(IDC_EDIT_GAIN, &CCameraDlg::OnChangeEditGain)
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDC_BUTTON_PROJ, &CCameraDlg::OnBnClickedButtonProj)
END_MESSAGE_MAP()


// CCameraDlg 消息处理程序

BOOL CCameraDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// TODO:  在此添加额外的初始化代码
	GetDlgItem(IDC_STATIC_SOHW)->SetWindowPos(NULL, 0, 0, 960, 768, NULL);
	m_hWnd = GetDlgItem(IDC_STATIC_SOHW)->m_hWnd;

	//namedWindow("1");
	m_nColorMode = IS_SET_CM_Y8;
	m_nBitsPerPixel = 8;
	m_open = true;
	m_hG = 0; //camera ID equals 0
	m_open = OpenCamera();

	AttachDisplay();
	namedWindow("project", CV_WINDOW_NORMAL);
	m_phWnd = (HWND)cvGetWindowHandle("project");

	Mat img(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3);
	img.setTo(Scalar(0, 0, 0));
	imshow("project", img);

	waitKey(100);
	HWND hShowWnd = (HWND)cvGetWindowHandle("project");
	HWND hParentProject = ::GetParent(hShowWnd);
	setWindowProperty("project", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);//1
	::SetWindowPos(hParentProject, HWND_TOPMOST, GetSystemMetrics(SM_CXSCREEN) / 2 - 1, 0, -1, -1, SWP_NOSIZE);

	GetDlgItem(IDC_BUTTON_PROJ)->EnableWindow(FALSE);

	while (m_open != true)
	{
		int m = AfxMessageBox(L"fail to open camera!", MB_RETRYCANCEL);
		if (m == IDRETRY)
		{
			m_open = OpenCamera();
		}
		else
		{
			return TRUE;
		}
	}
	pCameraThread = AfxBeginThread((AFX_THREADPROC)CameraThread, this, 0U, 0UL, 0, 0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCameraDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCameraDlg::OnPaint()
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
HCURSOR CCameraDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CCameraDlg::OnBnClickedButtonOpen()
{
	// TODO:  在此添加控件通知处理程序代码
	//stop live
	if (m_live == false)
	{
		m_live = true;
		GetDlgItem(IDC_BUTTON_PROJ)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_OPEN)->SetWindowTextW(L"CLose");
	}
	else
	{
		m_project = false;
		cvDestroyWindow("project");
		
		is_FreeImageMem(m_hG, m_pcImageMemory, m_lMemoryId);
		is_ExitCamera(m_hG);
		m_hG = NULL;
		CDialog::OnCancel();
	}
	
}

bool CCameraDlg::OpenCamera()
{
	if (m_hG != 0)
	{
		//free old image mem.
		is_FreeImageMem(m_hG, m_pcImageMemory, m_lMemoryId);
		is_ExitCamera(m_hG);
	}

	// init camera
	m_hG = (HCAM)0;							// open next camera
	m_Ret = is_InitCamera(&m_hG, NULL);		// init camera - no window handle for live required

	if (m_Ret == IS_SUCCESS)
	{
		// retrieve original image size
		SENSORINFO sInfo;
		is_GetSensorInfo(m_hG, &sInfo);

		GetMaxImageSize(&m_nSizeX, &m_nSizeY);

		// setup the color depth to the current windows setting
		//is_GetColorDepth(m_hG, &m_nBitsPerPixel, &m_nColorMode);
		is_SetColorMode(m_hG, m_nColorMode);

		// memory initialization
		is_AllocImageMem(m_hG,
			m_nSizeX,
			m_nSizeY,
			m_nBitsPerPixel,
			&m_pcImageMemory,
			&m_lMemoryId);
		is_SetImageMem(m_hG, m_pcImageMemory, m_lMemoryId);	// set memory active


		// display initialization
		is_SetImageSize(m_hG, m_nSizeX, m_nSizeY);
		is_SetDisplayMode(m_hG, IS_SET_DM_DIB);

		/*
		// enable the dialog based error report
		m_Ret = is_SetErrorReport(m_hG, IS_ENABLE_ERR_REP); //IS_DISABLE_ERR_REP);
		if( m_Ret != IS_SUCCESS )
		{
		AfxMessageBox( "ERROR: Can not enable the automatic error report!" , MB_ICONEXCLAMATION, 0 );
		return false;
		}
		*/
	}
	else
	{
		AfxMessageBox(L"ERROR: Can not open a camera!", MB_ICONEXCLAMATION, 0);
		return false;
	}

	return true;
}

void CCameraDlg::GetMaxImageSize(INT *pnSizeX, INT *pnSizeY)
{
	// Check if the camera supports an arbitrary AOI
	INT nAOISupported = 0;
	BOOL bAOISupported = TRUE;
	if (is_ImageFormat(m_hG,
		IMGFRMT_CMD_GET_ARBITRARY_AOI_SUPPORTED,
		(void*)&nAOISupported,
		sizeof(nAOISupported)) == IS_SUCCESS)
	{
		bAOISupported = (nAOISupported != 0);
	}

	if (bAOISupported)
	{
		// Get maximum image size
		SENSORINFO sInfo;
		is_GetSensorInfo(m_hG, &sInfo);
		*pnSizeX = sInfo.nMaxWidth;
		*pnSizeY = sInfo.nMaxHeight;
	}
	else
	{
		// Get image size of the current format
		*pnSizeX = is_SetImageSize(m_hG, IS_GET_IMAGE_SIZE_X, 0);
		*pnSizeY = is_SetImageSize(m_hG, IS_GET_IMAGE_SIZE_Y, 0);
	}

}



void CCameraDlg::OnChangeEditExposure()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_exposure < 0)
	{
		m_exposure = 0;
	}
	else if (m_exposure > 197)
	{
		m_exposure = 197;
	}
	//set exposure

	slider_exposure = m_exposure;
	UpdateData(FALSE);
}


void CCameraDlg::OnChangeEditGain()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_gain < 0)
	{
		m_gain = 0;
	}
	else if (m_gain > 100)
	{
		m_gain = 100;
	}
	//set exposure

	slider_gain = m_gain;
	UpdateData(FALSE);

}


void CCameraDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CSliderCtrl *pSlidCtrl1 = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_GAIN);
	CSliderCtrl *pSlidCtrl2 = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_EXPOSURE);

	slider_gain = pSlidCtrl1->GetPos();
	slider_exposure = pSlidCtrl2->GetPos();


	if (slider_gain > 100)
	{
		slider_gain = 100;
	}
	else if (slider_gain < 0)
	{
		slider_gain = 0;
	}

	if (slider_exposure > 197)
	{
		slider_exposure = 197;
	}
	else if (slider_exposure < 0)
	{
		slider_exposure = 0;
	}


	m_gain = slider_gain;
	m_exposure = slider_exposure;
	UpdateData(FALSE);

	//	is_SetGainBoost(m_hG, IS_SET_GAINBOOST_ON);
	//is_SetHardwareGain(m_hG, m_gain, m_gain, m_gain, m_gain);

	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}


UINT __cdecl CameraThread(LPVOID pParam)
{
	Mat img(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3);
	img.setTo(Scalar(0, 0, 0));
	
	CCameraDlg *p_this = (CCameraDlg *)pParam;

	is_SetGainBoost(p_this->m_hG, IS_SET_GAINBOOST_ON);

	while (p_this->m_open == true)
	{
		//INT is_Exposure (HIDS hCam, UINT nCommand, void* pParam, UINT cbSizeOfParam)
		double exposure = p_this->m_exposure;
		is_Exposure(p_this->m_hG, IS_EXPOSURE_CMD_SET_EXPOSURE, &exposure, 8);

		//INT is_SetHardwareGain (HIDS hCam, INT nMaster, INT nRed, INT nGreen, INT nBlue)
		int gain = p_this->m_gain;
		is_SetHardwareGain(p_this->m_hG, INT(gain), IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);

		if (p_this->m_live == true)
		{
			imshow("project", img);
			is_FreezeVideo(p_this->m_hG, IS_WAIT);
			

			//
			if (p_this->m_project == true)
			{
				is_RenderBitmap(p_this->m_hG, p_this->m_lMemoryId, p_this->m_phWnd, IS_RENDER_FIT_TO_WINDOW);
				is_RenderBitmap(p_this->m_hG, p_this->m_lMemoryId, p_this->m_hWnd, IS_RENDER_FIT_TO_WINDOW);
				waitKey(200);

			}
			else
			{
				is_RenderBitmap(p_this->m_hG, p_this->m_lMemoryId, p_this->m_hWnd, IS_RENDER_FIT_TO_WINDOW);
			}
			
		}

		

		
	}
	return 0;
}


void CCameraDlg::OnBnClickedButtonProj()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_project == false)
	{
		m_project = true;
		GetDlgItem(IDC_BUTTON_PROJ)->SetWindowTextW(L"StopProj");
	}
	else
	{
		m_project = false;
		GetDlgItem(IDC_BUTTON_PROJ)->SetWindowTextW(L"Project");
	}
}

void CCameraDlg::AttachDisplay()
{
	BOOL            FoundSecondaryDisp = FALSE;
	DWORD           DispNum = 0;
	DISPLAY_DEVICE  DisplayDevice;
	LONG            Result;
	//	TCHAR           szTemp[200];
	int             i = 0;
	DEVMODE   defaultMode;

	// initialize DisplayDevice
	ZeroMemory(&DisplayDevice, sizeof(DisplayDevice));
	DisplayDevice.cb = sizeof(DisplayDevice);

	// get all display devices
	while (EnumDisplayDevices(NULL, DispNum, &DisplayDevice, 0))
	{
		ZeroMemory(&defaultMode, sizeof(DEVMODE));
		defaultMode.dmSize = sizeof(DEVMODE);
		if (!EnumDisplaySettings((LPCWSTR)DisplayDevice.DeviceName, ENUM_REGISTRY_SETTINGS, &defaultMode))
			OutputDebugString(L"Store default failed\n");

		if (!(DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
		{
			DEVMODE    DevMode;
			DevMode = defaultMode;
			DevMode.dmFields |= DM_POSITION | DM_PELSHEIGHT | DM_PELSWIDTH;
			DevMode.dmPelsWidth = GetSystemMetrics(SM_CXSCREEN);
			DevMode.dmPelsHeight = GetSystemMetrics(SM_CYSCREEN);
			DevMode.dmPosition.x = GetSystemMetrics(SM_CXSCREEN);
			DevMode.dmPosition.y = 0;
			DevMode.dmSize = sizeof(DevMode);

			Result = ChangeDisplaySettingsEx((LPCWSTR)DisplayDevice.DeviceName,
				&DevMode,
				NULL,
				CDS_UPDATEREGISTRY | CDS_RESET | CDS_GLOBAL,
				NULL);
			ChangeDisplaySettingsEx(NULL, NULL, NULL, NULL, NULL);
		}

		// Reinit DisplayDevice just to be extra clean
		ZeroMemory(&DisplayDevice, sizeof(DisplayDevice));
		DisplayDevice.cb = sizeof(DisplayDevice);
		DispNum++;
	}
}
