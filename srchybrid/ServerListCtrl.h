#pragma once
#include "MuleListCtrl.h"

class CServerList; 
class CServer;

class CServerListCtrl : public CMuleListCtrl 
{
	DECLARE_DYNAMIC(CServerListCtrl)
public:
	CServerListCtrl();
	virtual ~CServerListCtrl();

	bool	Init(CServerList* in_list);
	bool	AddServer(CServer* toadd,bool bAddToList = true);
	void	RemoveServer(CServer* todel,bool bDelToList = true);
	bool	AddServermetToList(CString strFile);
	void	RefreshServer(CServer* server);
	void	RemoveDeadServer();
	void	Hide() {ShowWindow(SW_HIDE);}
	void	Visable() {ShowWindow(SW_SHOW);}
	void	Localize();
	void	ShowFilesCount();

protected:
	void SetAllIcons();
	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSysColorChange();
	afx_msg	void OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMLdblclk (NMHDR *pNMHDR, LRESULT *pResult);

private:
	CServerList*	server_list;

	// Barry - New methods
	bool StaticServerFileAppend(CServer *server);
	bool StaticServerFileRemove(CServer *server);
};