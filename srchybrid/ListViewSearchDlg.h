#pragma once

//////////////////////////////////////////////////////////////////////////////
// CListViewSearchDlg

class CListViewSearchDlg : public CDialog
{
	DECLARE_DYNAMIC(CListViewSearchDlg)

public:
	CListViewSearchDlg(CWnd* pParent = NULL);	  // standard constructor
	virtual ~CListViewSearchDlg();

// Dialog Data
	enum { IDD = IDD_LISTVIEW_SEARCH };

	CListCtrl* m_pListView;
	CString m_strFindText;
	int m_iSearchColumn;

protected:
	CComboBox m_ctlSearchCol;

	void UpdateControls();

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnEnChangeSearchText();
};
