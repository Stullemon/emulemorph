#pragma once
#include "TreeOptionsCtrlEx.h"

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
    bool	m_bInitializedTreeOpts;
	CTreeOptionsCtrlEx	m_ctrlTreeOptions;

	// Added by MoNKi [ MoNKi: -Wap Server- ]
	HTREEITEM	m_htiWapRoot;
	HTREEITEM	m_htiWapEnable;
	HTREEITEM	m_htiWapPort;
	HTREEITEM	m_htiWapTemplate;
	HTREEITEM	m_htiWapPass;
	HTREEITEM	m_htiWapLowEnable;
	HTREEITEM	m_htiWapLowPass;
	BOOL		m_bWapEnable;
	int			m_iWapPort;
	CString		m_sWapTemplate;
	CString		m_sWapPass;
	BOOL		m_bWapLowEnable;
	CString		m_sWapLowPass;
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	void LoadSettings(void);

	void SetModified(BOOL bChanged = TRUE){
		m_bModified = bChanged;
		CPropertyPage::SetModified(bChanged);
	}

	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSettingsChange() {SetModified();}
    afx_msg void OnDestroy();
	afx_msg void OnDataChange()		{SetModified();}
	afx_msg void OnEnChangeDynDNSEnabled();
};
