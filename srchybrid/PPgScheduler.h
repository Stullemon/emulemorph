#pragma once

class CPPgScheduler : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgScheduler)

public:
	CPPgScheduler();
	virtual ~CPPgScheduler();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }


	enum { IDD = IDD_PPG_SCHEDULER };

	CPreferences *app_prefs;

	void Localize(void);

	virtual BOOL OnInitDialog();
	
// Dialog Data
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	CString GetActionLabel(uint8 index);
	CString GetDayLabel(uint8 index);
	void LoadSchedule(uint8 index);
	void RecheckSchedules();
	void FillScheduleList();
	CComboBox m_timesel;
	CDateTimeCtrl m_time;
	CDateTimeCtrl m_timeTo;
	CListCtrl m_list;
	CListCtrl m_actions;
public:
	virtual BOOL OnApply();
	afx_msg void OnNMDblclkList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedApply();
	afx_msg void OnBnClickedRemove();
	afx_msg void OnSettingsChange() {SetModified();}
	afx_msg void OnEnableChange();
	afx_msg void OnDisableTime2();
	afx_msg void OnNMDblclkActionlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickActionlist(NMHDR *pNMHDR, LRESULT *pResult);

};
