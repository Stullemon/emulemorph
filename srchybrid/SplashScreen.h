#pragma once

#include "enbitmap.h"

class CSplashScreen : public CDialog
{
	DECLARE_DYNAMIC(CSplashScreen)

public:
	CSplashScreen(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSplashScreen();

// Dialog Data
	enum { IDD = IDD_SPLASH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	void OnTimer(UINT_PTR nIDEvent);
	BOOL PreTranslateMessage(MSG* pMsg);
	void OnPaint(); 
private:
	uint32 m_timer;
	uint16 m_translucency;
	CEnBitmap m_imgSplash;
};
