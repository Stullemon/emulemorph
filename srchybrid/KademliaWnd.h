#pragma once
#include "ResizableLib\ResizableDialog.h"

class CKadContactListCtrl;
class CKadSearchListCtrl;

// KademliaWnd dialog

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

	void Localize();
	void UpdateControlsState();
	BOOL SaveAllSettings();

// Dialog Data
	enum { IDD = IDD_KADEMLIAWND };

protected:
	CCustomAutoComplete* m_pacONBSIPs;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedBootstrapbutton();
	afx_msg void OnBnConnect();
	afx_msg void OnBnClickedFirewallcheckbutton();
};
