#pragma once

struct SSearchParams;
class CSearchResultsWnd;
class CSearchParamsWnd;
class CSearchFile;
class CClosableTabCtrl;


///////////////////////////////////////////////////////////////////////////////
// CSearchDlg frame

class CSearchDlg : public CFrameWnd
{
	DECLARE_DYNCREATE(CSearchDlg)

public:
	CSearchDlg();           // protected constructor used by dynamic creation
	virtual ~CSearchDlg();

	BOOL Create(CWnd* pParent);

	void Localize();
	void CreateMenus();

	void RemoveResult(const CSearchFile* toremove);

	bool DoNewEd2kSearch(SSearchParams* pParams);
	bool DoNewKadSearch(SSearchParams* pParams);
	void CancelSearch();

	void DownloadSelected();
	void DownloadSelected(bool paused);

	bool CanDeleteSearch(uint32 nSearchID) const;
	bool CanDeleteAllSearches() const;
	void DeleteSearch(uint32 nSearchID);
	void DeleteAllSearchs();

	void LocalSearchEnd(uint16 count, bool bMoreResultsAvailable);
	void AddUDPResult(uint16 count);

	bool CreateNewTab(SSearchParams* pParams);
	void ShowSearchSelector(bool visible);
	CClosableTabCtrl& GetSearchSelector();

	uint8 GetSelectedCat();
	void UpdateCatTabs();
	void SaveAllSettings();
	BOOL SaveSearchStrings();
	void ResetHistory();

	void SetToolTipsDelay(UINT uDelay);
	void DeleteAllSearchListCtrlItems();

	bool IsSearchParamsWndVisible() const;
	void OpenParametersWnd();

protected:
	CSearchParamsWnd* m_pwndParams;
	CSearchResultsWnd* m_pwndResults;

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnClose();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
};
