#pragma once
#include "DialogMinTrayBtn.h"
#include "ResizableLib\ResizableDialog.h"

class CTrayDialog : public CDialogMinTrayBtn<CResizableDialog>{
	// Construction
protected:
	typedef CDialogMinTrayBtn<CResizableDialog> CTrayDialogBase;

public:
	void TraySetMinimizeToTray(uint8* bMinimizeToTray);
	BOOL TraySetMenu(UINT nResourceID,UINT nDefaultPos=0);	
	BOOL TraySetMenu(HMENU hMenu,UINT nDefaultPos=0);	
	BOOL TraySetMenu(LPCTSTR lpszMenuName,UINT nDefaultPos=0);	
	BOOL TrayUpdate();
	BOOL TrayShow();
	BOOL TrayHide();
	void TraySetToolTip(LPCTSTR lpszToolTip);
	void TraySetIcon(HICON hIcon, bool bDelete= false);
	void TraySetIcon(UINT nResourceID, bool bDelete= false);
	void TraySetIcon(LPCTSTR lpszResourceName, bool bDelete= false);
	void TrayMinimizeToTrayChange();

	BOOL TrayIsVisible();
	CTrayDialog(UINT uIDD,CWnd* pParent = NULL);   // standard constructor
	
	virtual void OnTrayLButtonDown(CPoint pt);
	virtual void OnTrayLButtonDblClk(CPoint pt);
	
	virtual void OnTrayRButtonUp(CPoint pt);
	virtual void OnTrayRButtonDblClk(CPoint pt);

	virtual void OnTrayMouseMove(CPoint pt);

// Implementation
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);	
	
	DECLARE_MESSAGE_MAP()	

private:
	afx_msg LRESULT OnTrayNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTaskBarCreated(WPARAM wParam, LPARAM lParam);

	uint8*			m_bMinimizeToTray;
    bool			m_bCurIconDelete;   // #zegzav (added)
    HICON           m_hPrevIconDelete;  // #zegzav (added)
	bool			m_bdoubleclicked;
	BOOL			m_bTrayIconVisible;
	NOTIFYICONDATA	m_nidIconData;
	CMenu			m_mnuTrayMenu;
	UINT			m_nDefaultMenuItem;
};
