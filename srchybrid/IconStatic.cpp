// IconStatic.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "IconStatic.h"
#include "VisualStylesXP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIconStatic

CIconStatic::CIconStatic()
{
	m_strText = "";
	m_nIconID = 0;     // i_a 
}

CIconStatic::~CIconStatic()
{
	m_MemBMP.DeleteObject();
}


BEGIN_MESSAGE_MAP(CIconStatic, CStatic)
	//{{AFX_MSG_MAP(CIconStatic)
		ON_WM_SYSCOLORCHANGE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CIconStatic 
bool CIconStatic::Init(UINT nIconID)
{
	m_nIconID = nIconID;

	CString strText;	
	GetWindowText(strText);
	SetWindowText("");
	if(strText != "")
		m_strText = strText;

	CRect rRect;
	GetClientRect(rRect);

	CDC *pDC = GetDC();
	CDC MemDC;
	CBitmap *pOldBMP;
	
	MemDC.CreateCompatibleDC(pDC);

	CFont *pOldFont = MemDC.SelectObject(GetFont());

	CRect rCaption(0,0,0,0);
	MemDC.DrawText(m_strText, rCaption, DT_CALCRECT);
	if(rCaption.Height() < 16)
		rCaption.bottom = rCaption.top + 16;
	rCaption.right += 25;
	if(rCaption.Width() > rRect.Width() - 16)
		rCaption.right = rCaption.left + rRect.Width() - 16;

	m_MemBMP.DeleteObject();
	m_MemBMP.CreateCompatibleBitmap(pDC, rCaption.Width(), rCaption.Height());
	pOldBMP = MemDC.SelectObject(&m_MemBMP);

	MemDC.FillSolidRect(rCaption, GetSysColor(COLOR_BTNFACE));
	
    DrawState( MemDC.m_hDC, NULL, NULL,
		(LPARAM)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_nIconID), IMAGE_ICON, 16, 16, LR_SHARED), // added LR_SHARED to make the icon handle reusable
		// i_a: long --> LPARAM -- reduce number of warnings in VC7
		NULL, 3, 0, 16, 16, DST_ICON | DSS_NORMAL);

	rCaption.left += 22;
	
	if(g_xpStyle.IsAppThemed())
    {
		HTHEME hTheme = g_xpStyle.OpenThemeData(NULL, L"BUTTON"); 
		USES_CONVERSION;
		LPOLESTR oleText = T2OLE(m_strText); 
		g_xpStyle.DrawThemeText(hTheme, MemDC.m_hDC, 4, 1, oleText, ocslen (oleText), 
			DT_WORDBREAK | DT_CENTER | DT_WORD_ELLIPSIS, NULL, &rCaption); 
		g_xpStyle.CloseThemeData(hTheme);
	}
	else
	{	
		MemDC.SetTextColor(pDC->GetTextColor());
		MemDC.DrawText(m_strText, rCaption, DT_SINGLELINE | DT_LEFT | DT_END_ELLIPSIS);
	}

	ReleaseDC( pDC );

	MemDC.SelectObject(pOldBMP);
	MemDC.SelectObject(pOldFont);
	
	if(m_wndPicture.m_hWnd == NULL)
		m_wndPicture.Create(NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP, CRect(0,0,0,0), this);

	m_wndPicture.SetWindowPos(NULL, rRect.left+8, rRect.top, rCaption.Width()+22, rCaption.Height(), SWP_SHOWWINDOW);
	m_wndPicture.SetBitmap(m_MemBMP);

	CWnd *pParent = GetParent();
	if(pParent == NULL)
		pParent = GetDesktopWindow();
	
	CRect r;
	GetWindowRect(r);
	r.bottom = r.top + 20;
	GetParent()->ScreenToClient(&r);
	GetParent()->RedrawWindow(r);

	return true;
}

bool CIconStatic::SetText(CString strText)
{
	m_strText = strText;
	return Init(m_nIconID);
}

bool CIconStatic::SetIcon(UINT nIconID)
{
	return Init(nIconID);
}

void CIconStatic::OnSysColorChange() {
	Init(m_nIconID);
}
