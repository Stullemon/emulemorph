#pragma once
#include "TreeOptionsCtrlEx.h"
// CPPgMorphShare dialog

class CPPgMorphShare : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgMorphShare)

public:
	CPPgMorphShare();
	virtual ~CPPgMorphShare();

// Dialog Data
	enum { IDD = IDD_PPG_MORPH_SHARE };
protected:

	int m_iPowershareMode; //MORPH - Added by SiRoB, Avoid misusing of powersharing
	int	m_iSpreadbar; //MORPH	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	int m_iHideOS;	//MORPH - Added by SiRoB, SLUGFILLER: hideOS
	bool m_bSelectiveShare;  //MORPH - Added by SiRoB, SLUGFILLER: hideOS
	int m_iShareOnlyTheNeed; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	int m_iPowerShareLimit; //MORPH - Added by SiRoB, POWERSHARE Limit
	int m_iPermissions; //MORPH - Added by SiRoB, Show Permissions
	bool m_bPowershareInternalPrio; //Morph - added by AndCyle, selective PS internal Prio
	bool m_bFolderIcons;
	
	CTreeOptionsCtrlEx m_ctrlTreeOptions;
	bool m_bInitializedTreeOpts;
	HTREEITEM m_htiSFM;
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	HTREEITEM m_htiPowershareMode;
	HTREEITEM m_htiPowershareDisabled;
	HTREEITEM m_htiPowershareActivated;
	HTREEITEM m_htiPowershareAuto;
	HTREEITEM m_htiPowershareLimited;
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing

	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	HTREEITEM m_htiSpreadbar;
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file

	HTREEITEM m_htiHideOS;	//MORPH - Added by SiRoB, SLUGFILLER: hideOS
	HTREEITEM m_htiSelectiveShare; //MORPH - Added by SiRoB, SLUGFILLER: hideOS
	HTREEITEM m_htiShareOnlyTheNeed; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	HTREEITEM m_htiPowerShareLimit; //MORPH - Added by SiRoB, POWERSHARE Limit
	HTREEITEM m_htiPowershareInternalPrio; //Morph - added by AndCyle, selective PS internal Prio
	//MORPH START - Added by SiRoB, Show Permission
	HTREEITEM m_htiPermissions;
	HTREEITEM m_htiPermAll;
	HTREEITEM m_htiPermFriend;
	HTREEITEM m_htiPermNone;
	// Mighty Knife: Community visible filelist
	HTREEITEM m_htiPermCommunity;
	// [end] Mighty Knife
	//MORPH END   - Added by SiRoB, Show Permission
	HTREEITEM m_htiDisplay;
	HTREEITEM m_htiFolderIcons;
	
	afx_msg void OnDestroy();
	afx_msg LRESULT OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo); // leuk_he :no help and no stack overflow. 
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	void Localize(void);	
	void LoadSettings(void);
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	virtual BOOL OnKillActive();
	afx_msg void OnSettingsChange()			{ SetModified(); }
};
