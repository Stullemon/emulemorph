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

// StatisticsDlg.cpp : implementation file
//
#include "stdafx.h"
#include "emule.h"
#include "StatisticsDlg.h"
#include "uploadqueue.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
extern _CRT_ALLOC_HOOK g_pfnPrevCrtAllocHook;
#endif


// CStatisticsDlg dialog

IMPLEMENT_DYNAMIC(CStatisticsDlg, CDialog)
CStatisticsDlg::CStatisticsDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CStatisticsDlg::IDD, pParent) , m_DownloadOMeter( 3 ),m_Statistics(4),m_UploadOMeter(5)
{
	m_oldcx=0;
	m_oldcy=0;
}

CStatisticsDlg::~CStatisticsDlg()
{
#ifdef _DEBUG
	POSITION pos = blockFiles.GetStartPosition();
	while (pos != NULL) {
		const unsigned char* fileName;
		HTREEITEM* pTag;
		blockFiles.GetNextAssoc(pos, fileName, pTag);
		delete pTag;
	}
#endif
}

void CStatisticsDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExoSliderDlg)
	DDX_Control(pDX, IDC_STATTREE, stattree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatisticsDlg, CResizableDialog)
	//{{AFX_MSG_MAP(CStatisticsDlg)
	ON_WM_SHOWWINDOW()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	// -khaos--+++> Menu button.
	ON_BN_CLICKED(IDC_BNMENU, OnMenuButtonClicked)	
	// <-----khaos-
END_MESSAGE_MAP()


// CStatisticsDlg message handlers

BOOL CStatisticsDlg::OnInitDialog(){

	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	EnableWindow( FALSE );

	Localize();

	// Setup ImageList For Tree (2-18-03)
	imagelistStatTree.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 12, 0);
	imagelistStatTree.Add(theApp.LoadIcon(IDI_TVI_GENERIC));		// Dots & Arrow (Default icon for stats)
	imagelistStatTree.Add(theApp.LoadIcon(IDI_UP1DOWN1));			// Transfer
	imagelistStatTree.Add(theApp.LoadIcon(IDI_CONNECTEDHIGH));		// Connection
	imagelistStatTree.Add(theApp.LoadIcon(IDI_USER));				// Clients
	imagelistStatTree.Add(theApp.LoadIcon(IDI_PREF_SERVER));		// Server
	imagelistStatTree.Add(theApp.LoadIcon(IDI_SHAREDFILES));		// Shared Files
	imagelistStatTree.Add(theApp.LoadIcon(IDI_UPLOAD));				// Transfer > Upload
	imagelistStatTree.Add(theApp.LoadIcon(IDI_DIRECTDOWNLOAD));		// Transfer > Download
	imagelistStatTree.Add(theApp.LoadIcon(IDI_SMALLSTATISTICS));	// Session Sections
	imagelistStatTree.Add(theApp.LoadIcon(IDI_TVI_CUMULATIVE));		// Cumulative Sections
	imagelistStatTree.Add(theApp.LoadIcon(IDI_PREF_TWEAK));			// Records
	imagelistStatTree.Add(theApp.LoadIcon(IDI_PREF_CONNECTION));	// Connection > General
	imagelistStatTree.Add(theApp.LoadIcon(IDI_PREF_SCHEDULER));		// Time Section
	imagelistStatTree.Add(theApp.LoadIcon(IDI_PREF_STATISTICS));	// Time > Averages and Projections
	imagelistStatTree.Add(theApp.LoadIcon(IDI_TVI_DAY));			// Time > Averages and Projections > Daily
	imagelistStatTree.Add(theApp.LoadIcon(IDI_TVI_MONTH));			// Time > Averages and Projections > Monthly
	imagelistStatTree.Add(theApp.LoadIcon(IDI_TVI_YEAR));			// Time > Averages and Projections > Yearly
	imagelistStatTree.Add(theApp.LoadIcon(IDI_STATS_HD));		// Diskspace

	stattree.SetImageList(&imagelistStatTree, TVSIL_NORMAL);

	CreateMyTree();

	// <-----khaos- End Additions

#ifdef _DEBUG
	if (g_pfnPrevCrtAllocHook){
		h_debug = stattree.InsertItem( "Debug info" );stattree.SetItemData(h_debug,0);
		h_blocks = stattree.InsertItem("Blocks",h_debug);stattree.SetItemData(h_blocks,1);
		debug1 =  stattree.InsertItem("Free",h_blocks);stattree.SetItemData(debug1,1);
		debug2 =  stattree.InsertItem("Normal",h_blocks);stattree.SetItemData(debug2,1);
		debug3 =  stattree.InsertItem("CRT",h_blocks);stattree.SetItemData(debug3,1);
		debug4 =  stattree.InsertItem("Ignore",h_blocks);stattree.SetItemData(debug4,1);
		debug5 =  stattree.InsertItem("Client",h_blocks);stattree.SetItemData(debug5,1);
		stattree.Expand(h_debug,TVE_EXPAND);
		stattree.Expand(h_blocks,TVE_EXPAND);
	}
#endif
	// Setup download-scope
	CRect rect;
	GetDlgItem(IDC_SCOPE_D)->GetWindowRect(rect) ;
	ScreenToClient(rect) ;
	m_DownloadOMeter.Create(WS_VISIBLE | WS_CHILD, rect, this) ; 
	//MORPH START - Changed by SiRoB, New Graph
	SetARange(true,theApp.glob_prefs->GetMaxGraphDownloadRate());
	//MORPH END   - Changed by SiRoB, New Graph
	m_DownloadOMeter.SetYUnits(GetResString(IDS_KBYTESEC)) ;
	
	// Setup upload-scope
	GetDlgItem(IDC_SCOPE_U)->GetWindowRect(rect) ;
	ScreenToClient(rect) ;
	m_UploadOMeter.Create(WS_VISIBLE | WS_CHILD, rect, this) ; 
	//MORPH START - Changed by SiRoB, New Graph
	SetARange(false,theApp.glob_prefs->GetMaxGraphUploadRate());
	//MORPH END   - Changed by SiRoB, New Graph
	m_UploadOMeter.SetYUnits(GetResString(IDS_KBYTESEC)) ;
	
	// Setup additional graph-scope
	GetDlgItem(IDC_STATSSCOPE)->GetWindowRect(rect) ;
	ScreenToClient(rect) ;
	m_Statistics.Create(WS_VISIBLE | WS_CHILD, rect, this) ; 
	m_Statistics.SetRanges(0, theApp.glob_prefs->GetStatsMax()) ;
	m_Statistics.autofitYscale=false;
	// -khaos--+++> Set the trend ratio of the Active Connections trend in the Connection Statistics scope.
	m_Statistics.SetTrendRatio(0, theApp.glob_prefs->GetStatsConnectionsGraphRatio());
	// <-----khaos-

	m_Statistics.SetYUnits("") ;
	m_Statistics.SetXUnits(GetResString(IDS_TIME));

	RepaintMeters();
	m_Statistics.SetBackgroundColor(theApp.glob_prefs->GetStatsColor(0)) ;
	m_Statistics.SetGridColor(theApp.glob_prefs->GetStatsColor(1)) ;
	
	m_DownloadOMeter.InvalidateCtrl();
	m_UploadOMeter.InvalidateCtrl();
	m_Statistics.InvalidateCtrl();

	if (theApp.glob_prefs->GetStatsInterval()==0) GetDlgItem(IDC_STATTREE)->EnableWindow(false);

	UpdateData(FALSE);

	EnableWindow( TRUE );

	m_ilastMaxConnReached = 0;
	peakconnections = 0;
	totalconnectionchecks = 0;
	averageconnections = 0;
	activeconnections = 0;
	maxDown=0;
	maxDownavg=0;
	// -khaos--+++> New vars need to be initialized...  And changed anchors, borrows a little from eMule Plus v1
	maxcumDown =			theApp.glob_prefs->GetConnMaxDownRate();
	cumUpavg =				theApp.glob_prefs->GetConnAvgUpRate();
	maxcumDownavg =			theApp.glob_prefs->GetConnMaxAvgDownRate();
	cumDownavg =			theApp.glob_prefs->GetConnAvgDownRate();
	maxcumUpavg =			theApp.glob_prefs->GetConnMaxAvgUpRate();
	maxcumUp =				theApp.glob_prefs->GetConnMaxUpRate();
	maxUp =					0;
	maxUpavg =				0;
	rateDown =				0;
	rateUp =				0;
	timeTransfers =			0;
	timeDownloads =			0;
	timeUploads =			0;
	start_timeTransfers =	0;
	start_timeDownloads =	0;
	start_timeUploads =		0;
	time_thisTransfer =		0;
	time_thisDownload =		0;
	time_thisUpload =		0;
	timeServerDuration =	0;
	time_thisServerDuration=0;
	cntDelay =				0;
	//MORPH START - Removed by SiRoB, New Graph
	/*AddAnchor(IDC_STATTREE,TOP_LEFT, BOTTOM_LEFT );

	AddAnchor(IDC_STATIC_CONNSTATS,BOTTOM_LEFT);
	AddAnchor(m_Led3[0],BOTTOM_LEFT);
	AddAnchor(IDC_STATIC_S0,BOTTOM_LEFT);
	AddAnchor(m_Led3[1],BOTTOM_LEFT);
	AddAnchor(IDC_STATIC_S2,BOTTOM_LEFT);
	AddAnchor(m_Led3[3],BOTTOM_LEFT);
	AddAnchor(IDC_STATIC_S1,BOTTOM_LEFT);

	AddAnchor(m_Statistics,BOTTOM_LEFT);

	AddAnchor(m_DownloadOMeter, TOP_LEFT, CSize(100,50) );
	AddAnchor(m_UploadOMeter,CSize(0,50), CSize(100,100) );

	AddAnchor(m_Led2[0],CSize(0,50));
	AddAnchor(m_Led2[1],CSize(0,50));
	AddAnchor(m_Led2[2],CSize(0,50));

	AddAnchor(IDC_STATIC_U,CSize(0,50));
	AddAnchor(IDC_STATIC_U2,CSize(0,50));
	AddAnchor(IDC_TIMEAVG2,CSize(0,50));
	AddAnchor(IDC_STATIC_U3,CSize(0,50));*/
	//MORPH END - Removed by SiRoB, New Graph
	//MORPH START - Added by SiRoB, New Graph
	AddAnchor(IDC_STATTREE, TOP_LEFT, BOTTOM_CENTER);
	AddAnchor(IDC_STATIC_LASTRESET, TOP_LEFT, TOP_CENTER);
	AddAnchor(IDC_BNMENU, TOP_CENTER);

	AddAnchor(IDC_SCOPE_D, TOP_CENTER, CSize(100,34));
	AddAnchor(m_DownloadOMeter.m_hWnd, TOP_CENTER, CSize(100,34));
	AddAnchor(IDC_SCOPE_U, CSize(50,34), CSize(100,68));
	AddAnchor(m_UploadOMeter.m_hWnd, CSize(50,34), CSize(100,68));
	AddAnchor(IDC_STATSSCOPE, CSize(50,68), BOTTOM_RIGHT);
	AddAnchor(m_Statistics.m_hWnd, CSize(50,68), BOTTOM_RIGHT);
	//MORPH END   - Added by SiRoB, New Graph

	RepaintMeters();


	ShowStatistics(true);
	
	// <-----khaos-

	return true;
}

//MORPH START - Removed by SiRoB, New Graph
/*void CStatisticsDlg::SetupLegend( int ResIdx, int ElmtIdx, int legendNr){
	CRect Rect;

	GetDlgItem( ResIdx )->GetWindowRect( Rect );
	ScreenToClient( Rect );
	
	if (legendNr==1){
		if (!m_Led1[ ElmtIdx ]) m_Led1[ ElmtIdx ].Create( WS_VISIBLE | WS_CHILD, Rect, this );
		m_Led1[ ElmtIdx ].SetBackgroundColor( m_DownloadOMeter.GetPlotColor( ElmtIdx ) );
		m_Led1[ ElmtIdx ].SetFrameColor( RGB( 0x00, 0x00, 0x00 ) );
	} else if (legendNr==2) {
		if (!m_Led2[ ElmtIdx ]) m_Led2[ ElmtIdx ].Create( WS_VISIBLE | WS_CHILD, Rect, this );
		m_Led2[ ElmtIdx ].SetBackgroundColor( m_UploadOMeter.GetPlotColor( ElmtIdx ) );
		m_Led2[ ElmtIdx ].SetFrameColor( RGB( 0x00, 0x00, 0x00 ) );
	} else if (legendNr==3){
		if (!m_Led3[ ElmtIdx ]) m_Led3[ ElmtIdx ].Create( WS_VISIBLE | WS_CHILD, Rect, this );
		m_Led3[ ElmtIdx ].SetBackgroundColor( m_Statistics.GetPlotColor( ElmtIdx ) );
		m_Led3[ ElmtIdx ].SetFrameColor( RGB( 0x00, 0x00, 0x00 ) );
	}
}*/
//MORPH END  - Removed by SiRoB, New Graph

void CStatisticsDlg::RepaintMeters() {
	//MORPH START - Added by SiRoB, New Graph
	CString Buffer;
	//MORPH END   - Added by SiRoB, New Graph
	m_DownloadOMeter.SetBackgroundColor(theApp.glob_prefs->GetStatsColor(0)) ;
	m_DownloadOMeter.SetGridColor(theApp.glob_prefs->GetStatsColor(1)) ;
	m_DownloadOMeter.SetPlotColor( theApp.glob_prefs->GetStatsColor(4) ,0) ;
	m_DownloadOMeter.SetPlotColor( theApp.glob_prefs->GetStatsColor(3) ,1) ;
	m_DownloadOMeter.SetPlotColor( theApp.glob_prefs->GetStatsColor(2) ,2) ;

	m_UploadOMeter.SetBackgroundColor(theApp.glob_prefs->GetStatsColor(0)) ;
	m_UploadOMeter.SetGridColor(theApp.glob_prefs->GetStatsColor(1)) ;
	m_UploadOMeter.SetPlotColor( theApp.glob_prefs->GetStatsColor(7) ,0) ;
	m_UploadOMeter.SetPlotColor( theApp.glob_prefs->GetStatsColor(6) ,1) ;
	m_UploadOMeter.SetPlotColor( theApp.glob_prefs->GetStatsColor(5) ,2) ;
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	// friends line
	m_UploadOMeter.SetPlotColor( theApp.glob_prefs->GetStatsColor(13) ,3) ;
	// current upload without overhead included
	m_UploadOMeter.SetPlotColor( theApp.glob_prefs->GetStatsColor(14) ,4) ;
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	m_Statistics.SetBackgroundColor(theApp.glob_prefs->GetStatsColor(0)) ;
	m_Statistics.SetGridColor(theApp.glob_prefs->GetStatsColor(1)) ;
	m_Statistics.SetPlotColor( theApp.glob_prefs->GetStatsColor(8),0) ;
	m_Statistics.SetPlotColor( theApp.glob_prefs->GetStatsColor(10),1) ;
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	m_Statistics.SetPlotColor( theApp.glob_prefs->GetStatsColor(9),2) ;
	m_Statistics.SetPlotColor( theApp.glob_prefs->GetStatsColor(12/*11*/),3) ; //MORPH - HotFix by SiRoB, 11 was for Systray color
	//MORPH END - Added by Yun.SF3, ZZ Upload System

	m_DownloadOMeter.SetYUnits(GetResString(IDS_ST_DOWNLOAD));
	m_DownloadOMeter.SetLegendLabel(GetResString(IDS_ST_SESSION),0);
	Buffer.Format(" (%u %s)",theApp.glob_prefs->GetStatsAverageMinutes(),GetResString(IDS_MINS));
	m_DownloadOMeter.SetLegendLabel(GetResString(IDS_AVG)+Buffer,1);
	m_DownloadOMeter.SetLegendLabel(GetResString(IDS_ST_CURRENT),2);

	m_UploadOMeter.SetYUnits(GetResString(IDS_ST_UPLOAD));
	m_UploadOMeter.SetLegendLabel(GetResString(IDS_ST_SESSION),0);
	Buffer.Format(" (%u %s)",theApp.glob_prefs->GetStatsAverageMinutes(),GetResString(IDS_MINS));
	m_UploadOMeter.SetLegendLabel(GetResString(IDS_AVG)+Buffer,1);
	m_UploadOMeter.SetLegendLabel(GetResString(IDS_ST_ULCURRENT),2);
	m_UploadOMeter.SetLegendLabel(GetResString(IDS_ST_ULFRIEND),3);
	m_UploadOMeter.SetLegendLabel(GetResString(IDS_ST_ULSLOTSNOOVERHEAD),4);

	m_Statistics.SetYUnits(GetResString(IDS_FSTAT_CONNECTION/*IDS_CONNECTIONSTATISTICS*/));
	Buffer.Format("%s (1:%u)", GetResString(IDS_ST_ACTIVEC), theApp.glob_prefs->GetStatsConnectionsGraphRatio());
	m_Statistics.SetLegendLabel(Buffer,0);
	m_Statistics.SetLegendLabel(GetResString(IDS_ST_ACTIVEU_ZZ),1);
	m_Statistics.SetLegendLabel(GetResString(IDS_SP_TOTALUL),2);
	m_Statistics.SetLegendLabel(GetResString(IDS_ST_ACTIVED),3);
}

 //MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
void CStatisticsDlg::SetCurrentRate(float uploadrate, float downloadrate, float uploadtonetworkrate, float uploadrateControlPackets) {
	double m_dPlotDataUp[ 5 ];
 //MORPH END   - Added by Yun.SF3, ZZ Upload System 20030818-1923
	double m_dPlotDataDown[ 3 ];
	// -khaos--+++> Must accomodate changed GetDownloadStats...
	int myStats[19];
	// <-----khaos-

	if (!theApp.emuledlg->IsRunning()) return;

	// current rate
	m_dPlotDataDown[2]=downloadrate;

	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	// current rate to network (standardPackets+controlPackets)
	m_dPlotDataUp[2]=uploadrate;
	// current rate to network (friends excluded)
	m_dPlotDataUp[3]=uploadtonetworkrate;
	// current rate (overhead excluded)
	m_dPlotDataUp[4]=uploadrate-uploadrateControlPackets;
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923

	// -khaos--+++>
	//I moved code from here that updated maxDown in the original code...
	// <-----khaos-
	
	// Websever [kuchin]
	UpDown updown;
	updown.upload = uploadrate;
	updown.download = downloadrate;
	updown.connections=activeconnections;
	theApp.webserver->AddStatsLine(updown);

	// averages
	m_dPlotDataDown[0]=	GetAvgDownloadRate(AVG_SESSION);
	m_dPlotDataUp[0]=	GetAvgUploadRate(AVG_SESSION);

	m_dPlotDataDown[1]=	GetAvgDownloadRate(AVG_TIME);
	m_dPlotDataUp[1]=	GetAvgUploadRate(AVG_TIME);

	// show
	m_DownloadOMeter.AppendPoints(m_dPlotDataDown);
	m_UploadOMeter.AppendPoints(m_dPlotDataUp);


	// get Partialfiles summary
	theApp.downloadqueue->GetDownloadStats(myStats);
	// -khaos--+++> Ratio is now handled in the scope itself
	m_dPlotDataMore[0]=activeconnections;
	// <-----khaos-
 	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	m_dPlotDataMore[1]=theApp.uploadqueue->GetActiveUploadsCount();
	m_dPlotDataMore[2]=theApp.uploadqueue->GetUploadQueueLength();
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	m_dPlotDataMore[3]=myStats[1];

	//for (int i=0;i<4;i++) if (m_dPlotDataMore[i]>theApp.glob_prefs->GetStatsMax()) {resize=true;theApp.glob_prefs->GetStatsMax()=(int)m_dPlotDataMore[i];}
	//if (resize) m_Statistics.SetRanges(0, theApp.glob_prefs->GetStatsMax()+15) ;
	m_Statistics.AppendPoints(m_dPlotDataMore);
}

// -khaos--+++> This function is going to basically calculate and save a bunch of averages.
//				I made a seperate funtion so that it would always run instead of having
//				the averages not be calculated if the graphs are disabled (Which is bad!).
void CStatisticsDlg::UpdateConnectionStats(float uploadrate, float downloadrate){
	rateUp = uploadrate;
	rateDown = downloadrate;

	if (maxUp<uploadrate) maxUp=uploadrate;
	if (maxcumUp<maxUp) {
		maxcumUp=maxUp;
		theApp.glob_prefs->Add2ConnMaxUpRate(maxcumUp);
	}

	if (maxDown<downloadrate) maxDown=downloadrate; // MOVED from SetCurrentRate!
	if (maxcumDown<maxDown) {
		maxcumDown=maxDown;
		theApp.glob_prefs->Add2ConnMaxDownRate(maxcumDown);
	}

	cumDownavg = GetAvgDownloadRate(AVG_TOTAL);
	if (maxcumDownavg<cumDownavg) {
		maxcumDownavg=cumDownavg;
		theApp.glob_prefs->Add2ConnMaxAvgDownRate(maxcumDownavg);
	}

	cumUpavg = GetAvgUploadRate(AVG_TOTAL);
	if (maxcumUpavg<cumUpavg) {
		maxcumUpavg=cumUpavg;
		theApp.glob_prefs->Add2ConnMaxAvgUpRate(maxcumUpavg);
	}
	

	// Transfer Times (Increment Session)
	if (uploadrate > 0 || downloadrate > 0) {
		if (start_timeTransfers == 0) start_timeTransfers = GetTickCount();
		else time_thisTransfer = (GetTickCount() - start_timeTransfers) / 1000;

		if (uploadrate > 0) {
			if (start_timeUploads == 0) start_timeUploads = GetTickCount();
			else time_thisUpload = (GetTickCount() - start_timeUploads) / 1000;
		}

		if (downloadrate > 0) {
			if (start_timeDownloads == 0) start_timeDownloads = GetTickCount();
			else time_thisDownload = (GetTickCount() - start_timeDownloads) / 1000;
		}
	}

	if (uploadrate == 0 && downloadrate == 0 && (time_thisTransfer > 0 || start_timeTransfers > 0)) {
		timeTransfers += time_thisTransfer;
		time_thisTransfer = 0;
		start_timeTransfers = 0;
	}

	if (uploadrate == 0 && (time_thisUpload > 0 || start_timeUploads > 0)) {
		timeUploads += time_thisUpload;
		time_thisUpload = 0;
		start_timeUploads = 0;
	}

	if (downloadrate == 0 && (time_thisDownload > 0 || start_timeDownloads > 0)) {
		timeDownloads += time_thisDownload;
		time_thisDownload = 0;
		start_timeDownloads = 0;
	}

	// Server Durations
	if (theApp.stat_serverConnectTime==0) time_thisServerDuration = 0;
	else time_thisServerDuration = ( GetTickCount() - theApp.stat_serverConnectTime ) / 1000;
}
// <-----khaos-

void CStatisticsDlg::UpdateConnectionsStatus(){
	activeconnections = theApp.listensocket->GetOpenSockets();
	if( peakconnections < activeconnections )
		peakconnections = activeconnections;
	// -khaos--+++>
	if (peakconnections>theApp.glob_prefs->GetConnPeakConnections()) theApp.glob_prefs->Add2ConnPeakConnections(peakconnections);
	// <-----khaos-
	if( theApp.IsConnected() ){
		totalconnectionchecks++;
		float percent;
		percent = (float)(totalconnectionchecks-1)/(float)totalconnectionchecks;
		if( percent > .99f )
			percent = .99f;
		averageconnections = (averageconnections*percent) + (float)activeconnections*(1.0f-percent);
	}
}

float CStatisticsDlg::GetMaxConperFiveModifier(){
	//This is a alpha test.. Will clean up for b version.
	float SpikeSize = theApp.listensocket->GetOpenSockets() - averageconnections ;
	if ( SpikeSize < 1 )
		return 1;
	float SpikeTolerance = 25.0f*(float)theApp.glob_prefs->GetMaxConperFive()/10.0f;
	if ( SpikeSize > SpikeTolerance )
		return 0;
	float Modifier = (1.0f-(SpikeSize/SpikeTolerance));
	return Modifier;
}

void CStatisticsDlg::RecordRate() {
	
	if (theApp.stat_transferStarttime==0) return;

	// every second By BadWolf - Accurate datarate Calculation
	uint32 stick =  ::GetTickCount();
	TransferredData newitemUP = {theApp.stat_sessionSentBytes, stick};
	TransferredData newitemDN = {theApp.stat_sessionReceivedBytes, stick};
	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	TransferredData newitemFriends = {theApp.stat_sessionSentBytesToFriend, stick};
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923

	downrateHistory.push_front(newitemDN);
	uprateHistory.push_front(newitemUP);
	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	uprateHistoryFriends.push_front(newitemFriends);
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923

	// limit to maxmins
	int iAverageSeconds = theApp.glob_prefs->GetStatsAverageMinutes()*60;
	while ((float)(downrateHistory.front().timestamp - downrateHistory.back().timestamp) / 1000.0 > iAverageSeconds)
		downrateHistory.pop_back();
	while ((float)(uprateHistory.front().timestamp - uprateHistory.back().timestamp) / 1000.0 > iAverageSeconds)
		uprateHistory.pop_back();
	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	while ((float)(uprateHistoryFriends.front().timestamp - uprateHistoryFriends.back().timestamp) / 1000.0 > iAverageSeconds)
		uprateHistoryFriends.pop_back();
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923
	//theApp.emuledlg->ShowTransferRate(false);
}

// -khaos--+++> Completely rewritten ShowStatistics
void CStatisticsDlg::ShowStatistics(bool forceUpdate) {
	stattree.SetRedraw(false);
	CString	cbuffer;
	// Set Tree Values

	// TRANSFER SECTION
	// If a section is not expanded, don't waste CPU cycles updating it.
	if (forceUpdate || stattree.IsExpanded(h_transfer)) {
		uint32	statGoodSessions =				0;
		uint32	statBadSessions =				0;
		double	percentSessions =				0;
		// Transfer Ratios
		if ( theApp.stat_sessionReceivedBytes>0 && theApp.stat_sessionSentBytes>0 ) {
			// Session
			if (theApp.stat_sessionReceivedBytes<theApp.stat_sessionSentBytes) {
				cbuffer.Format("%s %.2f : 1",GetResString(IDS_STATS_SRATIO),(float)theApp.stat_sessionSentBytes/theApp.stat_sessionReceivedBytes);
				stattree.SetItemText(trans[0], cbuffer);
			} else {
				cbuffer.Format("%s 1 : %.2f",GetResString(IDS_STATS_SRATIO),(float)theApp.stat_sessionReceivedBytes/theApp.stat_sessionSentBytes);
				stattree.SetItemText(trans[0], cbuffer);
			}
		}
		else {
			cbuffer.Format("%s %s", GetResString(IDS_STATS_SRATIO), GetResString(IDS_FSTAT_WAITING)); // Localize
			stattree.SetItemText(trans[0], cbuffer);
		}
		//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
		if ( theApp.stat_sessionReceivedBytes>0 && theApp.stat_sessionSentBytes>0) {
			// Session
			if (theApp.stat_sessionSentBytes > theApp.stat_sessionSentBytesToFriend && theApp.stat_sessionReceivedBytes<theApp.stat_sessionSentBytes-theApp.stat_sessionSentBytesToFriend) {
				cbuffer.Format("%s %.2f : 1",GetResString(IDS_STATS_FRATIO),(float)(theApp.stat_sessionSentBytes-theApp.stat_sessionSentBytesToFriend)/theApp.stat_sessionReceivedBytes);
				stattree.SetItemText(trans[1], cbuffer);
			} else {
				cbuffer.Format("%s 1 : %.2f",GetResString(IDS_STATS_FRATIO),(float)theApp.stat_sessionReceivedBytes/(theApp.stat_sessionSentBytes-theApp.stat_sessionSentBytesToFriend));
				stattree.SetItemText(trans[1], cbuffer);
			}
		}
		else {
			cbuffer.Format("%s %s", GetResString(IDS_STATS_FRATIO), GetResString(IDS_FSTAT_WAITING)); // Localize
			stattree.SetItemText(trans[1], cbuffer);
		}
		if ( (theApp.glob_prefs->GetTotalDownloaded()>0 && theApp.glob_prefs->GetTotalUploaded()>0) || (theApp.stat_sessionReceivedBytes>0 && theApp.stat_sessionSentBytes>0) ) {
			// Cumulative
			if ((theApp.stat_sessionReceivedBytes+theApp.glob_prefs->GetTotalDownloaded())<(theApp.stat_sessionSentBytes+theApp.glob_prefs->GetTotalUploaded())) {
				cbuffer.Format("%s %.2f : 1",GetResString(IDS_STATS_CRATIO),(float)(theApp.stat_sessionSentBytes+theApp.glob_prefs->GetTotalUploaded())/(theApp.stat_sessionReceivedBytes+theApp.glob_prefs->GetTotalDownloaded()));
				stattree.SetItemText(trans[2], cbuffer);
			} else {
				cbuffer.Format("%s 1 : %.2f",GetResString(IDS_STATS_CRATIO),(float)(theApp.stat_sessionReceivedBytes+theApp.glob_prefs->GetTotalDownloaded())/(theApp.stat_sessionSentBytes+theApp.glob_prefs->GetTotalUploaded()));
				stattree.SetItemText(trans[2], cbuffer);
			}
		}
		else {
			cbuffer.Format("%s %s", GetResString(IDS_STATS_CRATIO), GetResString(IDS_FSTAT_WAITING)); // Localize
			stattree.SetItemText(trans[2], cbuffer);
		}
		//MORPH END - Added by SiRoB, ZZ Upload system 20030818-1923
		// TRANSFER -> DOWNLOADS SECTION
		if (forceUpdate || stattree.IsExpanded(h_download)) {
			uint64	DownOHTotal = 0;
			uint64	DownOHTotalPackets = 0;
			//uint64	lostgainedStats[2]; khaos::compcorruptfix+
			int		myStats[19];
			theApp.downloadqueue->GetDownloadStats(myStats);
			// TRANSFER -> DOWNLOADS -> SESSION SECTION
			if (forceUpdate || stattree.IsExpanded(h_down_session)) {
				// Downloaded Data
				cbuffer.Format( GetResString( IDS_STATS_DDATA ) , CastItoXBytes( theApp.stat_sessionReceivedBytes ) );
				stattree.SetItemText( down_S[0] , cbuffer );
				if (forceUpdate || stattree.IsExpanded(down_S[0])) {
					// Downloaded Data By Client
					if (forceUpdate || stattree.IsExpanded(hdown_scb)) {
						int i = 0;
						uint64 DownDataTotal =		theApp.glob_prefs->GetDownSessionClientData();
						uint64 DownDataClient =		theApp.glob_prefs->GetDownData_EMULE();
						double percentClientTransferred = 0;
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						cbuffer.Format( "eMule: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ) , percentClientTransferred);
						stattree.SetItemText( down_scb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetDownData_EDONKEYHYBRID();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "eD Hybrid: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ) , percentClientTransferred );
						stattree.SetItemText( down_scb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetDownData_EDONKEY();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "eDonkey: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_scb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetDownData_MLDONKEY();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "MLdonkey: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_scb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetDownData_XMULE();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "xMule: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_scb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetDownData_CDONKEY();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "cDonkey: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_scb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetDownData_SHAREAZA();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "Shareaza: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_scb[i] , cbuffer ); i++;

					}
					// Downloaded Data By Port
					if (forceUpdate || stattree.IsExpanded(hdown_spb)) {
						int i = 0;
						uint64	PortDataDefault =	theApp.glob_prefs->GetDownDataPort_4662();
						uint64	PortDataOther =		theApp.glob_prefs->GetDownDataPort_OTHER();
						uint64	PortDataTotal =		theApp.glob_prefs->GetDownSessionDataPort();
						double	percentPortTransferred = 0;

						if ( PortDataTotal!=0 && PortDataDefault!=0 )
							percentPortTransferred = (double) 100 * PortDataDefault / PortDataTotal;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTDEF) , CastItoXBytes( PortDataDefault ) , percentPortTransferred);
						stattree.SetItemText( down_spb[i] , cbuffer ); i++;

						if ( PortDataTotal!=0 && PortDataOther!=0 )
							percentPortTransferred = (double) 100 * PortDataOther / PortDataTotal;
						else
							percentPortTransferred = 0;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTOTHER) , CastItoXBytes( PortDataOther ) , percentPortTransferred);
						stattree.SetItemText( down_spb[i] , cbuffer ); i++;
					}
				}
				// Completed Downloads
				cbuffer.Format( "%s: %u" , GetResString( IDS_STATS_COMPDL ) , theApp.glob_prefs->GetDownSessionCompletedFiles() );
				stattree.SetItemText( down_S[1] , cbuffer );
				// Active Downloads
				cbuffer.Format( GetResString( IDS_STATS_ACTDL ) , myStats[1] );
				stattree.SetItemText( down_S[2] , cbuffer );
				// Found Sources
				cbuffer.Format( GetResString( IDS_STATS_FOUNDSRC ) , myStats[0] );
				stattree.SetItemText( down_S[3] , cbuffer );
				if (forceUpdate || stattree.IsExpanded(down_S[3])) { 
					int i = 0;
					// Sources By Status
					cbuffer.Format( "%s: %u" , GetResString( IDS_ONQUEUE ) , myStats[2] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString( IDS_QUEUEFULL ) , myStats[3] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString( IDS_NONEEDEDPARTS ) , myStats[4] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString( IDS_ASKING ) , myStats[5] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString( IDS_RECHASHSET ) , myStats[6] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString( IDS_CONNECTING ) , myStats[7] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString(IDS_CONNVIASERVER) , myStats[8] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString(IDS_TOOMANYCONNS) , myStats[9] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString(IDS_NOCONNECTLOW2LOW) , myStats[10] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString(IDS_STATS_PROBLEMATIC) , myStats[12] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString(IDS_BANNED) , myStats[13] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString(IDS_ASKED4ANOTHERFILE) , myStats[15] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString(IDS_UNKNOWN) , myStats[11] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;

					// where from? (3)
					cbuffer.Format( "%s: %u" , GetResString( IDS_VIAED2KSQ ) , myStats[16] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString( IDS_VIAKAD ) , myStats[17] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;
					cbuffer.Format( "%s: %u" , GetResString( IDS_VIASE ) , myStats[18] );
					stattree.SetItemText( down_sources[i] , cbuffer ); i++;

				}
				// Set Download Sessions
				statGoodSessions =	theApp.glob_prefs->GetDownS_SuccessfulSessions() + myStats[1]; // Add Active Downloads
				statBadSessions =	theApp.glob_prefs->GetDownS_FailedSessions();
				cbuffer.Format( "%s: %u" , GetResString(IDS_STATS_DLSES) , statGoodSessions + statBadSessions );
				stattree.SetItemText( down_S[4] , cbuffer );
				if (forceUpdate || stattree.IsExpanded(down_S[4])) {
					// Set Successful Download Sessions and Average Downloaded Per Session
					percentSessions = 0;
					if (statGoodSessions > 0) {
						percentSessions = (double) 100 * statGoodSessions / (statGoodSessions + statBadSessions);
						cbuffer.Format( "%s: %s" , GetResString(IDS_STATS_AVGDATADLSES) , CastItoXBytes( (uint64) theApp.stat_sessionReceivedBytes / statGoodSessions ) ); }
					else cbuffer.Format( "%s: %s" , GetResString(IDS_STATS_AVGDATADLSES) , CastItoXBytes(0) );
					stattree.SetItemText( down_ssessions[2] , cbuffer ); // Set Avg DL/Session
					cbuffer.Format( "%s: %u (%1.1f%%)" , GetResString(IDS_STATS_SDLSES) , statGoodSessions , percentSessions );
					stattree.SetItemText( down_ssessions[0] , cbuffer ); // Set Succ Sessions
					// Set Failed Download Sessions (Avoid Division)
					if (percentSessions != 0 && statBadSessions > 0) percentSessions = 100 - percentSessions; // There were some good sessions and bad ones...
					else if (percentSessions == 0 && statBadSessions > 0) percentSessions = 100; // There were bad sessions and no good ones, must be 100%
					else percentSessions = 0; // No sessions at all, or no bad ones.
					cbuffer.Format( "%s: %u (%1.1f%%)" , GetResString(IDS_STATS_FDLSES) , statBadSessions , percentSessions );
					stattree.SetItemText( down_ssessions[1] , cbuffer );
					// Set Average Download Time
					cbuffer.Format("%s: %s", GetResString(IDS_STATS_AVGDLTIME), CastSecondsToLngHM(theApp.glob_prefs->GetDownS_AvgTime()));
					stattree.SetItemText( down_ssessions[3] , cbuffer );
				}
				// Set Gain Due To Compression
				cbuffer.Format( GetResString( IDS_STATS_GAINCOMP ) , CastItoXBytes( theApp.glob_prefs->GetSesSavedFromCompression()) );
				stattree.SetItemText( down_S[5] , cbuffer );
				// Set Lost Due To Corruption
				cbuffer.Format( GetResString( IDS_STATS_LOSTCORRUPT ) , CastItoXBytes( theApp.glob_prefs->GetSesLostFromCorruption() ) );
				stattree.SetItemText( down_S[6] , cbuffer );
				// Set Parts Saved Due To ICH
				//MORPH START - Changed by SiRoB, Khaos Stats
				cbuffer.Format( GetResString( IDS_STATS_ICHSAVED ) , theApp.glob_prefs->GetSesPartsSavedByICH() );
				//MORPH END   - Changed by SiRoB, Khaos Stats
				stattree.SetItemText( down_S[7] , cbuffer );

				// Calculate downline OH totals
				DownOHTotal = theApp.downloadqueue->GetDownDataOverheadFileRequest() + theApp.downloadqueue->GetDownDataOverheadSourceExchange() + theApp.downloadqueue->GetDownDataOverheadServer() + theApp.downloadqueue->GetDownDataOverheadOther();
				DownOHTotalPackets = theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets() + theApp.downloadqueue->GetDownDataOverheadServerPackets() + theApp.downloadqueue->GetDownDataOverheadOtherPackets();

				// Downline Overhead
				cbuffer.Format( GetResString( IDS_TOVERHEAD ) , CastItoXBytes( DownOHTotal ) , CastItoIShort( DownOHTotalPackets ) );
				stattree.SetItemText( hdown_soh , cbuffer );
				if (forceUpdate || stattree.IsExpanded(hdown_soh)) {
					int i = 0;
					// Set down session file req OH
					cbuffer.Format( GetResString( IDS_FROVERHEAD ) , CastItoXBytes( theApp.downloadqueue->GetDownDataOverheadFileRequest() ) , CastItoIShort( theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() ) );
					stattree.SetItemText( down_soh[i] , cbuffer ); i++;
					// Set down session source exch OH
					cbuffer.Format( GetResString( IDS_SSOVERHEAD ) , CastItoXBytes( theApp.downloadqueue->GetDownDataOverheadSourceExchange() ), CastItoIShort( theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets() ) );
					stattree.SetItemText( down_soh[i] , cbuffer ); i++;
					// Set down session server OH
					cbuffer.Format( GetResString( IDS_SOVERHEAD ) , CastItoXBytes( theApp.downloadqueue->GetDownDataOverheadServer() ), CastItoIShort( theApp.downloadqueue->GetDownDataOverheadServerPackets() ) );
					stattree.SetItemText( down_soh[i] , cbuffer ); i++;
				}

			}
			// TRANSFER -> DOWNLOADS -> CUMULATIVE SECTION
			if (forceUpdate || stattree.IsExpanded(h_down_total)) {
				// Downloaded Data
				cbuffer.Format(GetResString(IDS_STATS_DDATA),CastItoXBytes( theApp.stat_sessionReceivedBytes+theApp.glob_prefs->GetTotalDownloaded() ));
				stattree.SetItemText(down_T[0], cbuffer);
				if (forceUpdate || stattree.IsExpanded(down_T[0])) {
					// Downloaded Data By Client
					if (forceUpdate || stattree.IsExpanded(hdown_tcb)) {
						int i = 0;
						uint64 DownDataTotal =		theApp.glob_prefs->GetDownTotalClientData();
						uint64 DownDataClient =		theApp.glob_prefs->GetCumDownData_EMULE();
						double percentClientTransferred = 0;
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						cbuffer.Format( "eMule: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ) , percentClientTransferred);
						stattree.SetItemText( down_tcb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetCumDownData_EDONKEYHYBRID();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "eD Hybrid: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ) , percentClientTransferred );
						stattree.SetItemText( down_tcb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetCumDownData_EDONKEY();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "eDonkey: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_tcb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetCumDownData_MLDONKEY();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "MLdonkey: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_tcb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetCumDownData_XMULE();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "xMule: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_tcb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetCumDownData_CDONKEY();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "cDonkey: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_tcb[i] , cbuffer ); i++;

						DownDataClient = theApp.glob_prefs->GetCumDownData_SHAREAZA();
						if ( DownDataTotal!=0 && DownDataClient!=0 )
							percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "Shareaza: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
						stattree.SetItemText( down_tcb[i] , cbuffer ); i++;
					}
					// Downloaded Data By Port
					if (forceUpdate || stattree.IsExpanded(hdown_tpb)) {
						int i = 0;
						uint64	PortDataDefault =	theApp.glob_prefs->GetCumDownDataPort_4662();
						uint64	PortDataOther =		theApp.glob_prefs->GetCumDownDataPort_OTHER();
						uint64	PortDataTotal =		theApp.glob_prefs->GetDownTotalPortData();
						double	percentPortTransferred = 0;

						if ( PortDataTotal!=0 && PortDataDefault!=0 )
							percentPortTransferred = (double) 100 * PortDataDefault / PortDataTotal;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTDEF) , CastItoXBytes( PortDataDefault ) , percentPortTransferred);
						stattree.SetItemText( down_tpb[i] , cbuffer ); i++;

						if ( PortDataTotal!=0 && PortDataOther!=0 )
							percentPortTransferred = (double) 100 * PortDataOther / PortDataTotal;
						else
							percentPortTransferred = 0;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTOTHER), CastItoXBytes( PortDataOther ) , percentPortTransferred);
						stattree.SetItemText( down_tpb[i] , cbuffer ); i++;
					}
				}
				// Set Cum Completed Downloads
				cbuffer.Format("%s: %u", GetResString(IDS_STATS_COMPDL), theApp.glob_prefs->GetDownCompletedFiles() );
				stattree.SetItemText(down_T[1], cbuffer);
				// Set Cum Download Sessions
				statGoodSessions = theApp.glob_prefs->GetDownC_SuccessfulSessions() + myStats[1]; // Need to reset these from the session section.  Declared up there.
				statBadSessions = theApp.glob_prefs->GetDownC_FailedSessions(); // ^^^^^^^^^^^^^^
				cbuffer.Format("%s: %u", GetResString(IDS_STATS_DLSES), statGoodSessions+statBadSessions );
				stattree.SetItemText(down_T[2], cbuffer);
				if (forceUpdate || stattree.IsExpanded(down_T[2])) {
					// Set Cum Successful Download Sessions & Cum Average Download Per Sessions (Save an if-else statement)
					if (statGoodSessions > 0) {
						percentSessions = (double) 100 * statGoodSessions / (statGoodSessions + statBadSessions);
						cbuffer.Format( "%s: %s" , GetResString(IDS_STATS_AVGDATADLSES) , CastItoXBytes( (uint64) (theApp.stat_sessionReceivedBytes + theApp.glob_prefs->GetTotalDownloaded()) / statGoodSessions ) );
					}
					else {
						percentSessions = 0;
						cbuffer.Format( "%s: %s" , GetResString(IDS_STATS_AVGDATADLSES) , CastItoXBytes(0) );
					}
					stattree.SetItemText( down_tsessions[2] , cbuffer ); // Set Avg DL/Session
					cbuffer.Format( "%s: %u (%1.1f%%)", GetResString(IDS_STATS_SDLSES) , statGoodSessions , percentSessions );
					stattree.SetItemText( down_tsessions[0] , cbuffer ); // Set Successful Sessions
					// Set Cum Failed Download Sessions
					if (percentSessions != 0 && statBadSessions > 0) percentSessions = 100 - percentSessions; // There were some good sessions and bad ones...
					else if (percentSessions == 0 && statBadSessions > 0) percentSessions = 100; // There were bad sessions and no good ones, must be 100%
					else percentSessions = 0; // No sessions at all, or no bad ones.
					cbuffer.Format( "%s: %u (%1.1f%%)" , GetResString(IDS_STATS_FDLSES) , statBadSessions , percentSessions);
					stattree.SetItemText( down_tsessions[1] , cbuffer );
					// Set Cumulative Average Download Time
					uint32 avgDownTime = theApp.glob_prefs->GetDownS_AvgTime();
					if (theApp.glob_prefs->GetDownC_AvgTime()<=0) theApp.glob_prefs->Add2DownCAvgTime(avgDownTime);
					avgDownTime = (uint32) (avgDownTime+theApp.glob_prefs->GetDownC_AvgTime())/2;
					cbuffer.Format("%s: %s", GetResString(IDS_STATS_AVGDLTIME), CastSecondsToLngHM(avgDownTime));
					stattree.SetItemText(down_tsessions[3], cbuffer);
				}
				// Set Cumulative Gained Due To Compression
				cbuffer.Format( GetResString( IDS_STATS_GAINCOMP ) , CastItoXBytes( theApp.glob_prefs->GetSesSavedFromCompression() + theApp.glob_prefs->GetCumSavedFromCompression() ) );
				stattree.SetItemText( down_T[3] , cbuffer );
				// Set Cumulative Lost Due To Corruption
				cbuffer.Format( GetResString( IDS_STATS_LOSTCORRUPT ) , CastItoXBytes( theApp.glob_prefs->GetSesLostFromCorruption() + theApp.glob_prefs->GetCumLostFromCorruption() ) );
				stattree.SetItemText( down_T[4] , cbuffer );
				// Set Cumulative Saved Due To ICH
				//MORPH START - Changed by SiRoB, Khaos Stats
				cbuffer.Format( GetResString( IDS_STATS_ICHSAVED ) , theApp.glob_prefs->GetSesPartsSavedByICH() + theApp.glob_prefs->GetPartsSavedByICH() );
				//MORPH END   - Changed by SiRoB, Khaos Stats
				stattree.SetItemText( down_T[5] , cbuffer );

				if (DownOHTotal == 0 || DownOHTotalPackets == 0) {
					DownOHTotal = theApp.downloadqueue->GetDownDataOverheadFileRequest() + theApp.downloadqueue->GetDownDataOverheadSourceExchange() + theApp.downloadqueue->GetDownDataOverheadServer() + theApp.downloadqueue->GetDownDataOverheadOther();
					DownOHTotalPackets = theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets() + theApp.downloadqueue->GetDownDataOverheadServerPackets() + theApp.downloadqueue->GetDownDataOverheadOtherPackets();
				}
				// Total Overhead
				cbuffer.Format(GetResString(IDS_TOVERHEAD),CastItoXBytes(DownOHTotal + theApp.glob_prefs->GetDownOverheadTotal()), CastItoIShort(DownOHTotalPackets + theApp.glob_prefs->GetDownOverheadTotalPackets()));
				stattree.SetItemText(hdown_toh, cbuffer);
				if (forceUpdate || stattree.IsExpanded(hdown_toh)) {
					int i = 0;
					// File Request Overhead
					cbuffer.Format(GetResString(IDS_FROVERHEAD), CastItoXBytes( theApp.downloadqueue->GetDownDataOverheadFileRequest() + theApp.glob_prefs->GetDownOverheadFileReq()), CastItoIShort(theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + theApp.glob_prefs->GetDownOverheadFileReqPackets()));
					stattree.SetItemText(down_toh[i], cbuffer); i++;
					// Source Exchange Overhead
					cbuffer.Format(GetResString(IDS_SSOVERHEAD), CastItoXBytes( theApp.downloadqueue->GetDownDataOverheadSourceExchange()+theApp.glob_prefs->GetDownOverheadSrcEx()), CastItoIShort(theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets()+theApp.glob_prefs->GetDownOverheadSrcExPackets()));
					stattree.SetItemText(down_toh[i], cbuffer); i++;
					// Server Overhead
					cbuffer.Format(GetResString(IDS_SOVERHEAD), CastItoXBytes( theApp.downloadqueue->GetDownDataOverheadServer()+theApp.glob_prefs->GetDownOverheadServer()), CastItoIShort(theApp.downloadqueue->GetDownDataOverheadServerPackets()+theApp.glob_prefs->GetDownOverheadServerPackets()));
					stattree.SetItemText(down_toh[i], cbuffer); i++;
				}
			} // - End Transfer -> Downloads -> Cumulative Section
		} // - End Transfer -> Downloads Section
		// TRANSFER-> UPLOADS SECTION
		if (forceUpdate || stattree.IsExpanded(h_upload)) {
			uint64 UpOHTotal =			0;
			uint64 UpOHTotalPackets =	0;
			// TRANSFER -> UPLOADS -> SESSION SECTION
			if (forceUpdate || stattree.IsExpanded(h_up_session)) {
				// Uploaded Data
				cbuffer.Format(GetResString(IDS_STATS_UDATA),CastItoXBytes(theApp.stat_sessionSentBytes));
				stattree.SetItemText(up_S[0], cbuffer);
				if (forceUpdate || stattree.IsExpanded(up_S[0])) {
					// Uploaded Data By Client
					if (forceUpdate || stattree.IsExpanded(hup_scb)) {
						int i = 0;
						uint64 UpDataTotal =		theApp.glob_prefs->GetUpSessionClientData();
						uint64 UpDataClient =		theApp.glob_prefs->GetUpData_EMULE();
						double percentClientTransferred = 0;
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						cbuffer.Format( "eMule: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ) , percentClientTransferred);
						stattree.SetItemText( up_scb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetUpData_EDONKEYHYBRID();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "eD Hybrid: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ) , percentClientTransferred );
						stattree.SetItemText( up_scb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetUpData_EDONKEY();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "eDonkey: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_scb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetUpData_MLDONKEY();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "MLdonkey: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_scb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetUpData_XMULE();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "xMule: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_scb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetUpData_CDONKEY();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "cDonkey: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_scb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetUpData_SHAREAZA();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "Shareaza: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_scb[i] , cbuffer ); i++;
					}
					// Uploaded Data By Port
					if (forceUpdate || stattree.IsExpanded(hup_spb)) {
						int i = 0;
						uint64	PortDataDefault =	theApp.glob_prefs->GetUpDataPort_4662();
						uint64	PortDataOther =		theApp.glob_prefs->GetUpDataPort_OTHER();
						uint64	PortDataTotal =		theApp.glob_prefs->GetUpSessionPortData();
						double	percentPortTransferred = 0;

						if ( PortDataTotal!=0 && PortDataDefault!=0 )
							percentPortTransferred = (double) 100 * PortDataDefault / PortDataTotal;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTDEF) , CastItoXBytes( PortDataDefault ) , percentPortTransferred);
						stattree.SetItemText( up_spb[i] , cbuffer ); i++;

						if ( PortDataTotal!=0 && PortDataOther!=0 )
							percentPortTransferred = (double) 100 * PortDataOther / PortDataTotal;
						else
							percentPortTransferred = 0;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTOTHER) , CastItoXBytes( PortDataOther ) , percentPortTransferred);
						stattree.SetItemText( up_spb[i] , cbuffer ); i++;
					}
					// Uploaded Data By Source
					if (forceUpdate || stattree.IsExpanded(hup_ssb)) {
						int i = 0;
						uint64	DataSourceFile =	theApp.glob_prefs->GetUpData_File();
						uint64	DataSourcePF =		theApp.glob_prefs->GetUpData_Partfile();
						uint64	DataSourceTotal =		theApp.glob_prefs->GetUpSessionDataFile();
						double	percentFileTransferred = 0;

						if ( DataSourceTotal!=0 && DataSourceFile!=0 )
							percentFileTransferred = (double) 100 * DataSourceFile / DataSourceTotal;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_DSFILE) , CastItoXBytes( DataSourceFile ) , percentFileTransferred);
						stattree.SetItemText( up_ssb[i] , cbuffer ); i++;

						if ( DataSourceTotal!=0 && DataSourcePF!=0 )
							percentFileTransferred = (double) 100 * DataSourcePF / DataSourceTotal;
						else
							percentFileTransferred = 0;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_DSPF) , CastItoXBytes( DataSourcePF ) , percentFileTransferred);
						stattree.SetItemText( up_ssb[i] , cbuffer ); i++;
					}
				}
                
				//MORPH START - Added by Yun.SF3, ZZ Upload System
                // Amount of data uploaded to friends
				cbuffer.Format(GetResString(IDS_STATS_UDATA_FRIENDS),CastItoXBytes(theApp.stat_sessionSentBytesToFriend));
				stattree.SetItemText(up_S[1], cbuffer);

				// Set fully Active Uploads
				cbuffer.Format(GetResString(IDS_STATS_ACTUL_ZZ),theApp.uploadqueue->GetActiveUploadsCount()); //theApp.uploadqueue->GetUploadQueueLength()
				stattree.SetItemText(up_S[2], cbuffer);

                // Set Set Total Uploads                
				cbuffer.Format(GetResString(IDS_STATS_TOTALUL),theApp.uploadqueue->GetUploadQueueLength());
				stattree.SetItemText(up_S[3], cbuffer);
				//MORPH END - Added by Yun.SF3, ZZ Upload System

				// Set Queue Length
				cbuffer.Format(GetResString(IDS_STATS_WAITINGUSERS),theApp.uploadqueue->GetWaitingUserCount());
				//MORPH START - Added by Yun.SF3, ZZ Upload System
				stattree.SetItemText(up_S[4], cbuffer);
				//MORPH END   - Added by Yun.SF3, ZZ Upload System


				// Set Upload Sessions
				//MORPH START - Changed by SiRoB, ZZ Upload system 20030818-1923
				statGoodSessions = theApp.uploadqueue->GetSuccessfullUpCount(); // + theApp.uploadqueue->GetUploadQueueLength();
				//MORPH END   - Changed by SiRoB, ZZ Upload system 20030818-1923
				statBadSessions = theApp.uploadqueue->GetFailedUpCount();
				cbuffer.Format("%s: %u", GetResString(IDS_STATS_ULSES), statGoodSessions + statBadSessions);
				//MORPH - Added by Yun.SF3, ZZ Upload System
				stattree.SetItemText(up_S[5], cbuffer);
				if (forceUpdate || stattree.IsExpanded(up_S[5])) {
				//MORPH - Added by Yun.SF3, ZZ Upload System
					// Set Successful Upload Sessions & Average Uploaded Per Session
					if (statGoodSessions>0) { // Blackholes are when God divided by 0
						percentSessions = (double) 100*statGoodSessions/(statGoodSessions+statBadSessions);
						//cbuffer.Format("%s: %s", GetResString(IDS_STATS_AVGDATAULSES), CastItoXBytes((uint64) theApp.stat_sessionSentBytes / statGoodSessions) ); }
						cbuffer.Format("%s: %s", GetResString(IDS_STATS_AVGDATAULSES), CastItoXBytes((uint64) theApp.uploadqueue->GetTotalCompletedBytes() / statGoodSessions) ); }
					else {
						percentSessions = 0;
						cbuffer.Format( "%s: %s" , GetResString(IDS_STATS_AVGDATAULSES) , GetResString(IDS_FSTAT_WAITING) );
                    }
					stattree.SetItemText(up_ssessions[2], cbuffer);
					cbuffer.Format(GetResString(IDS_STATS_SUCCUPCOUNT)+" (%.2f%%)" ,statGoodSessions,percentSessions);
					stattree.SetItemText(up_ssessions[0], cbuffer);
					// Set Failed Upload Sessions
					if (percentSessions != 0 && statBadSessions > 0) percentSessions = 100 - percentSessions; // There were some good sessions and bad ones...
					else if (percentSessions == 0 && statBadSessions > 0) percentSessions = 100; // There were bad sessions and no good ones, must be 100%
					else percentSessions = 0; // No sessions at all, or no bad ones.
					cbuffer.Format(GetResString(IDS_STATS_FAILUPCOUNT)+" (%.2f%%)",statBadSessions,percentSessions);
					stattree.SetItemText(up_ssessions[1], cbuffer);
					// Set Avg Upload time
					//DWORD running=theApp.uploadqueue->GetAverageUpTime();
					cbuffer.Format(GetResString(IDS_STATS_AVEUPTIME),CastSecondsToLngHM(theApp.uploadqueue->GetAverageUpTime()));
					stattree.SetItemText(up_ssessions[3], cbuffer);
				}
				// Calculate Upline OH Totals
				UpOHTotal = theApp.uploadqueue->GetUpDataOverheadFileRequest() + theApp.uploadqueue->GetUpDataOverheadSourceExchange() + theApp.uploadqueue->GetUpDataOverheadServer() + theApp.uploadqueue->GetUpDataOverheadOther();
				UpOHTotalPackets = theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets() + theApp.uploadqueue->GetUpDataOverheadServerPackets() + theApp.uploadqueue->GetUpDataOverheadOtherPackets();
				// Total Upline Overhead
				cbuffer.Format(GetResString(IDS_TOVERHEAD), CastItoXBytes( UpOHTotal), CastItoIShort(UpOHTotalPackets));
				stattree.SetItemText(hup_soh, cbuffer);
				if (forceUpdate || stattree.IsExpanded(hup_soh)) {
					int i = 0;
					// File Request Overhead
					cbuffer.Format(GetResString(IDS_FROVERHEAD), CastItoXBytes( theApp.uploadqueue->GetUpDataOverheadFileRequest()), CastItoIShort(theApp.uploadqueue->GetUpDataOverheadFileRequestPackets()));
					stattree.SetItemText(up_soh[i], cbuffer); i++;
					// Source Exchanged Overhead
					cbuffer.Format(GetResString(IDS_SSOVERHEAD), CastItoXBytes( theApp.uploadqueue->GetUpDataOverheadSourceExchange()), CastItoIShort(theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets()));
					stattree.SetItemText(up_soh[i], cbuffer); i++;
					// Server Overhead
					cbuffer.Format(GetResString(IDS_SOVERHEAD), CastItoXBytes( theApp.uploadqueue->GetUpDataOverheadServer()), CastItoIShort(theApp.uploadqueue->GetUpDataOverheadServerPackets()));
					stattree.SetItemText(up_soh[i], cbuffer); i++;
				}
			} // - End Transfer -> Uploads -> Session Section
			// TRANSFER -> UPLOADS -> CUMULATIVE SECTION
			if (forceUpdate || stattree.IsExpanded(h_up_total)) {
				
				// Uploaded Data
				cbuffer.Format(GetResString(IDS_STATS_UDATA),CastItoXBytes( theApp.stat_sessionSentBytes+theApp.glob_prefs->GetTotalUploaded()));
				stattree.SetItemText(up_T[0],cbuffer);
				if (forceUpdate || stattree.IsExpanded(up_T[0])) {
					// Uploaded Data By Client
					if (forceUpdate || stattree.IsExpanded(hup_tcb)) {
						int i = 0;
						uint64 UpDataTotal =		theApp.glob_prefs->GetUpTotalClientData();
						uint64 UpDataClient =		theApp.glob_prefs->GetCumUpData_EMULE();
						double percentClientTransferred = 0;
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						cbuffer.Format( "eMule: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ) , percentClientTransferred);
						stattree.SetItemText( up_tcb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetCumUpData_EDONKEYHYBRID();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "eD Hybrid: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ) , percentClientTransferred );
						stattree.SetItemText( up_tcb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetCumUpData_EDONKEY();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "eDonkey: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_tcb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetCumUpData_MLDONKEY();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "MLdonkey: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_tcb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetCumUpData_XMULE();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "xMule: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_tcb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetCumUpData_CDONKEY();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "cDonkey: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_tcb[i] , cbuffer ); i++;

						UpDataClient = theApp.glob_prefs->GetCumUpData_SHAREAZA();
						if ( UpDataTotal!=0 && UpDataClient!=0 )
							percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
						else
							percentClientTransferred = 0;
						cbuffer.Format( "Shareaza: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
						stattree.SetItemText( up_tcb[i] , cbuffer ); i++;
					}
					// Uploaded Data By Port
					if (forceUpdate || stattree.IsExpanded(hup_tpb)) {
						int i = 0;
						uint64	PortDataDefault =	theApp.glob_prefs->GetCumUpDataPort_4662();
						uint64	PortDataOther =		theApp.glob_prefs->GetCumUpDataPort_OTHER();
						uint64	PortDataTotal =		theApp.glob_prefs->GetUpTotalPortData();
						double	percentPortTransferred = 0;

						if ( PortDataTotal!=0 && PortDataDefault!=0 )
							percentPortTransferred = (double) 100 * PortDataDefault / PortDataTotal;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTDEF) , CastItoXBytes( PortDataDefault ) , percentPortTransferred);
						stattree.SetItemText( up_tpb[i] , cbuffer ); i++;

						if ( PortDataTotal!=0 && PortDataOther!=0 )
							percentPortTransferred = (double) 100 * PortDataOther / PortDataTotal;
						else
							percentPortTransferred = 0;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTOTHER) , CastItoXBytes( PortDataOther ) , percentPortTransferred);
						stattree.SetItemText( up_tpb[i] , cbuffer ); i++;
					}
					// Uploaded Data By Source
					if (forceUpdate || stattree.IsExpanded(hup_tsb)) {
						int i = 0;
						uint64	DataSourceFile =	theApp.glob_prefs->GetCumUpData_File();
						uint64	DataSourcePF =		theApp.glob_prefs->GetCumUpData_Partfile();
						uint64	DataSourceTotal =	theApp.glob_prefs->GetUpTotalDataFile();
						double	percentFileTransferred = 0;

						if ( DataSourceTotal!=0 && DataSourceFile!=0 )
							percentFileTransferred = (double) 100 * DataSourceFile / DataSourceTotal;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_DSFILE) , CastItoXBytes( DataSourceFile ) , percentFileTransferred);
						stattree.SetItemText( up_tsb[i] , cbuffer ); i++;

						if ( DataSourceTotal!=0 && DataSourcePF!=0 )
							percentFileTransferred = (double) 100 * DataSourcePF / DataSourceTotal;
						else
							percentFileTransferred = 0;
						cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_DSPF) , CastItoXBytes( DataSourcePF ) , percentFileTransferred);
						stattree.SetItemText( up_tsb[i] , cbuffer ); i++;
					}
				}
				// Upload Sessions
				statGoodSessions = theApp.uploadqueue->GetSuccessfullUpCount() + theApp.glob_prefs->GetUpSuccessfulSessions() + theApp.uploadqueue->GetUploadQueueLength();
				statBadSessions = theApp.uploadqueue->GetFailedUpCount() + theApp.glob_prefs->GetUpFailedSessions();
				cbuffer.Format("%s: %u", GetResString(IDS_STATS_ULSES), statGoodSessions + statBadSessions);
				stattree.SetItemText(up_T[1], cbuffer);
				if (forceUpdate || stattree.IsExpanded(up_T[1])) {
					// Set Successful Upload Sessions & Average Uploaded Per Session
					if (statGoodSessions>0) { // Blackholes are when God divided by 0
						percentSessions = (double) 100*statGoodSessions/(statGoodSessions+statBadSessions);
						cbuffer.Format("%s: %s", GetResString(IDS_STATS_AVGDATAULSES), CastItoXBytes((uint64) (theApp.stat_sessionSentBytes + theApp.glob_prefs->GetTotalUploaded()) / statGoodSessions) ); }
					else {
						percentSessions = 0;
						cbuffer.Format( "%s: %s" , GetResString(IDS_STATS_AVGDATAULSES) , GetResString(IDS_FSTAT_WAITING) ); }
					stattree.SetItemText(up_tsessions[2], cbuffer);
					cbuffer.Format(GetResString(IDS_STATS_SUCCUPCOUNT)+" (%.2f%%)",statGoodSessions,percentSessions);
					stattree.SetItemText(up_tsessions[0], cbuffer);
					// Set Failed Upload Sessions
					if (percentSessions != 0 && statBadSessions > 0) percentSessions = 100 - percentSessions; // There were some good sessions and bad ones...
					else if (percentSessions == 0 && statBadSessions > 0) percentSessions = 100; // There were bad sessions and no good ones, must be 100%
					else percentSessions = 0; // No sessions at all, or no bad ones.
					cbuffer.Format(GetResString(IDS_STATS_FAILUPCOUNT)+" (%.2f%%)",statBadSessions,percentSessions);
					stattree.SetItemText(up_tsessions[1], cbuffer);
					// Set Avg Upload time
					uint32 avguptime = theApp.uploadqueue->GetAverageUpTime();
					if (theApp.glob_prefs->GetUpAvgTime()<=0) theApp.glob_prefs->Add2UpAvgTime(avguptime);
					avguptime = (uint32) (avguptime+theApp.glob_prefs->GetUpAvgTime())/2;
					cbuffer.Format(GetResString(IDS_STATS_AVEUPTIME),CastSecondsToLngHM(avguptime));
					stattree.SetItemText(up_tsessions[3], cbuffer);
				}

				if (UpOHTotal == 0 || UpOHTotalPackets == 0) {
					// Calculate Upline OH Totals
					UpOHTotal = theApp.uploadqueue->GetUpDataOverheadFileRequest() + theApp.uploadqueue->GetUpDataOverheadSourceExchange() + theApp.uploadqueue->GetUpDataOverheadServer() + theApp.uploadqueue->GetUpDataOverheadOther();
					UpOHTotalPackets = theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets() + theApp.uploadqueue->GetUpDataOverheadServerPackets() + theApp.uploadqueue->GetUpDataOverheadOtherPackets();
				}
				// Set Cumulative Total Overhead
				cbuffer.Format(GetResString(IDS_TOVERHEAD),CastItoXBytes(UpOHTotal + theApp.glob_prefs->GetUpOverheadTotal()), CastItoIShort(UpOHTotalPackets + theApp.glob_prefs->GetUpOverheadTotalPackets()));
				stattree.SetItemText(hup_toh, cbuffer);
				if (forceUpdate || stattree.IsExpanded(hup_toh)) {
					int i = 0;
					// Set up total file req OH
					cbuffer.Format(GetResString(IDS_FROVERHEAD), CastItoXBytes( theApp.uploadqueue->GetUpDataOverheadFileRequest() + theApp.glob_prefs->GetUpOverheadFileReq()), CastItoIShort(theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + theApp.glob_prefs->GetUpOverheadFileReqPackets()));
					stattree.SetItemText(up_toh[i], cbuffer); i++;
					// Set up total source exch OH
					cbuffer.Format(GetResString(IDS_SSOVERHEAD), CastItoXBytes( theApp.uploadqueue->GetUpDataOverheadSourceExchange()+theApp.glob_prefs->GetUpOverheadSrcEx()), CastItoIShort(theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets()+theApp.glob_prefs->GetUpOverheadSrcExPackets()));
					stattree.SetItemText(up_toh[i], cbuffer); i++;
					// Set up total server OH
					cbuffer.Format(GetResString(IDS_SOVERHEAD), CastItoXBytes( theApp.uploadqueue->GetUpDataOverheadServer()+theApp.glob_prefs->GetUpOverheadServer()), CastItoIShort(theApp.uploadqueue->GetUpDataOverheadServerPackets()+theApp.glob_prefs->GetUpOverheadServerPackets()));
					stattree.SetItemText(up_toh[i], cbuffer); i++;
				}
			} // - End Transfer -> Uploads -> Cumulative Section
		} // - End Transfer -> Uploads Section
	} // - END TRANSFER SECTION


	// CONNECTION SECTION
	if (forceUpdate || stattree.IsExpanded(h_connection)) {		
		// CONNECTION -> SESSION SECTION
		if (forceUpdate || stattree.IsExpanded(h_conn_session)) {			
			// CONNECTION -> SESSION -> GENERAL SECTION
			if (forceUpdate || stattree.IsExpanded(hconn_sg)) {
				int i = 0;
				// Server Reconnects
				if (theApp.stat_reconnects>0) cbuffer.Format(GetResString(IDS_STATS_RECONNECTS),theApp.stat_reconnects-1);
				else cbuffer.Format(GetResString(IDS_STATS_RECONNECTS),0);
				stattree.SetItemText(conn_sg[i], cbuffer); i++;
				// Active Connections
				cbuffer.Format("%s: %i",GetResString(IDS_SF_ACTIVECON),activeconnections);
				stattree.SetItemText(conn_sg[i], cbuffer); i++;
				// Average Connections
				cbuffer.Format("%s: %i",GetResString(IDS_SF_AVGCON),(int)averageconnections);
				stattree.SetItemText(conn_sg[i], cbuffer); i++;
				// Peak Connections
				cbuffer.Format("%s: %i",GetResString(IDS_SF_PEAKCON),peakconnections);
				stattree.SetItemText(conn_sg[i], cbuffer); i++;
				// Connect Limit Reached
				uint32 m_itemp = theApp.listensocket->GetMaxConnectionReached();
				if( m_itemp != m_ilastMaxConnReached ){
					char osDate[30],osTime[30];
					_strtime( osTime );
					_strdate( osDate );
					cbuffer.Format("%s: %i : %s %s",GetResString(IDS_SF_MAXCONLIMITREACHED),m_itemp,osDate,osTime);
					stattree.SetItemText(conn_sg[i], cbuffer);
					m_ilastMaxConnReached = m_itemp;
				}
				else if( m_itemp == 0 ){
					cbuffer.Format("%s: %i",GetResString(IDS_SF_MAXCONLIMITREACHED),m_itemp);
					stattree.SetItemText(conn_sg[i], cbuffer);
				}
				i++;
			} // - End Connection -> Session -> General Section
			// CONNECTION -> SESSION -> UPLOADS SECTION
			if (forceUpdate || stattree.IsExpanded(hconn_su)) {
				int i = 0;
				// Upload Rate
				cbuffer.Format("%s: %.2f %s", GetResString(IDS_ST_UPLOAD), rateUp,GetResString(IDS_KBYTESEC));			stattree.SetItemText(conn_su[i], cbuffer); i++;
				// Average Upload Rate
				cbuffer.Format(GetResString(IDS_STATS_AVGUL),GetAvgUploadRate(AVG_SESSION));	stattree.SetItemText(conn_su[i], cbuffer); i++;
				// Max Upload Rate
				cbuffer.Format("%s: %.2f %s", GetResString(IDS_STATS_MAXUL), maxUp,GetResString(IDS_KBYTESEC));			stattree.SetItemText(conn_su[i], cbuffer); i++;
				// Max Average Upload Rate
				float myAverageUpRate = GetAvgUploadRate(AVG_SESSION);
				if (myAverageUpRate>maxUpavg) maxUpavg = myAverageUpRate;
				cbuffer.Format("%s: %.2f %s", GetResString(IDS_STATS_MAXAVGUL), maxUpavg,GetResString(IDS_KBYTESEC));	stattree.SetItemText(conn_su[i], cbuffer); i++;
			} // - End Connection -> Session -> Uploads Section
			// CONNECTION -> SESSION -> DOWNLOADS SECTION
			if (forceUpdate || stattree.IsExpanded(hconn_sd)) {
				int i = 0;
				// Download Rate
				cbuffer.Format("%s: %.2f KB/s", GetResString(IDS_ST_DOWNLOAD), rateDown);		stattree.SetItemText(conn_sd[i], cbuffer); i++;
				// Average Download Rate
				cbuffer.Format(GetResString(IDS_STATS_AVGDL),GetAvgDownloadRate(AVG_SESSION));	stattree.SetItemText(conn_sd[i], cbuffer); i++;
				// Max Download Rate
				cbuffer.Format(GetResString(IDS_STATS_MAXDL),maxDown);							stattree.SetItemText(conn_sd[i], cbuffer); i++;
				// Max Average Download Rate
				float myAverageDownRate = GetAvgDownloadRate(AVG_SESSION);
				if (myAverageDownRate>maxDownavg) maxDownavg = myAverageDownRate;
				cbuffer.Format(GetResString(IDS_STATS_MAXAVGDL), maxDownavg);					stattree.SetItemText(conn_sd[i], cbuffer); i++;
			} // - End Connection -> Session -> Downloads Section		
		} // - End Connection -> Session Section
		// CONNECTION -> CUMULATIVE SECTION
		if (forceUpdate || stattree.IsExpanded(h_conn_total)) {			
			// CONNECTION -> CUMULATIVE -> GENERAL SECTION
			if (forceUpdate || stattree.IsExpanded(hconn_tg)) {
				int i = 0;
				// Server Reconnects
				if(theApp.stat_reconnects>0)
					cbuffer.Format(GetResString(IDS_STATS_RECONNECTS),theApp.stat_reconnects - 1 + theApp.glob_prefs->GetConnNumReconnects());
				else
					cbuffer.Format(GetResString(IDS_STATS_RECONNECTS),theApp.glob_prefs->GetConnNumReconnects());
				stattree.SetItemText(conn_tg[i], cbuffer); i++;
				// Average Connections
				cbuffer.Format("%s: %i", GetResString(IDS_SF_AVGCON), (int) (activeconnections + theApp.glob_prefs->GetConnAvgConnections()) / 2 );
				stattree.SetItemText(conn_tg[i], cbuffer); i++;
				// Peak Connections
				cbuffer.Format("%s: %i", GetResString(IDS_SF_PEAKCON), theApp.glob_prefs->GetConnPeakConnections());
				stattree.SetItemText(conn_tg[i], cbuffer); i++;
				// Connection Limit Reached
				cbuffer.Format("%s: %i", GetResString(IDS_SF_MAXCONLIMITREACHED), theApp.listensocket->GetMaxConnectionReached() + theApp.glob_prefs->GetConnMaxConnLimitReached());
				stattree.SetItemText(conn_tg[i], cbuffer); i++;
			} // - End Connection -> Cumulative -> General Section
			// CONNECTION -> CUMULATIVE -> UPLOADS SECTION
			if (forceUpdate || stattree.IsExpanded(hconn_tu)) {
				int i = 0;
				// Average Upload Rate
				cbuffer.Format(GetResString(IDS_STATS_AVGUL),cumUpavg);
				stattree.SetItemText(conn_tu[i], cbuffer); i++;
				// Max Upload Rate
				cbuffer.Format("%s: %.2f %s", GetResString(IDS_STATS_MAXUL), maxcumUp,GetResString(IDS_KBYTESEC));
				stattree.SetItemText(conn_tu[i], cbuffer); i++;
				// Max Average Upload Rate
				cbuffer.Format("%s: %.2f %s", GetResString(IDS_STATS_MAXAVGUL), maxcumUpavg,GetResString(IDS_KBYTESEC));
				stattree.SetItemText(conn_tu[i], cbuffer); i++;
			} // - End Connection -> Cumulative -> Uploads Section
			// CONNECTION -> CUMULATIVE -> DOWNLOADS SECTION
			if (forceUpdate || stattree.IsExpanded(hconn_td)) {
				int i = 0;
				// Average Download Rate
				cbuffer.Format(GetResString(IDS_STATS_AVGDL), cumDownavg);
				stattree.SetItemText(conn_td[i], cbuffer); i++;
				// Max Download Rate
				cbuffer.Format(GetResString(IDS_STATS_MAXDL), maxcumDown);
				stattree.SetItemText(conn_td[i], cbuffer); i++;
				// Max Average Download Rate
				cbuffer.Format(GetResString(IDS_STATS_MAXAVGDL), maxcumDownavg);
				stattree.SetItemText(conn_td[i], cbuffer); i++;
			} // - End Connection -> Cumulative -> Downloads Section
		} // - End Connection -> Cumulative Section
	} // - END CONNECTION SECTION


	// TIME STATISTICS SECTION
	if (forceUpdate || stattree.IsExpanded(h_time)) {
		// Statistics Last Reset
		cbuffer.Format(GetResString(IDS_STATS_LASTRESETSTATIC), theApp.glob_prefs->GetStatsLastResetStr(true));
        stattree.SetItemText(tvitime[0], cbuffer);
		// Time Since Last Reset
		__int64 timeDiff;
		if (theApp.glob_prefs->GetStatsLastResetLng()) {
			time_t	timeNow;

			time ( &timeNow );
			timeDiff = timeNow - theApp.glob_prefs->GetStatsLastResetLng(); // In seconds
			cbuffer.Format(GetResString(IDS_STATS_TIMESINCERESET), CastSecondsToLngHM(timeDiff));
		}
		else {
			timeDiff = 0;
			cbuffer.Format(GetResString(IDS_STATS_TIMESINCERESET), GetResString(IDS_UNKNOWN));
		}
		stattree.SetItemText(tvitime[1], cbuffer);
		// TIME STATISTICS -> SESSION SECTION
		if (forceUpdate || stattree.IsExpanded(htime_s)) {
			int i = 0;
			// Run Time
			__int64 sessionRunTime = (__int64)((GetTickCount()-theApp.stat_starttime)/1000);
			cbuffer.Format("%s: %s", GetResString(IDS_STATS_RUNTIME), CastSecondsToLngHM(sessionRunTime));
			stattree.SetItemText(tvitime_s[i], cbuffer); i++;
			if (!sessionRunTime) sessionRunTime = 1;
			// Transfer Time
			cbuffer.Format("%s: %s (%1.1f%%)", GetResString(IDS_STATS_TRANSTIME), CastSecondsToLngHM(GetTransferTime()), (double) (100 * GetTransferTime()) / sessionRunTime);
			stattree.SetItemText(tvitime_s[i], cbuffer);
			if (forceUpdate || stattree.IsExpanded(tvitime_s[i])) {
				int x = 0;
				// Upload Time
				cbuffer.Format("%s: %s (%1.1f%%)", GetResString(IDS_STATS_UPTIME), CastSecondsToLngHM(GetUploadTime()), (double) (100 * GetUploadTime()) / sessionRunTime);
				stattree.SetItemText(tvitime_st[x], cbuffer); x++;;
				// Download Time
				cbuffer.Format("%s: %s (%1.1f%%)", GetResString(IDS_STATS_DOWNTIME), CastSecondsToLngHM(GetDownloadTime()), (double) (100 * GetDownloadTime()) / sessionRunTime);
				stattree.SetItemText(tvitime_st[x], cbuffer); x++;
			} i++;
			// Current Server Duration				
			cbuffer.Format("%s: %s (%1.1f%%)", GetResString(IDS_STATS_CURRSRVDUR), CastSecondsToLngHM(time_thisServerDuration), (double) (100 * time_thisServerDuration) / sessionRunTime);
			stattree.SetItemText(tvitime_s[i], cbuffer); i++;
			// Total Server Duration
			cbuffer.Format("%s: %s (%1.1f%%)", GetResString(IDS_STATS_TOTALSRVDUR), CastSecondsToLngHM(GetServerDuration()), (double) (100 * GetServerDuration()) / sessionRunTime);
			stattree.SetItemText(tvitime_s[i], cbuffer); i++;
		}
		// TIME STATISTICS -> CUMULATIVE SECTION
		if (forceUpdate || stattree.IsExpanded(htime_t)) {
			int i = 0;
			// Run Time
			__int64 totalRunTime = (__int64)((GetTickCount()-theApp.stat_starttime)/1000)+theApp.glob_prefs->GetConnRunTime();
			cbuffer.Format("%s: %s", GetResString(IDS_STATS_RUNTIME), CastSecondsToLngHM(totalRunTime));
			stattree.SetItemText(tvitime_t[i], cbuffer); i++;
			if (!totalRunTime) totalRunTime = 1;
			// Transfer Time
			cbuffer.Format("%s: %s (%1.1f%%)", GetResString(IDS_STATS_TRANSTIME), CastSecondsToLngHM(GetTransferTime() + theApp.glob_prefs->GetConnTransferTime()), (double) (100 * (GetTransferTime() + theApp.glob_prefs->GetConnTransferTime())) / totalRunTime);
			stattree.SetItemText(tvitime_t[i], cbuffer);
			if (forceUpdate || stattree.IsExpanded(tvitime_t[i])) {
				int x = 0;
				// Upload Time
				cbuffer.Format("%s: %s (%1.1f%%)", GetResString(IDS_STATS_UPTIME), CastSecondsToLngHM(GetUploadTime() + theApp.glob_prefs->GetConnUploadTime()), (double) (100 * (GetUploadTime() + theApp.glob_prefs->GetConnUploadTime())) / totalRunTime);
				stattree.SetItemText(tvitime_tt[x], cbuffer); x++;;
				// Download Time
				cbuffer.Format("%s: %s (%1.1f%%)", GetResString(IDS_STATS_DOWNTIME), CastSecondsToLngHM(GetDownloadTime() + theApp.glob_prefs->GetConnDownloadTime()), (double) (100 * (GetDownloadTime() + theApp.glob_prefs->GetConnDownloadTime())) / totalRunTime);
				stattree.SetItemText(tvitime_tt[x], cbuffer); x++;
			} i++;
			// Total Server Duration
			cbuffer.Format("%s: %s (%1.1f%%)", GetResString(IDS_STATS_TOTALSRVDUR), CastSecondsToLngHM(GetServerDuration() + theApp.glob_prefs->GetConnServerDuration()), (double) (100 * (GetServerDuration() + theApp.glob_prefs->GetConnServerDuration())) / totalRunTime);
			stattree.SetItemText(tvitime_t[i], cbuffer); i++;
		}
		// TIME STATISTICS -> PROJECTED AVERAGES SECTION
		if ( (forceUpdate || stattree.IsExpanded(htime_aap)) && timeDiff > 0 ) {
			double avgModifier[3];
			avgModifier[0] = (double) 86400 / timeDiff; // Days
			avgModifier[1] = (double) 2628000 / timeDiff; // Months
			avgModifier[2] = (double) 31536000 / timeDiff; // Years
			// TIME STATISTICS -> PROJECTED AVERAGES -> TIME PERIODS
			// This section is completely scalable.  Might add "Week" to it in the future.
			// For each time period that we are calculating a projected average for...
			for (int mx = 0; mx<3; mx++) {
				if (forceUpdate || stattree.IsExpanded(time_aaph[mx])) {
					// TIME STATISTICS -> PROJECTED AVERAGES -> TIME PERIOD -> UPLOADS SECTION
					if (forceUpdate || stattree.IsExpanded(time_aap_hup[mx])) {
						// Uploaded Data
						cbuffer.Format(GetResString(IDS_STATS_UDATA),CastItoXBytes( (uint64)(theApp.stat_sessionSentBytes+theApp.glob_prefs->GetTotalUploaded())*avgModifier[mx]));
						stattree.SetItemText(time_aap_up[mx][0],cbuffer);
						if (forceUpdate || stattree.IsExpanded(time_aap_up[mx][0])) {
							// Uploaded Data By Client
							if (forceUpdate || stattree.IsExpanded(time_aap_up_hd[mx][0])) {
								int i = 0;
								uint64 UpDataTotal =		(uint64) theApp.glob_prefs->GetUpTotalClientData() * avgModifier[mx];
								uint64 UpDataClient =		(uint64) theApp.glob_prefs->GetCumUpData_EMULE() * avgModifier[mx];
								double percentClientTransferred = 0;
								if ( UpDataTotal!=0 && UpDataClient!=0 )
									percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
								cbuffer.Format( "eMule: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ) , percentClientTransferred);
								stattree.SetItemText( time_aap_up_dc[mx][i] , cbuffer ); i++;

								UpDataClient = (uint64) theApp.glob_prefs->GetCumUpData_EDONKEYHYBRID() * avgModifier[mx];
								if ( UpDataTotal!=0 && UpDataClient!=0 )
									percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "eD Hybrid: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ) , percentClientTransferred );
								stattree.SetItemText( time_aap_up_dc[mx][i] , cbuffer ); i++;

								UpDataClient = (uint64) theApp.glob_prefs->GetCumUpData_EDONKEY() * avgModifier[mx];
								if ( UpDataTotal!=0 && UpDataClient!=0 )
									percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "eDonkey: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_up_dc[mx][i] , cbuffer ); i++;

								UpDataClient = (uint64) theApp.glob_prefs->GetCumUpData_MLDONKEY() * avgModifier[mx];
								if ( UpDataTotal!=0 && UpDataClient!=0 )
									percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "MLdonkey: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_up_dc[mx][i] , cbuffer ); i++;

								UpDataClient = (uint64) theApp.glob_prefs->GetCumUpData_XMULE() * avgModifier[mx];
								if ( UpDataTotal!=0 && UpDataClient!=0 )
									percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "xMule: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_up_dc[mx][i] , cbuffer ); i++;

								UpDataClient = (uint64) theApp.glob_prefs->GetCumUpData_CDONKEY() * avgModifier[mx];
								if ( UpDataTotal!=0 && UpDataClient!=0 )
									percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "cDonkey: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_up_dc[mx][i] , cbuffer ); i++;

								UpDataClient = (uint64) theApp.glob_prefs->GetCumUpData_SHAREAZA() * avgModifier[mx];
								if ( UpDataTotal!=0 && UpDataClient!=0 )
									percentClientTransferred = (double) 100 * UpDataClient / UpDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "Shareaza: %s (%1.1f%%)" , CastItoXBytes( UpDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_up_dc[mx][i] , cbuffer ); i++;
							}
							// Uploaded Data By Port
							if (forceUpdate || stattree.IsExpanded(time_aap_up_hd[mx][1])) {
								int i = 0;
								uint64	PortDataDefault =	(uint64) theApp.glob_prefs->GetCumUpDataPort_4662() * avgModifier[mx];
								uint64	PortDataOther =		(uint64) theApp.glob_prefs->GetCumUpDataPort_OTHER() * avgModifier[mx];
								uint64	PortDataTotal =		(uint64) theApp.glob_prefs->GetUpTotalPortData() * avgModifier[mx];
								double	percentPortTransferred = 0;

								if ( PortDataTotal!=0 && PortDataDefault!=0 )
									percentPortTransferred = (double) 100 * PortDataDefault / PortDataTotal;
								cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTDEF) , CastItoXBytes( PortDataDefault ) , percentPortTransferred);
								stattree.SetItemText( time_aap_up_dp[mx][i] , cbuffer ); i++;

								if ( PortDataTotal!=0 && PortDataOther!=0 )
									percentPortTransferred = (double) 100 * PortDataOther / PortDataTotal;
								else
									percentPortTransferred = 0;
								cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTOTHER) , CastItoXBytes( PortDataOther ) , percentPortTransferred);
								stattree.SetItemText( time_aap_up_dp[mx][i] , cbuffer ); i++;
							}
							// Uploaded Data By Source
							if (forceUpdate || stattree.IsExpanded(time_aap_up_hd[mx][2])) {
								int i = 0;
								uint64	DataSourceFile =	(uint64) theApp.glob_prefs->GetCumUpData_File() * avgModifier[mx];
								uint64	DataSourcePF =		(uint64) theApp.glob_prefs->GetCumUpData_Partfile() * avgModifier[mx];
								uint64	DataSourceTotal =	(uint64) theApp.glob_prefs->GetUpTotalDataFile() * avgModifier[mx];
								double	percentFileTransferred = 0;

								if ( DataSourceTotal!=0 && DataSourceFile!=0 )
									percentFileTransferred = (double) 100 * DataSourceFile / DataSourceTotal;
								cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_DSFILE) , CastItoXBytes( DataSourceFile ) , percentFileTransferred);
								stattree.SetItemText( time_aap_up_ds[mx][i] , cbuffer ); i++;

								if ( DataSourceTotal!=0 && DataSourcePF!=0 )
									percentFileTransferred = (double) 100 * DataSourcePF / DataSourceTotal;
								else
									percentFileTransferred = 0;
								cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_DSPF) , CastItoXBytes( DataSourcePF ) , percentFileTransferred);
								stattree.SetItemText( time_aap_up_ds[mx][i] , cbuffer ); i++;
							}
						}
						// Upload Sessions
						uint32 statGoodSessions = (uint32) (theApp.uploadqueue->GetSuccessfullUpCount() + theApp.glob_prefs->GetUpSuccessfulSessions() + theApp.uploadqueue->GetUploadQueueLength()) * avgModifier[mx];
						uint32 statBadSessions = (uint32) (theApp.uploadqueue->GetFailedUpCount() + theApp.glob_prefs->GetUpFailedSessions()) * avgModifier[mx];
						double percentSessions;
						cbuffer.Format("%s: %u", GetResString(IDS_STATS_ULSES), statGoodSessions + statBadSessions);
						stattree.SetItemText(time_aap_up[mx][1], cbuffer);
						if (forceUpdate || stattree.IsExpanded(time_aap_up[mx][1])) {
							// Set Successful Upload Sessions
							if (statGoodSessions>0) percentSessions = (double) 100*statGoodSessions/(statGoodSessions+statBadSessions);
							else percentSessions = 0;
							cbuffer.Format(GetResString(IDS_STATS_SUCCUPCOUNT),statGoodSessions,percentSessions);
							stattree.SetItemText(time_aap_up_s[mx][0], cbuffer);
							// Set Failed Upload Sessions
							if (percentSessions != 0 && statBadSessions > 0) percentSessions = 100 - percentSessions; // There were some good sessions and bad ones...
							else if (percentSessions == 0 && statBadSessions > 0) percentSessions = 100; // There were bad sessions and no good ones, must be 100%
							else percentSessions = 0; // No sessions at all, or no bad ones.
							cbuffer.Format(GetResString(IDS_STATS_FAILUPCOUNT),statBadSessions,percentSessions);
							stattree.SetItemText(time_aap_up_s[mx][1], cbuffer);
						}

						// Calculate Upline OH Totals
						uint64 UpOHTotal = (uint64) (theApp.uploadqueue->GetUpDataOverheadFileRequest() + theApp.uploadqueue->GetUpDataOverheadSourceExchange() + theApp.uploadqueue->GetUpDataOverheadServer() + theApp.uploadqueue->GetUpDataOverheadOther()) * avgModifier[mx];
						uint64 UpOHTotalPackets = (uint64) (theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets() + theApp.uploadqueue->GetUpDataOverheadServerPackets() + theApp.uploadqueue->GetUpDataOverheadOtherPackets()) * avgModifier[mx];

						// Set Cumulative Total Overhead
						cbuffer.Format(GetResString(IDS_TOVERHEAD),CastItoXBytes(UpOHTotal + ((uint64)theApp.glob_prefs->GetUpOverheadTotal() * avgModifier[mx])), CastItoIShort(UpOHTotalPackets + ((uint64)theApp.glob_prefs->GetUpOverheadTotalPackets() * avgModifier[mx])));
						stattree.SetItemText(time_aap_up[mx][2], cbuffer);
						if (forceUpdate || stattree.IsExpanded(time_aap_up[mx][2])) {
							int i = 0;
							// Set up total file req OH
							cbuffer.Format(GetResString(IDS_FROVERHEAD), CastItoXBytes( (uint64) (theApp.uploadqueue->GetUpDataOverheadFileRequest() + theApp.glob_prefs->GetUpOverheadFileReq()) * avgModifier[mx]), CastItoIShort( (uint64) (theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + theApp.glob_prefs->GetUpOverheadFileReqPackets()) * avgModifier[mx]));
							stattree.SetItemText(time_aap_up_oh[mx][i], cbuffer); i++;
							// Set up total source exch OH
							cbuffer.Format(GetResString(IDS_SSOVERHEAD), CastItoXBytes( (uint64) (theApp.uploadqueue->GetUpDataOverheadSourceExchange()+theApp.glob_prefs->GetUpOverheadSrcEx()) * avgModifier[mx]), CastItoIShort( (uint64) (theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets()+theApp.glob_prefs->GetUpOverheadSrcExPackets()) * avgModifier[mx]));
							stattree.SetItemText(time_aap_up_oh[mx][i], cbuffer); i++;
							// Set up total server OH
							cbuffer.Format(GetResString(IDS_SOVERHEAD), CastItoXBytes( (uint64) (theApp.uploadqueue->GetUpDataOverheadServer()+theApp.glob_prefs->GetUpOverheadServer()) * avgModifier[mx]), CastItoIShort( (uint64) (theApp.uploadqueue->GetUpDataOverheadServerPackets()+theApp.glob_prefs->GetUpOverheadServerPackets()) * avgModifier[mx]));
							stattree.SetItemText(time_aap_up_oh[mx][i], cbuffer); i++;
						}
					} // - End Time Statistics -> Projected Averages -> Time Period -> Uploads Section
					// TIME STATISTICS -> PROJECTED AVERAGES -> TIME PERIOD -> DOWNLOADS SECTION
					if (forceUpdate || stattree.IsExpanded(time_aap_hdown[mx])) {
						int		myStats[19];
						theApp.downloadqueue->GetDownloadStats(myStats);
						// Downloaded Data
						cbuffer.Format(GetResString(IDS_STATS_DDATA),CastItoXBytes( (uint64) (theApp.stat_sessionReceivedBytes+theApp.glob_prefs->GetTotalDownloaded()) * avgModifier[mx] ));
						stattree.SetItemText(time_aap_down[mx][0], cbuffer);
						if (forceUpdate || stattree.IsExpanded(time_aap_down[mx][0])) {
							// Downloaded Data By Client
							if (forceUpdate || stattree.IsExpanded(time_aap_down_hd[mx][0])) {
								int i = 0;
								uint64 DownDataTotal =		(uint64) theApp.glob_prefs->GetDownTotalClientData() * avgModifier[mx];
								uint64 DownDataClient =		(uint64) theApp.glob_prefs->GetCumDownData_EMULE() * avgModifier[mx];
								double percentClientTransferred = 0;
								if ( DownDataTotal!=0 && DownDataClient!=0 )
									percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
								cbuffer.Format( "eMule: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ) , percentClientTransferred);
								stattree.SetItemText( time_aap_down_dc[mx][i] , cbuffer ); i++;

								DownDataClient = (uint64) theApp.glob_prefs->GetCumDownData_EDONKEYHYBRID() * avgModifier[mx];
								if ( DownDataTotal!=0 && DownDataClient!=0 )
									percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "eD Hybrid: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ) , percentClientTransferred );
								stattree.SetItemText( time_aap_down_dc[mx][i] , cbuffer ); i++;

								DownDataClient = (uint64) theApp.glob_prefs->GetCumDownData_EDONKEY() * avgModifier[mx];
								if ( DownDataTotal!=0 && DownDataClient!=0 )
									percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "eDonkey: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_down_dc[mx][i] , cbuffer ); i++;

								DownDataClient = (uint64) theApp.glob_prefs->GetCumDownData_MLDONKEY() * avgModifier[mx];
								if ( DownDataTotal!=0 && DownDataClient!=0 )
									percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "MLdonkey: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_down_dc[mx][i] , cbuffer ); i++;

								DownDataClient = (uint64) theApp.glob_prefs->GetCumDownData_XMULE() * avgModifier[mx];
								if ( DownDataTotal!=0 && DownDataClient!=0 )
									percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "xMule: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_down_dc[mx][i] , cbuffer ); i++;

								DownDataClient = (uint64) theApp.glob_prefs->GetCumDownData_CDONKEY() * avgModifier[mx];
								if ( DownDataTotal!=0 && DownDataClient!=0 )
									percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "cDonkey: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_down_dc[mx][i] , cbuffer ); i++;

								DownDataClient = (uint64) theApp.glob_prefs->GetCumDownData_SHAREAZA() * avgModifier[mx];
								if ( DownDataTotal!=0 && DownDataClient!=0 )
									percentClientTransferred = (double) 100 * DownDataClient / DownDataTotal;
								else
									percentClientTransferred = 0;
								cbuffer.Format( "Shareaza: %s (%1.1f%%)" , CastItoXBytes( DownDataClient ), percentClientTransferred );
								stattree.SetItemText( time_aap_down_dc[mx][i] , cbuffer ); i++;
							}
							// Downloaded Data By Port
							if (forceUpdate || stattree.IsExpanded(time_aap_down_hd[mx][1])) {
								int i = 0;
								uint64	PortDataDefault =	(uint64) theApp.glob_prefs->GetCumDownDataPort_4662() * avgModifier[mx];
								uint64	PortDataOther =		(uint64) theApp.glob_prefs->GetCumDownDataPort_OTHER() * avgModifier[mx];
								uint64	PortDataTotal =		(uint64) theApp.glob_prefs->GetDownTotalPortData() * avgModifier[mx];
								double	percentPortTransferred = 0;

								if ( PortDataTotal!=0 && PortDataDefault!=0 )
									percentPortTransferred = (double) 100 * PortDataDefault / PortDataTotal;
								cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTDEF) , CastItoXBytes( PortDataDefault ) , percentPortTransferred);
								stattree.SetItemText( time_aap_down_dp[mx][i] , cbuffer ); i++;

								if ( PortDataTotal!=0 && PortDataOther!=0 )
									percentPortTransferred = (double) 100 * PortDataOther / PortDataTotal;
								else
									percentPortTransferred = 0;
								cbuffer.Format( "%s: %s (%1.1f%%)" , GetResString(IDS_STATS_PRTOTHER), CastItoXBytes( PortDataOther ) , percentPortTransferred);
								stattree.SetItemText( time_aap_down_dp[mx][i] , cbuffer ); i++;
							}
						}
						// Set Cum Completed Downloads
						cbuffer.Format("%s: %I64u", GetResString(IDS_STATS_COMPDL), (uint64) (theApp.glob_prefs->GetDownCompletedFiles() * avgModifier[mx]) );
						stattree.SetItemText(time_aap_down[mx][1], cbuffer);
						// Set Cum Download Sessions
						uint32	statGoodSessions = (uint32) (theApp.glob_prefs->GetDownC_SuccessfulSessions() + myStats[1]) * avgModifier[mx];
						uint32	statBadSessions = (uint32) theApp.glob_prefs->GetDownC_FailedSessions() * avgModifier[mx];
						double	percentSessions;
						cbuffer.Format("%s: %u", GetResString(IDS_STATS_DLSES), statGoodSessions+statBadSessions );
						stattree.SetItemText(time_aap_down[mx][2], cbuffer);
						if (forceUpdate || stattree.IsExpanded(time_aap_down[mx][2])) {
							// Set Cum Successful Download Sessions
							if (statGoodSessions > 0) percentSessions = (double) 100 * statGoodSessions / (statGoodSessions + statBadSessions);
							else percentSessions = 0;
							cbuffer.Format( "%s: %u (%1.1f%%)", GetResString(IDS_STATS_SDLSES) , statGoodSessions , percentSessions );
							stattree.SetItemText( time_aap_down_s[mx][0] , cbuffer ); // Set Successful Sessions
							// Set Cum Failed Download Sessions
							if (percentSessions != 0 && statBadSessions > 0) percentSessions = 100 - percentSessions; // There were some good sessions and bad ones...
							else if (percentSessions == 0 && statBadSessions > 0) percentSessions = 100; // There were bad sessions and no good ones, must be 100%
							else percentSessions = 0; // No sessions at all, or no bad ones.
							cbuffer.Format( "%s: %u (%1.1f%%)" , GetResString(IDS_STATS_FDLSES) , statBadSessions , percentSessions);
							stattree.SetItemText( time_aap_down_s[mx][1] , cbuffer );
						}
						// Set Cumulative Gained Due To Compression
						cbuffer.Format( GetResString( IDS_STATS_GAINCOMP ) , CastItoXBytes( (uint64) (theApp.glob_prefs->GetSesSavedFromCompression()+ theApp.glob_prefs->GetCumSavedFromCompression()) * avgModifier[mx] ) );
						stattree.SetItemText( time_aap_down[mx][3] , cbuffer );
						// Set Cumulative Lost Due To Corruption
						cbuffer.Format( GetResString( IDS_STATS_LOSTCORRUPT ) , CastItoXBytes( (uint64) (theApp.glob_prefs->GetSesLostFromCorruption() + theApp.glob_prefs->GetCumLostFromCorruption()) * avgModifier[mx] ) );
						stattree.SetItemText( time_aap_down[mx][4] , cbuffer );
						// Set Cumulative Saved Due To ICH
						//MORPH START - Changed by SiRoB, Khaos stats
						//cbuffer.Format( GetResString( IDS_STATS_ICHSAVED ) , (uint32) ((myStats[14] + theApp.glob_prefs->GetPartsSavedByICH()) * avgModifier[mx]) );
						cbuffer.Format( GetResString( IDS_STATS_ICHSAVED ) , (uint32) ((theApp.glob_prefs->GetSesPartsSavedByICH() + theApp.glob_prefs->GetPartsSavedByICH()) * avgModifier[mx]) );
						//MORPH END   - Changed by SiRoB, Khaos stats
						stattree.SetItemText( time_aap_down[mx][5] , cbuffer );

						uint64 DownOHTotal = theApp.downloadqueue->GetDownDataOverheadFileRequest() + theApp.downloadqueue->GetDownDataOverheadSourceExchange() + theApp.downloadqueue->GetDownDataOverheadServer() + theApp.downloadqueue->GetDownDataOverheadOther();
						uint64 DownOHTotalPackets = theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets() + theApp.downloadqueue->GetDownDataOverheadServerPackets() + theApp.downloadqueue->GetDownDataOverheadOtherPackets();
						// Total Overhead
						cbuffer.Format(GetResString(IDS_TOVERHEAD),CastItoXBytes( (uint64) (DownOHTotal + theApp.glob_prefs->GetDownOverheadTotal()) * avgModifier[mx]), CastItoIShort((uint64)(DownOHTotalPackets + theApp.glob_prefs->GetDownOverheadTotalPackets()) * avgModifier[mx]));
						stattree.SetItemText(time_aap_down[mx][6], cbuffer);
						if (forceUpdate || stattree.IsExpanded(time_aap_down[mx][6])) {
							int i = 0;
							// File Request Overhead
							cbuffer.Format(GetResString(IDS_FROVERHEAD), CastItoXBytes( (uint64)(theApp.downloadqueue->GetDownDataOverheadFileRequest() + theApp.glob_prefs->GetDownOverheadFileReq()) * avgModifier[mx]), CastItoIShort((uint64)(theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + theApp.glob_prefs->GetDownOverheadFileReqPackets()) * avgModifier[mx]));
							stattree.SetItemText(time_aap_down_oh[mx][i], cbuffer); i++;
							// Source Exchange Overhead
							cbuffer.Format(GetResString(IDS_SSOVERHEAD), CastItoXBytes( (uint64)(theApp.downloadqueue->GetDownDataOverheadSourceExchange()+theApp.glob_prefs->GetDownOverheadSrcEx()) * avgModifier[mx]), CastItoIShort((uint64)(theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets()+theApp.glob_prefs->GetDownOverheadSrcExPackets()) * avgModifier[mx]));
							stattree.SetItemText(time_aap_down_oh[mx][i], cbuffer); i++;
							// Server Overhead
							cbuffer.Format(GetResString(IDS_SOVERHEAD), CastItoXBytes( (uint64)(theApp.downloadqueue->GetDownDataOverheadServer()+theApp.glob_prefs->GetDownOverheadServer()) * avgModifier[mx]), CastItoIShort((uint64)(theApp.downloadqueue->GetDownDataOverheadServerPackets()+theApp.glob_prefs->GetDownOverheadServerPackets()) * avgModifier[mx]));
							stattree.SetItemText(time_aap_down_oh[mx][i], cbuffer); i++;
						}
					} // - End Time Statistics -> Projected Averages -> Time Period -> Downloads Section
				} // - End Time Statistics -> Projected Averages -> Time Period Sections
			} // - End Time Statistics -> Projected Averages Section
		} // - End Time Statistics -> Projected Averages Section Loop
	} // - END TIME STATISTICS SECTION
	
	
	// CLIENTS SECTION		[Original idea and code by xrmb]
	//						Note:	This section now has dynamic tree items.  This technique
	//								may appear in other areas, however, there is usually an
	//								advantage to displaying 0 datems.  Here, with the ver-
	//								sions being displayed the way they are, it makes sense.
	//								Who wants to stare at totally blank tree items?  ;)
	if (forceUpdate || stattree.IsExpanded(h_clients)) {
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
/*		CMap<uint16, uint16, uint32, uint32>	clientVersionEDonkey;
		CMap<uint16, uint16, uint32, uint32>	clientVersionEDonkeyHybrid;
		CMap<uint16, uint16, uint32, uint32>	clientVersionEMule;
		CMap<uint16, uint16, uint32, uint32>	clientVersionLMule;
*/ 
//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
		uint32									totalclient;
		int										myStats[15];
		theApp.clientlist->GetStatistics(totalclient, myStats); //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
		//theApp.clientlist->GetStatistics(totalclient, myStats, &clientVersionEDonkey, &clientVersionEDonkeyHybrid, &clientVersionEMule, &clientVersionLMule);

		cbuffer.Format("%s: %u ", GetResString(IDS_CLIENTLIST), totalclient);
		stattree.SetItemText(cligen[5], cbuffer);

		cbuffer.Format("%s: %u (%.2f%%) : %u (%.2f%%)", GetResString(IDS_STATS_SECUREIDENT), myStats[12] , (myStats[2]>0)?((double)100*myStats[12] / myStats[2]):0, myStats[13] , (myStats[2]>0)?((double)100*myStats[13] / myStats[2] ):0);
		stattree.SetItemText(cligen[3], cbuffer);

		cbuffer.Format("%s: %u (%.2f%%)", GetResString(IDS_IDLOW), myStats[14] , (totalclient>0)?((double)100*myStats[14] / totalclient):0);
		stattree.SetItemText(cligen[4], cbuffer);

		totalclient -= myStats[0];
		if( !totalclient ) totalclient = 1;

		// CLIENTS -> CLIENT SOFTWARE SECTION
		if (forceUpdate || stattree.IsExpanded(hclisoft)) {			
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
			// Maella -Support for tag ET_MOD_VERSION 0x55 II-
			const uint32 totalClients = theApp.clientlist->GetTotalclient();

			// Update the root of all types of client
			if(totalClients > 0){
				// eMule	
				uint32 count = 0;
				for(ClientMap::const_iterator it = theApp.clientlist->GeteMuleMap().begin(); 
					it != theApp.clientlist->GeteMuleMap().end(); 
					it++) count += it->second;
				cbuffer.Format(_T("eMule: %i (%1.1f%%)"), count, (float)count*100.0f/totalClients);
				stattree.SetItemText(clientSoft[CS_EMULE][0], cbuffer);
				updateClientBranch(clientSoft[CS_EMULE], theApp.clientlist->GeteMuleMap(), count);
	
				// xMule
				count = 0;
				for(ClientMap::const_iterator it = theApp.clientlist->GetlMuleMap().begin(); 
					it != theApp.clientlist->GetlMuleMap().end(); 
					it++) count += it->second;
				cbuffer.Format(_T("xMule: %i (%1.1f%%)"), count, (float)count*100.0f/totalClients);
				stattree.SetItemText(clientSoft[CS_XMULE][0], cbuffer);
				updateClientBranch(clientSoft[CS_XMULE], theApp.clientlist->GetlMuleMap(), count);
				// eDonkey
				count = 0;
				for(ClientMap::const_iterator it = theApp.clientlist->GeteDonkeyMap().begin(); 
					it != theApp.clientlist->GeteDonkeyMap().end(); 
					it++) count += it->second;
				cbuffer.Format(_T("eDonkey: %i (%1.1f%%)"), count, (float)count*100.0f/totalClients);
				stattree.SetItemText(clientSoft[CS_EDONKEY][0], cbuffer);
				updateClientBranch(clientSoft[CS_EDONKEY], theApp.clientlist->GeteDonkeyMap(), count);
				// eDonkeyHybrid
				count = 0;
				for(ClientMap::const_iterator it = theApp.clientlist->GeteDonkeyHybridMap().begin(); 
					it != theApp.clientlist->GeteDonkeyHybridMap().end(); 
					it++) count += it->second;
				cbuffer.Format(_T("eD Hybrid: %i (%1.1f%%)"), count, (float)count*100.0f/totalClients);
				stattree.SetItemText(clientSoft[CS_EDONKEYHYBRID][0], cbuffer);
				updateClientBranch(clientSoft[CS_EDONKEYHYBRID], theApp.clientlist->GeteDonkeyHybridMap(), count);
				// cDonkey
				count = 0;
				for(ClientMap::const_iterator it = theApp.clientlist->GetcDonkeyMap().begin(); 
					it != theApp.clientlist->GetcDonkeyMap().end(); 
					it++) count += it->second;
				cbuffer.Format(_T("cDonkey: %i (%1.1f%%)"), count, (float)count*100.0f/totalClients);
				stattree.SetItemText(clientSoft[CS_CDONKEY][0], cbuffer);
				updateClientBranch(clientSoft[CS_CDONKEY], theApp.clientlist->GetcDonkeyMap(), count);
				// MlDonkey
				count = 0;
				for(ClientMap::const_iterator it = theApp.clientlist->GetoldMlDonkeyMap().begin(); 
					it != theApp.clientlist->GetoldMlDonkeyMap().end(); 
					it++) count += it->second;
				cbuffer.Format(_T("MLdonkey: %i (%1.1f%%)"), count, (float)count*100.0f/totalClients);
				stattree.SetItemText(clientSoft[CS_MLDONKEY][0], cbuffer);
				updateClientBranch(clientSoft[CS_MLDONKEY], theApp.clientlist->GetoldMlDonkeyMap(), count);
				// Shareaza
				count = 0;
				for(ClientMap::const_iterator it = theApp.clientlist->GetShareazaMap().begin(); 
					it != theApp.clientlist->GetShareazaMap().end(); 
					it++) count += it->second;
				cbuffer.Format(_T("Shareaza: %i (%1.1f%%)"), count, (float)count*100.0f/totalClients);
				stattree.SetItemText(clientSoft[CS_SHAREAZA][0], cbuffer);
				updateClientBranch(clientSoft[CS_SHAREAZA], theApp.clientlist->GetShareazaMap(), count);
				// Unknown
				count = 0;
				for(ClientMap::const_iterator it = theApp.clientlist->GetUnknownMap().begin(); 
					it != theApp.clientlist->GetUnknownMap().end(); 
					it++) count += it->second;
				cbuffer.Format(_T("%s: %i (%1.1f%%)"), GetResString(IDS_UNKNOWN), count, (float)count*100.0f/totalClients);
				stattree.SetItemText(clientSoft[CS_UNKNOWN][0], cbuffer);
				updateClientBranch(clientSoft[CS_UNKNOWN], theApp.clientlist->GetUnknownMap(), count);
					}
			// Maella end
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-

		} // - End Clients -> Client Software Section

/*					verCount++;
				}
				if (verCount > 4) {
					cbuffer.Format("%s: %i (%1.1f%%)", GetResString(IDS_STATS_OLDCLIENTS), totalOther, (double) 100 * totalOther / myStats[4]);
					stattree.SetItemText(cli_other[1], cbuffer);
				}
				if (verCount < cli_lastCount[1]) for (uint32 i=0; i<(cli_lastCount[1]-verCount); i++) {
					stattree.DeleteItem(cli_versions[cli_lastCount[1]+7-i]);
					if ((cli_lastCount[1]+7-i) == 4) stattree.DeleteItem(cli_other[1]);
				}
				cli_lastCount[1] = verCount;
			} // End Clients -> Client Software -> eD Hybrid Section

			// CLIENTS -> CLIENT SOFTWARE -> EMULE SECTION
			if (forceUpdate || stattree.IsExpanded(clisoft[0]) || cli_lastCount[0] == 0) {				
				uint32 verMaj = 0;
				uint32 verMin = 0;
				uint32 verUp = 0;
				uint32 totcnt = 0;
				uint32 verCount = 0;
				
				//--- find top 4 eMule client versions ---
				uint32	currtop = 0;
				uint32	lasttop = 0xFFFFFFFF;
				uint32	totalOther = 0;
				for(uint32 i=0; i<8; i++)
				{
					POSITION pos=clientVersionEMule.GetStartPosition();
					uint32 topver=0;
					uint32 topcnt=0;
					double topper=0;
					while(pos)
					{
						uint16	ver;
						uint32	cnt;
						clientVersionEMule.GetNextAssoc(pos, ver, cnt);
						if(currtop<ver && ver<lasttop )
						{
							if (ver==0xFFFFFFFF ) continue;
							topper=(double)cnt/myStats[2];
							topver=ver;
							topcnt=cnt;
							currtop=ver;
						}
					}
					lasttop=currtop;
					currtop=0;

					verMaj = topver/(100*10*100);
					verMin = (topver - (verMaj*100*10*100))/(100*10);
					verUp = (topver - (verMaj*100*10*100) - (verMin*100*10))/(100);

					if (topcnt)
					{
						cbuffer.Format("v%u.%u.%u: %i (%1.1f%%)", verMaj, verMin, verUp, topcnt, topper*100);
					}
					else 
						continue;

					if (i > 3) totalOther += topcnt;
					
					if (i >= cli_lastCount[0]) {
						if (i == 4) cli_other[0] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), clisoft[0]);
						if (i > 3) cli_versions[i] = stattree.InsertItem(cbuffer, cli_other[0]);
						else cli_versions[i] = stattree.InsertItem(cbuffer, clisoft[0]);
					}
					else stattree.SetItemText(cli_versions[i], cbuffer);

					verCount++;
				}
				if (verCount > 4) {
					cbuffer.Format("%s: %i (%1.1f%%)", GetResString(IDS_STATS_OLDCLIENTS), totalOther, (double) 100 * totalOther / myStats[2]);
					stattree.SetItemText(cli_other[0], cbuffer);
				}
				if (verCount < cli_lastCount[0]) for (uint32 i=0; i<(cli_lastCount[0]-verCount); i++) {
					stattree.DeleteItem(cli_versions[cli_lastCount[0]-1-i]);
					if ((cli_lastCount[0]-1-i) == 4) stattree.DeleteItem(cli_other[0]);
				}
				cli_lastCount[0] = verCount;

			} // - End Clients -> Client Software -> eMule Section

			// CLIENTS -> CLIENT SOFTWARE -> XMULE SECTION
			if (forceUpdate || stattree.IsExpanded(clisoft[3]) || cli_lastCount[3] == 0) {				
				uint32 verMaj = 0;
				uint32 verMin = 0;
				uint32 verUp = 0;
				uint32 totcnt = 0;
				uint32 verCount = 0;
				
				//--- find top 4 xMule client versions ---
				uint32	currtop = 0;
				uint32	lasttop = 0xFFFFFFFF;
				uint32	totalOther = 0;
				for(uint32 i=0; i<8; i++)
				{
					POSITION pos=clientVersionLMule.GetStartPosition();
					uint32 topver=0;
					uint32 topcnt=0;
					double topper=0;
					while(pos)
					{
						uint16	ver;
						uint32	cnt;
						clientVersionLMule.GetNextAssoc(pos, ver, cnt);
						if(currtop<ver && ver<lasttop )
						{
							if (ver==0xFFFFFFFF || ver==0xFF  ) continue;
							topper=(double)cnt/myStats[10];
							topver=ver;
							topcnt=cnt;
							currtop=ver;
						}
					}
					lasttop=currtop;
					currtop=0;

					verMaj = topver/(100*10*100);
					verMin = (topver - (verMaj*100*10*100))/(100*10);
					verUp = (topver - (verMaj*100*10*100) - (verMin*100*10))/(100);

					if (topcnt) cbuffer.Format("v%u.%u.%u: %i (%1.1f%%)", verMaj, verMin, verUp, topcnt, topper*100);
					else continue;

					if (i > 3) totalOther += topcnt;
					
					if (i >= cli_lastCount[3]) {
						if (i == 4) cli_other[3] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), clisoft[3]);
						if (i > 3) cli_versions[i+24] = stattree.InsertItem(cbuffer, cli_other[3]);
						else cli_versions[i+24] = stattree.InsertItem(cbuffer, clisoft[3]);
					}
					else stattree.SetItemText(cli_versions[i+24], cbuffer);

					verCount++;
				}
				if (verCount > 4) {
					cbuffer.Format("%s: %i (%1.1f%%)", GetResString(IDS_STATS_OLDCLIENTS), totalOther, (double) 100 * totalOther / myStats[10]);
					stattree.SetItemText(cli_other[3], cbuffer);
				}
				if (verCount < cli_lastCount[3]) for (uint32 i=0; i<(cli_lastCount[3]-verCount); i++) {
					stattree.DeleteItem(cli_versions[cli_lastCount[3]+23-i]);
					if ((cli_lastCount[3]+23-i) == 4) stattree.DeleteItem(cli_other[3]);
				}
				cli_lastCount[3] = verCount;
			} // - End Clients -> Client Software -> xMule Section

		} // - End Clients -> Client Software Section*/
		// CLIENTS -> PORT SECTION
		if (forceUpdate || stattree.IsExpanded(hcliport)) {
			
			cbuffer.Format("%s: %u (%1.1f%%)", GetResString(IDS_STATS_PRTDEF), myStats[8], myStats[8]>0?((double)100*myStats[8]/(myStats[8]+myStats[9])):0);stattree.SetItemText(cliport[0], cbuffer);
			cbuffer.Format("%s: %u (%1.1f%%)", GetResString(IDS_STATS_PRTOTHER), myStats[9], myStats[9]>0?((double)100*myStats[9]/(myStats[8]+myStats[9])):0);stattree.SetItemText(cliport[1], cbuffer);
		} // - End Clients -> Port Section

		// General Client Statistics
		cbuffer.Format("%s: %u", GetResString(IDS_STATS_PROBLEMATIC), myStats[6]);stattree.SetItemText(cligen[0], cbuffer);
		cbuffer.Format("%s: %u", GetResString(IDS_BANNED), myStats[7]);stattree.SetItemText(cligen[1], cbuffer);
		cbuffer.Format(GetResString(IDS_STATS_FILTEREDCLIENTS),theApp.stat_filteredclients);stattree.SetItemText(cligen[2], cbuffer);
		//MORPH START - Added by IceCream, secure/leecher client in stats
		cbuffer.Format(GetResString(IDS_STATS_LEECHERCLIENTS),theApp.stat_leecherclients);stattree.SetItemText(cligen[6], cbuffer);
		//MORPH END   - Added by IceCream, secure/leecher client in stats
	} // - END CLIENTS SECTION


	// UPDATE RECORDS FOR SERVERS AND SHARED FILES
	theApp.glob_prefs->SetRecordStructMembers();

	// SERVERS SECTION
	if (forceUpdate || stattree.IsExpanded(h_servers)) {		
		// Get stat values
		uint32	servtotal, servfail, servuser, servfile, servtuser, servtfile;
		float	servocc;
		theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servtuser, servtfile, servocc);
		// Set working servers value
		cbuffer.Format("%s: %i",GetResString(IDS_SF_WORKING),servtotal-servfail);stattree.SetItemText(srv[0], cbuffer);
		if (forceUpdate || stattree.IsExpanded(srv[0])) {			
			// Set users on working servers value
			cbuffer.Format("%s: %i",GetResString(IDS_SF_WUSER),servuser);stattree.SetItemText(srv_w[0], cbuffer);
			// Set files on working servers value
			cbuffer.Format("%s: %i",GetResString(IDS_SF_WFILE),servfile);stattree.SetItemText(srv_w[1], cbuffer);
			// Set server occ value
			cbuffer.Format(GetResString(IDS_SF_SRVOCC),servocc);stattree.SetItemText(srv_w[2], cbuffer);
		}
		// Set failed servers value
		cbuffer.Format("%s: %i",GetResString(IDS_SF_FAIL),servfail);stattree.SetItemText(srv[1], cbuffer);
		// Set deleted servers value
		cbuffer.Format("%s: %i",GetResString(IDS_SF_DELCOUNT),theApp.serverlist->GetDeletedServerCount());stattree.SetItemText(srv[2], cbuffer);
		// Set total servers value
		cbuffer.Format("%s: %i",GetResString(IDS_SF_TOTAL),servtotal);stattree.SetItemText(srv[3], cbuffer);
		// Set total users value
		cbuffer.Format("%s: %i",GetResString(IDS_SF_USER),servtuser);stattree.SetItemText(srv[4], cbuffer);
		// Set total files value
		cbuffer.Format("%s: %i",GetResString(IDS_SF_FILE),servtfile);stattree.SetItemText(srv[5], cbuffer);
		// SERVERS -> RECORDS SECTION
		if (forceUpdate || stattree.IsExpanded(hsrv_records)) {			
			// Set most working servers
			cbuffer.Format("%s: %u", GetResString(IDS_STATS_SVRECWORKING), theApp.glob_prefs->GetSrvrsMostWorkingServers() );
			stattree.SetItemText(srv_r[0], cbuffer);
			// Set most users online
			cbuffer.Format("%s: %u", GetResString(IDS_STATS_SVRECUSERS), theApp.glob_prefs->GetSrvrsMostUsersOnline() );
			stattree.SetItemText(srv_r[1], cbuffer);
			// Set most files avail
			cbuffer.Format("%s: %u", GetResString(IDS_STATS_SVRECFILES), theApp.glob_prefs->GetSrvrsMostFilesAvail() );
			stattree.SetItemText(srv_r[2], cbuffer);
		} // - End Servers -> Records Section
	} // - END SERVERS SECTION

	
	// SHARED FILES SECTION
	if (forceUpdate || stattree.IsExpanded(h_shared)) {		
		// Set Number of Shared Files
		cbuffer.Format(GetResString(IDS_SHAREDFILESCOUNT),theApp.sharedfiles->GetCount());
		// SLUGFILLER: SafeHash - extra statistics
		if (theApp.sharedfiles->GetHashingCount()){
			CString tempbuffer;
			tempbuffer.Format(GetResString(IDS_HASHINGFILESCOUNT),theApp.sharedfiles->GetHashingCount());
			cbuffer += tempbuffer;
		}
		// SLUGFILLER: SafeHash
		stattree.SetItemText(shar[0], cbuffer);
		// Set Average File Size
		uint64 bytesLargestFile = 0;
		uint64 allsize=theApp.sharedfiles->GetDatasize(bytesLargestFile); // Func returns total share size and sets pointeredd uint64 to largest single filesize
		CString cbuffer2;
		if(theApp.sharedfiles->GetCount() != 0)
			cbuffer2.Format( "%s", CastItoXBytes((uint64)allsize/theApp.sharedfiles->GetCount()));
		else {
			cbuffer2.Format( "%s", CastItoXBytes(0) );
		}
		cbuffer.Format(GetResString(IDS_SF_AVERAGESIZE),cbuffer2);
		stattree.SetItemText(shar[1], cbuffer);
		// Set Largest File Size
		cbuffer.Format("%s: %s", GetResString(IDS_STATS_LARGESTFILE), CastItoXBytes(bytesLargestFile) );
		stattree.SetItemText(shar[2], cbuffer);
		// Set Total Share Size
		cbuffer.Format(GetResString(IDS_SF_SIZE),CastItoXBytes(allsize));
		stattree.SetItemText(shar[3], cbuffer);

		// SHARED FILES -> RECORDS SECTION
		if (forceUpdate || stattree.IsExpanded(hshar_records)) {			
			// Set Most Files Shared
			cbuffer.Format("%s: %u", GetResString(IDS_STATS_SHRECNUM), theApp.glob_prefs->GetSharedMostFilesShared() );
			stattree.SetItemText(shar_r[0], cbuffer);
			// Set largest avg file size
			cbuffer.Format("%s: %s", GetResString(IDS_STATS_SHRECASIZE), CastItoXBytes(theApp.glob_prefs->GetSharedLargestAvgFileSize()) );
			stattree.SetItemText(shar_r[1], cbuffer);
			// Set largest file size
			cbuffer.Format("%s: %s", GetResString(IDS_STATS_LARGESTFILE), CastItoXBytes(theApp.glob_prefs->GetSharedLargestFileSize()) );
			stattree.SetItemText(shar_r[2], cbuffer);
			// Set largest share size
			cbuffer.Format("%s: %s", GetResString(IDS_STATS_SHRECSIZE), CastItoXBytes(theApp.glob_prefs->GetSharedLargestShareSize()) );
			stattree.SetItemText(shar_r[3], cbuffer);
		} // - End Shared Files -> Records Section
	} // - END SHARED FILES SECTION

	if (forceUpdate || stattree.IsExpanded(h_total_downloads)) {			
		// diskspace stats [emule+]
		int myRateStats[3];
		uint64 ui64TotFileSize=0; 
		uint64 ui64TotBytesLeftToTransfer=0;
		uint64 ui64TotNeededSpace=0;
		uint64 t_FreeBytes=0;
		theApp.downloadqueue->GetDownloadStats(myRateStats,ui64TotFileSize,ui64TotBytesLeftToTransfer,ui64TotNeededSpace);

		uint64 ui64BytesTransfered;
		float fPercent = 0.0f;

		cbuffer.Format(GetResString(IDS_DWTOT_NR),myRateStats[2]); 
		stattree.SetItemText(h_total_num_of_dls, cbuffer);

		cbuffer.Format(GetResString(IDS_DWTOT_TSD),CastItoXBytes(ui64TotFileSize)); 
		stattree.SetItemText(h_total_size_of_dls, cbuffer);

		ui64BytesTransfered = (ui64TotFileSize-ui64TotBytesLeftToTransfer);
		if(ui64TotFileSize != 0)
			fPercent = (float)((ui64BytesTransfered*100)/(ui64TotFileSize)); //kuchin
		cbuffer.Format(GetResString(IDS_DWTOT_TCS),CastItoXBytes(ui64BytesTransfered),fPercent); 
		stattree.SetItemText(h_total_size_dld, cbuffer);

		cbuffer.Format(GetResString(IDS_DWTOT_TSL),CastItoXBytes(ui64TotBytesLeftToTransfer)); 
		stattree.SetItemText(h_total_size_left_to_dl, cbuffer);

		cbuffer.Format(GetResString(IDS_DWTOT_TSN),CastItoXBytes(ui64TotNeededSpace)); 
		stattree.SetItemText(h_total_size_needed, cbuffer);

		CString buffer2;
		t_FreeBytes = GetFreeDiskSpaceX(theApp.glob_prefs->GetTempDir());
		buffer2.Format(GetResString(IDS_DWTOT_FS), CastItoXBytes(t_FreeBytes));

		if(ui64TotNeededSpace > t_FreeBytes  )
			cbuffer.Format(GetResString(IDS_NEEDFREEDISKSPACE),buffer2,CastItoXBytes(ui64TotNeededSpace - t_FreeBytes));
		else
			cbuffer=buffer2;

		stattree.SetItemText(h_total_size_left_on_drive, cbuffer);
	}
	// - End Set Tree Values


#ifdef _DEBUG
	if (g_pfnPrevCrtAllocHook){
		_CrtMemState memState;
		_CrtMemCheckpoint(&memState);

		cbuffer.Format("%s: %u bytes in %u blocks","Free",memState.lSizes[ 0 ],memState.lCounts[ 0 ] );
		stattree.SetItemText(debug1, cbuffer);
		cbuffer.Format("%s: %u bytes in %u blocks","Normal",memState.lSizes[ 1 ],memState.lCounts[ 1 ] );
		stattree.SetItemText(debug2, cbuffer);
		cbuffer.Format("%s: %u bytes in %u blocks","CRT",memState.lSizes[ 2 ],memState.lCounts[ 2 ] );
		stattree.SetItemText(debug3, cbuffer);
		cbuffer.Format("%s: %u bytes in %u blocks","Ignore",memState.lSizes[ 3 ],memState.lCounts[ 3 ] );
		stattree.SetItemText(debug4, cbuffer);
		cbuffer.Format("%s: %u bytes in %u blocks","Client",memState.lSizes[ 4 ],memState.lCounts[ 4 ] );
		stattree.SetItemText(debug5, cbuffer);

		extern CMap<const unsigned char*, const unsigned char*, UINT, UINT> g_allocations;

		POSITION pos = blockFiles.GetStartPosition();
		while (pos != NULL) {
			const unsigned char* pszFileName;
			HTREEITEM* pTag;
			blockFiles.GetNextAssoc(pos, pszFileName, pTag);
			stattree.SetItemText(*pTag, "");
		}
		pos = g_allocations.GetStartPosition();
		while (pos != NULL) {
			const unsigned char* pszFileName;
			UINT count;
			g_allocations.GetNextAssoc(pos, pszFileName, count);
			HTREEITEM* pTag;
			if (blockFiles.Lookup(pszFileName, pTag) == 0) {
				pTag = new HTREEITEM;
				*pTag = stattree.InsertItem("0", debug2);
				stattree.SetItemData(*pTag, 1);
				blockFiles.SetAt(pszFileName, pTag);
			}
			cbuffer.Format("%s : %u blocks", pszFileName, count);
			stattree.SetItemText(*pTag, cbuffer);
		}
	}
#endif
	stattree.SetRedraw(true);

} // ShowStatistics(bool forceRedraw = false){}

void CStatisticsDlg::UpdateConnectionsGraph()
{
	// This updates the Y-Axis scale of the Connections Statistics graph...
	// And it updates the trend ratio for the active connections trend.

	CString myBuffer;
	m_Statistics.SetRanges(0, theApp.glob_prefs->GetStatsMax());
	m_Statistics.SetTrendRatio(0, theApp.glob_prefs->GetStatsConnectionsGraphRatio());
	myBuffer.Format("%s (1:%u)", GetResString(IDS_ST_ACTIVEC), theApp.glob_prefs->GetStatsConnectionsGraphRatio());
	//MORPH START - Removed by SiRoB, New Graph
	//GetDlgItem(IDC_STATIC_S0)->SetWindowText(myBuffer);
	//MORPH END   - Removed by SiRoB, New Graph
} // UpdateConnectionsGraph()

// Changed these two functions (khaos)...
float CStatisticsDlg::GetAvgDownloadRate(int averageType) {
	DWORD running;
	switch(averageType) {
		case AVG_SESSION:
			if (theApp.stat_transferStarttime == 0) return 0;            
			running=(GetTickCount()-theApp.stat_transferStarttime)/1000;
			if (running<5) return 0;
			return (float) (theApp.stat_sessionReceivedBytes/1024) / running;

		case AVG_TOTAL:
			if (theApp.stat_transferStarttime == 0) return theApp.glob_prefs->GetConnAvgDownRate();
			running=(GetTickCount()-theApp.stat_transferStarttime)/1000;
			if (running<5) return theApp.glob_prefs->GetConnAvgDownRate();
			return (float) ((( (float) (theApp.stat_sessionReceivedBytes/1024) / running ) + theApp.glob_prefs->GetConnAvgDownRate() ) / 2 );

		default:
			// By BadWolf - Accurate datarate Calculation
			if (downrateHistory.size()==0) return 0;
			float deltat = (float)(downrateHistory.front().timestamp - downrateHistory.back().timestamp) / 1000.0;
			if (deltat > 0.0) 
				return (float)((float)(downrateHistory.front().datalen-downrateHistory.back().datalen) / deltat)/1024;
			else
				return 0;
			// END By BadWolf - Accurate datarate Calculation
	}
}

float CStatisticsDlg::GetAvgUploadRate(int averageType) {
	DWORD running;
	switch(averageType) {
		case AVG_SESSION:
			if (theApp.stat_transferStarttime == 0) return 0;            
			running=(GetTickCount()-theApp.stat_transferStarttime)/1000;
			if (running<5) return 0;
			return (float) (theApp.stat_sessionSentBytes/1024) / running;

		case AVG_TOTAL:
			if (theApp.stat_transferStarttime == 0) return theApp.glob_prefs->GetConnAvgUpRate();
			running=(GetTickCount()-theApp.stat_transferStarttime)/1000;
			if (running<5) return theApp.glob_prefs->GetConnAvgUpRate();
			return (float) ((( (float) (theApp.stat_sessionSentBytes/1024) / running ) + theApp.glob_prefs->GetConnAvgUpRate() ) / 2 );

		default:
			// By BadWolf - Accurate datarate Calculation
			if (uprateHistory.size()==0) return 0;
			float deltat = (float)(uprateHistory.front().timestamp - uprateHistory.back().timestamp) / 1000.0;
			if (deltat > 0.0) 
				return (float)((float)(uprateHistory.front().datalen-uprateHistory.back().datalen) / deltat)/1024;
			else
				return 0;
			// END By BadWolf - Accurate datarate Calculation
	}
}
// <-----khaos-

void CStatisticsDlg::OnShowWindow(BOOL bShow,UINT nStatus){

}

void CStatisticsDlg::OnSize(UINT nType, int cx, int cy)
{
	CResizableDialog::OnSize(nType, cx, cy);
	if ( (cx>0 && cy>0) && (cx!=m_oldcx && cy!=m_oldcy) )
	{
		m_oldcx=cx;
		m_oldcy=cy;
		ShowInterval();
	}

//	GetWindowRect(rect);

}  //OnSize

void CStatisticsDlg::ShowInterval(){
	if (!theApp.emuledlg->IsRunning()) return;
	// Check if OScope already initialized
	if(m_DownloadOMeter.GetSafeHwnd() != NULL && m_UploadOMeter.GetSafeHwnd() != NULL){ 
		// Retrieve the size (in pixel) of the OScope
		CRect plotRect;
		m_UploadOMeter.GetPlotRect(plotRect);
		
		// Dynamic update of time scale [Maella]
		int shownSecs = plotRect.Width() * theApp.glob_prefs->GetTrafficOMeterInterval(); 
		if(shownSecs == 0){
			m_DownloadOMeter.SetXUnits(GetResString(IDS_STOPPED)); 
			m_UploadOMeter.SetXUnits(GetResString(IDS_STOPPED)); 
			//MORPH START - Added by SiRoB, New Graph
			m_Statistics.SetXUnits(GetResString(IDS_STOPPED)); 
			//MORPH END   - Added by SiRoB, New Graph
		} 
		else {
			const CString buffer = CastSecondsToHM(shownSecs);
			m_UploadOMeter.SetXUnits(buffer);
			m_DownloadOMeter.SetXUnits(buffer); 
			//MORPH START - Added by SiRoB, New Graph
			m_Statistics.SetXUnits(buffer); 
			//MORPH END   - Added by SiRoB, New Graph
		}
		UpdateData(FALSE);
	}
}

//MORPH START - Changed by SiRoB, New Graph
void CStatisticsDlg::SetARange(bool SetDownload,int maxValue){
	if (SetDownload) {
		m_DownloadOMeter.SetRange(0, maxValue/*maxValue+4*/, 0);
		m_DownloadOMeter.SetRange(0, maxValue/*maxValue+4*/, 1);
		m_DownloadOMeter.SetRange(0, maxValue/*maxValue+4*/, 2);
	}else{
		m_UploadOMeter.SetRange(0, maxValue/*maxValue+4*/, 0) ;
		m_UploadOMeter.SetRange(0, maxValue/*maxValue+4*/, 1);
		m_UploadOMeter.SetRange(0, maxValue/*maxValue+4*/, 2);
		m_UploadOMeter.SetRange(0, maxValue/*maxValue+4*/, 3);
		m_UploadOMeter.SetRange(0, maxValue/*maxValue+4*/, 4);
	}
}
//MORPH END   - Changed by SiRoB, New Graph
// -khaos--+++> Various changes in Localize() and a new button event...
void CStatisticsDlg::Localize(){
	// Used for formatting the Active Connections ratio and the time for average graphs
	CString myBuffer;

	//MORPH START - Removed by SiRoB, New Graph
	/*GetDlgItem(IDC_STATIC_D3)->SetWindowText(GetResString(IDS_ST_DOWNLOAD));
	GetDlgItem(IDC_STATIC_U)->SetWindowText(GetResString(IDS_ST_UPLOAD));
	GetDlgItem(IDC_STATIC_D)->SetWindowText(GetResString(IDS_ST_CURRENT));
	GetDlgItem(IDC_STATIC_U2)->SetWindowText(GetResString(IDS_ST_CURRENT));
	GetDlgItem(IDC_STATIC_D2)->SetWindowText(GetResString(IDS_ST_SESSION));
	GetDlgItem(IDC_STATIC_U3)->SetWindowText(GetResString(IDS_ST_SESSION));
	GetDlgItem(IDC_STATIC_S2)->SetWindowText(GetResString(IDS_ST_ACTIVED));
	
	// I added a customizable ratio to this trend, so lets display the proper ratio next to Active Connections
	myBuffer.Format("%s (1:%u)", GetResString(IDS_ST_ACTIVEC), theApp.glob_prefs->GetStatsConnectionsGraphRatio());
	GetDlgItem(IDC_STATIC_S0)->SetWindowText(myBuffer); //
	
	GetDlgItem(IDC_STATIC_S1)->SetWindowText(GetResString(IDS_ST_ACTIVEU));
	//GetDlgItem(IDC_STATIC_S3)->SetWindowText(GetResString(IDS_ST_WAITINGU));
	
	// Changed this to use myBuffer just because it's more efficient
	myBuffer.Format(" (%u %s)",theApp.glob_prefs->GetStatsAverageMinutes(),GetResString(IDS_MINS) );
	GetDlgItem(IDC_TIMEAVG1)->SetWindowText(GetResString(IDS_AVG) + myBuffer );
	GetDlgItem(IDC_TIMEAVG2)->SetWindowText(GetResString(IDS_AVG) + myBuffer ); //
	
	// Localize the new controls...
	GetDlgItem(IDC_STATIC_CONNSTATS)->SetWindowText(GetResString(IDS_CONNECTIONSTATISTICS));*/
	//MORPH END - Removed by SiRoB, New Graph
	myBuffer.Format(GetResString(IDS_STATS_LASTRESETSTATIC), theApp.glob_prefs->GetStatsLastResetStr(true));
	GetDlgItem(IDC_STATIC_LASTRESET)->SetWindowText(myBuffer);
}
// End Localize

// Menu Button: Displays the menu of stat tree commands.
void CStatisticsDlg::OnMenuButtonClicked()
{
	CRect rectBn;
	CPoint thePoint;
	GetDlgItem(IDC_BNMENU)->GetWindowRect(&rectBn);
	thePoint = rectBn.BottomRight();
	stattree.DoMenu(thePoint);
}
// <-----khaos-


void CStatisticsDlg::CreateMyTree() {
	stattree.DeleteAllItems();

	// Setup Tree
	h_transfer = stattree.InsertItem(GetResString(IDS_FSTAT_TRANSFER),1,1);				// Transfers Section
		CString buffer;
		buffer.Format("%s %s",GetResString(IDS_STATS_SRATIO),GetResString(IDS_FSTAT_WAITING));// Make It Pretty
		trans[0]= stattree.InsertItem(buffer, h_transfer);										// Session Ratio
		buffer.Format("%s %s",GetResString(IDS_STATS_FRATIO),GetResString(IDS_FSTAT_WAITING));// Make It Pretty
		trans[1]= stattree.InsertItem(buffer, h_transfer);										// Friend Ratio

		buffer.Format("%s %s",GetResString(IDS_STATS_CRATIO),GetResString(IDS_FSTAT_WAITING));// Make It Pretty
		trans[2]= stattree.InsertItem(buffer, h_transfer);										// Cumulative Ratio

	h_upload = stattree.InsertItem(GetResString(IDS_TW_UPLOADS), 6,6,h_transfer);		// Uploads Section
	
	h_up_session= stattree.InsertItem(GetResString(IDS_STATS_SESSION), 8,8,h_upload);		// Session Section (Uploads)
	for(int i = 0; i<6; i++) up_S[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_up_session); //MORPH - Added by Yun.SF3, ZZ Upload System

	  hup_scb= stattree.InsertItem(GetResString(IDS_CLIENTS),up_S[0]);							// Clients Section
		for(int i = 0; i<7; i++) up_scb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hup_scb);
	  hup_spb= stattree.InsertItem(GetResString(IDS_PORT),up_S[0]);								// Ports Section
		for(int i = 0; i<2; i++) up_spb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hup_spb);
	  hup_ssb= stattree.InsertItem(GetResString(IDS_STATS_DATASOURCE),up_S[0]);					// Data Source Section
		for(int i = 0; i<2; i++) up_ssb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hup_ssb);
		for(int i = 0; i<4; i++) up_ssessions[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), up_S[5]); //MORPH - Added by Yun.SF3, ZZ Upload System

	hup_soh= stattree.InsertItem(GetResString(IDS_STATS_OVRHD),h_up_session);					// Upline Overhead (Session)
		for(int i = 0; i<3; i++) up_soh[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hup_soh);

	h_up_total= stattree.InsertItem(GetResString(IDS_STATS_CUMULATIVE),9,9, h_upload);		// Cumulative Section (Uploads)
	up_T[0]= stattree.InsertItem(GetResString(IDS_FSTAT_WAITING),h_up_total);				// Uploaded Data (Total)
	  hup_tcb= stattree.InsertItem(GetResString(IDS_CLIENTS),up_T[0]);							// Clients Section
		for(int i = 0; i<7; i++) up_tcb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hup_tcb);
	  hup_tpb= stattree.InsertItem(GetResString(IDS_PORT),up_T[0]);								// Ports Section
		for(int i = 0; i<2; i++) up_tpb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hup_tpb);
	  hup_tsb= stattree.InsertItem(GetResString(IDS_STATS_DATASOURCE),up_T[0]);					// Data Source Section
		for(int i = 0; i<2; i++) up_tsb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hup_tsb);
	up_T[1]= stattree.InsertItem(GetResString(IDS_FSTAT_WAITING),h_up_total);				// Upload Sessions (Total)
		for(int i = 0; i<4; i++) up_tsessions[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), up_T[1]);
	hup_toh= stattree.InsertItem(GetResString(IDS_STATS_OVRHD),h_up_total);						// Upline Overhead (Total)
		for(int i = 0; i<3; i++) up_toh[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hup_toh);

	h_download = stattree.InsertItem(GetResString(IDS_TW_DOWNLOADS), 7,7,h_transfer);	// Downloads Section
	h_down_session= stattree.InsertItem(GetResString(IDS_STATS_SESSION),8,8, h_download);	// Session Section (Downloads)
	for(int i = 0; i<8; i++) down_S[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_down_session);
	  hdown_scb= stattree.InsertItem(GetResString(IDS_CLIENTS),down_S[0]);						// Clients Section
		for(int i = 0; i<7; i++) down_scb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hdown_scb);
	  hdown_spb= stattree.InsertItem(GetResString(IDS_PORT),down_S[0]);							// Ports Section
		for(int i = 0; i<2; i++) down_spb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hdown_spb);
		for(int i = 0; i<16; i++) down_sources[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), down_S[3]);
		for(int i = 0; i<4; i++) down_ssessions[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), down_S[4]);
	hdown_soh= stattree.InsertItem(GetResString(IDS_STATS_OVRHD),h_down_session);				// Downline Overhead (Session)
		for(int i = 0; i<3; i++) down_soh[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hdown_soh);

	h_down_total= stattree.InsertItem(GetResString(IDS_STATS_CUMULATIVE),9,9, h_download);	// Cumulative Section (Downloads)
	for(int i = 0; i<6; i++) down_T[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_down_total);
	  hdown_tcb= stattree.InsertItem(GetResString(IDS_CLIENTS),down_T[0]);						// Clients Section
		for(int i = 0; i<7; i++) down_tcb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hdown_tcb);
	  hdown_tpb= stattree.InsertItem(GetResString(IDS_PORT),down_T[0]);							// Ports Section
		for(int i = 0; i<2; i++) down_tpb[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hdown_tpb);
		for(int i = 0; i<4; i++) down_tsessions[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), down_T[2]);
	hdown_toh= stattree.InsertItem(GetResString(IDS_STATS_OVRHD),h_down_total);					// Downline Overhead (Total)
		for(int i = 0; i<3; i++) down_toh[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hdown_toh);

	h_connection = stattree.InsertItem(GetResString(IDS_FSTAT_CONNECTION),2,2);				// Connection Section
		h_conn_session= stattree.InsertItem(GetResString(IDS_STATS_SESSION),8,8,h_connection);	// Session Section (Connection)
			hconn_sg= stattree.InsertItem(GetResString(IDS_STATS_GENERAL),11,11,h_conn_session);	// General Section (Session)
				for(int i = 0; i<5; i++) conn_sg[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hconn_sg);
			hconn_su= stattree.InsertItem(GetResString(IDS_PW_CON_UPLBL),6,6,h_conn_session);		// Uploads Section (Session)
				for(int i = 0; i<4; i++) conn_su[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hconn_su);
			hconn_sd= stattree.InsertItem(GetResString(IDS_DOWNLOAD),7,7,h_conn_session);			// Downloads Section (Session)
				for(int i = 0; i<4; i++) conn_sd[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hconn_sd);
		h_conn_total= stattree.InsertItem(GetResString(IDS_STATS_CUMULATIVE),9,9,h_connection);	// Cumulative Section (Connection)
			hconn_tg= stattree.InsertItem(GetResString(IDS_STATS_GENERAL),11,11,h_conn_total);		// General (Total)
				for(int i = 0; i<4; i++) conn_tg[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hconn_tg);
			hconn_tu= stattree.InsertItem(GetResString(IDS_PW_CON_UPLBL),6,6,h_conn_total);			// Uploads (Total)
				for(int i = 0; i<3; i++) conn_tu[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hconn_tu);
			hconn_td= stattree.InsertItem(GetResString(IDS_DOWNLOAD),7,7,h_conn_total);				// Downloads (Total)
				for(int i = 0; i<3; i++) conn_td[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hconn_td);

	h_time = stattree.InsertItem(GetResString(IDS_STATS_TIMESTATS),12,12);					// Time Statistics Section
		for(int i = 0; i<2; i++) tvitime[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_time);
		htime_s = stattree.InsertItem(GetResString(IDS_STATS_SESSION),8,8,h_time);				// Session Section (Time)
			for(int i = 0; i<4; i++) tvitime_s[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), htime_s);
				for(int i = 0; i<2; i++) tvitime_st[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), tvitime_s[1]);
		htime_t = stattree.InsertItem(GetResString(IDS_STATS_CUMULATIVE),9,9,h_time);			// Cumulative Section (Time)
			for(int i = 0; i<3; i++) tvitime_t[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), htime_t);
				for(int i = 0; i<2; i++) tvitime_tt[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), tvitime_t[1]);
		htime_aap = stattree.InsertItem(GetResString(IDS_STATS_AVGANDPROJ),13,13,h_time);		// Projected Averages Section
			time_aaph[0] = stattree.InsertItem(GetResString(IDS_DAYLY),14,14,htime_aap);			// Daily Section
			time_aaph[1] = stattree.InsertItem(GetResString(IDS_STATS_MONTHLY),15,15,htime_aap);	// Monthly Section
			time_aaph[2] = stattree.InsertItem(GetResString(IDS_STATS_YEARLY),16,16,htime_aap);		// Yearly Section
			for(int x = 0; x<3; x++) {
				time_aap_hup[x] = stattree.InsertItem(GetResString(IDS_TW_UPLOADS),6,6,time_aaph[x]);	// Upload Section
					for(int i = 0; i<3; i++) time_aap_up[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING),time_aap_hup[x]);
						time_aap_up_hd[x][0] = stattree.InsertItem(GetResString(IDS_CLIENTS),time_aap_up[x][0]);							// Clients Section
					for(int i = 0; i<7; i++) time_aap_up_dc[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), time_aap_up_hd[x][0]);
						time_aap_up_hd[x][1] = stattree.InsertItem(GetResString(IDS_PORT),time_aap_up[x][0]);								// Ports Section
					for(int i = 0; i<2; i++) time_aap_up_dp[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), time_aap_up_hd[x][1]);
						time_aap_up_hd[x][2] = stattree.InsertItem(GetResString(IDS_STATS_DATASOURCE),time_aap_up[x][0]);					// Data Source Section
					for(int i = 0; i<2; i++) time_aap_up_ds[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), time_aap_up_hd[x][2]);
					for(int i = 0; i<2; i++) time_aap_up_s[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), time_aap_up[x][1]);
					for(int i = 0; i<3; i++) time_aap_up_oh[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), time_aap_up[x][2]);

				time_aap_hdown[x] = stattree.InsertItem(GetResString(IDS_TW_DOWNLOADS),7,7,time_aaph[x]);// Download Section
					for(int i = 0; i<7; i++) time_aap_down[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING),time_aap_hdown[x]);
						time_aap_down_hd[x][0] = stattree.InsertItem(GetResString(IDS_CLIENTS),time_aap_down[x][0]);							// Clients Section
					for(int i = 0; i<7; i++) time_aap_down_dc[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), time_aap_down_hd[x][0]);
						time_aap_down_hd[x][1] = stattree.InsertItem(GetResString(IDS_PORT),time_aap_down[x][0]);								// Ports Section
					for(int i = 0; i<2; i++) time_aap_down_dp[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), time_aap_down_hd[x][1]);
					for(int i = 0; i<2; i++) time_aap_down_s[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), time_aap_down[x][2]);
					for(int i = 0; i<3; i++) time_aap_down_oh[x][i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), time_aap_down[x][6]);
			}

	h_clients = stattree.InsertItem(GetResString(IDS_CLIENTS),3,3);							// Clients Section
	cligen[5] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_clients);
		hclisoft = stattree.InsertItem(GetResString(IDS_CLIENTSOFTWARE),h_clients);				// Client Software Section
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
//			for(int i = 0; i<8; i++) clisoft[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hclisoft);
		    clientSoft[CS_EMULE].Add(stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hclisoft));
			clientSoft[CS_CDONKEY].Add(stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hclisoft));
			clientSoft[CS_XMULE].Add(stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hclisoft));
			clientSoft[CS_SHAREAZA].Add(stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hclisoft));
			clientSoft[CS_EDONKEYHYBRID].Add(stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hclisoft));
			clientSoft[CS_EDONKEY].Add(stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hclisoft));
			clientSoft[CS_MLDONKEY].Add(stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hclisoft));
			clientSoft[CS_UNKNOWN].Add(stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hclisoft));
		hcliport = stattree.InsertItem(GetResString(IDS_PORT),h_clients);						// Client Port Section
			for(int i = 0; i<2; i++) cliport[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hcliport);
		cligen[4] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_clients);
		cligen[3] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_clients);
		
		for(int i = 0; i<3; i++) cligen[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_clients);
	cligen[6] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_clients); //MORPH - Added by SiRoB, Leecher counter
	h_servers = stattree.InsertItem(GetResString(IDS_FSTAT_SERVERS),4,4);					// Servers section
		for(int i = 0; i<6; i++) srv[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_servers);		// Servers Items
			for(int i = 0; i<3; i++) srv_w[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), srv[0]);	// Working Servers Items
		hsrv_records = stattree.InsertItem(GetResString(IDS_STATS_RECORDS),10,10,h_servers);	// Servers Records Section
			for(int i = 0; i<3; i++) srv_r[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hsrv_records);	// Record Items

	h_shared = stattree.InsertItem( GetResString(IDS_SHAREDFILES),5,5 );					// Shared Files Section
		for(int i = 0; i<4; i++) shar[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), h_shared);
		hshar_records= stattree.InsertItem(GetResString(IDS_STATS_RECORDS),10,10,h_shared);		// Shared Records Section
			for(int i = 0; i<4; i++) shar_r[i] = stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), hshar_records);

	h_total_downloads=stattree.InsertItem(GetResString(IDS_DWTOT),17,17);
	h_total_num_of_dls=stattree.InsertItem(GetResString(IDS_DWTOT_NR),h_total_downloads);
	h_total_size_of_dls=stattree.InsertItem(GetResString(IDS_DWTOT_TSD),h_total_downloads);
	h_total_size_dld=stattree.InsertItem(GetResString(IDS_DWTOT_TCS),h_total_downloads);
	h_total_size_left_to_dl=stattree.InsertItem(GetResString(IDS_DWTOT_TSL),h_total_downloads);
	h_total_size_left_on_drive=stattree.InsertItem(GetResString(IDS_DWTOT_FS),h_total_downloads);
	h_total_size_needed=stattree.InsertItem(GetResString(IDS_DWTOT_TSN),h_total_downloads);

	// Make section headers bold in order to make the tree easier to view at a glance.
	stattree.SetItemState(h_transfer, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_connection, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_time, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(htime_s, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(htime_t, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(htime_aap, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_total_downloads, TVIS_BOLD, TVIS_BOLD);
	for(int i = 0; i<3; i++) {
		stattree.SetItemState(time_aaph[i], TVIS_BOLD, TVIS_BOLD);
		stattree.SetItemState(time_aap_hup[i], TVIS_BOLD, TVIS_BOLD);
		stattree.SetItemState(time_aap_hdown[i], TVIS_BOLD, TVIS_BOLD);
	}
	stattree.SetItemState(h_clients, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_servers, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_shared, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_upload, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_download, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_up_session, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_up_total, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_down_session, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_down_total, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_conn_session, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(h_conn_total, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(hsrv_records, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(hshar_records, TVIS_BOLD, TVIS_BOLD);
	stattree.SetItemState(hconn_sg, TVIS_BOLD, TVIS_BOLD);	
	stattree.SetItemState(hconn_su, TVIS_BOLD, TVIS_BOLD);	
	stattree.SetItemState(hconn_sd, TVIS_BOLD, TVIS_BOLD);	
	stattree.SetItemState(hconn_tg, TVIS_BOLD, TVIS_BOLD);	
	stattree.SetItemState(hconn_tu, TVIS_BOLD, TVIS_BOLD);	
	stattree.SetItemState(hconn_td, TVIS_BOLD, TVIS_BOLD);	
	
	// Expand our purdy new tree...
	stattree.ApplyExpandedMask(theApp.glob_prefs->GetExpandedTreeItems());
	
	// Select the top item so that the tree is not scrolled to the bottom when first viewed.
	stattree.SelectItem(h_transfer);
	stattree.Init();

	// -khaos--+++>  Initialize our client version counts
	for (uint32 i = 0; i < 5; i++) cli_lastCount[i] = 0;

	// End Tree Setup
}

 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
// Maella -Support for tag ET_MOD_VERSION 0x55 II-
void CStatisticsDlg::updateClientBranch(CArray<HTREEITEM>& clientArray, const ClientMap& clientMap, uint32 count){

	// Check number of active subBranchs
	while(clientArray.GetCount() < (int)clientMap.size()+1){ // +1 for root 
		// Add branch(s)
		clientArray.Add(stattree.InsertItem(GetResString(IDS_FSTAT_WAITING), clientArray[0]));
	}
	while(clientArray.GetCount() > (int)clientMap.size()+1){ // +1 for root
		// Remove branch(s)
		stattree.DeleteItem(clientArray[clientArray.GetCount()-1]);
		clientArray.RemoveAt(clientArray.GetCount()-1);
	}

	// Update Subtree for expanded client branch
	if(stattree.GetItemState(clientArray[0], TVIS_EXPANDED) != 0) {
		if(clientMap.size() > 0){
			// init branch(s)
			int index = 0;
			CString description;
			for(ClientMap::const_reverse_iterator it = clientMap.rbegin(); it != clientMap.rend(); it++){
				// Set text for all branch
				description.Format(_T("%s: %i (%1.1f%%)"), it->first, it->second, (float)it->second*100.0f/count);
				stattree.SetItemText(clientArray.GetAt(++index), description);
			}
		}
	}
}
// Maella end
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-

