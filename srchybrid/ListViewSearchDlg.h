#pragma once

//////////////////////////////////////////////////////////////////////////////
// CListViewSearchDlg

class CListViewSearchDlg : public CDialog
{
	DECLARE_DYNAMIC(CListViewSearchDlg)

public:
	CListViewSearchDlg(CWnd* pParent = NULL);	  // standard constructor

// Dialog Data
	enum { IDD = IDD_LISTVIEW_SEARCH };

	CListCtrl* m_pListView;
	CString m_strFindText;
	int m_iSearchColumn;

protected:
	CComboBox m_ctlSearchCol;

	void UpdateControls();

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnEnChangeSearchText();
	virtual BOOL OnInitDialog();
};
