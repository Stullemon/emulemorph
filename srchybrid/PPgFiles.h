#pragma once

class CPPgFiles : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgFiles)

public:
	CPPgFiles();
	virtual ~CPPgFiles();

// Dialog Data
	enum { IDD = IDD_PPG_FILES };

	void Localize(void);

protected:
	CListBox m_uncfolders;

	void LoadSettings(void);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

	afx_msg void OnSetCleanupFilter();
	afx_msg void BrowseVideoplayer();
	afx_msg void OnSettingsChange() {
		SetModified();
		GetDlgItem(IDC_STARTNEXTFILECAT)->EnableWindow(IsDlgButtonChecked(IDC_STARTNEXTFILE));
	}
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
};
