// khaos::categorymod+
#pragma once


// CSelCategoryDlg dialog

class CSelCategoryDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelCategoryDlg)

public:
	CSelCategoryDlg(CWnd* pWnd = NULL);
	virtual	~CSelCategoryDlg();

	virtual BOOL	OnInitDialog();
	afx_msg void	OnOK();
	afx_msg void	OnCancel();
	
	uint8			GetInput()		{ return m_Return; }
	bool			CreatedNewCat()	{ return m_bCreatedNew; }

// Dialog Data
	enum { IDD = IDD_SELCATEGORY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	uint8	m_Return;
	bool	m_bCreatedNew;
};
