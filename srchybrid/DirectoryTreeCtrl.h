#pragma once
/////////////////////////////////////////////
// written by robert rostek - tecxx@rrs.at //
/////////////////////////////////////////////

// CDirectoryTreeCtrl
#include "TitleMenu.h"

#define USRMSG_ITEMSTATECHANGED		(WM_USER + 0x101)
#define MP_SHAREDFOLDERS_FIRST	46901

class CDirectoryTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNAMIC(CDirectoryTreeCtrl)

public:
	// initialize control
	void Init(void);
	// get all shared directories
	void GetSharedDirectories(CStringList* list);
	// set shared directories
	void SetSharedDirectories(CStringList* list);

private:
	CImageList m_image; 
	// add a new item
	HTREEITEM AddChildItem(HTREEITEM hRoot, CString strText);
	// add subdirectory items
	void AddSubdirectories(HTREEITEM hRoot, CString strDir);
	// return the full path of an item (like C:\abc\somewhere\inheaven\)
	CString GetFullPath(HTREEITEM hItem);
	// returns true if strDir has at least one subdirectory
	bool HasSubdirectories(CString strDir);
	// check status of an item has changed
	void CheckChanged(HTREEITEM hItem, bool bChecked);
	// returns true if a subdirectory of strDir is shared
	bool HasSharedSubdirectory(CString strDir);
	// when sharing a directory, make all parent directories bold
	void UpdateParentItems(HTREEITEM hChild);

	// share list access
	bool IsShared(CString strDir);
	void AddShare(CString strDir);
	void DelShare(CString strDir);
	void MarkChilds(HTREEITEM hChild,bool mark);

	CStringList m_lstShared;
	CString m_strLastRightClicked;
	bool m_bSelectSubDirs;

public:
	// construction / destruction
	CDirectoryTreeCtrl();
	virtual ~CDirectoryTreeCtrl();
	virtual BOOL OnCommand(WPARAM wParam,LPARAM lParam );

protected:
	afx_msg void OnNMRclickSharedList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
};


