#pragma once

class CPPgMorph3 : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgMorph3)

public:
	CPPgMorph3();
	virtual ~CPPgMorph3();

// Dialog Data
	enum { IDD = IDD_PPG_MORPH3 };

	void Localize(void);

protected:
	bool m_bModified;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	void SetModified(BOOL bChanged = TRUE){
		m_bModified = bChanged;
		CPropertyPage::SetModified(bChanged);
	}

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSettingsChange() {SetModified();}
    afx_msg void OnDestroy();
	afx_msg void OnDataChange()		{SetModified();}
	afx_msg void OnEnChangeDynDNSEnabled();
};
