//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "emule.h"
#include "enbitmap.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "emuledlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


///////////////////////////////////////////////////////////////////////////////
// CDlgPageWizard dialog

class CDlgPageWizard : public CPropertyPageEx
{
	DECLARE_DYNCREATE(CDlgPageWizard)

// Construction
public:
	CDlgPageWizard();
	CDlgPageWizard(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CPropertyPageEx(nIDTemplate)
	{
		if (pszCaption)
		{
			m_strCaption = pszCaption; // "convenience storage"
			m_psp.pszTitle = m_strCaption;
			m_psp.dwFlags |= PSP_USETITLE;
		}
		if (pszHeaderTitle && pszHeaderTitle[0] != _T('\0'))
		{
			m_strHeaderTitle = pszHeaderTitle;
			m_psp.dwSize = sizeof(m_psp);
		}
		if (pszHeaderSubTitle && pszHeaderSubTitle[0] != _T('\0'))
		{
			m_strHeaderSubTitle = pszHeaderSubTitle;
			m_psp.dwSize = sizeof(m_psp);
		}
	}
	~CDlgPageWizard(){}

// Dialog Data


// Overrides
public:
	virtual BOOL OnSetActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	CString m_strCaption;
	DECLARE_MESSAGE_MAP()

};

IMPLEMENT_DYNCREATE(CDlgPageWizard, CPropertyPageEx)

BEGIN_MESSAGE_MAP(CDlgPageWizard, CPropertyPageEx)
END_MESSAGE_MAP()

CDlgPageWizard::CDlgPageWizard() 
	: CPropertyPageEx()
{
}

void CDlgPageWizard::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
}

BOOL CDlgPageWizard::OnSetActive() 
{
	CPropertySheetEx* pSheet = (CPropertySheetEx*)GetParent();
	if (pSheet->IsWizard())
	{
		int iPages = pSheet->GetPageCount();
		int iActPage = pSheet->GetActiveIndex();
		DWORD dwButtons = 0;
		if (iActPage > 0)
			dwButtons |= PSWIZB_BACK;
		if (iActPage < iPages)
			dwButtons |= PSWIZB_NEXT;
		if (iActPage == iPages-1)
		{
			if (pSheet->m_psh.dwFlags & PSH_WIZARDHASFINISH)
				dwButtons &= ~PSWIZB_NEXT;
			dwButtons |= PSWIZB_FINISH;
		}
		pSheet->SetWizardButtons(dwButtons);
	}
	return CPropertyPageEx::OnSetActive();
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1Welcome dialog

class CPPgWiz1Welcome : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1Welcome)

public:
	CPPgWiz1Welcome();
	CPPgWiz1Welcome(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
	}
	virtual ~CPPgWiz1Welcome();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_WELCOME };

protected:
	CFont m_FontTitle;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1Welcome, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1Welcome, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1Welcome::CPPgWiz1Welcome()
	: CDlgPageWizard(CPPgWiz1Welcome::IDD)
{
}

CPPgWiz1Welcome::~CPPgWiz1Welcome()
{
}

void CPPgWiz1Welcome::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
}

BOOL CPPgWiz1Welcome::OnInitDialog()
{
	CFont fontVerdanaBold;
	fontVerdanaBold.CreatePointFont(120, _T("Verdana Bold"));
	LOGFONT lf;
	fontVerdanaBold.GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	m_FontTitle.CreateFontIndirect(&lf);

	CStatic* pStatic = (CStatic*)GetDlgItem(IDC_WIZ1_TITLE);
	pStatic->SetFont(&m_FontTitle);

	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_WIZ1_TITLE)->SetWindowText(GetResString(IDS_WIZ1_WELCOME_TITLE));
	GetDlgItem(IDC_WIZ1_ACTIONS)->SetWindowText(GetResString(IDS_WIZ1_WELCOME_ACTIONS));
	GetDlgItem(IDC_WIZ1_BTN_HINT)->SetWindowText(GetResString(IDS_WIZ1_WELCOME_BTN_HINT));
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1General dialog

class CPPgWiz1General : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1General)

public:
	CPPgWiz1General();
	CPPgWiz1General(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
	}
	virtual ~CPPgWiz1General();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_GENERAL };

	CString m_strNick;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1General, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1General, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1General::CPPgWiz1General()
	: CDlgPageWizard(CPPgWiz1General::IDD)
{
}

CPPgWiz1General::~CPPgWiz1General()
{
}

void CPPgWiz1General::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NICK, m_strNick);
}

BOOL CPPgWiz1General::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	((CEdit*)GetDlgItem(IDC_NICK))->SetLimitText(thePrefs.GetMaxUserNickLength());
	GetDlgItem(IDC_NICK_FRM)->SetWindowText(GetResString(IDS_ENTERUSERNAME));
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1DlPrio dialog

class CPPgWiz1DlPrio : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1DlPrio)

public:
	CPPgWiz1DlPrio();
	CPPgWiz1DlPrio(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
		m_iDAP = 1;
	}
	virtual ~CPPgWiz1DlPrio();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_DL_PRIO };

	int m_iDAP;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1DlPrio, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1DlPrio, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1DlPrio::CPPgWiz1DlPrio()
	: CDlgPageWizard(CPPgWiz1DlPrio::IDD)
{
}

CPPgWiz1DlPrio::~CPPgWiz1DlPrio()
{
	m_iDAP = 1;
}

void CPPgWiz1DlPrio::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_DAP, m_iDAP);
}

BOOL CPPgWiz1DlPrio::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_DAP)->SetWindowText(GetResString(IDS_FIRSTAUTODOWN));
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1UlPrio dialog

class CPPgWiz1UlPrio : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1UlPrio)

public:
	CPPgWiz1UlPrio();
	CPPgWiz1UlPrio(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
		m_iUAP = 1;
	}
	virtual ~CPPgWiz1UlPrio();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_UL_PRIO };

	int m_iUAP;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1UlPrio, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1UlPrio, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1UlPrio::CPPgWiz1UlPrio()
	: CDlgPageWizard(CPPgWiz1UlPrio::IDD)
{
	m_iUAP = 1;
}

CPPgWiz1UlPrio::~CPPgWiz1UlPrio()
{
}

void CPPgWiz1UlPrio::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_UAP, m_iUAP);
}

BOOL CPPgWiz1UlPrio::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_UAP)->SetWindowText(GetResString(IDS_FIRSTAUTOUP));
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1Upload dialog

class CPPgWiz1Upload : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1Upload)

public:
	CPPgWiz1Upload();
	CPPgWiz1Upload(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
		m_iULFullChunks = 1;
	}
	virtual ~CPPgWiz1Upload();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_UPLOAD };

	int m_iULFullChunks;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1Upload, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1Upload, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1Upload::CPPgWiz1Upload()
	: CDlgPageWizard(CPPgWiz1Upload::IDD)
{
	m_iULFullChunks = 1;
}

CPPgWiz1Upload::~CPPgWiz1Upload()
{
}

void CPPgWiz1Upload::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_FULLCHUNKTRANS, m_iULFullChunks);
}

BOOL CPPgWiz1Upload::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_FULLCHUNKTRANS)->SetWindowText(GetResString(IDS_FIRSTFULLCHUNK));
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1Server dialog

class CPPgWiz1Server : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1Server)

public:
	CPPgWiz1Server();
	CPPgWiz1Server(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
		m_iSafeServerConnect = 0;
		m_iAutoConnectAtStart = 0;
		m_iKademlia = 0;
		m_iED2K = 1;
	}
	virtual ~CPPgWiz1Server();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_SERVER };

	int m_iSafeServerConnect;
	int m_iAutoConnectAtStart;
	int m_iKademlia;
	int m_iED2K;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1Server, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1Server, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1Server::CPPgWiz1Server()
	: CDlgPageWizard(CPPgWiz1Server::IDD)
{
	m_iSafeServerConnect = 0;
	m_iAutoConnectAtStart = 0;
	m_iKademlia = 0;
	m_iED2K = 1;
}

CPPgWiz1Server::~CPPgWiz1Server()
{
}

void CPPgWiz1Server::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SAFESERVERCONNECT, m_iSafeServerConnect);
	DDX_Check(pDX, IDC_AUTOCONNECT, m_iAutoConnectAtStart);
	DDX_Check(pDX, IDC_WIZARD_NETWORK_KADEMLIA, m_iKademlia);
	DDX_Check(pDX, IDC_WIZARD_NETWORK_ED2K, m_iED2K);
}

BOOL CPPgWiz1Server::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_SAFESERVERCONNECT)->SetWindowText(GetResString(IDS_FIRSTSAFECON));
	GetDlgItem(IDC_AUTOCONNECT)->SetWindowText(GetResString(IDS_FIRSTAUTOCON));
	GetDlgItem(IDC_WIZARD_NETWORK)->SetWindowText(GetResString(IDS_WIZARD_NETWORK));
	GetDlgItem(IDC_WIZARD_ED2K)->SetWindowText(GetResString(IDS_WIZARD_ED2K));
	GetDlgItem(IDC_KADALPHA)->SetWindowText(GetResString(IDS_KADALPHA));

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1End dialog

class CPPgWiz1End : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1End)

public:
	CPPgWiz1End();
	CPPgWiz1End(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
	}
	virtual ~CPPgWiz1End();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_END };

protected:
	CFont m_FontTitle;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1End, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1End, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1End::CPPgWiz1End()
	: CDlgPageWizard(CPPgWiz1End::IDD)
{
}

CPPgWiz1End::~CPPgWiz1End()
{
}

void CPPgWiz1End::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
}

BOOL CPPgWiz1End::OnInitDialog()
{
	CFont fontVerdanaBold;
	fontVerdanaBold.CreatePointFont(120, _T("Verdana Bold"));
	LOGFONT lf;
	fontVerdanaBold.GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	m_FontTitle.CreateFontIndirect(&lf);

	CStatic* pStatic = (CStatic*)GetDlgItem(IDC_WIZ1_TITLE);
	pStatic->SetFont(&m_FontTitle);

	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_WIZ1_TITLE)->SetWindowText(GetResString(IDS_WIZ1_END_TITLE));
	GetDlgItem(IDC_WIZ1_ACTIONS)->SetWindowText(GetResString(IDS_FIRSTCOMPLETE));
	GetDlgItem(IDC_WIZ1_BTN_HINT)->SetWindowText(GetResString(IDS_WIZ1_END_BTN_HINT));

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPShtWiz1

class CPShtWiz1 : public CPropertySheetEx
{
	DECLARE_DYNAMIC(CPShtWiz1)

public:
	CPShtWiz1(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CPShtWiz1();

protected:
	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPShtWiz1, CPropertySheetEx)

BEGIN_MESSAGE_MAP(CPShtWiz1, CPropertySheetEx)
END_MESSAGE_MAP()

CPShtWiz1::CPShtWiz1(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheetEx(nIDCaption, pParentWnd, iSelectPage)
{
}

CPShtWiz1::~CPShtWiz1()
{
}

BOOL FirstTimeWizard()
{
	CEnBitmap bmWatermark;
	VERIFY( bmWatermark.LoadImage(IDR_WIZ1_WATERMARK, _T("GIF"), NULL, GetSysColor(COLOR_WINDOW)) );
	CEnBitmap bmHeader;
	VERIFY( bmHeader.LoadImage(IDR_WIZ1_HEADER, _T("GIF"), NULL, GetSysColor(COLOR_WINDOW)) );
	CPropertySheetEx sheet(GetResString(IDS_WIZ1), NULL, 0, bmWatermark, NULL, bmHeader);
	sheet.m_psh.dwFlags |= PSH_WIZARD;
#ifdef _DEBUG
	sheet.m_psh.dwFlags |= PSH_WIZARDHASFINISH;
#endif
	//sheet.m_psh.dwFlags |= 0x00002000;	// PSH_WIZARD97 for (_WIN32_IE < 0x0500)
	sheet.m_psh.dwFlags |= 0x01000000;		// PSH_WIZARD97 for (_WIN32_IE >= 0x0500)

	CPPgWiz1Welcome	page1(IDD_WIZ1_WELCOME, GetResString(IDS_WIZ1));
	page1.m_psp.dwFlags |= PSP_HIDEHEADER;
	sheet.AddPage(&page1);

	CPPgWiz1General page2(IDD_WIZ1_GENERAL, GetResString(IDS_WIZ1), GetResString(IDS_PW_GENERAL), GetResString(IDS_QL_USERNAME));
	sheet.AddPage(&page2);

	CPPgWiz1DlPrio page3(IDD_WIZ1_DL_PRIO, GetResString(IDS_WIZ1), GetResString(IDS_DOWNLOAD), GetResString(IDS_PRIORITY));
	sheet.AddPage(&page3);
	
	CPPgWiz1UlPrio page4(IDD_WIZ1_UL_PRIO, GetResString(IDS_WIZ1), GetResString(IDS_PW_CON_UPLBL), GetResString(IDS_PRIORITY));
	sheet.AddPage(&page4);
	
	CPPgWiz1Upload page5(IDD_WIZ1_UPLOAD, GetResString(IDS_WIZ1), GetResString(IDS_PW_CON_UPLBL), GetResString(IDS_WIZ1_UPLOAD_SUBTITLE));
	sheet.AddPage(&page5);
	
	CPPgWiz1Server page6(IDD_WIZ1_SERVER, GetResString(IDS_WIZ1), GetResString(IDS_PW_SERVER), GetResString(IDS_PW_CONNECTION));
	sheet.AddPage(&page6);
	
	CPPgWiz1End page7(IDD_WIZ1_END, GetResString(IDS_WIZ1));
	page7.m_psp.dwFlags |= PSP_HIDEHEADER;
	sheet.AddPage(&page7);

	page2.m_strNick = thePrefs.GetUserNick();
	if (page2.m_strNick.IsEmpty())
		page2.m_strNick = DEFAULT_NICK;
	page3.m_iDAP = 1;
	page4.m_iUAP = 1;
	page5.m_iULFullChunks = 1;
	page6.m_iSafeServerConnect = 0;
	page6.m_iAutoConnectAtStart = 0;
	page6.m_iKademlia = 0;
	page6.m_iED2K = 1;

	int iResult = sheet.DoModal();
	if (iResult == IDCANCEL)
		return FALSE;

	page2.m_strNick.Trim();
	if (page2.m_strNick.IsEmpty())
		page2.m_strNick = DEFAULT_NICK;

	thePrefs.SetUserNick(page2.m_strNick);
	thePrefs.SetNewAutoDown(page3.m_iDAP);
	thePrefs.SetNewAutoUp(page4.m_iUAP);
	thePrefs.SetTransferFullChunks(page5.m_iULFullChunks);
	thePrefs.SetSafeServerConnectEnabled(page6.m_iSafeServerConnect);
	thePrefs.SetAutoConnect(page6.m_iAutoConnectAtStart);
	thePrefs.SetNetworkKademlia(page6.m_iKademlia);
	thePrefs.SetNetworkED2K(page6.m_iED2K);

	theApp.emuledlg->SetKadButtonState();
	return TRUE;
}
