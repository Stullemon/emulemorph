// CClosableTab class definitions
// by enkeyDEV(Ottavio84)

#include "stdafx.h"
#include "emule.h"
#include "ClosableTabCtrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CClosableTabCtrl

IMPLEMENT_DYNAMIC(CClosableTabCtrl, CTabCtrl)
CClosableTabCtrl::CClosableTabCtrl()
{
	m_pImgLst.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,10);
	m_pImgLst.SetBkColor(CLR_NONE);
	m_pImgLst.Add(theApp.LoadIcon(IDI_CLOSE));
}

CClosableTabCtrl::~CClosableTabCtrl()
{
}


BEGIN_MESSAGE_MAP(CClosableTabCtrl, CTabCtrl)
	ON_WM_LBUTTONUP()
	ON_WM_CREATE()
END_MESSAGE_MAP()



// CClosableTabCtrl message handlers


void CClosableTabCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	int ntabs = GetItemCount();
	CRect clsbutrect;
	for (int i = 0; i < ntabs; i++) {
		GetItemRect(i, clsbutrect);
		clsbutrect.DeflateRect(2, 2, clsbutrect.right - clsbutrect.left - 16, 0);
		if (clsbutrect.PtInRect(point)) {
			GetParent()->SendMessage(WM_CLOSETAB, (WPARAM) i);
			return; 
		}
	}
	
	CTabCtrl::OnLButtonDown(nFlags, point);
}

void CClosableTabCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{

	CRect rect = lpDrawItemStruct->rcItem;
	IMAGEINFO info;
	int nTabIndex = lpDrawItemStruct->itemID;
	if (nTabIndex < 0) return;
	BOOL bSelected = (nTabIndex == GetCurSel());

	char label[64];
	TC_ITEM tci;
	tci.mask = TCIF_TEXT|TCIF_IMAGE;
	tci.pszText = label;     
	tci.cchTextMax = 63;    	
	if (!GetItem(nTabIndex, &tci )) return;

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	if (!pDC) return;
	int nSavedDC = pDC->SaveDC();

	rect.top += ::GetSystemMetrics(SM_CYEDGE);

	pDC->SetBkMode(TRANSPARENT);

	// Draw image
	CImageList* pImageList = &m_pImgLst;
	if (pImageList && tci.iImage >= 0) {

		rect.left += pDC->GetTextExtent(_T(" ")).cx;		// Margin

		// Get height of image so we 
		pImageList->GetImageInfo(0, &info);
		CRect ImageRect(info.rcImage);
		int nYpos = rect.top;

		pImageList->Draw(pDC, 0, CPoint(rect.left, nYpos), ILD_TRANSPARENT);
		rect.left += ImageRect.Width();
	}

	if (bSelected) {
		rect.top -= ::GetSystemMetrics(SM_CYEDGE);
		pDC->DrawText(label, rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_NOPREFIX);
		rect.top += ::GetSystemMetrics(SM_CYEDGE);
	} else {
		pDC->DrawText(label, rect, DT_SINGLELINE|DT_BOTTOM|DT_CENTER|DT_NOPREFIX);
	}

	if (nSavedDC)
		pDC->RestoreDC(nSavedDC);
}

void CClosableTabCtrl::PreSubclassWindow()
{
	CTabCtrl::PreSubclassWindow();
	ModifyStyle(0, TCS_OWNERDRAWFIXED);
}

int CClosableTabCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTabCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	ModifyStyle(0, TCS_OWNERDRAWFIXED);
	return 0;
}

