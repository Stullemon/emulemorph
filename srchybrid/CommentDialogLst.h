#pragma once 
#include "ResizableLib/ResizablePage.h"

class CPartFile;

///////////////////////////////////////////////////////////////////////////////
// CCommentDialogLst

class CCommentDialogLst : public CResizablePage
{ 
	DECLARE_DYNAMIC(CCommentDialogLst) 

public: 
	CCommentDialogLst(); 
	virtual ~CCommentDialogLst(); 

	void SetFiles(const CSimpleArray<CObject*>* paFiles) { m_paFiles = paFiles; m_bDataChanged = true; }

// Dialog Data 
	enum { IDD = IDD_COMMENTLST }; 

protected: 
	CString m_strCaption;
	CListCtrl m_lstComments;
	const CSimpleArray<CObject*>* m_paFiles;
	bool m_bDataChanged;

	void Localize();
	void RefreshData();

	virtual BOOL OnInitDialog(); 
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support 
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnSetActive();

	DECLARE_MESSAGE_MAP() 
	afx_msg void OnBnClickedApply(); 
	afx_msg void OnBnClickedRefresh(); 
	afx_msg void OnBnClickedSearchKad(); 
	afx_msg void OnNMDblclkLst(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
};
