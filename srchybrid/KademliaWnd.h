#pragma once
#include "ResizableLib\ResizableDialog.h"
#include "IconStatic.h"

class CKadContactListCtrl;
class CKadSearchListCtrl;
class CCustomAutoComplete;

class CKademliaWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CKademliaWnd)

public:
	CKademliaWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CKademliaWnd();

	CKadContactListCtrl* contactList;
	CKadSearchListCtrl* searchList;
	CStatic kadContactLab;
	CStatic kadSearchLab;
	CIconStatic m_ctrlBootstrap;

	void Localize();
	void UpdateControlsState();
	BOOL SaveAllSettings();

// Dialog Data
	enum { IDD = IDD_KADEMLIAWND };

protected:
	CCustomAutoComplete* m_pacONBSIPs;

	void SetAllIcons();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedBootstrapbutton();
	afx_msg void OnBnConnect();
	afx_msg void OnBnClickedFirewallcheckbutton();
	afx_msg void OnSysColorChange();
	afx_msg void OnEnSetfocusBootstrapip();

private:
	HICON icon_kadcont;
	HICON icon_kadsea;

};
