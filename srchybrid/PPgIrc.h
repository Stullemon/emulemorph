#pragma once

class CPPgIRC : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgIRC)

public:
	CPPgIRC();
	virtual ~CPPgIRC();

// Dialog Data
	enum { IDD = IDD_PPG_IRC };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	void LoadSettings(void);
	bool m_bnickModified;
public:
	virtual BOOL OnApply();
	afx_msg void OnSettingsChange()					{ SetModified(); }
	afx_msg void OnEnChangeNick()					{ SetModified(); m_bnickModified = true;}
	afx_msg void OnBtnClickPerform();
	void Localize(void);
};
