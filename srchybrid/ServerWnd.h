//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#include "ResizableLib\ResizableDialog.h"
#include "ServerListCtrl.h"
#include "LogEditCtrl.h"
#include "IconStatic.h"
#include "RichEditCtrlX.h"
#include "ClosableTabCtrl.h"
//MORPH START - Added by SiRoB, XML News [O�]
#include "types.h" // Added by N_OxYdE: XML News
//MORPH END    - Added by SiRoB, XML News [O�]

class CHTRichEditCtrl;
class CCustomAutoComplete;

// Mighty Knife: News feeds
// grrr, why is this d*mn "pug" implementation completely contained in a
// .h-file and not splitted in a .h/.cpp pair... 
// therefore we mustn't include the .h-file here, otherwise there will be linker
// errors :(
namespace pug {
	class xml_node;
}
// [end] Mighty Knife

class CServerWnd : public CResizableDialog, public CLoggable
{
	DECLARE_DYNAMIC(CServerWnd)

public:
	CServerWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CServerWnd();
	void Localize();
	bool UpdateServerMetFromURL(CString strURL);
	void ToggleDebugWindow();
	void UpdateMyInfo();
	void UpdateLogTabSelection();
	void SaveAllSettings();
	BOOL SaveServerMetStrings();
	void ShowNetworkInfo();
	void UpdateControlsState();
	void ResetHistory();
	void PasteServerFromClipboard();
	bool AddServer(uint16 uPort, CString strIP, CString strName = _T(""), bool bShowErrorMB = true);

	//MORPH START - Added by SiRoB, XML News [O�]
	void ListFeeds();
	void DownloadFeed();
	void DownloadAllFeeds(); //Commander - Added: Update All Feeds at once
	void ParseNewsNode(pug::xml_node _node, CString _xmlbuffer);
	void ParseNewsFile(CString strTempFilename);
	void OnFeedListSelChange();
	CArray<CString> aFeedUrls;
	CArray<CString> aXMLUrls;
	CArray<CString> aXMLNames;
	//MORPH END   - Added by SiRoB, XML News [O�]

// Dialog Data
	enum { IDD = IDD_SERVER };

	enum ELogPaneItems
	{
		PaneServerInfo	= 0, // those are CTabCtrl item indices
		PaneLog			= 1,
		//MORPH START - Changed by SiRoB, XML News [O�]
		/*
		PaneVerboseLog	= 2
		*/
		PaneNews 		= 2,
		PaneVerboseLog	= 3
		//MORPH END   - Changed by SiRoB, XML News [O�]
	};

	CServerListCtrl serverlistctrl;
	CHTRichEditCtrl* servermsgbox;
	CLogEditCtrl logbox;
	CLogEditCtrl debuglog;
	CClosableTabCtrl StatusSelector;

	//MORPH START - Added by SiRoB, XML News [O�]
	CHTRichEditCtrl* newsmsgbox;
	CComboBox m_feedlist;
	afx_msg void OnEnLinkNewsBox(NMHDR *pNMHDR, LRESULT *pResult);
	//MORPH END - Added by SiRoB, XML News [O�]
	
	// Mighty Knife: Context menu for editing news feeds
	CMenu m_FeedsMenu;
	void ReadXMLList (CStringList& _names, CStringList& _urls);
	void WriteXMLList (CStringList& _names, CStringList& _urls);
	// [end] Mighty Knife

protected:
	void SetAllIcons();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedAddserver();
	afx_msg void OnBnClickedUpdateservermetfromurl();
	afx_msg void OnBnClickedResetLog();
	afx_msg void OnBnConnect();
	afx_msg void OnTcnSelchangeTab3(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnLinkServerBox(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();
	afx_msg void OnDDClicked();
	afx_msg void OnSvrTextChange();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	CIconStatic m_ctrlNewServerFrm;
	CIconStatic m_ctrlUpdateServerFrm;
	CIconStatic m_ctrlMyInfo;
	CImageList m_imlLogPanes;
	HICON icon_srvlist;
	bool	debug;
	//MORPH START - Added by SiRoB, XML News [O�]
	bool	news;
	//MORPH END   - Added by SiRoB, XML News [O�]
	CRichEditCtrlX m_MyInfo;
	CHARFORMAT m_cfDef;
	CHARFORMAT m_cfBold;
	CCustomAutoComplete* m_pacServerMetURL;
	CString m_strClickNewVersion;
	LCID m_uLangID;
public:
	afx_msg void OnBnClickedFeedchange();
};