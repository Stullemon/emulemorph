#pragma once
#include "DialogMinTrayBtn.h"
#include "ResizableLib\ResizableDialog.h"

#define	IDT_SINGLE_CLICK	100

class CTrayDialog : public CDialogMinTrayBtn<CResizableDialog>
{
protected:
	typedef CDialogMinTrayBtn<CResizableDialog> CTrayDialogBase;

public:
	CTrayDialog(UINT uIDD, CWnd* pParent = NULL);   // standard constructor

	void TraySetMinimizeToTray(bool* pbMinimizeToTray);
	BOOL TraySetMenu(UINT nResourceID, UINT nDefaultPos = 0);
	BOOL TraySetMenu(HMENU hMenu, UINT nDefaultPos = 0);
	BOOL TraySetMenu(LPCTSTR lpszMenuName, UINT nDefaultPos = 0);
	BOOL TrayUpdate();
	BOOL TrayShow();
	BOOL TrayHide();
	void TraySetToolTip(LPCTSTR lpszToolTip);
	void TraySetIcon(HICON hIcon, bool bDelete = false);
	void TraySetIcon(UINT nResourceID, bool bDelete = false);
	void TraySetIcon(LPCTSTR lpszResourceName, bool bDelete = false);
	void TrayMinimizeToTrayChange();
	BOOL TrayIsVisible();

	virtual void RestoreWindow();
	virtual void OnTrayLButtonDown(CPoint pt);
	virtual void OnTrayLButtonUp(CPoint pt);
	virtual void OnTrayLButtonDblClk(CPoint pt);
	virtual void OnTrayRButtonUp(CPoint pt);
	virtual void OnTrayRButtonDblClk(CPoint pt);
	virtual void OnTrayMouseMove(CPoint pt);

protected:
	bool* m_pbMinimizeToTray;
    bool m_bCurIconDelete;
    HICON m_hPrevIconDelete;
	bool m_bdoubleclicked;
	BOOL m_bTrayIconVisible;
	NOTIFYICONDATA m_nidIconData;
	CMenu m_mnuTrayMenu;
	UINT m_nDefaultMenuItem;
	UINT m_uSingleClickTimer;

	void KillSingleClickTimer();

	DECLARE_MESSAGE_MAP()	
	afx_msg LRESULT OnTrayNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTaskBarCreated(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);	
	afx_msg void OnTimer(UINT nIDEvent);
};
