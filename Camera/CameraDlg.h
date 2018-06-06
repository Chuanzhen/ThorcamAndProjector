
// CameraDlg.h : 头文件
//

#pragma once
#include <uc480.h>
//#include <uc480_tools.h>
//#include <uc480CaptureInterface.h>
#include <uc480_deprecated.h>


// CCameraDlg 对话框
class CCameraDlg : public CDialogEx
{
// 构造
public:
	CCameraDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_CAMERA_DIALOG };

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
	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonProj();

	afx_msg void OnChangeEditExposure();
	afx_msg void OnChangeEditGain();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	
	
	bool m_open;
	int m_exposure;
	int m_gain;
	int slider_exposure;
	int slider_gain;
	bool m_live;
	bool m_project;

	CWinThread* pCameraThread;
	CWinThread* pVideoThread;

	// camera varibles
	HCAM	m_hG;			// handle to frame grabber
	HWND	m_hWnd;			// handle to diplay window
	INT		m_Ret;			// return value for SDK functions
	INT		m_nColorMode;	// Y8/RGB16/RGB24/REG32
	INT		m_nBitsPerPixel;// number of bits needed store one pixel
	INT		m_nSizeX;		// width of video 
	INT		m_nSizeY;		// height of video
	INT		m_lMemoryId;	// grabber memory - buffer ID
	char*	m_pcImageMemory;// grabber memory - pointer to buffer

	//char data[1280*1024];
	bool OpenCamera();
	void GetMaxImageSize(INT *pnSizeX, INT *pnSizeY);
	
	void AttachDisplay();
	HWND m_phWnd;
};
