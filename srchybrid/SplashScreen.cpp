// SplashScreen.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "SplashScreen.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CSplashScreen dialog

IMPLEMENT_DYNAMIC(CSplashScreen, CDialog)
CSplashScreen::CSplashScreen(CWnd* pParent /*=NULL*/)
	: CDialog(CSplashScreen::IDD, pParent)
{
	m_timer = 0;
}

CSplashScreen::~CSplashScreen()
{
	m_imgSplash.DeleteObject();
}

void CSplashScreen::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CSplashScreen::OnInitDialog(){
	CDialog::OnInitDialog();
	InitWindowStyles(this);
	//GetParent()->ShowWindow(SW_SHOW);
	VERIFY (m_imgSplash.LoadImage(IDR_SPLASH,"JPG"));
	m_translucency = 0;
	VERIFY( (m_timer = SetTimer(300,90,0)) );
	return true;
}

BEGIN_MESSAGE_MAP(CSplashScreen, CDialog)
	ON_WM_TIMER()
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CSplashScreen::OnTimer(UINT_PTR nIDEvent){
	m_translucency += 18;
	if (m_translucency > 255){
		if (m_translucency < 450){
			//::SetLayeredWindowAttributes(GetParent()->m_hWnd, 0, 255, LWA_ALPHA);
			//::SetLayeredWindowAttributes(m_hWnd, 0, 255 - (m_translucency-255), LWA_ALPHA);
			return;
		}
		if (m_timer){
			KillTimer(m_timer);
			m_timer = 0;
		}
		//::SetWindowLong(GetParent()->m_hWnd,GWL_EXSTYLE,::GetWindowLong(GetParent()->m_hWnd,GWL_EXSTYLE) ^ WS_EX_LAYERED);
		OnOK();
		GetParent()->RedrawWindow();
		return;
	}
	//::SetLayeredWindowAttributes(GetParent()->m_hWnd, 0, m_translucency, LWA_ALPHA);
}

BOOL CSplashScreen::PreTranslateMessage(MSG* pMsg) {
   if(pMsg->message == WM_LBUTTONDOWN){
		m_translucency = 500;
   }

   return CDialog::PreTranslateMessage(pMsg);
}

void CSplashScreen::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	if (m_imgSplash.GetSafeHandle())
	{
		CDC dcMem;

		if (dcMem.CreateCompatibleDC(&dc))
		{
			CBitmap* pOldBM = dcMem.SelectObject(&m_imgSplash);
			BITMAP BM;
			m_imgSplash.GetBitmap(&BM);
			dc.BitBlt(0, 0, BM.bmWidth, BM.bmHeight, &dcMem, 0, 0, SRCCOPY);
			dcMem.SelectObject(pOldBM);
		}
	}
}
// CSplashScreen message handlers
