// DirectoryTreeCtrl.cpp : implementation file
//
/////////////////////////////////////////////
// written by robert rostek - tecxx@rrs.at //
/////////////////////////////////////////////

#include "stdafx.h"
#include "DirectoryTreeCtrl.h"
#include "otherfunctions.h"
#include "emule.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CDirectoryTreeCtrl

IMPLEMENT_DYNAMIC(CDirectoryTreeCtrl, CTreeCtrl)
CDirectoryTreeCtrl::CDirectoryTreeCtrl()
{
	m_bSelectSubDirs = false;
}

CDirectoryTreeCtrl::~CDirectoryTreeCtrl()
{
	// don't destroy the system's image list
	m_image.Detach();
}


BEGIN_MESSAGE_MAP(CDirectoryTreeCtrl, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnTvnItemexpanding)
	ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnTvnGetdispinfo)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclickSharedList)
END_MESSAGE_MAP()



// CDirectoryTreeCtrl message handlers

void CDirectoryTreeCtrl::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	// remove all subitems
	HTREEITEM hRemove = GetChildItem(hItem);
	while(hRemove)
	{
		DeleteItem(hRemove);
		hRemove = GetChildItem(hItem);
	}

	// get the directory
	CString strDir = GetFullPath(hItem);

	// fetch all subdirectories and add them to the node
	AddSubdirectories(hItem, strDir);

	*pResult = 0;
}

void CDirectoryTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point) { 
	//VQB adjustments to provide for sharing or unsharing of subdirectories when control key is Down 
	UINT uFlags; 
	HTREEITEM hItem = HitTest(point, &uFlags); 
	HTREEITEM tItem = GetFirstVisibleItem(); // VQB mark initial window position 
	if ( (hItem) && (uFlags & TVHT_ONITEMSTATEICON)) { 
		CheckChanged(hItem, !GetCheck(hItem)); 
		if (nFlags & MK_CONTROL) { // Is control key down? 
			CWaitCursor curWait;
			Expand(hItem, TVE_TOGGLE); //VQB - make sure tree has entries 
			HTREEITEM hChild; 
			hChild = GetChildItem(hItem); 
			while (hChild != NULL) 
			{ 
				MarkChilds(hChild,!GetCheck(hItem)); 
				hChild = GetNextSiblingItem( hChild ); 
			} 
			Expand(hItem, TVE_TOGGLE); // VQB - restore tree to initial disposition 
		} 
	} 
	SelectSetFirstVisible(tItem); // VQB - restore window scroll to initial position 
	CTreeCtrl::OnLButtonDown(nFlags, point); 
} 

void CDirectoryTreeCtrl::MarkChilds(HTREEITEM hChild,bool mark) { 
	CheckChanged(hChild, mark); 
	SetCheck(hChild,mark);  
	Expand(hChild, TVE_TOGGLE); // VQB - make sure tree has entries 
	HTREEITEM hChild2; 
	hChild2 = GetChildItem(hChild); 
	while( hChild2 != NULL) 
	{ 
		MarkChilds(hChild2,mark); 
		hChild2 = GetNextSiblingItem( hChild2 ); 
	} 
	Expand(hChild, TVE_TOGGLE); // VQB - restore tree to initial disposition 
}

void CDirectoryTreeCtrl::OnTvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	pTVDispInfo->item.cChildren = 1;
	*pResult = 0;
}

void CDirectoryTreeCtrl::Init(void)
{
	ShowWindow(SW_HIDE);
	DeleteAllItems();

	ModifyStyle( 0, TVS_CHECKBOXES );

	// START: added by FoRcHa /////////////
	WORD wWinVer = theApp.glob_prefs->GetWindowsVersion();	// maybe causes problems on 98 & nt4
	if(wWinVer == _WINVER_2K_ || wWinVer == _WINVER_XP_ || wWinVer == _WINVER_ME_)		
	{
		SHFILEINFO shFinfo;
		HIMAGELIST hImgList = NULL;

		// Get the system image list using a "path" which is available on all systems. [patch by bluecow]
		hImgList = (HIMAGELIST)SHGetFileInfo(".", 0, &shFinfo, sizeof(shFinfo),
												SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		if(!hImgList)
		{
			TRACE(_T("Cannot retrieve the Handle of SystemImageList!"));
			//return;
		}

		m_image.m_hImageList = hImgList;
		SetImageList(&m_image, TVSIL_NORMAL);
	}
	////////////////////////////////


	char drivebuffer[500];
	::GetLogicalDriveStrings(500, drivebuffer); // e.g. "a:\ c:\ d:\"

	const char* pos = drivebuffer;
	while(*pos != '\0'){

		// Copy drive name
		char drive[4];
		memccpy(drive, pos, '\0', 4);

		switch(drive[0]){
			case 'a':
			case 'A':
			case 'b':
			case 'B':
			// Skip floppy disk
			break;
		default:
			drive[2] = '\0'; 
			AddChildItem(NULL, drive); // e.g. ("c:")
		}

		// Point to the next drive (4 chars interval)
		pos = &pos[4];
	}
	ShowWindow(SW_SHOW);
}

HTREEITEM CDirectoryTreeCtrl::AddChildItem(HTREEITEM hRoot, CString strText)
{
	CString strPath = GetFullPath(hRoot);
	if (hRoot != NULL && strPath.Right(1) != "\\")
		strPath += "\\";
	CString strDir = strPath + strText;
	TV_INSERTSTRUCT itInsert;
	memset(&itInsert, 0, sizeof(itInsert));
	
	// START: changed by FoRcHa /////
	WORD wWinVer = theApp.glob_prefs->GetWindowsVersion();
	if(wWinVer == _WINVER_2K_ || wWinVer == _WINVER_XP_ || wWinVer == _WINVER_ME_)		
	{
		itInsert.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_TEXT |
							TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		itInsert.item.stateMask = TVIS_BOLD | TVIS_STATEIMAGEMASK;
	}
	else
	{
		itInsert.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_TEXT | TVIF_STATE;
		itInsert.item.stateMask = TVIS_BOLD;
	}
	// END: changed by FoRcHa ///////
	
	if (HasSharedSubdirectory(strDir))
		itInsert.item.state = TVIS_BOLD;
	else
		itInsert.item.state = 0;
	if (HasSubdirectories(strDir))
		itInsert.item.cChildren = I_CHILDRENCALLBACK;		// used to display the + symbol next to each item
	else
		itInsert.item.cChildren = 0;

	itInsert.item.pszText = strText.GetBuffer();
	itInsert.item.cchTextMax = strText.GetLength();
	itInsert.hInsertAfter = TVI_SORT;
	itInsert.hParent = hRoot;
	
	// START: added by FoRcHa ////////////////
	if(wWinVer == _WINVER_2K_ || wWinVer == _WINVER_XP_ || wWinVer == _WINVER_ME_)		
	{
	CString strTemp = strDir;
	if(strTemp.Right(1) != "\\")
		strTemp += "\\";
	
	UINT nType = GetDriveType(strTemp);
	if(DRIVE_REMOVABLE <= nType && nType <= DRIVE_RAMDISK)
		itInsert.item.iImage = nType;

	SHFILEINFO shFinfo;
	if(!SHGetFileInfo(strTemp, 0, &shFinfo,	sizeof(shFinfo),
					SHGFI_ICON | SHGFI_SMALLICON))
	{
		TRACE(_T("Error Gettting SystemFileInfo!"));
		itInsert.itemex.iImage = 0; // :(
	}
	else
	{
		itInsert.itemex.iImage = shFinfo.iIcon;
		DestroyIcon(shFinfo.hIcon);
	}
	
	if(!SHGetFileInfo(strTemp, 0, &shFinfo, sizeof(shFinfo),
						SHGFI_ICON | SHGFI_OPENICON | SHGFI_SMALLICON))
	{
		TRACE(_T("Error Gettting SystemFileInfo!"));
		itInsert.itemex.iImage = 0;
	}
	else
	{
		itInsert.itemex.iSelectedImage = shFinfo.iIcon;
		DestroyIcon(shFinfo.hIcon);
	}
	}
	// END: added by FoRcHa //////////////
	
	HTREEITEM hItem = InsertItem(&itInsert);
	if (IsShared(strDir))
		SetCheck(hItem);
	strText.ReleaseBuffer();

	return hItem;
}

CString CDirectoryTreeCtrl::GetFullPath(HTREEITEM hItem)
{
	CString strDir;
	HTREEITEM hSearchItem = hItem;
	while(hSearchItem != NULL)
	{
		strDir = GetItemText(hSearchItem) + "\\" + strDir;
		hSearchItem = GetParentItem(hSearchItem);
	}
	return strDir;
}

void CDirectoryTreeCtrl::AddSubdirectories(HTREEITEM hRoot, CString strDir)
{
	if (strDir.Right(1) != "\\")
		strDir += "\\";
	if (!::SetCurrentDirectory(strDir))
		return;
	CFileFind finder;
	BOOL bWorking = finder.FindFile("*.*");
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		if (finder.IsSystem())
			continue;
		if (!finder.IsDirectory())
			continue;
		
		CString strFilename = finder.GetFileName();
		if (strFilename.ReverseFind('\\') != -1)
			strFilename = strFilename.Mid(strFilename.ReverseFind('\\') + 1);
		AddChildItem(hRoot, strFilename);
	}
	finder.Close();
}

bool CDirectoryTreeCtrl::HasSubdirectories(CString strDir)
{
	if (strDir.Right(1) != '\\')
		strDir += '\\';
	::SetCurrentDirectory(strDir);
	CFileFind finder;
	BOOL bWorking = finder.FindFile("*.*");
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		if (finder.IsSystem())
			continue;
		if (!finder.IsDirectory())
			continue;
		finder.Close();
		return true;
	}
	finder.Close();
	return false;
}


void CDirectoryTreeCtrl::GetSharedDirectories(CStringList* list)
{
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; )
		list->AddTail(m_lstShared.GetNext(pos));
}
void CDirectoryTreeCtrl::SetSharedDirectories(CStringList* list)
{
	m_lstShared.RemoveAll();

	for (POSITION pos = list->GetHeadPosition(); pos != NULL; )
	{
		CString str = list->GetNext(pos);
		if (str.Left(2)=="\\\\") continue;
		if (str.Right(1) != '\\')
			str += '\\';
		m_lstShared.AddTail(str);
	}
	Init();
}

bool CDirectoryTreeCtrl::HasSharedSubdirectory(CString strDir)
{
	if (strDir.Right(1) != '\\')
		strDir += '\\';
	strDir.MakeLower();
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; )
	{
		CString str = m_lstShared.GetNext(pos);
		str.MakeLower();
		if (str.Find(strDir) == 0 && strDir != str)//strDir.GetLength() != str.GetLength())
			return true;
	}
	return false;
}

void CDirectoryTreeCtrl::CheckChanged(HTREEITEM hItem, bool bChecked)
{
	CString strDir = GetFullPath(hItem);
	if (bChecked)
		AddShare(strDir);
	else
		DelShare(strDir);

	UpdateParentItems(hItem);
	GetParent()->SendMessage(WM_COMMAND, USRMSG_ITEMSTATECHANGED, (long)m_hWnd);
}

bool CDirectoryTreeCtrl::IsShared(CString strDir)
{
	if (strDir.Right(1) != '\\')
		strDir += '\\';
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; )
	{
		CString str = m_lstShared.GetNext(pos);
		if (str.Right(1) != '\\')
			str += '\\';
		if (str.CompareNoCase(strDir) == 0)
			return true;
	}
	return false;
}

void CDirectoryTreeCtrl::AddShare(CString strDir)
{
	if (strDir.Right(1) != '\\')
		strDir += '\\';
	
	if (IsShared(strDir) || !strDir.CompareNoCase(CString(theApp.glob_prefs->GetConfigDir()) ))
		return;

	m_lstShared.AddTail(strDir);
}

void CDirectoryTreeCtrl::DelShare(CString strDir)
{
	if (strDir.Right(1) != '\\')
		strDir += '\\';
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; )
	{
		POSITION pos2 = pos;
		CString str = m_lstShared.GetNext(pos);
		if (str.CompareNoCase(strDir) == 0)
			m_lstShared.RemoveAt(pos2);
	}
}

void CDirectoryTreeCtrl::UpdateParentItems(HTREEITEM hChild)
{
	HTREEITEM hSearch = GetParentItem(hChild);
	while(hSearch != NULL)
	{
		if (HasSharedSubdirectory(GetFullPath(hSearch)))
			SetItemState(hSearch, TVIS_BOLD, TVIS_BOLD);
		else
			SetItemState(hSearch, 0, TVIS_BOLD);
		hSearch = GetParentItem(hSearch);
	}
}


void CDirectoryTreeCtrl::OnNMRclickSharedList(NMHDR *pNMHDR, LRESULT *pResult)
{
	// get item under cursor
	POINT point;
	::GetCursorPos(&point);
	CPoint p = point;
	ScreenToClient(&p);
	HTREEITEM hItem = HitTest(p);

	// create the menu
	CTitleMenu SharedMenu;
	SharedMenu.CreatePopupMenu();
	if (m_lstShared.GetCount() == 0)
		SharedMenu.AddMenuTitle(GetResString(IDS_NOSHAREDFOLDERS));
	else
		SharedMenu.AddMenuTitle(GetResString(IDS_SHAREDFOLDERS));

	// add right clicked folder, if any
	if (hItem)
	{
		m_strLastRightClicked = GetFullPath(hItem);
		if (!IsShared(m_strLastRightClicked))
			SharedMenu.AppendMenu(MF_STRING, MP_SHAREDFOLDERS_FIRST-1, (LPCTSTR)(GetResString(IDS_VIEW1) + m_strLastRightClicked +GetResString(IDS_VIEW2)));
	}

	// add all shared directories
	int iCnt = 0;
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; iCnt++)
		SharedMenu.AppendMenu(MF_STRING,MP_SHAREDFOLDERS_FIRST+iCnt, (LPCTSTR)m_lstShared.GetNext(pos));

	// display menu
	SharedMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( SharedMenu.DestroyMenu() );
	*pResult = 0;
}

BOOL CDirectoryTreeCtrl::OnCommand(WPARAM wParam,LPARAM lParam ){
	if (wParam < MP_SHAREDFOLDERS_FIRST)
	{
		ShellExecute(NULL, "open", m_strLastRightClicked, NULL, NULL, SW_SHOW);
		return false;
	}
	int cnt = 0;
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; )
	{
		CString str = m_lstShared.GetNext(pos);
		if (cnt == wParam-MP_SHAREDFOLDERS_FIRST)
		{
			ShellExecute(NULL, "open", str, NULL, NULL, SW_SHOW);
			return true;
		}
		cnt++;
	}
	return true;
}