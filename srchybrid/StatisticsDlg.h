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

#pragma once
#include "afxdlgs.h"
#include "ResizableLib\ResizableDialog.h"
#include "OScopeCtrl.h"
#include "otherfunctions.h"
// -khaos--+++>
#include "StatisticsTree.h"
// <-----khaos-
#include "ColorFrameCtrl.h"
#include <list>
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
#include <map>
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
using namespace std;

// CStatisticsDlg dialog

class CStatisticsDlg : public CResizableDialog
{
	DECLARE_DYNAMIC(CStatisticsDlg)
public:
	CStatisticsDlg(CWnd* pParent = NULL);   // standard constructor
	~CStatisticsDlg();
	enum { IDD = IDD_STATISTICS };

	void Localize();
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	void SetCurrentRate(float uploadrate, float downloadrate, float uploadtonetworkrate, float uploadrateWithoutOverhead);
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	void ShowInterval();
	// -khaos--+++> Optional force update parameter.
	void ShowStatistics(bool forceUpdate = false);
	// <-----khaos-
	void SetARange(bool SetDownload,int maxValue);
	void RecordRate();
	float GetAvgDownloadRate(int averageType);
	float GetAvgUploadRate(int averageType);
	void RepaintMeters();
	void UpdateConnectionsStatus();
	float GetMaxConperFiveModifier();

	// -khaos--+++> (2-11-03)
	uint32	GetPeakConnections()		{ return peakconnections; }
	uint32	GetTotalConnectionChecks()	{ return totalconnectionchecks; }
	float	GetAverageConnections()		{ return averageconnections; }
	uint32	GetActiveConnections()		{ return activeconnections; }
	uint32	GetTransferTime()			{ return timeTransfers + time_thisTransfer; }
	uint32	GetUploadTime()				{ return timeUploads + time_thisUpload; }
	uint32	GetDownloadTime()			{ return timeDownloads + time_thisDownload; }
	uint32	GetServerDuration()			{ return timeServerDuration + time_thisServerDuration; }
	void	Add2TotalServerDuration()	{ timeServerDuration += time_thisServerDuration;
										  time_thisServerDuration = 0; }
	void	UpdateConnectionsGraph();
	void	UpdateConnectionStats(float uploadrate, float downloadrate);
	void	DoTreeMenu();
	void	CreateMyTree();
	// -khaos--+++> New class for our humble little tree.
	CStatisticsTree stattree;
	// <-----khaos-

private:
    COScopeCtrl m_DownloadOMeter,m_UploadOMeter,m_Statistics;
	CColorFrameCtrl m_Led1[ 3 ];
	CColorFrameCtrl m_Led2[ 3 ];
	CColorFrameCtrl m_Led3[ 4 ];

	int dl_users,dl_active;
	double m_dPlotDataMore[4];
	int stat_max;
	float maxDownavg;
	float maxDown;
	uint32 m_ilastMaxConnReached;

	// -khaos--+++>
	//	Cumulative Stats
	float	cumDownavg;
	float	maxcumDownavg;
	float	maxcumDown;
	float	cumUpavg;
	float	maxcumUpavg;
	float	maxcumUp;
	float	maxUp;
	float	maxUpavg;
	float	rateDown;
	float	rateUp;
	uint32	timeTransfers;
	uint32	timeDownloads;
	uint32	timeUploads;
	uint32	start_timeTransfers;
	uint32	start_timeDownloads;
	uint32	start_timeUploads;
	uint32	time_thisTransfer;
	uint32	time_thisDownload;
	uint32	time_thisUpload;
	uint32	timeServerDuration;
	uint32	time_thisServerDuration;
	uint32	cli_lastCount[5];
	uint8	cntDelay;
	//	Tree ImageList (2-18-03)
	CImageList	imagelistStatTree;
	//	Tree Font (2-24-03)
	//CFont		fontStatTree;
	//	Tree Declarations (2-10-03)
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	HTREEITEM	h_transfer, trans[3]; // Transfer Header and Items
	HTREEITEM	h_upload, h_up_session, up_S[6], h_up_total, up_T[2]; // Uploads Session and Total Items and Headers
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	HTREEITEM	hup_scb, up_scb[7], hup_spb, up_spb[2], hup_ssb, up_ssb[2]; // Session Uploaded Byte Breakdowns
	HTREEITEM	hup_tcb, up_tcb[7], hup_tpb, up_tpb[2], hup_tsb, up_tsb[2]; // Total Uploaded Byte Breakdowns
	HTREEITEM	hup_soh, up_soh[3], hup_toh, up_toh[3]; // Upline Overhead
	HTREEITEM	up_ssessions[4], up_tsessions[4]; // Breakdown of Upload Sessions
	HTREEITEM	h_download, h_down_session, down_S[8], h_down_total, down_T[6]; // Downloads Session and Total Items and Headers
	HTREEITEM	hdown_scb, down_scb[7], hdown_spb, down_spb[2]; // Session Downloaded Byte Breakdowns
	HTREEITEM	hdown_tcb, down_tcb[7], hdown_tpb, down_tpb[2]; // Total Downloaded Byte Breakdowns
	HTREEITEM	hdown_soh, down_soh[3], hdown_toh, down_toh[3]; // Downline Overhead
	HTREEITEM	down_ssessions[4], down_tsessions[4], down_sources[16]; // Breakdown of Download Sessions and Sources
	HTREEITEM	h_connection, h_conn_session, h_conn_total; // Connection Section Headers
	HTREEITEM	hconn_sg, conn_sg[5], hconn_su, conn_su[4], hconn_sd, conn_sd[4]; // Connection Session Section Headers and Items
	HTREEITEM	hconn_tg, conn_tg[4], hconn_tu, conn_tu[3], hconn_td, conn_td[3]; // Connection Total Section Headers and Items
	HTREEITEM	h_clients, cligen[7], hclisoft, clisoft[8], cli_versions[32], cli_other[5], hcliport, cliport[2]; // Clients Section //MORPH - Modified by IceCream, add one new section in cligen[6] for leecher
	HTREEITEM	h_servers, srv[6], srv_w[3], hsrv_records, srv_r[3]; // Servers Section
	HTREEITEM	h_shared, shar[4], hshar_records, shar_r[4]; // Shared Section
	// The time/projections section.  Yes, it's huge.
	HTREEITEM	h_time, tvitime[2], htime_s, tvitime_s[4], tvitime_st[2], htime_t, tvitime_t[3], tvitime_tt[2];
	HTREEITEM	htime_aap, time_aaph[3], time_aap_hup[3], time_aap_hdown[3];
	HTREEITEM	time_aap_up_hd[3][3], time_aap_down_hd[3][2];
	HTREEITEM	time_aap_up[3][3], time_aap_up_dc[3][7], time_aap_up_dp[3][2];
	HTREEITEM	time_aap_up_ds[3][2], time_aap_up_s[3][2], time_aap_up_oh[3][3];
	HTREEITEM	time_aap_down[3][7], time_aap_down_dc[3][7], time_aap_down_dp[3][2];
	HTREEITEM	time_aap_down_s[3][2], time_aap_down_oh[3][3];
	// <-----khaos- End Changes

	HTREEITEM h_total_downloads;
	HTREEITEM h_total_num_of_dls;
	HTREEITEM h_total_size_of_dls;
	HTREEITEM h_total_size_dld;
	HTREEITEM h_total_size_left_to_dl;
	HTREEITEM h_total_size_left_on_drive;
	HTREEITEM h_total_size_needed;

	void SetupLegend( int ResIdx, int ElmtIdx, int legendNr);
	void SetStatsRanges(int min, int max);

	const static int AVG_SESSION =0;
	// -khaos--+++> For the total average
	const static int AVG_TOTAL =2;
	// <-----khaos-
	const static int AVG_TIME =1;
	list<TransferredData> uprateHistory; // By BadWolf
	list<TransferredData> downrateHistory; // By BadWolf
	list<TransferredData> uprateHistoryFriends; //MORPH - Added by Yun.SF3, ZZ Upload System

	uint32	peakconnections;
	uint32	totalconnectionchecks;
	float	averageconnections;
	uint32	activeconnections;
	int		m_oldcx;
	int		m_oldcy;

#ifdef _DEBUG
	HTREEITEM h_debug,h_blocks,debug1,debug2,debug3,debug4,debug5;
	CMap<const unsigned char *,const unsigned char *,HTREEITEM *,HTREEITEM *> blockFiles;
#endif
 	//MORPH START - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
	typedef std::map<CString, uint32> ClientMap; // Pseudo-foward declaration
	void updateClientBranch(CArray<HTREEITEM>& clientArray, const ClientMap& clientMap, uint32 count);

	enum EClientSoftware{
		CS_EMULE,
		CS_CDONKEY,
		CS_XMULE,
		CS_SHAREAZA,
		CS_EDONKEYHYBRID,
		CS_EDONKEY,
		CS_MLDONKEY,
		CS_OLDEMULE,
		CS_UNKNOWN
	};
	CArray<HTREEITEM> clientSoft[CS_UNKNOWN+1];
 	//MORPH END   - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
protected:
	virtual void OnSize(UINT nType,int cx,int cy);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support	
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog(); 
	void	OnShowWindow(BOOL bShow,UINT nStatus);
	// -khaos--+++> Buttons, stuff.
	afx_msg void OnMenuButtonClicked();
	// <-----khaos-
public:
	afx_msg void OnSysColorChange();
};
