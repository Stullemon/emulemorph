
#include "stdafx.h" 
#include "emule.h"
#include "CommentDialogLst.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CCommentDialogLst, CResizablePage) 

BEGIN_MESSAGE_MAP(CCommentDialogLst, CResizablePage) 
   ON_BN_CLICKED(IDCOK, OnBnClickedApply) 
   ON_BN_CLICKED(IDCREF, OnBnClickedRefresh) 
   ON_NOTIFY(NM_DBLCLK, IDC_LST, OnNMDblclkLst)
END_MESSAGE_MAP() 

CCommentDialogLst::CCommentDialogLst() 
   : CResizablePage(CCommentDialogLst::IDD, IDS_CMT_READALL) 
{ 
	m_strCaption = GetResString(IDS_CMT_READALL);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_file = NULL; 
} 

CCommentDialogLst::~CCommentDialogLst() 
{ 
} 

void CCommentDialogLst::DoDataExchange(CDataExchange* pDX) 
{ 
	CResizablePage::DoDataExchange(pDX); 
   DDX_Control(pDX, IDC_LST, pmyListCtrl);
} 

void CCommentDialogLst::OnBnClickedApply() 
{ 
	CResizablePage::OnOK(); 
} 

void CCommentDialogLst::OnBnClickedRefresh() 
{ 
   CompleteList(); 
} 

BOOL CCommentDialogLst::OnInitDialog()
{ 
	CResizablePage::OnInitDialog(); 
	InitWindowStyles(this);

   AddAnchor(IDC_LST,TOP_LEFT,BOTTOM_RIGHT);
   AddAnchor(IDCREF,BOTTOM_RIGHT);
   AddAnchor(IDC_CMSTATUS,BOTTOM_LEFT);

   pmyListCtrl.InsertColumn(0, GetResString(IDS_CD_UNAME), LVCFMT_LEFT, 130, -1); 
   pmyListCtrl.InsertColumn(1, GetResString(IDS_DL_FILENAME), LVCFMT_LEFT, 130, -1); 
   pmyListCtrl.InsertColumn(2, GetResString(IDS_QL_RATING), LVCFMT_LEFT, 80, 1); 
   pmyListCtrl.InsertColumn(3, GetResString(IDS_COMMENT), LVCFMT_LEFT, 340, 1); 

   Localize(); 
   CompleteList(); 

   return TRUE; 
} 

void CCommentDialogLst::Localize(void)
{ 
   GetDlgItem(IDCREF)->SetWindowText(GetResString(IDS_CMT_REFRESH)); 
} 

void CCommentDialogLst::CompleteList ()
{ 
   pmyListCtrl.DeleteAllItems();
   
	int count=0; 
	for (int sl=0;sl<SOURCESSLOTS;sl++)
	{
		for (POSITION pos = m_file->srclists[sl].GetHeadPosition(); pos != NULL; )
		{ 
			CUpDownClient* cur_src = m_file->srclists[sl].GetNext(pos);
			if (cur_src->GetFileComment().GetLength()>0 || cur_src->GetFileRate()>0)
			{
			pmyListCtrl.InsertItem(LVIF_TEXT|LVIF_PARAM,count,cur_src->GetUserName(),0,0,1,(LPARAM)cur_src);
			pmyListCtrl.SetItemText(count, 1, cur_src->GetClientFilename()); 
			pmyListCtrl.SetItemText(count, 2, GetRateString(cur_src->GetFileRate())); 
			pmyListCtrl.SetItemText(count, 3, cur_src->GetFileComment()); 
			count++;
	  } 
   } 
	}

	CString info;
	if (count==0)
		info=_T("(") + GetResString(IDS_CMT_NONE) + _T(")");
   GetDlgItem(IDC_CMSTATUS)->SetWindowText(info);
   m_file->UpdateFileRatingCommentAvail();
}

void CCommentDialogLst::OnNMDblclkLst(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (pmyListCtrl.GetSelectedCount()==0)
		return;

	CUpDownClient* client = (CUpDownClient*)pmyListCtrl.GetItemData(pmyListCtrl.GetSelectionMark());
	if (client)
		theApp.emuledlg->chatwnd.StartSession(client);
	theApp.emuledlg->SetActiveDialog(&theApp.emuledlg->chatwnd);
	*pResult = 0;
}
