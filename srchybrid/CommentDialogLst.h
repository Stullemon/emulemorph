#pragma once 

#include "ResizableLib/ResizablePage.h"

///////////////////////////////////////////////////////////////////////////////
// CCommentDialogLst

class CCommentDialogLst : public CResizablePage
{ 
	DECLARE_DYNAMIC(CCommentDialogLst) 

public: 
	CCommentDialogLst(); 
	virtual ~CCommentDialogLst(); 

	void SetMyfile(CPartFile* file)	{m_file=file;}

// Dialog Data 
	enum { IDD = IDD_COMMENTLST }; 
protected: 
	CString m_strCaption;
	CListCtrl pmyListCtrl;
	CPartFile* m_file; 

	void Localize(); 
	void CompleteList(); 

	virtual BOOL OnInitDialog(); 
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support 

	DECLARE_MESSAGE_MAP() 
	afx_msg void OnBnClickedApply(); 
	afx_msg void OnBnClickedRefresh(); 
	afx_msg void OnNMDblclkLst(NMHDR *pNMHDR, LRESULT *pResult);
};