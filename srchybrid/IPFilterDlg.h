#pragma once
#include "ResizableLib/ResizableDialog.h"
#include "ListCtrlX.h"

class CIPFilterDlg : public CResizableDialog
{
	DECLARE_DYNAMIC(CIPFilterDlg)

public:
	CIPFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CIPFilterDlg();

// Dialog Data
	enum { IDD = IDD_IPFILTER };

protected:
	CMenu* m_pMenuIPFilter;
	CListCtrlX m_ipfilter;
	ULONG m_ulFilteredIPs;
	void UpdateItems();
	void InitIPFilters();
	static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static int sm_iSortColumn;

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnLvnColumnClickIPFilter(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeyDownIPFilter(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnDeleteAllItemsIPfilter(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedAppend();
	afx_msg void OnBnClickedCopy();
	afx_msg void OnBnClickedDelete();
	afx_msg void OnBnClickedSave();
	afx_msg void OnCopyIPFilter();
	afx_msg void OnDeleteIPFilter();
	afx_msg void OnSelectAllIPFilter();
	afx_msg void OnFind();
};
