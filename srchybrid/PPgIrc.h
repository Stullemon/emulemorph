#pragma once
#include "TreeOptionsCtrlEx.h"

class CPPgIRC : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgIRC)

public:
	CPPgIRC();
	virtual ~CPPgIRC();

// Dialog Data
	enum { IDD = IDD_PPG_IRC };

	void Localize(void);

protected:
	int m_iSoundEvents;
	int m_iTimeStamp;
	int m_iInfoMessage;
	int m_iMiscMessage;
	int m_iJoinMessage;
	int m_iPartMessage;
	int m_iQuitMessage;
	int m_iEmuleProto;
	int m_iAcceptLinks;
	int m_iAcceptLinksFriends;
	int m_iHelpChannel;

	CTreeOptionsCtrlEx m_ctrlTreeOptions;
	bool m_bInitializedTreeOpts;
	HTREEITEM m_htiSoundEvents;
	HTREEITEM m_htiTimeStamp;
	HTREEITEM m_htiInfoMessage;
	HTREEITEM m_htiMiscMessage;
	HTREEITEM m_htiJoinMessage;
	HTREEITEM m_htiPartMessage;
	HTREEITEM m_htiQuitMessage;
	HTREEITEM m_htiEmuleProto;
	HTREEITEM m_htiAcceptLinks;
	HTREEITEM m_htiAcceptLinksFriends;
	HTREEITEM m_htiHelpChannel;

	bool m_bnickModified;
	void LoadSettings(void);
	void UpdateControls();

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnSettingsChange()					{ SetModified(); }
	afx_msg void OnEnChangeNick()					{ SetModified(); m_bnickModified = true;}
	afx_msg void OnBtnClickPerform();
	afx_msg LRESULT OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam);
};
