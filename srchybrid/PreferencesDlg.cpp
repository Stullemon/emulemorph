// PreferencesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PreferencesDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPreferencesDlg

IMPLEMENT_DYNAMIC(CPreferencesDlg, CPropertySheet)
CPreferencesDlg::CPreferencesDlg(){
	this->m_psh.dwFlags &= ~PSH_HASHELP;
	m_wndGeneral.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndDisplay.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndConnection.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndServer.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndDirectories.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndFiles.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndStats.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndIRC.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndWebServer.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndTweaks.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndSecurity.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndMorph.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by IceCream, Morph Prefs
	m_wndMorph2.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, Morph Prefs
	m_wndScheduler.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndProxy.m_psp.dwFlags &= ~PSH_HASHELP; // deadlake PROXYSUPPORT
	m_wndBackup.m_psp.dwFlags &= ~PSH_HASHELP; //EastShare - Added by Pretender, TBH-AutoBackup
	m_wndEastShare.m_psp.dwFlags &= ~PSH_HASHELP; //EastShare - Added by Pretender, ES Prefs

	AddPage(&m_wndGeneral);
	AddPage(&m_wndDisplay);
	AddPage(&m_wndConnection);
	AddPage(&m_wndProxy);
	AddPage(&m_wndServer);
	AddPage(&m_wndDirectories);
	AddPage(&m_wndFiles);
	AddPage(&m_wndNotify);
	AddPage(&m_wndStats);
	AddPage(&m_wndIRC);
	AddPage(&m_wndSecurity);
	AddPage(&m_wndScheduler);
	AddPage(&m_wndWebServer);
	AddPage(&m_wndTweaks);
	AddPage(&m_wndBackup); //EastShare - Added by Pretender, TBH-AutoBackup
	AddPage(&m_wndMorph); //MORPH - Added by IceCream, Morph Prefs
	AddPage(&m_wndMorph2); //MORPH - Added by SiRoB, Morph Prefs
	AddPage(&m_wndEastShare); //EastShare - Added by Pretender, ES Prefs
	m_nActiveWnd = 0;
	m_iPrevPage = -1;
	isEnlarged = false; // EastShare, Added by TAHO, enlarge Windows
}

CPreferencesDlg::~CPreferencesDlg()
{
}

BEGIN_MESSAGE_MAP(CPreferencesDlg, CPropertySheet)
	ON_WM_DESTROY()
	ON_LBN_SELCHANGE(111,OnSelChanged)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

void CPreferencesDlg::OnDestroy()
{
	CPropertySheet::OnDestroy();
	app_prefs->Save();
	m_nActiveWnd = GetActiveIndex();
	isEnlarged = false; // EastShare, Added by TAHO, enlarge Windows
}

BOOL CPreferencesDlg::OnInitDialog()
{		
	EnableStackedTabs(FALSE);
	BOOL bResult = CPropertySheet::OnInitDialog();

	m_listbox.CreateEx(WS_EX_CLIENTEDGE,"Listbox",0,WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_TABSTOP|LBS_HASSTRINGS|LBS_OWNERDRAWVARIABLE|WS_BORDER,CRect(0,0,0,0),this,111);
	::SendMessage(m_listbox.m_hWnd, WM_SETFONT, (WPARAM) ::GetStockObject(DEFAULT_GUI_FONT),0);
	m_groupbox.Create(0,BS_GROUPBOX|WS_CHILD|WS_VISIBLE|BS_FLAT,CRect(0,0,0,0),this,666);
	::SendMessage(m_groupbox.m_hWnd, WM_SETFONT, (WPARAM) ::GetStockObject(DEFAULT_GUI_FONT),0);
	InitWindowStyles(this);

	SetActivePage(m_nActiveWnd);
	Localize();	
	m_listbox.SetFocus();
	CString currenttext;
	int curSel=m_listbox.GetCurSel();
	m_listbox.GetText(curSel,currenttext);
	m_groupbox.SetWindowText(currenttext);
	m_iPrevPage = curSel;
	isEnlarged = false; // EastShare , Added by TAHO, enlarge Windows
	return bResult;
}

void CPreferencesDlg::OnSelChanged()
{
	int curSel=m_listbox.GetCurSel();
	if (!SetActivePage(curSel)){
		if (m_iPrevPage != -1){
			m_listbox.SetCurSel(m_iPrevPage);
			return;
		}
	}
	CString currenttext;
	m_listbox.GetText(curSel,currenttext);
	m_groupbox.SetWindowText(currenttext);
	m_listbox.SetFocus();
	m_iPrevPage = curSel;
}

void CPreferencesDlg::Localize()
{
	ImageList.DeleteImageList();
	ImageList.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	ImageList.Add(CTempIconLoader("PREF_GENERAL"));
	ImageList.Add(CTempIconLoader("PREF_DISPLAY"));
	ImageList.Add(CTempIconLoader("PREF_CONNECTION"));
	ImageList.Add(CTempIconLoader("PREF_PROXY"));
	ImageList.Add(CTempIconLoader("PREF_SERVER"));
	ImageList.Add(CTempIconLoader("PREF_FOLDERS"));
	ImageList.Add(CTempIconLoader("PREF_FILES"));
	ImageList.Add(CTempIconLoader("PREF_NOTIFICATIONS"));
	ImageList.Add(CTempIconLoader("PREF_STATISTICS"));
	ImageList.Add(CTempIconLoader("PREF_IRC"));
	ImageList.Add(CTempIconLoader("PREF_SECURITY"));
	ImageList.Add(CTempIconLoader("PREF_SCHEDULER"));
	ImageList.Add(CTempIconLoader("PREF_WEBSERVER"));
	ImageList.Add(CTempIconLoader("PREF_TWEAK"));
	ImageList.Add(CTempIconLoader("PREF_BACKUP")); //EastShare - Added by Pretender, TBH-AutoBackup
	ImageList.Add(CTempIconLoader("PREF_TWEAK"));  //MORPH - Added by IceCream, Morph Prefs
	ImageList.Add(CTempIconLoader("PREF_TWEAK"));  //MORPH - Added by SiRoB, Morph Prefs
	ImageList.Add(CTempIconLoader("PREF_TWEAK"));  //MORPH - Added by IceCream, Morph Prefs  //EastShare - Modified by Pretender
	m_listbox.SetImageList(&ImageList);

	CString title = GetResString(IDS_EM_PREFS); 
	title.Remove('&'); 
	SetTitle(title); 

	m_wndGeneral.Localize();
	m_wndDisplay.Localize();
	m_wndConnection.Localize();
	m_wndServer.Localize();
	m_wndDirectories.Localize();
	m_wndFiles.Localize();
	m_wndStats.Localize();
	m_wndNotify.Localize();
	m_wndIRC.Localize();
	m_wndSecurity.Localize();
	m_wndTweaks.Localize();
	m_wndWebServer.Localize();
	m_wndScheduler.Localize();
	m_wndProxy.Localize();
	m_wndMorph.Localize();//MORPH - Added by IceCream, Morph Prefs
	m_wndMorph2.Localize();//MORPH - Added by SiRoB, Morph Prefs
	
	TC_ITEM item; 
	item.mask = TCIF_TEXT; 

	CStringArray buffer; 
	buffer.Add(GetResString(IDS_PW_GENERAL)); 
	buffer.Add(GetResString(IDS_PW_DISPLAY)); 
	buffer.Add(GetResString(IDS_PW_CONNECTION)); 
	buffer.Add(GetResString(IDS_PW_PROXY)); 
	buffer.Add(GetResString(IDS_PW_SERVER)); 
	buffer.Add(GetResString(IDS_PW_DIR)); 
	buffer.Add(GetResString(IDS_PW_FILES)); 
	buffer.Add(GetResString(IDS_PW_EKDEV_OPTIONS)); 
	buffer.Add(GetResString(IDS_STATSSETUPINFO)); 
	buffer.Add(GetResString(IDS_IRC));
	buffer.Add(GetResString(IDS_SECURITY)); 
	buffer.Add(GetResString(IDS_SCHEDULER));
	buffer.Add(GetResString(IDS_PW_WS));
	buffer.Add(GetResString(IDS_PW_TWEAK)); 
	buffer.Add(GetResString(IDS_BACKUP)); //EastShare - Added by Pretender, TBH-AutoBackup
	buffer.Add("Morph"); //MORPH - Added by IceCream, Morph Prefs
	buffer.Add("Morph II"); //MORPH - Added by SiRoB, Morph Prefs
	buffer.Add("Morph III"); //EastShare - Added by Pretender, ES Prefs
	for (int i = 0; i < buffer.GetCount(); i++)
		buffer[i].Remove(_T('&'));

	m_listbox.ResetContent();
	int width = 0;
	CTabCtrl* tab = GetTabControl();
	CClientDC dc(this);
	CFont *pOldFont = dc.SelectObject(m_listbox.GetFont());
	CSize sz;
	for(int i = 0; i < GetPageCount(); i++) 
	{ 
		item.pszText = buffer[i].GetBuffer(); 
		tab->SetItem (i, &item); 
		buffer[i].ReleaseBuffer();
		m_listbox.AddString(buffer[i].GetBuffer(),i);
		sz = dc.GetTextExtent(buffer[i]);
		if(sz.cx > width)
			width = sz.cx;
	}
	m_groupbox.SetWindowText(GetResString(IDS_PW_GENERAL));
	width+=50;
	CRect rectOld;
	m_listbox.GetWindowRect(&rectOld);
	int xoffset, yoffset;
	if(IsWindowVisible())
	{
		yoffset=0;
		xoffset=width-rectOld.Width();
	}
	else
	{
		xoffset=width-rectOld.Width()+10;
		tab->GetItemRect(0,rectOld);
		yoffset=-rectOld.Height();
	}
	GetWindowRect(rectOld);
	// EastShare START - Modified by TAHO, enlarge Preferences Windows
	int offset2 = (isEnlarged) ? 0 : 30;
	int offset3 = (isEnlarged) ? 0 : 38;
	//SetWindowPos(NULL,0,0,rectOld.Width()+xoffset,rectOld.Height()+yoffset,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
	SetWindowPos(NULL,0,0,rectOld.Width()+xoffset,rectOld.Height()+yoffset+offset2,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
	tab->GetWindowRect (rectOld);
	ScreenToClient (rectOld);
	tab->SetWindowPos(NULL,rectOld.left+xoffset,rectOld.top+yoffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	CPropertyPage* activepage = GetActivePage();
	activepage->GetWindowRect(rectOld);
	ScreenToClient (rectOld);
	activepage->SetWindowPos(NULL,rectOld.left+xoffset,rectOld.top+yoffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	activepage->GetWindowRect(rectOld);
	ScreenToClient (rectOld);
	//m_groupbox.SetWindowPos(NULL,rectOld.left,2,rectOld.Width()+4,rectOld.Height()+10,SWP_NOZORDER|SWP_NOACTIVATE);
	m_groupbox.SetWindowPos(NULL,rectOld.left,2,rectOld.Width()+4,rectOld.Height()+10+offset2,SWP_NOZORDER|SWP_NOACTIVATE);
	m_groupbox.GetWindowRect(rectOld);
	ScreenToClient(rectOld);
	//m_listbox.SetWindowPos(NULL,6,rectOld.top+5,width,rectOld.Height()-4,SWP_NOZORDER|SWP_NOACTIVATE);
	m_listbox.SetWindowPos(NULL,6,rectOld.top+5,width,rectOld.Height()-4+offset3,SWP_NOZORDER|SWP_NOACTIVATE);	// EastShare END - Modified by TAHO, enlarge Preferences Windows
	int _PropSheetButtons[] = {IDOK, IDCANCEL, ID_APPLY_NOW, IDHELP };
	CWnd* PropSheetButton;
	for (int i = 0; i < sizeof (_PropSheetButtons) / sizeof(_PropSheetButtons[0]); i++)
	{
		if (PropSheetButton=GetDlgItem(_PropSheetButtons[i]))
		{
			PropSheetButton->GetWindowRect (rectOld);
			ScreenToClient (rectOld);
			//PropSheetButton->SetWindowPos (NULL, rectOld.left+xoffset,rectOld.top+yoffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
			PropSheetButton->SetWindowPos (NULL, rectOld.left+xoffset,rectOld.top+yoffset+offset2,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}
	isEnlarged = true;
	// EastShare END - Modified by TAHO, enlarge Preferences Windows
	tab->ShowWindow(SW_HIDE);
	m_listbox.SetCurSel(GetActiveIndex());		
	CenterWindow();
	Invalidate();
	RedrawWindow();
	dc.SelectObject(pOldFont); //restore default font object
}

HBRUSH CPreferencesDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CPropertySheet::OnCtlColor(pDC, pWnd, nCtlColor);
	if (m_groupbox.m_hWnd == pWnd->m_hWnd) 
	{
		pDC->SetBkColor(GetSysColor(COLOR_BTNFACE));
		hbr = GetSysColorBrush(COLOR_BTNFACE);
	}
	return hbr;
}