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
	void	RemoveServer(CServer* todel);
	bool	AddServermetToList(const CString& strFile);
	void	RefreshServer(const CServer* server);
	void	RefreshAllServer();//EastShare - added by AndCycle, IP to Country
	void	RemoveAllDeadServers();
	void	Hide() {ShowWindow(SW_HIDE);}
	void	Visable() {ShowWindow(SW_SHOW);}
	void	Localize();
	void	ShowServerCount();

protected:
	CServerList*	server_list;

	bool StaticServerFileAppend(CServer *server);
	bool StaticServerFileRemove(const CServer *server);
	void SetAllIcons();
	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

        //Commander - Added: CountryFlag - Start
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	CImageList imagelist;
        //Commander - Added: CountryFlag - End


	DECLARE_MESSAGE_MAP()
	afx_msg void OnSysColorChange();
	afx_msg	void OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMLdblclk (NMHDR *pNMHDR, LRESULT *pResult);
};