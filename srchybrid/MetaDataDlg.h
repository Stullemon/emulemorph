#pragma once
#include "ResizableLib/ResizablePage.h"
#include "ListCtrlX.h"
#include <list>

class CAbstractFile;
namespace Kademlia {
class CTag;
typedef std::list<CTag*> TagList;
};

class CMetaDataDlg : public CResizablePage
{
	DECLARE_DYNAMIC(CMetaDataDlg)

public:
	CMetaDataDlg();
	virtual ~CMetaDataDlg();

	void SetFile(const CAbstractFile* file) { m_file = file; }
	void SetTagList(Kademlia::TagList* taglist);

// Dialog Data
	enum { IDD = IDD_META_DATA };

protected:
	const CAbstractFile* m_file;
	Kademlia::TagList* m_taglist;
	CString m_strCaption;
	CMenu* m_pMenuTags;
	CListCtrlX m_tags;

	void InitTags();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnKeydownTags(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCopyTags();
	afx_msg void OnSelectAllTags();
	afx_msg void OnDestroy();
};
