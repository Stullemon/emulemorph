// PPgStats.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgStats.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPPgStats dialog

IMPLEMENT_DYNAMIC(CPPgStats, CPropertyPage)
CPPgStats::CPPgStats()
	: CPropertyPage(CPPgStats::IDD)
{
}

CPPgStats::~CPPgStats()
{
}

void CPPgStats::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COLORSELECTOR, m_colors);
	DDX_Control(pDX, IDC_COLOR_BUTTON, m_ctlColor);
	// -khaos--+++> Data exchange for the combo
	DDX_Control(pDX, IDC_CRATIO, m_cratio);
	// <-----khaos
}


BEGIN_MESSAGE_MAP(CPPgStats, CPropertyPage)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COLORSELECTOR, OnCbnSelchangeColorselector)
    ON_MESSAGE(CPN_SELCHANGE, OnSelChange)
	// -khaos--+++>
	ON_CBN_SELCHANGE(IDC_CRATIO, OnCbnSelchangeCRatio)
	ON_EN_CHANGE(IDC_CGRAPHSCALE, OnEnChangeCGraphScale)
	// <-----khaos-
END_MESSAGE_MAP()

BOOL CPPgStats::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	((CSliderCtrl*)GetDlgItem(IDC_SLIDER))->SetPos(app_prefs->GetTrafficOMeterInterval());
	// -khaos--+++> Changed this:  Tree update speed ranges 1-100 instead of 5-104.  Power to the people! ;D
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER2))->SetPos(app_prefs->GetStatsInterval());
	// <-----khaos-
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER3))->SetPos(app_prefs->GetStatsAverageMinutes()-1);
	mystats1=app_prefs->GetTrafficOMeterInterval();
	mystats2=app_prefs->GetStatsInterval();
	mystats3=app_prefs->GetStatsAverageMinutes();

	// -khaos--+++> Borrows from eMule Plus
	// Set the Connections Statistics Y-Axis Scale
	CString graphScale;
	graphScale.Format("%u",app_prefs->GetStatsMax());
	GetDlgItem(IDC_CGRAPHSCALE)->SetWindowText(graphScale);

	// Build our ratio combo and select the item corresponding to the currently set preference
	m_cratio.AddString("1:1");
	m_cratio.AddString("1:2");
	m_cratio.AddString("1:3");
	m_cratio.AddString("1:4");
	m_cratio.AddString("1:5");
	m_cratio.AddString("1:10");
	m_cratio.AddString("1:20");
	int n = app_prefs->GetStatsConnectionsGraphRatio();
	m_cratio.SetCurSel((n==10)?5:((n==20)?6:n-1));
	// <-----khaos-

	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgStats::OnApply()
{
	theApp.glob_prefs->SetTrafficOMeterInterval(mystats1);
	theApp.glob_prefs->SetStatsInterval(mystats2);
	theApp.glob_prefs->SetStatsAverageMinutes(mystats3);

	// -khaos--+++> Save our new preferences, borrows from eMule+
	char buffer[100];
	GetDlgItem(IDC_CGRAPHSCALE)->GetWindowText(buffer,20);
	int statsMax = atoi(buffer);
	if(statsMax > (app_prefs->GetMaxConnections() + 5))
	{
		app_prefs->SetStatsMax(app_prefs->GetMaxConnections() + 5);
		sprintf(buffer, "%d", app_prefs->GetStatsMax());
		GetDlgItem(IDC_CGRAPHSCALE)->SetWindowText(buffer);
	} else {
		app_prefs->SetStatsMax(statsMax);
	}
	int n = m_cratio.GetCurSel();
	app_prefs->SetStatsConnectionsGraphRatio((n==5)?10:((n==6)?20:n+1)); // Index 5 = 1:10 and 6 = 1:20

	theApp.emuledlg->statisticswnd.UpdateConnectionsGraph(); // Set new Y upper bound and Y ratio for active connections.
	// <-----khaos-

	theApp.emuledlg->statisticswnd.Localize();
	theApp.emuledlg->statisticswnd.ShowInterval();
	SetModified(FALSE);

	theApp.emuledlg->statisticswnd.RepaintMeters();
	theApp.emuledlg->statisticswnd.GetDlgItem(IDC_STATTREE)->EnableWindow(theApp.glob_prefs->GetStatsInterval()>0);

//	app_prefs->Save();
	return CPropertyPage::OnApply();
}

void CPPgStats::Localize(void)
{
	if(m_hWnd)
	{
		GetDlgItem(IDC_GRAPHS)->SetWindowText(GetResString(IDS_GRAPHS));
		GetDlgItem(IDC_STREE)->SetWindowText(GetResString(IDS_STREE));
		// -khaos--+++>
		GetDlgItem(IDC_STATIC_CGRAPHSCALE)->SetWindowText(GetResString(IDS_PPGSTATS_YSCALE));
		GetDlgItem(IDC_STATIC_CGRAPHRATIO)->SetWindowText(GetResString(IDS_PPGSTATS_ACRATIO));
		// <-----khaos-
		SetWindowText(GetResString(IDS_STATSSETUPINFO));

		GetDlgItem(IDC_PREFCOLORS)->SetWindowText(GetResString(IDS_COLORS));

		while (m_colors.GetCount()>0) m_colors.DeleteString(0);
		m_colors.AddString(GetResString(IDS_SP_BACKGROUND));
		m_colors.AddString(GetResString(IDS_SP_GRID));
		m_colors.AddString(GetResString(IDS_SP_DL1));
		m_colors.AddString(GetResString(IDS_SP_DL2));
		m_colors.AddString(GetResString(IDS_SP_DL3));
		m_colors.AddString(GetResString(IDS_SP_UL1));
		m_colors.AddString(GetResString(IDS_SP_UL2));
		m_colors.AddString(GetResString(IDS_SP_UL3));
		m_colors.AddString(GetResString(IDS_SP_ACTCON));
		//MORPH - Added by Yun.SF3, ZZ Upload System
		m_colors.AddString(GetResString(IDS_SP_TOTALUL));
		m_colors.AddString(GetResString(IDS_SP_ACTUL));
		//MORPH - Added by Yun.SF3, ZZ Upload System
		//m_colors.AddString(GetResString(IDS_SP_ACTDL)); //MORPH - Removed by SiRoB, To preserve SystrayColor index
		m_colors.AddString(GetResString(IDS_SP_ICONBAR));
		m_colors.AddString(GetResString(IDS_SP_ACTDL)); //MORPH - Moved by SiRoB, To preserve SystrayColor index
		m_colors.AddString(GetResString(IDS_SP_ULFRIENDS)); //MORPH - Added by Yun.SF3, ZZ Upload System
		m_colors.AddString(GetResString(IDS_SP_ULSLOTSNOOVERHEAD)); //MORPH - Added by SiRoB, ZZ Upload System 20030818-1923

		m_ctlColor.CustomText = _T(GetResString(IDS_COL_MORECOLORS));
		m_ctlColor.DefaultText = NULL;	

		m_colors.SetCurSel(0);
		OnCbnSelchangeColorselector();
		ShowInterval();
	}
}

void CPPgStats::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);
	
	CSliderCtrl* slider =(CSliderCtrl*)pScrollBar;
	int position = slider->GetPos();

	if (pScrollBar==GetDlgItem(IDC_SLIDER)) {
		mystats1=position;
	} else if (pScrollBar==GetDlgItem(IDC_SLIDER2)) {
			// -khaos--+++> Changed for faster statistics tree updates...
			mystats2=position;
			// <-----khaos-
	} else mystats3=position+1;

	ShowInterval();

	UpdateData(false); 
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPgStats::ShowInterval() {
	CString m_SliderValue;
	
	if (mystats1==0) m_SliderValue.Format(GetResString(IDS_DISABLED));
		else m_SliderValue.Format(GetResString(IDS_STATS_UPDATELABEL), mystats1);
	GetDlgItem(IDC_SLIDERINFO)->SetWindowText(m_SliderValue);

	if (mystats2==0) m_SliderValue.Format(GetResString(IDS_DISABLED));
		else m_SliderValue.Format(GetResString(IDS_STATS_UPDATELABEL), mystats2);
	GetDlgItem(IDC_SLIDERINFO2)->SetWindowText(m_SliderValue);

	m_SliderValue.Format(GetResString(IDS_STATS_AVGLABEL), mystats3);
	GetDlgItem(IDC_SLIDERINFO3)->SetWindowText(m_SliderValue);
}

void CPPgStats::OnCbnSelchangeColorselector()
{
	int sel=m_colors.GetCurSel();
	COLORREF selcolor=theApp.glob_prefs->GetStatsColor(sel);
	m_ctlColor.SetColor(selcolor);
}

LONG CPPgStats::OnSelChange(UINT /*lParam*/, LONG /*wParam*/)
{
	COLORREF setcolor=m_ctlColor.GetColor();
	theApp.glob_prefs->SetStatsColor(m_colors.GetCurSel(),setcolor);
	SetModified(TRUE);
	return TRUE;
}
