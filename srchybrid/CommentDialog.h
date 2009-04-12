#pragma once
#include "ResizableLib/ResizablePage.h"
#include "ComboBoxEx2.h"
#include "CommentListCtrl.h"

class CKnownFile;
namespace Kademlia {
	class CEntry;
};

class CCommentDialog : public CResizablePage
{
	DECLARE_DYNAMIC(CCommentDialog)

public:
	CCommentDialog();	// standard constructor
	virtual ~CCommentDialog();

	void SetFiles(const CSimpleArray<CObject*>* paFiles) { m_paFiles = paFiles; m_bDataChanged = true; }

	// Dialog Data
	enum { IDD = IDD_COMMENT };

	void Localize();

protected:
	const CSimpleArray<CObject*>* m_paFiles;
	bool m_bDataChanged;
	CComboBoxEx2 m_ratebox;
	CImageList m_imlRating;
	CCommentListCtrl m_lstComments;
	bool m_bMergedComment;
	bool m_bSelf;
	uint32 m_timer;
	bool m_bEnabled;

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	void RefreshData(bool deleteOld = true);
	void EnableDialog(bool bEnabled);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedSearchKad(); 
	afx_msg void OnBnClickedReset();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
	afx_msg void OnEnChangeCmtText();
	afx_msg void OnCbnSelendokRatelist();
	afx_msg void OnCbnSelchangeRatelist();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
};
