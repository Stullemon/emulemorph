#pragma once
#include "ServerList.h"
#include "server.h"
#include "titlemenu.h"
#include "MuleListCtrl.h"

// CServerListCtrl
class CServerList; 
class CServerListCtrl : public CMuleListCtrl 
{
	DECLARE_DYNAMIC(CServerListCtrl)
public:
	CServerListCtrl();
//	void	ShowServers();
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
	afx_msg	void	OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnContextMenu(CWnd* pWnd, CPoint point);
	static	int		CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	virtual BOOL	OnCommand(WPARAM wParam,LPARAM lParam );
	DECLARE_MESSAGE_MAP()
private:
	CServerList*	server_list;
	afx_msg void OnNMLdblclk (NMHDR *pNMHDR, LRESULT *pResult); //<-- mod bb 27.09.02

	// Barry - New methods
	bool StaticServerFileAppend(CServer *server);
	bool StaticServerFileRemove(CServer *server);

};