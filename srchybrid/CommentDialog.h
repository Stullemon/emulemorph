#pragma once
#include "ResizableLib/ResizablePage.h"
#include "ComboBoxEx2.h"

class CKnownFile;

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
	bool m_bMergedComment;
	bool m_bSelf;

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedReset();
	afx_msg void OnDestroy();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
	afx_msg void OnEnChangeCmtText();
	afx_msg void OnCbnSelendokRatelist();
	afx_msg void OnCbnSelchangeRatelist();
};
