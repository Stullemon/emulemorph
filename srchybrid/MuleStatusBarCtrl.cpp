// MuleStatusBarCtrl.cpp : implementation file
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MuleStatusBarCtrl.h"
#include "emule.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CMuleStatusBarCtrl

IMPLEMENT_DYNAMIC(CMuleStatusBarCtrl, CStatusBarCtrl)
CMuleStatusBarCtrl::CMuleStatusBarCtrl()
{
}

CMuleStatusBarCtrl::~CMuleStatusBarCtrl()
{
}

BEGIN_MESSAGE_MAP(CMuleStatusBarCtrl, CStatusBarCtrl)
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


void CMuleStatusBarCtrl::Init(void)
{
	EnableToolTips();
}

void CMuleStatusBarCtrl::OnLButtonDblClk(UINT nFlags,CPoint point) {
	int pane=GetPaneAtPosition(point);
	switch (pane) {
		case -1 : return;
		case 0 : AfxMessageBox(_T( "eMule "+GetResString(IDS_SV_LOG)+"\n\n") + GetText(0));break;
		case 1 : break;
		case 2 : theApp.emuledlg->SetActiveDialog(&theApp.emuledlg->statisticswnd);break;
		case 3 : 
			//theApp.emuledlg->SetActiveDialog(&theApp.emuledlg->serverwnd);
			theApp.emuledlg->serverwnd.ShowServerInfo();			
			break;
		case 4 : theApp.emuledlg->SetActiveDialog(&theApp.emuledlg->chatwnd);break;
	}
}

int CMuleStatusBarCtrl::GetPaneAtPosition(CPoint& point) {
	CRect rect;
	
	int nParts = GetParts( 0, NULL );

	for (int i = 0; i<nParts; i++)
	{
		GetRect(i, rect);
		if (rect.PtInRect(point)) return i;
	}
	return -1;
}
