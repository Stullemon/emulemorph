//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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


// SharedFilesWnd.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "SharedFilesWnd.h"
#include "otherfunctions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CSharedFilesWnd dialog

IMPLEMENT_DYNAMIC(CSharedFilesWnd, CDialog)
CSharedFilesWnd::CSharedFilesWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CSharedFilesWnd::IDD, pParent)
{
	md4clr(shownFileHash);
	icon_files = NULL;
}

CSharedFilesWnd::~CSharedFilesWnd()
{
	if (icon_files)
		VERIFY( DestroyIcon(icon_files) );
}

void CSharedFilesWnd::DoDataExchange(CDataExchange* pDX){
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SFLIST, sharedfilesctrl);
	DDX_Control(pDX, IDC_POPBAR, pop_bar);
	DDX_Control(pDX, IDC_POPBAR2, pop_baraccept);
	DDX_Control(pDX, IDC_POPBAR3, pop_bartrans);
	DDX_Control(pDX, IDC_STATISTICS, m_ctrlStatisticsFrm);
}

BOOL CSharedFilesWnd::OnInitDialog(){
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	sharedfilesctrl.Init();
	
	pop_bar.SetGradientColors(RGB(255,255,240),RGB(255,255,0));
	pop_bar.SetTextColor(RGB(20,70,255));
	pop_baraccept.SetGradientColors(RGB(255,255,240),RGB(255,255,0));
	pop_baraccept.SetTextColor(RGB(20,70,255));
	pop_bartrans.SetGradientColors(RGB(255,255,240),RGB(255,255,0));
	pop_bartrans.SetTextColor(RGB(20,70,255));
	
	icon_files=(HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SHAREDFILES), IMAGE_ICON, 16, 16, 0);
	((CStatic*)GetDlgItem(IDC_FILES_ICO))->SetIcon(icon_files);

	LOGFONT lf;
	GetFont()->GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	bold.CreateFontIndirect(&lf);
	m_ctrlStatisticsFrm.Init(IDI_SMALLSTATISTICS);

    // i_a: XXX: should run Init() *first* before setting font bold. 
    m_ctrlStatisticsFrm.SetFont(&bold); 
    // i_a: XXX: localize static icon: 
    m_ctrlStatisticsFrm.SetText(GetResString(IDS_SF_STATISTICS)); // i_a: XXX: moved here from 'Localize()' 
    Localize(); // i_a 
	GetDlgItem(IDC_CURSESSION_LBL)->SetFont(&bold);
	GetDlgItem(IDC_TOTAL_LBL)->SetFont(&bold);
	
	AddAnchor(IDC_FILES_ICO, TOP_LEFT);
	AddAnchor(IDC_TRAFFIC_TEXT, TOP_LEFT);
	AddAnchor(IDC_SFLIST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_RELOADSHAREDFILES, BOTTOM_RIGHT);
	AddAnchor(IDC_STATISTICS,BOTTOM_LEFT);
	AddAnchor(IDC_CURSESSION_LBL, BOTTOM_LEFT);
	AddAnchor(IDC_TOTAL_LBL, BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC4, BOTTOM_LEFT);
	AddAnchor(IDC_SREQUESTED,BOTTOM_LEFT);
	AddAnchor(IDC_POPBAR,BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC7,BOTTOM_LEFT);
	AddAnchor(IDC_SREQUESTED2,BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC5,BOTTOM_LEFT);
	AddAnchor(IDC_SACCEPTED,BOTTOM_LEFT);
	AddAnchor(IDC_POPBAR2,BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC8,BOTTOM_LEFT);
	AddAnchor(IDC_SACCEPTED2,BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC6,BOTTOM_LEFT);
	AddAnchor(IDC_STRANSFERED,BOTTOM_LEFT);
	AddAnchor(IDC_POPBAR3,BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC9,BOTTOM_LEFT);
	AddAnchor(IDC_STRANSFERED2,BOTTOM_LEFT);
	
//	theApp.sharedfiles->SetOutputCtrl(&sharedfilesctrl); // Tried to delay some to see if it fixes win98's crash.
	return true;
}

BEGIN_MESSAGE_MAP(CSharedFilesWnd, CResizableDialog)
	ON_BN_CLICKED(IDC_RELOADSHAREDFILES, OnBnClickedReloadsharedfiles)
	//ON_BN_CLICKED(IDC_MANUAL_CHECKDISKSPACE, OnBnClickedManualCheckDiskSpace)//Morph - Added By Yun.SF3, Manual CheckDiskSpace
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_SFLIST, OnLvnItemActivateSflist)
	ON_NOTIFY(NM_CLICK, IDC_SFLIST, OnNMClickSflist)
END_MESSAGE_MAP()


// CSharedFilesWnd message handlers

void CSharedFilesWnd::OnBnClickedReloadsharedfiles()
{
	theApp.sharedfiles->Reload();
}

void CSharedFilesWnd::Check4StatUpdate(CKnownFile* file){
	if (!md4cmp(file->GetFileHash(),shownFileHash)) ShowDetails(file);
}

void CSharedFilesWnd::OnLvnItemActivateSflist(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (sharedfilesctrl.GetSelectionMark() != (-1) ) {
		CKnownFile* cur_file = (CKnownFile*)sharedfilesctrl.GetItemData(sharedfilesctrl.GetSelectionMark());

		ShowDetails(cur_file);
	}
}

void CSharedFilesWnd::ShowDetails(CKnownFile* cur_file) {
	ASSERT( cur_file != NULL);
	CString buffer;

	pop_bartrans.SetRange32(0,theApp.knownfiles->transferred/1024);
	pop_bartrans.SetPos(cur_file->statistic.GetTransferred()/1024);
	pop_bartrans.SetShowPercent();			
	GetDlgItem(IDC_STRANSFERED)->SetWindowText(CastItoXBytes(cur_file->statistic.GetTransferred()));

	pop_bar.SetRange32(0,theApp.knownfiles->requested);
	pop_bar.SetPos(cur_file->statistic.GetRequests());
	pop_bar.SetShowPercent();			
	buffer.Format("%u",cur_file->statistic.GetRequests());
	GetDlgItem(IDC_SREQUESTED)->SetWindowText(buffer);

	buffer.Format("%u",cur_file->statistic.GetAccepts());
	pop_baraccept.SetRange32(0,theApp.knownfiles->accepted);
	pop_baraccept.SetPos(cur_file->statistic.GetAccepts());
	pop_baraccept.SetShowPercent();
	GetDlgItem(IDC_SACCEPTED)->SetWindowText(buffer);

	GetDlgItem(IDC_STRANSFERED2)->SetWindowText(CastItoXBytes(cur_file->statistic.GetAllTimeTransferred()));

	buffer.Format("%u",cur_file->statistic.GetAllTimeRequests());
	GetDlgItem(IDC_SREQUESTED2)->SetWindowText(buffer);

	buffer.Format("%u",cur_file->statistic.GetAllTimeAccepts());
	GetDlgItem(IDC_SACCEPTED2)->SetWindowText(buffer);

	md4cpy(shownFileHash,cur_file->GetFileHash());

	CString title=GetResString(IDS_SF_STATISTICS)+" ("+ MakeStringEscaped(cur_file->GetFileName()) +")";
	m_ctrlStatisticsFrm.SetText(title);
}

void CSharedFilesWnd::OnNMClickSflist(NMHDR *pNMHDR, LRESULT *pResult){
	OnLvnItemActivateSflist(pNMHDR,pResult);
	*pResult = 0;
}

BOOL CSharedFilesWnd::PreTranslateMessage(MSG* pMsg) 
{
   if(pMsg->message == WM_KEYUP){
	   if (pMsg->hwnd == GetDlgItem(IDC_SFLIST)->m_hWnd)
			OnLvnItemActivateSflist(0,0);
   }
   if (pMsg->message == WM_MBUTTONUP) {
		   	POINT point;
			::GetCursorPos(&point);
			CPoint p = point; 
			sharedfilesctrl.ScreenToClient(&p); 
			int it = sharedfilesctrl.HitTest(p); 
		if (it == -1) return FALSE;

			sharedfilesctrl.SetSelectionMark(it);   // display selection mark correctly! 

			sharedfilesctrl.ShowComments(it);

			return TRUE;
   }

   return CResizableDialog::PreTranslateMessage(pMsg);
}

void CSharedFilesWnd::Localize(){
	sharedfilesctrl.Localize();
	GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_SF_FILES));
	GetDlgItem(IDC_RELOADSHAREDFILES)->SetWindowText(GetResString(IDS_SF_RELOAD));
	
	m_ctrlStatisticsFrm.SetWindowText(GetResString(IDS_SF_STATISTICS));
	m_ctrlStatisticsFrm.SetText(GetResString(IDS_SF_STATISTICS));

	GetDlgItem(IDC_CURSESSION_LBL)->SetWindowText(GetResString(IDS_SF_CURRENT));
	GetDlgItem(IDC_TOTAL_LBL)->SetWindowText(GetResString(IDS_SF_TOTAL));
	GetDlgItem(IDC_FSTATIC6)->SetWindowText(GetResString(IDS_SF_TRANS));
	GetDlgItem(IDC_FSTATIC5)->SetWindowText(GetResString(IDS_SF_ACCEPTED));
	GetDlgItem(IDC_FSTATIC4)->SetWindowText(GetResString(IDS_SF_REQUESTS)+":");
	GetDlgItem(IDC_FSTATIC9)->SetWindowText(GetResString(IDS_SF_TRANS));
	GetDlgItem(IDC_FSTATIC8)->SetWindowText(GetResString(IDS_SF_ACCEPTED));
	GetDlgItem(IDC_FSTATIC7)->SetWindowText(GetResString(IDS_SF_REQUESTS)+":");
}
