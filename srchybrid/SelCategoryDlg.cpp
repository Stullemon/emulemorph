// khaos::categorymod+
// SelCategoryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "SelCategoryDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// CSelCategoryDlg dialog

IMPLEMENT_DYNAMIC(CSelCategoryDlg, CDialog)

CSelCategoryDlg::CSelCategoryDlg(CWnd* pWnd)
	: CDialog(CSelCategoryDlg::IDD, pWnd)
{
	// If they have selected to use the active category as the default
	// when adding links, then set m_Return to it.  Otherwise, use 'All' (0).
	if (theApp.glob_prefs->UseActiveCatForLinks())
		m_Return =	theApp.emuledlg->transferwnd.GetActiveCategory();
	else
		m_Return = 0;

	m_bCreatedNew = false;
}

CSelCategoryDlg::~CSelCategoryDlg()
{
}

void CSelCategoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSelCategoryDlg, CDialog)
END_MESSAGE_MAP()

BOOL CSelCategoryDlg::OnInitDialog()
{
	// Load the language strings.
	GetDlgItem(IDC_STATIC_INS)->SetWindowText(GetResString(IDS_CAT_SELDLGTXT));
	GetDlgItem(IDCANCEL)->SetWindowText(GetResString(IDS_CANCEL));
	SetWindowText(GetResString(IDS_CAT_SELDLGCAP));

	// 'All' is always an option.
	((CComboBox*)GetDlgItem(IDC_CATCOMBO))->AddString(GetResString(IDS_ALL) + "/" + GetResString(IDS_CAT_UNASSIGN));

	// If there are more categories, add them to the list.
	if (theApp.glob_prefs->GetCatCount() > 1)
		for (int i=1; i < theApp.glob_prefs->GetCatCount(); i++)
					((CComboBox*)GetDlgItem(IDC_CATCOMBO))->AddString(theApp.glob_prefs->GetCategory(i)->title);

	// Select the category that is currently visible in the transfer dialog as default, or 0 if they are
	// not using "New Downloads Default To Active Category"
	((CComboBox*)GetDlgItem(IDC_CATCOMBO))->SetCurSel(theApp.glob_prefs->UseActiveCatForLinks()?theApp.emuledlg->transferwnd.GetActiveCategory():0);

	return TRUE;
}

void CSelCategoryDlg::OnOK()
{
	int	comboIndex = ((CComboBox*)GetDlgItem(IDC_CATCOMBO))->GetCurSel();

	CString* catTitle = new CString(theApp.glob_prefs->GetCategory(comboIndex)->title);
	catTitle->Trim();

	CString	comboText;
	((CComboBox*)GetDlgItem(IDC_CATCOMBO))->GetWindowText(comboText);
	comboText.Trim();

	if (catTitle->CompareNoCase(comboText) == 0 || (comboIndex == 0 && comboText.Compare(GetResString(IDS_ALL) + "/" + GetResString(IDS_CAT_UNASSIGN)) == 0))
		m_Return = comboIndex;
	else {
		m_bCreatedNew = true;
		m_Return = theApp.emuledlg->transferwnd.AddCategorie(comboText, theApp.glob_prefs->GetIncomingDir(), "");
	}

	delete catTitle;
	catTitle = NULL;

	CDialog::OnOK();
}

void CSelCategoryDlg::OnCancel()
{
	// m_Return will still be default, so we don't have to do a darn thing here.
	CDialog::OnCancel();
}