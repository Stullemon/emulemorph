#pragma once

#define IDC_TOOLBAR			16127
#define IDC_TOOLBARBUTTON	16129

class CMuleToolbarCtrl : public CToolBarCtrl
{
	DECLARE_DYNAMIC(CMuleToolbarCtrl)

public:
	CMuleToolbarCtrl();
	virtual ~CMuleToolbarCtrl();

	void Init(void);
	void Localize(void);
	void ReloadConfig();

	// Customization might splits up the button-group, so we have to (un-)press them on our own
	void PressMuleButton(int nID)
	{
		if(m_iLastPressedButton != -1)
			CheckButton(m_iLastPressedButton, FALSE);
		CheckButton(nID, TRUE);
		m_iLastPressedButton = nID;
	}

protected:
	int			m_iLastPressedButton;
	int			m_buttoncount;
	TBBUTTON	TBButtons[12];
	TCHAR		TBStrings[12][200];
	CStringArray bitmappaths;
	int			m_iToolbarLabelSettings;
	CStringArray aSkinPaths;

	virtual		BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSysColorChange();
	afx_msg void OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnQueryDelete(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnQueryInsert(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnGetButtonInfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnToolbarChange(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnReset(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnInitCustomize(NMHDR *pNMHDR, LRESULT *pResult);

	void SetBtnWidth();

	// [FoRcHa]
	void ChangeToolbarBitmap(CString path, bool refresh);
	void ChangeTextLabelStyle(int settings, bool refresh);
	void Refresh();
};
