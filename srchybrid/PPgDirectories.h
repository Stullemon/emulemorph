#pragma once
#include "DirectoryTreeCtrl.h"

class CPPgDirectories : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgDirectories)

public:
	CPPgDirectories();									// standard constructor
	virtual ~CPPgDirectories();

// Dialog Data
	enum { IDD = IDD_PPG_DIRECTORIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
public:
	virtual BOOL OnApply();
	afx_msg void OnSettingsChange()					{ SetModified(); }
	afx_msg void OnBnClickedSelincdir();
	afx_msg void OnBnClickedSeltempdir();
	afx_msg void OnBnClickedAddUNC();
	afx_msg void OnBnClickedRemUNC();
	void Localize(void);
	CDirectoryTreeCtrl m_ShareSelector;

private:
	void LoadSettings(void);
	CListCtrl* m_uncfolders;
	void FillUncList(void);
};
