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
#include "stdafx.h"
#include "emule.h"
#include "PPgStats.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "StatisticsDlg.h"
#include "HelpIDs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CPPgStats, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgStats, CPropertyPage)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COLORSELECTOR, OnCbnSelchangeColorselector)
    ON_MESSAGE(CPN_SELCHANGE, OnColorPopupSelChange)
	ON_CBN_SELCHANGE(IDC_CRATIO, OnCbnSelchangeCRatio)
	ON_EN_CHANGE(IDC_CGRAPHSCALE, OnEnChangeCGraphScale)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_SOLIDGRAPH, OnBnClickedSolidGraph) //MORPH - Added by SiRoB, New Graph
END_MESSAGE_MAP()

CPPgStats::CPPgStats()
	: CPropertyPage(CPPgStats::IDD)
{
	mystats1 = 0;
	mystats2 = 0;
	mystats3 = 0;
	m_bModified = FALSE;
}

CPPgStats::~CPPgStats()
{
}

void CPPgStats::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COLORSELECTOR, m_colors);
	DDX_Control(pDX, IDC_COLOR_BUTTON, m_ctlColor);
	DDX_Control(pDX, IDC_CRATIO, m_cratio);
}

void CPPgStats::SetModified(BOOL bChanged)
{
	m_bModified = bChanged;
	CPropertyPage::SetModified(bChanged);
}

BOOL CPPgStats::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	((CSliderCtrl*)GetDlgItem(IDC_SLIDER))->SetPos(thePrefs.GetTrafficOMeterInterval());
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER2))->SetPos(thePrefs.GetStatsInterval());
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER3))->SetPos(thePrefs.GetStatsAverageMinutes() - 1);
	mystats1 = thePrefs.GetTrafficOMeterInterval();
	mystats2 = thePrefs.GetStatsInterval();
	mystats3 = thePrefs.GetStatsAverageMinutes();

	// Set the Connections Statistics Y-Axis Scale
	CString graphScale;
	graphScale.Format(_T("%u"), thePrefs.GetStatsMax());
	GetDlgItem(IDC_CGRAPHSCALE)->SetWindowText(graphScale);

	// Build our ratio combo and select the item corresponding to the currently set preference
	m_cratio.AddString(_T("1:1"));
	m_cratio.AddString(_T("1:2"));
	m_cratio.AddString(_T("1:3"));
	m_cratio.AddString(_T("1:4"));
	m_cratio.AddString(_T("1:5"));
	m_cratio.AddString(_T("1:10"));
	m_cratio.AddString(_T("1:20"));
	int n = thePrefs.GetStatsConnectionsGraphRatio();
	m_cratio.SetCurSel((n==10)?5:((n==20)?6:n-1));
	// <-----khaos-
	//MORPH START - Added by SiRoB, New Graph
	if(thePrefs.IsSolidGraph())
		CheckDlgButton(IDC_SOLIDGRAPH,1);
	else
		CheckDlgButton(IDC_SOLIDGRAPH,0);
	//MORPH END   - Added by SiRoB, New Graph
	Localize();
	SetModified(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgStats::OnApply()
{
	//TODO: cache all parameters. stats should be redrawn (deleted) only if really needed
	if (m_bModified)
	{
		bool bInvalidateGraphs = false;
		if (thePrefs.GetTrafficOMeterInterval() != mystats1){
			thePrefs.SetTrafficOMeterInterval(mystats1);
			bInvalidateGraphs = true;
		}
		if (thePrefs.GetStatsInterval() != mystats2){
			thePrefs.SetStatsInterval(mystats2);
			bInvalidateGraphs = true;
		}
		if (thePrefs.GetStatsAverageMinutes() != mystats3){
			thePrefs.SetStatsAverageMinutes(mystats3);
			bInvalidateGraphs = true;
		}

		TCHAR buffer[20];
		GetDlgItem(IDC_CGRAPHSCALE)->GetWindowText(buffer, ARRSIZE(buffer));
		int statsMax = _tstoi(buffer);
		if (statsMax > thePrefs.GetMaxConnections() + 5)
		{
			if (thePrefs.GetStatsMax() != thePrefs.GetMaxConnections() + 5){
				thePrefs.SetStatsMax(thePrefs.GetMaxConnections() + 5);
				bInvalidateGraphs = true;
			}
			_sntprintf(buffer, ARRSIZE(buffer), _T("%d"), thePrefs.GetStatsMax());
			GetDlgItem(IDC_CGRAPHSCALE)->SetWindowText(buffer);
		}
		else {
			if (thePrefs.GetStatsMax() != statsMax){
				thePrefs.SetStatsMax(statsMax);
				bInvalidateGraphs = true;
			}
		}

		int n = m_cratio.GetCurSel();
		int iRatio = (n == 5) ? 10 : ((n == 6) ? 20 : n + 1); // Index 5 = 1:10 and 6 = 1:20
		if (thePrefs.GetStatsConnectionsGraphRatio() != iRatio){
			thePrefs.SetStatsConnectionsGraphRatio(iRatio); 
			bInvalidateGraphs = true;
		}
		//MORPH START - Added by SiRoB, New Graph
		bool bSolidGraph = IsDlgButtonChecked(IDC_SOLIDGRAPH);
		if(thePrefs.IsSolidGraph() != bSolidGraph){
			thePrefs.m_bSolidGraph = bSolidGraph;
			bInvalidateGraphs = true;
		}
		//MORPH END   - Added by SiRoB, New Graph
	
		if (bInvalidateGraphs){
			theApp.emuledlg->statisticswnd->UpdateConnectionsGraph(); // Set new Y upper bound and Y ratio for active connections.
			theApp.emuledlg->statisticswnd->Localize();
			theApp.emuledlg->statisticswnd->ShowInterval();
		}
		theApp.emuledlg->statisticswnd->RepaintMeters();
		theApp.emuledlg->statisticswnd->GetDlgItem(IDC_STATTREE)->EnableWindow(thePrefs.GetStatsInterval() > 0);
		SetModified(FALSE);
	}
	return CPropertyPage::OnApply();
}

void CPPgStats::Localize(void)
{
	if(m_hWnd)
	{
		GetDlgItem(IDC_GRAPHS)->SetWindowText(GetResString(IDS_GRAPHS));
		GetDlgItem(IDC_STREE)->SetWindowText(GetResString(IDS_STREE));
		GetDlgItem(IDC_STATIC_CGRAPHSCALE)->SetWindowText(GetResString(IDS_PPGSTATS_YSCALE));
		GetDlgItem(IDC_STATIC_CGRAPHRATIO)->SetWindowText(GetResString(IDS_PPGSTATS_ACRATIO));
		SetWindowText(GetResString(IDS_STATSSETUPINFO));
		GetDlgItem(IDC_PREFCOLORS)->SetWindowText(GetResString(IDS_COLORS));
	
		GetDlgItem(IDC_SOLIDGRAPH)->SetWindowText(GetResString(IDS_SOLIDGRAPH)); //MORPH - Added by SiRoB, New Graph
		m_colors.ResetContent();
		m_colors.AddString(GetResString(IDS_SP_BACKGROUND));
		m_colors.AddString(GetResString(IDS_SP_GRID));
		m_colors.AddString(GetResString(IDS_SP_DL1));
		m_colors.AddString(GetResString(IDS_SP_DL2));
		m_colors.AddString(GetResString(IDS_SP_DL3));
		m_colors.AddString(GetResString(IDS_SP_UL1));
		m_colors.AddString(GetResString(IDS_SP_UL2));
		m_colors.AddString(GetResString(IDS_SP_UL3));
		m_colors.AddString(GetResString(IDS_SP_ACTCON));
		m_colors.AddString(GetResString(IDS_SP_TOTALUL));
		m_colors.AddString(GetResString(IDS_SP_ACTUL));
		m_colors.AddString(GetResString(IDS_SP_ICONBAR));
		m_colors.AddString(GetResString(IDS_SP_ACTDL));
		m_colors.AddString(GetResString(IDS_SP_ULFRIENDS));
		m_colors.AddString(GetResString(IDS_SP_ULSLOTSNOOVERHEAD));

		m_ctlColor.CustomText = GetResString(IDS_COL_MORECOLORS);
		m_ctlColor.DefaultText = NULL;

		m_colors.SetCurSel(0);
		OnCbnSelchangeColorselector();
		ShowInterval();
	}
}

void CPPgStats::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CSliderCtrl* slider = (CSliderCtrl*)pScrollBar;
	int position = slider->GetPos();

	if (pScrollBar == GetDlgItem(IDC_SLIDER))
	{
		if (mystats1 != position){
			mystats1 = position;
			SetModified(TRUE);
		}
	}
	else if (pScrollBar == GetDlgItem(IDC_SLIDER2))
	{
		if (mystats2 != position){
			mystats2 = position;
			SetModified(TRUE);
		}
	}
	else
	{
		if (mystats3 != position + 1){
			mystats3 = position + 1;
			SetModified(TRUE);
		}
	}

	ShowInterval();

	UpdateData(false); 
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPgStats::ShowInterval()
{
	CString SliderValue;
	
	if (mystats1 == 0)
		SliderValue.Format(GetResString(IDS_DISABLED));
	else
		SliderValue.Format(GetResString(IDS_STATS_UPDATELABEL), mystats1);
	GetDlgItem(IDC_SLIDERINFO)->SetWindowText(SliderValue);

	if (mystats2 == 0)
		SliderValue.Format(GetResString(IDS_DISABLED));
	else
		SliderValue.Format(GetResString(IDS_STATS_UPDATELABEL), mystats2);
	GetDlgItem(IDC_SLIDERINFO2)->SetWindowText(SliderValue);

	SliderValue.Format(GetResString(IDS_STATS_AVGLABEL), mystats3);
	GetDlgItem(IDC_SLIDERINFO3)->SetWindowText(SliderValue);
}

void CPPgStats::OnCbnSelchangeColorselector()
{
	int sel = m_colors.GetCurSel();
	COLORREF selcolor = thePrefs.GetStatsColor(sel);
	m_ctlColor.SetColor(selcolor);
}

LONG CPPgStats::OnColorPopupSelChange(UINT /*lParam*/, LONG /*wParam*/)
{
	COLORREF setcolor = m_ctlColor.GetColor();
	int iCurColor = thePrefs.GetStatsColor(m_colors.GetCurSel());
	if (iCurColor != setcolor){
		thePrefs.SetStatsColor(m_colors.GetCurSel(), setcolor);
		theApp.emuledlg->ShowTransferRate(true);
		SetModified(TRUE);
	}
	return TRUE;
}

void CPPgStats::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Statistics);
}

BOOL CPPgStats::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgStats::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}
