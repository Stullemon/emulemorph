#pragma once

class CKnownFile;

class CCommentDialog : public CDialog
{
	DECLARE_DYNAMIC(CCommentDialog)
public:
	CCommentDialog(CKnownFile* file);	// standard constructor
	virtual ~CCommentDialog();

	// Dialog Data
	enum { IDD = IDD_COMMENT };

	void Localize();

protected:
	CComboBox	m_ratebox;
	CKnownFile* m_file;

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedReset();
};
