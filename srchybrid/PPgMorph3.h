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
protected:
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
	// End MoNKi

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
        afx_msg LRESULT OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam);

public:
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	virtual BOOL OnKillActive();
	void Localize(void);
	void LoadSettings(void);
	afx_msg void OnSettingsChange() {SetModified();}
        afx_msg void OnDestroy();
};
