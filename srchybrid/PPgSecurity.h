#pragma once
#include "loggable.h"

class CCustomAutoComplete;

class CPPgSecurity : public CPropertyPage, public CLoggable
{
	DECLARE_DYNAMIC(CPPgSecurity)

public:
	CPPgSecurity();
	virtual ~CPPgSecurity();

// Dialog Data
	enum { IDD = IDD_PPG_SECURITY };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	void LoadSettings(void);
public:
	CCustomAutoComplete* m_pacIPFilterURL;
	void DeleteDDB();
	virtual BOOL OnApply();
	void Localize(void);
	afx_msg void OnSettingsChange()					{ SetModified(); }
	afx_msg void OnReloadIPFilter();
	afx_msg void OnEditIPFilter();
	afx_msg void OnLoadIPFFromURL();
	afx_msg void OnEnChangeUpdateUrl();
	afx_msg void OnDDClicked();
};
