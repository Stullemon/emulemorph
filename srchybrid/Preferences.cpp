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

#include "StdAfx.h"
#include "preferences.h"
#include <time.h>
#include "opcodes.h"
#include "otherfunctions.h"
#include "ini2.h"
#include "stdlib.h"
#include "stdio.h"
#include "resource.h"
#include "emule.h"
#include <io.h>
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CPreferences::CPreferences(){
	srand((uint32)time(0)); // we need random numbers sometimes
	// khaos::kmod+ We need _better_ random numbers...  Sometimes.
	InitRandGen();
	// khaos::kmod-

	prefs = new Preferences_Struct;
	memset(prefs,0,sizeof(Preferences_Struct));
	prefsExt=new Preferences_Ext_Struct;
	memset(prefsExt,0,sizeof(Preferences_Ext_Struct));

	//get application start directory
	char buffer[490];
	::GetModuleFileName(0, buffer, 490);
	LPTSTR pszFileName = _tcsrchr(buffer, '\\') + 1;
	*pszFileName = '\0';

	appdir = buffer;
	configdir = appdir + _T(CONFIGFOLDER);
	m_strWebServerDir = appdir + _T("webserver\\");
	m_strLangDir = appdir + _T("lang\\");

	::CreateDirectory(GetConfigDir(),0);

	// lets move config-files in the appdir to the configdir (for upgraders <0.29a to >=0.29a )
	if ( PathFileExists(appdir+"preferences.ini")) MoveFile(appdir+"preferences.ini",configdir+"preferences.ini");
	if ( PathFileExists(appdir+"preferences.dat")) MoveFile(appdir+"preferences.dat",configdir+"preferences.dat");
	if ( PathFileExists(appdir+"adresses.dat")) MoveFile(appdir+"adresses.dat",configdir+"adresses.dat");
	if ( PathFileExists(appdir+"Category.ini")) MoveFile(appdir+"Category.ini",configdir+"Category.ini");
	if ( PathFileExists(appdir+"clients.met")) MoveFile(appdir+"clients.met",configdir+"clients.met");
	if ( PathFileExists(appdir+"emfriends.met")) MoveFile(appdir+"emfriends.met",configdir+"emfriends.met");
	if ( PathFileExists(appdir+"fileinfo.ini")) MoveFile(appdir+"fileinfo.ini",configdir+"fileinfo.ini");
	if ( PathFileExists(appdir+"ipfilter.dat")) MoveFile(appdir+"ipfilter.dat",configdir+"ipfilter.dat");
	if ( PathFileExists(appdir+"known.met")) MoveFile(appdir+"known.met",configdir+"known.met");
	if ( PathFileExists(appdir+"server.met")) MoveFile(appdir+"server.met",configdir+"server.met");
	if ( PathFileExists(appdir+"shareddir.dat")) MoveFile(appdir+"shareddir.dat",configdir+"shareddir.dat");
	if ( PathFileExists(appdir+"staticservers.dat")) MoveFile(appdir+"staticservers.dat",configdir+"staticservers.dat");
	if ( PathFileExists(appdir+"webservices.dat")) MoveFile(appdir+"webservices.dat",configdir+"webservices.dat");

	CreateUserHash();
	md4cpy(&prefs->userhash,&userhash);

	// load preferences.dat or set standart values
	char* fullpath = new char[strlen(configdir)+16];
	sprintf(fullpath,"%spreferences.dat",configdir);
	FILE* preffile = fopen(fullpath,"rb");
	delete[] fullpath;

	LoadPreferences();

	if (!preffile){
		SetStandartValues();
		//if (Ask4RegFix(true)) Ask4RegFix(false);
	}
	else{
		fread(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (ferror(preffile))
			SetStandartValues();

		// import old pref-files
		if (prefsExt->version<20) {


			if (prefsExt->version>17) {// v0.20b+
				prefsImport20b=new Preferences_Import20b_Struct;
				memset(prefsImport20b,0,sizeof(Preferences_Import20b_Struct));
				fseek(preffile,0,0);
				fread(prefsImport20b,sizeof(Preferences_Import20b_Struct),1,preffile);

				md4cpy(&prefs->userhash,&prefsImport20b->userhash);
				memcpy(&prefs->incomingdir,&prefsImport20b->incomingdir,510);
				memcpy(&prefs->tempdir,&prefsImport20b->tempdir,510);
				sprintf(prefs->nick,prefsImport20b->nick);

				prefs->totalDownloadedBytes=prefsImport20b->totalDownloadedBytes;
				prefs->totalUploadedBytes=prefsImport20b->totalUploadedBytes;

			} else if (prefsExt->version>7) { // v0.20a
				prefsImport20a=new Preferences_Import20a_Struct;
				memset(prefsImport20a,0,sizeof(Preferences_Import20a_Struct));
				fseek(preffile,0,0);
				fread(prefsImport20a,sizeof(Preferences_Import20a_Struct),1,preffile);

				md4cpy(&prefs->userhash,&prefsImport20a->userhash);
				memcpy(&prefs->incomingdir,&prefsImport20a->incomingdir,510);
				memcpy(&prefs->tempdir,&prefsImport20a->tempdir,510);
				sprintf(prefs->nick,prefsImport20a->nick);

				prefs->totalDownloadedBytes=prefsImport20a->totalDownloaded;
				prefs->totalUploadedBytes=prefsImport20a->totalUploaded;

			} else {	//v0.19c-
				prefsImport19c=new Preferences_Import19c_Struct;
				memset(prefsImport19c,0,sizeof(Preferences_Import19c_Struct));

				fseek(preffile,0,0);
				fread(prefsImport19c,sizeof(Preferences_Import19c_Struct),1,preffile);

				if (prefsExt->version<3) {
					CreateUserHash();
					md4cpy(&prefs->userhash,&userhash);
				} else {md4cpy(&prefs->userhash,&prefsImport19c->userhash);}
				memcpy(&prefs->incomingdir,&prefsImport19c->incomingdir,510);memcpy(&prefs->tempdir,&prefsImport19c->tempdir,510);
				sprintf(prefs->nick,prefsImport19c->nick);
			}
 		} else {
			md4cpy(&prefs->userhash,&prefsExt->userhash);
			prefs->EmuleWindowPlacement=prefsExt->EmuleWindowPlacement;
		}
		fclose(preffile);
		md4cpy(&userhash,&prefs->userhash);
		prefs->smartidstate=0;
	}

	// shared directories
	fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%sshareddir.dat",configdir);
	CStdioFile* sdirfile = new CStdioFile();
	if (sdirfile->Open(fullpath,CFile::modeRead)){
		CString toadd;
		while (sdirfile->ReadString(toadd))
		{
			TCHAR szFullPath[MAX_PATH];
			if (PathCanonicalize(szFullPath, toadd))
				toadd = szFullPath;

			if (IsInstallationDirectory(toadd))
				continue;

			if (_taccess(toadd, 0) == 0){ // only add directories which still exist
				if (toadd.Right(1) != _T('\\'))
					toadd.Append(_T("\\"));
				shareddir_list.AddHead(toadd);
			}
		}
		sdirfile->Close();
	}
	delete sdirfile;
	delete[] fullpath;
	
	//serverlist adresses
	fullpath = new char[strlen(configdir)+20];
	sprintf(fullpath,"%sadresses.dat",configdir);
	sdirfile = new CStdioFile();
	if (sdirfile->Open(fullpath,CFile::modeRead)){
		CString toadd;
		while (sdirfile->ReadString(toadd))
			adresses_list.AddHead(toadd);
		sdirfile->Close();
	}
	delete sdirfile;
	delete[] fullpath;	
	fullpath=NULL;

	userhash[5] = 14;
	userhash[14] = 111;

	// deadlake PROXYSUPPORT
	m_UseProxyListenPort = false;
	ListenPort = 0;

	// Explicitly inform the user about errors with incoming/temp folders!
	if (!PathFileExists(GetIncomingDir()) && !::CreateDirectory(GetIncomingDir(),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetIncomingDir(), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
		sprintf(prefs->incomingdir,"%sincoming",appdir);
		if (!PathFileExists(GetIncomingDir()) && !::CreateDirectory(GetIncomingDir(),0)){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetIncomingDir(), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
		sprintf(prefs->tempdir,"%stemp",appdir);
		if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0)){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	// khaos::kmod+ Source Lists directory
	CString sSourceListsPath = CString(GetTempDir()) + "\\Source Lists";
	if (UseSaveLoadSources() && !PathFileExists(sSourceListsPath.GetBuffer()) && !::CreateDirectory(sSourceListsPath.GetBuffer(), 0)) {
		CString strError;
		strError.Format(_T("Failed to create source lists directory \"%s\" - %s"), sSourceListsPath, GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
	}
	// khaos::kmod-

	if (((int*)prefs->userhash[0]) == 0 && ((int*)prefs->userhash[1]) == 0 && ((int*)prefs->userhash[2]) == 0 && ((int*)prefs->userhash[3]) == 0)
		CreateUserHash();
}

void CPreferences::SetStandartValues(){
	CreateUserHash();
	md4cpy(&prefs->userhash,&userhash);

	WINDOWPLACEMENT defaultWPM;
	defaultWPM.length = sizeof(WINDOWPLACEMENT);
	defaultWPM.rcNormalPosition.left=10;defaultWPM.rcNormalPosition.top=10;
	defaultWPM.rcNormalPosition.right=700;defaultWPM.rcNormalPosition.bottom=500;
	defaultWPM.showCmd=0;
	prefs->EmuleWindowPlacement=defaultWPM;
	prefs->versioncheckLastAutomatic=0;

//	Save();
}

bool CPreferences::IsTempFile(const CString& rstrDirectory, const CString& rstrName) const
{
	if (CompareDirectories(rstrDirectory, GetTempDir()))
		return false;

	// do not share a file from the temp directory, if it matches one of the following patterns
	CString strNameLower(rstrName);
	strNameLower.MakeLower();
	strNameLower += _T("|"); // append an EOS character which we can query for
	static const LPCTSTR _apszNotSharedExts[] = {
		_T("%u.part") _T("%c"), 
		_T("%u.part.met") _T("%c"), 
		_T("%u.part.met") PARTMET_BAK_EXT _T("%c"), 
		_T("%u.part.met") PARTMET_TMP_EXT _T("%c") 
	};
	for (int i = 0; i < ARRSIZE(_apszNotSharedExts); i++){
		UINT uNum;
		TCHAR iChar;
		// "misuse" the 'scanf' function for a very simple pattern scanning.
		if (_stscanf(strNameLower, _apszNotSharedExts[i], &uNum, &iChar) == 2 && iChar == _T('|'))
			return true;
	}

	return false;
}

// SLUGFILLER: SafeHash
bool CPreferences::IsConfigFile(const CString& rstrDirectory, const CString& rstrName) const
{
	if (CompareDirectories(rstrDirectory, configdir))
		return false;

	// do not share a file from the config directory, if it contains one of the following extensions
	static const LPCTSTR _apszNotSharedExts[] = { _T(".met.bak"), _T(".ini.old") };
	for (int i = 0; i < ARRSIZE(_apszNotSharedExts); i++){
		int iLen = _tcslen(_apszNotSharedExts[i]);
		if (rstrName.GetLength()>=iLen && rstrName.Right(iLen).CompareNoCase(_apszNotSharedExts[i])==0)
			return true;
	}

	// do not share following files from the config directory
	static const LPCTSTR _apszNotSharedFiles[] = 
	{
		_T("AC_SearchStrings.dat"),
		_T("AC_ServerMetURLs.dat"),
		_T("adresses.dat"),
		_T("category.ini"),
		_T("clients.met"),
		_T("cryptkey.dat"),
		_T("emfriends.met"),
		_T("fileinfo.ini"),
		_T("ipfilter.dat"),
		_T("known.met"),
		_T("preferences.dat"),
		_T("preferences.ini"),
		_T("server.met"),
		_T("server.met.new"),
		_T("server_met.download"),
		_T("server_met.old"),
		_T("shareddir.dat"),
		_T("sharedsubdir.dat"),
		_T("staticservers.dat"),
		_T("webservices.dat")
	};
	for (int i = 0; i < ARRSIZE(_apszNotSharedFiles); i++){
		if (rstrName.CompareNoCase(_apszNotSharedFiles[i])==0)
			return true;
	}

	return false;
}

// SLUGFILLER: SafeHash
//MORPH - Added by SiRoB, ZZ Ratio
bool CPreferences::IsZZRatioDoesWork(){
	if (theApp.stat_sessionSentBytesToFriend)
		return true;
	if (theApp.glob_prefs->IsSUCEnabled())
		return theApp.uploadqueue->GetMaxVUR()<10240;
	else if (theApp.glob_prefs->IsDynUpEnabled())
		return theApp.lastCommonRouteFinder->GetUpload()<10240;
	else
		return theApp.glob_prefs->GetMaxUpload()<10;
}
//MORPH - Added by SiRoB, ZZ ratio

uint16 CPreferences::GetMaxDownload(){
	//dont be a Lam3r :)
	//MORPH START - Added by SiRoB, ZZ Upload system
	if (IsZZRatioDoesWork())
		return prefs->maxdownload;
	//MORPH END   - Added by SiRoB, ZZ Upload system
	uint16 maxup=(GetMaxUpload()==UNLIMITED)?GetMaxGraphUploadRate():GetMaxUpload();
	if( maxup < 4 )
		return (( (maxup < 10) && (maxup*3 < prefs->maxdownload) )? maxup*3 : prefs->maxdownload);
	return (( (maxup < 10) && (maxup*4 < prefs->maxdownload) )? maxup*4 : prefs->maxdownload);
}

// -khaos--+++> A whole bunch of methods!  Keep going until you reach the end tag.
void CPreferences::SaveStats(int bBackUp){
	// This function saves all of the new statistics in my addon.  It is also used to
	// save backups for the Reset Stats function, and the Restore Stats function (Which is actually LoadStats)
	// bBackUp = 0: DEFAULT; save to preferences.ini
	// bBackUp = 1: Save to statbkup.ini, which is used to restore after a reset
	// bBackUp = 2: Save to statbkuptmp.ini, which is temporarily created during a restore and then renamed to statbkup.ini

	CString buffer;
	char* fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	if (bBackUp == 1) sprintf(fullpath,"%sstatbkup.ini",configdir);
	else if (bBackUp == 2) sprintf(fullpath,"%sstatbkuptmp.ini",configdir);
	else sprintf(fullpath,"%spreferences.ini",configdir);
	
	CIni ini(fullpath, "Statistics");

	delete[] fullpath;

	// Save cumulative statistics to preferences.ini, going in order as they appear in CStatisticsDlg::ShowStatistics.
	// We do NOT SET the values in prefs struct here.

    // Save Cum Down Data
	buffer.Format("%I64u",theApp.stat_sessionReceivedBytes+GetTotalDownloaded());
	ini.WriteString("TotalDownloadedBytes", buffer );
    // Save Complete Downloads - This is saved and incremented in partfile.cpp.
	// Save Successful Download Sessions
	ini.WriteInt("DownSuccessfulSessions", prefs->cumDownSuccessfulSessions);
	// Save Failed Download Sessions
	ini.WriteInt("DownFailedSessions", prefs->cumDownFailedSessions);
	ini.WriteInt("DownAvgTime", (GetDownC_AvgTime() + GetDownS_AvgTime()) / 2 );

	// Cumulative statistics for saved due to compression/lost due to corruption
	buffer.Format("%I64u",prefs->cumLostFromCorruption+prefs->sesLostFromCorruption);
	ini.WriteString("LostFromCorruption", buffer );
	buffer.Format("%I64u",prefs->sesSavedFromCompression+prefs->cumSavedFromCompression);
	ini.WriteString("SavedFromCompression", buffer );
	ini.WriteInt("PartsSavedByICH", prefs->cumPartsSavedByICH+prefs->sesPartsSavedByICH);

	// Save cumulative client breakdown stats for received bytes...
	buffer.Format("%I64u", GetCumDownData_EDONKEY() );
	ini.WriteString("DownData_EDONKEY", buffer );
	buffer.Format("%I64u", GetCumDownData_EDONKEYHYBRID() );
	ini.WriteString("DownData_EDONKEYHYBRID", buffer );
	buffer.Format("%I64u", GetCumDownData_EMULE() );
	ini.WriteString("DownData_EMULE", buffer );
	buffer.Format("%I64u", GetCumDownData_MLDONKEY() );
	ini.WriteString("DownData_MLDONKEY", buffer );
	buffer.Format("%I64u", GetCumDownData_XMULE() );
	ini.WriteString("DownData_LMULE", buffer );
	buffer.Format("%I64u", GetCumDownData_CDONKEY() );
	ini.WriteString("DownData_CDONKEY", buffer );
	buffer.Format("%I64u", GetCumDownData_SHAREAZA() );
	ini.WriteString("DownData_SHAREAZA", buffer );

	// Save cumulative port breakdown stats for received bytes
	buffer.Format("%I64u", GetCumDownDataPort_4662() );
	ini.WriteString("DownDataPort_4662", buffer );
	buffer.Format("%I64u", GetCumDownDataPort_OTHER() );
	ini.WriteString("DownDataPort_OTHER", buffer );

	// Save Cumulative Downline Overhead Statistics
	buffer.Format("%I64u",(uint64)theApp.downloadqueue->GetDownDataOverheadFileRequest()+theApp.downloadqueue->GetDownDataOverheadSourceExchange()+theApp.downloadqueue->GetDownDataOverheadServer()+theApp.downloadqueue->GetDownDataOverheadOther()+GetDownOverheadTotal());
	ini.WriteString("DownOverheadTotal", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadFileRequest() + GetDownOverheadFileReq());
	ini.WriteString("DownOverheadFileReq", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadSourceExchange() + GetDownOverheadSrcEx());
	ini.WriteString("DownOverheadSrcEx", buffer );
	buffer.Format("%I64u",(uint64) theApp.downloadqueue->GetDownDataOverheadServer()+GetDownOverheadServer());
	ini.WriteString("DownOverheadServer", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets() + theApp.downloadqueue->GetDownDataOverheadServerPackets() + theApp.downloadqueue->GetDownDataOverheadOtherPackets() + GetDownOverheadTotalPackets());
	ini.WriteString("DownOverheadTotalPackets", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + GetDownOverheadFileReqPackets());
	ini.WriteString("DownOverheadFileReqPackets", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets() + GetDownOverheadSrcExPackets());
	ini.WriteString("DownOverheadSrcExPackets", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadServerPackets() + GetDownOverheadServerPackets());
	ini.WriteString("DownOverheadServerPackets", buffer );

	// Save Cumulative Upline Statistics
	buffer.Format("%I64u",theApp.stat_sessionSentBytes+GetTotalUploaded());
	ini.WriteString("TotalUploadedBytes", buffer );
	ini.WriteInt("UpSuccessfulSessions", theApp.uploadqueue->GetSuccessfullUpCount()+GetUpSuccessfulSessions());
	ini.WriteInt("UpFailedSessions", theApp.uploadqueue->GetFailedUpCount()+GetUpFailedSessions());
	ini.WriteInt("UpAvgTime", (theApp.uploadqueue->GetAverageUpTime()+GetUpAvgTime())/2);

	// Save Cumulative Client Breakdown Stats For Sent Bytes
	buffer.Format("%I64u", GetCumUpData_EDONKEY() );
	ini.WriteString("UpData_EDONKEY", buffer );
	buffer.Format("%I64u", GetCumUpData_EDONKEYHYBRID() );
	ini.WriteString("UpData_EDONKEYHYBRID", buffer );
	buffer.Format("%I64u", GetCumUpData_EMULE() );
	ini.WriteString("UpData_EMULE", buffer );
	buffer.Format("%I64u", GetCumUpData_MLDONKEY() );
	ini.WriteString("UpData_MLDONKEY", buffer );
	buffer.Format("%I64u", GetCumUpData_XMULE() );
	ini.WriteString("UpData_LMULE", buffer );
	buffer.Format("%I64u", GetCumUpData_CDONKEY() );
	ini.WriteString("UpData_CDONKEY", buffer );
	buffer.Format("%I64u", GetCumUpData_SHAREAZA() );
	ini.WriteString("UpData_SHAREAZA", buffer );

	// Save cumulative port breakdown stats for sent bytes
	buffer.Format("%I64u", GetCumUpDataPort_4662() );
	ini.WriteString("UpDataPort_4662", buffer );
	buffer.Format("%I64u", GetCumUpDataPort_OTHER() );
	ini.WriteString("UpDataPort_OTHER", buffer );

	// Save cumulative source breakdown stats for sent bytes
	buffer.Format("%I64u", GetCumUpData_File() );
	ini.WriteString("UpData_File", buffer );
	buffer.Format("%I64u", GetCumUpData_Partfile() );
	ini.WriteString("UpData_Partfile", buffer );

	// Save Cumulative Upline Overhead Statistics
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadFileRequest() + theApp.uploadqueue->GetUpDataOverheadSourceExchange() + theApp.uploadqueue->GetUpDataOverheadServer() + theApp.uploadqueue->GetUpDataOverheadOther() + GetUpOverheadTotal());
	ini.WriteString("UpOverheadTotal", buffer);
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadFileRequest() + GetUpOverheadFileReq());
	ini.WriteString("UpOverheadFileReq", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadSourceExchange() + GetUpOverheadSrcEx());
	ini.WriteString("UpOverheadSrcEx", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadServer() + GetUpOverheadServer());
	ini.WriteString("UpOverheadServer", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets() + theApp.uploadqueue->GetUpDataOverheadServerPackets() + theApp.uploadqueue->GetUpDataOverheadOtherPackets() + GetUpOverheadTotalPackets());
	ini.WriteString("UpOverheadTotalPackets", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + GetUpOverheadFileReqPackets());
	ini.WriteString("UpOverheadFileReqPackets", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets() + GetUpOverheadSrcExPackets());
	ini.WriteString("UpOverheadSrcExPackets", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadServerPackets() + GetUpOverheadServerPackets());
	ini.WriteString("UpOverheadServerPackets", buffer );

	// Save Cumulative Connection Statistics
	float tempRate = 0;
	// Download Rate Average
	tempRate = theApp.emuledlg->statisticswnd.GetAvgDownloadRate(2);
	ini.WriteFloat("ConnAvgDownRate", tempRate);
	// Max Download Rate Average
	if (tempRate>GetConnMaxAvgDownRate()) Add2ConnMaxAvgDownRate(tempRate);
	ini.WriteFloat("ConnMaxAvgDownRate", GetConnMaxAvgDownRate());
	// Max Download Rate
	tempRate = (float) theApp.downloadqueue->GetDatarate() / 1024;
	if (tempRate>GetConnMaxDownRate()) Add2ConnMaxDownRate(tempRate);
	ini.WriteFloat("ConnMaxDownRate", GetConnMaxDownRate());
	// Upload Rate Average
	tempRate = theApp.emuledlg->statisticswnd.GetAvgUploadRate(2);
	ini.WriteFloat("ConnAvgUpRate", tempRate);
	// Max Upload Rate Average
	if (tempRate>GetConnMaxAvgUpRate()) Add2ConnMaxAvgUpRate(tempRate);
	ini.WriteFloat("ConnMaxAvgUpRate", GetConnMaxAvgUpRate());
	// Max Upload Rate
	tempRate = (float) theApp.uploadqueue->GetDatarate() / 1024;
	if (tempRate>GetConnMaxUpRate()) Add2ConnMaxUpRate(tempRate);
	ini.WriteFloat("ConnMaxUpRate", GetConnMaxUpRate());
	// Overall Run Time
	uint32 timeseconds = (GetTickCount()-theApp.stat_starttime)/1000;
	timeseconds+=GetConnRunTime();
	ini.WriteInt("ConnRunTime",timeseconds );
	// Number of Reconnects
	if (theApp.stat_reconnects>0) buffer.Format("%u",theApp.stat_reconnects - 1 + GetConnNumReconnects());
	else buffer.Format("%u",GetConnNumReconnects());
	ini.WriteString("ConnNumReconnects", buffer);
	// Average Connections
	if (theApp.serverconnect->IsConnected()){
		buffer.Format("%u",(uint32)(theApp.emuledlg->statisticswnd.GetAverageConnections()+prefs->cumConnAvgConnections)/2);
		ini.WriteString("ConnAvgConnections", buffer);
	}
	// Peak Connections
	if (theApp.emuledlg->statisticswnd.GetPeakConnections()>prefs->cumConnPeakConnections)
		prefs->cumConnPeakConnections = theApp.emuledlg->statisticswnd.GetPeakConnections();
	ini.WriteInt("ConnPeakConnections", prefs->cumConnPeakConnections);
	// Max Connection Limit Reached
	buffer.Format("%u",theApp.listensocket->GetMaxConnectionReached()+prefs->cumConnMaxConnLimitReached);
	if (atoi(buffer)>prefs->cumConnMaxConnLimitReached) ini.WriteString("ConnMaxConnLimitReached", buffer);
	// Time Stuff...
	ini.WriteInt("ConnTransferTime", GetConnTransferTime() + theApp.emuledlg->statisticswnd.GetTransferTime());
	ini.WriteInt("ConnUploadTime", GetConnUploadTime() + theApp.emuledlg->statisticswnd.GetUploadTime());
	ini.WriteInt("ConnDownloadTime", GetConnDownloadTime() + theApp.emuledlg->statisticswnd.GetDownloadTime());
	ini.WriteInt("ConnServerDuration", GetConnServerDuration() + theApp.emuledlg->statisticswnd.GetServerDuration());
	
	// Compare and Save Server Records
	uint32 servtotal, servfail, servuser, servfile, servtuser, servtfile; float servocc;
	theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servtuser, servtfile, servocc );
	
	if ((servtotal-servfail)>prefs->cumSrvrsMostWorkingServers)	prefs->cumSrvrsMostWorkingServers = servtotal-servfail;
	ini.WriteInt("SrvrsMostWorkingServers", prefs->cumSrvrsMostWorkingServers);
	if (servtuser>prefs->cumSrvrsMostUsersOnline) prefs->cumSrvrsMostUsersOnline = servtuser;
	ini.WriteInt("SrvrsMostUsersOnline", prefs->cumSrvrsMostUsersOnline);
	if (servtfile>prefs->cumSrvrsMostFilesAvail) prefs->cumSrvrsMostFilesAvail = servtfile;
	ini.WriteInt("SrvrsMostFilesAvail", prefs->cumSrvrsMostFilesAvail);

	// Compare and Save Shared File Records
	if (theApp.sharedfiles->GetCount()>prefs->cumSharedMostFilesShared)	prefs->cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
	ini.WriteInt("SharedMostFilesShared", prefs->cumSharedMostFilesShared);
	uint64 bytesLargestFile = 0;
	uint64 allsize=theApp.sharedfiles->GetDatasize(bytesLargestFile);
	if (allsize>prefs->cumSharedLargestShareSize) prefs->cumSharedLargestShareSize = allsize;
	buffer.Format("%I64u", prefs->cumSharedLargestShareSize);
	ini.WriteString("SharedLargestShareSize", buffer);
	if (bytesLargestFile>prefs->cumSharedLargestFileSize) prefs->cumSharedLargestFileSize = bytesLargestFile;
	buffer.Format("%I64u", prefs->cumSharedLargestFileSize);
	ini.WriteString("SharedLargestFileSize", buffer);
	if (theApp.sharedfiles->GetCount() != 0) {
		uint64 tempint = allsize/theApp.sharedfiles->GetCount();
		if (tempint>prefs->cumSharedLargestAvgFileSize)	prefs->cumSharedLargestAvgFileSize = tempint;
	}
	buffer.Format("%I64u",prefs->cumSharedLargestAvgFileSize);
	ini.WriteString("SharedLargestAvgFileSize", buffer);

	buffer.Format("%I64u",prefs->stat_datetimeLastReset);
	ini.WriteString("statsDateTimeLastReset", buffer);

	// If we are saving a back-up or a temporary back-up, return now.
	if (bBackUp != 0) return;

	// These aren't really statistics, but they're a part of my add-on, so we'll save them here and load them in LoadStats
	ini.WriteInt("statsConnectionsGraphRatio", prefs->statsConnectionsGraphRatio, "Statistics");
	ini.WriteString("statsExpandedTreeItems", prefs->statsExpandedTreeItems, "Statistics");

	// End SaveStats
}

void CPreferences::SetRecordStructMembers() {

	// The purpose of this function is to be called from CStatisticsDlg::ShowStatistics()
	// This was easier than making a bunch of functions to interface with the record
	// members of the prefs struct from ShowStatistics.

	// This function is going to compare current values with previously saved records, and if
	// the current values are greater, the corresponding member of prefs will be updated.
	// We will not write to INI here, because this code is going to be called a lot more often
	// than SaveStats()  - Khaos

	CString buffer;

	// Servers
	uint32 servtotal, servfail, servuser, servfile, servtuser, servtfile; float servocc;
	theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servtuser, servtfile, servocc );
	if ((servtotal-servfail)>prefs->cumSrvrsMostWorkingServers) prefs->cumSrvrsMostWorkingServers = (servtotal-servfail);
	if (servtuser>prefs->cumSrvrsMostUsersOnline) prefs->cumSrvrsMostUsersOnline = servtuser;
	if (servtfile>prefs->cumSrvrsMostFilesAvail) prefs->cumSrvrsMostFilesAvail = servtfile;

	// Shared Files
	if (theApp.sharedfiles->GetCount()>prefs->cumSharedMostFilesShared) prefs->cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
	uint64 bytesLargestFile = 0;
	uint64 allsize=theApp.sharedfiles->GetDatasize(bytesLargestFile);
	if (allsize>prefs->cumSharedLargestShareSize) prefs->cumSharedLargestShareSize = allsize;
	if (bytesLargestFile>prefs->cumSharedLargestFileSize) prefs->cumSharedLargestFileSize = bytesLargestFile;
	if (theApp.sharedfiles->GetCount() != 0) {
		uint64 tempint = allsize/theApp.sharedfiles->GetCount();
		if (tempint>prefs->cumSharedLargestAvgFileSize) prefs->cumSharedLargestAvgFileSize = tempint;
	}
} // SetRecordStructMembers()

void CPreferences::SaveCompletedDownloadsStat(){

	// This function saves the values for the completed
	// download members to INI.  It is called from
	// CPartfile::PerformFileComplete ...   - Khaos

	char* fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%spreferences.ini",configdir);
	
	CIni ini( fullpath, "eMule" );

	delete[] fullpath;

	ini.WriteInt("DownCompletedFiles",			GetDownCompletedFiles(),		"Statistics");
	ini.WriteInt("DownSessionCompletedFiles",	GetDownSessionCompletedFiles(),	"Statistics");
} // SaveCompletedDownloadsStat()

void CPreferences::Add2SessionTransferData(uint8 uClientID, uint16 uClientPort, BOOL bFromPF, BOOL bUpDown, uint32 bytes, bool sentToFriend = false){ //MORPH - Added by Yun.SF3, ZZ Upload System


	//	This function adds the transferred bytes to the appropriate variables,
	//	as well as to the totals for all clients. - Khaos
	//	PARAMETERS:
	//	uClientID - The identifier for which client software sent or received this data, eg SO_EMULE
	//	uClientPort - The remote port of the client that sent or received this data, eg 4662
	//	bFromPF - Applies only to uploads.  True is from partfile, False is from non-partfile.
	//	bUpDown - True is Up, False is Down
	//	bytes - Number of bytes sent by the client.  Subtract header before calling.

	switch (bUpDown){
		case true:
			//	Upline Data
			
			switch (uClientID){
				// Update session client breakdown stats for sent bytes...
				case SO_EDONKEY:		prefs->sesUpData_EDONKEY+=bytes;		break;
				case SO_EDONKEYHYBRID:	prefs->sesUpData_EDONKEYHYBRID+=bytes;	break;
				case SO_OLDEMULE:
				case SO_EMULE:			prefs->sesUpData_EMULE+=bytes;			break;
				case SO_MLDONKEY:		prefs->sesUpData_MLDONKEY+=bytes;		break;
				case SO_CDONKEY:		prefs->sesUpData_CDONKEY+=bytes;		break;
				case SO_XMULE:			prefs->sesUpData_XMULE+=bytes;			break;
				case SO_SHAREAZA:		prefs->sesUpData_SHAREAZA+=bytes;		break;
			}

			switch (uClientPort){
				// Update session port breakdown stats for sent bytes...
				case 4662:				prefs->sesUpDataPort_4662+=bytes;		break;
				default:				prefs->sesUpDataPort_OTHER+=bytes;		break;
			}

			if (bFromPF)				prefs->sesUpData_Partfile+=bytes;
			else						prefs->sesUpData_File+=bytes;

			//	Add to our total for sent bytes...
			theApp.UpdateSentBytes(bytes, sentToFriend); //MORPH - Added by Yun.SF3, ZZ Upload System


			break;

		case false:
			// Downline Data

			switch (uClientID){
                // Update session client breakdown stats for received bytes...
				case SO_EDONKEY:		prefs->sesDownData_EDONKEY+=bytes;		break;
				case SO_EDONKEYHYBRID:	prefs->sesDownData_EDONKEYHYBRID+=bytes;break;
				case SO_OLDEMULE:
				case SO_EMULE:			prefs->sesDownData_EMULE+=bytes;		break;
				case SO_MLDONKEY:		prefs->sesDownData_MLDONKEY+=bytes;		break;
				case SO_CDONKEY:		prefs->sesDownData_CDONKEY+=bytes;		break;
				case SO_XMULE:			prefs->sesDownData_XMULE+=bytes;		break;
				case SO_SHAREAZA:		prefs->sesDownData_SHAREAZA+=bytes;		break;
			}

			switch (uClientPort){
				// Update session port breakdown stats for received bytes...
				// For now we are only going to break it down by default and non-default.
				// A statistical analysis of all data sent from every single port/domain is
				// beyond the scope of this add-on.
				case 4662:				prefs->sesDownDataPort_4662+=bytes;		break;
				default:				prefs->sesDownDataPort_OTHER+=bytes;	break;
			}

			//	Add to our total for received bytes...
			theApp.UpdateReceivedBytes(bytes);

	}

}

// Reset Statistics by Khaos

void CPreferences::ResetCumulativeStatistics(){

	// Save a backup so that we can undo this action
	SaveStats(1);

	// SET ALL CUMULATIVE STAT VALUES TO 0  :'-(

	prefs->totalDownloadedBytes=0;
	prefs->totalUploadedBytes=0;
	prefs->cumDownOverheadTotal=0;
	prefs->cumDownOverheadFileReq=0;
	prefs->cumDownOverheadSrcEx=0;
	prefs->cumDownOverheadServer=0;
	prefs->cumDownOverheadTotalPackets=0;
	prefs->cumDownOverheadFileReqPackets=0;
	prefs->cumDownOverheadSrcExPackets=0;
	prefs->cumDownOverheadServerPackets=0;
	prefs->cumUpOverheadTotal=0;
	prefs->cumUpOverheadFileReq=0;
	prefs->cumUpOverheadSrcEx=0;
	prefs->cumUpOverheadServer=0;
	prefs->cumUpOverheadTotalPackets=0;
	prefs->cumUpOverheadFileReqPackets=0;
	prefs->cumUpOverheadSrcExPackets=0;
	prefs->cumUpOverheadServerPackets=0;
	prefs->cumUpSuccessfulSessions=0;
	prefs->cumUpFailedSessions=0;
	prefs->cumUpAvgTime=0;
	prefs->cumUpData_EDONKEY=0;
	prefs->cumUpData_EDONKEYHYBRID=0;
	prefs->cumUpData_EMULE=0;
	prefs->cumUpData_MLDONKEY=0;
	prefs->cumUpData_CDONKEY=0;
	prefs->cumUpData_XMULE=0;
	prefs->cumUpData_SHAREAZA=0;
	prefs->cumUpDataPort_4662=0;
	prefs->cumUpDataPort_OTHER=0;
	prefs->cumDownCompletedFiles=0;
	prefs->cumDownSuccessfulSessions=0;
	prefs->cumDownFailedSessions=0;
	prefs->cumDownAvgTime=0;
	prefs->cumLostFromCorruption=0;
	prefs->cumSavedFromCompression=0;
	prefs->cumPartsSavedByICH=0;
	prefs->cumDownData_EDONKEY=0;
	prefs->cumDownData_EDONKEYHYBRID=0;
	prefs->cumDownData_EMULE=0;
	prefs->cumDownData_MLDONKEY=0;
	prefs->cumDownData_CDONKEY=0;
	prefs->cumDownData_XMULE=0;
	prefs->cumDownData_SHAREAZA=0;
	prefs->cumDownDataPort_4662=0;
	prefs->cumDownDataPort_OTHER=0;
	prefs->cumConnAvgDownRate=0;
	prefs->cumConnMaxAvgDownRate=0;
	prefs->cumConnMaxDownRate=0;
	prefs->cumConnAvgUpRate=0;
	prefs->cumConnRunTime=0;
	prefs->cumConnNumReconnects=0;
	prefs->cumConnAvgConnections=0;
	prefs->cumConnMaxConnLimitReached=0;
	prefs->cumConnPeakConnections=0;
	prefs->cumConnDownloadTime=0;
	prefs->cumConnUploadTime=0;
	prefs->cumConnTransferTime=0;
	prefs->cumConnServerDuration=0;
	prefs->cumConnMaxAvgUpRate=0;
	prefs->cumConnMaxUpRate=0;
	prefs->cumSrvrsMostWorkingServers=0;
	prefs->cumSrvrsMostUsersOnline=0;
	prefs->cumSrvrsMostFilesAvail=0;
    prefs->cumSharedMostFilesShared=0;
	prefs->cumSharedLargestShareSize=0;
	prefs->cumSharedLargestAvgFileSize=0;

	// Set the time of last reset...
	time_t	timeNow;time(&timeNow);prefs->stat_datetimeLastReset = (__int64) timeNow;

	// Save the reset stats
	SaveStats();
	theApp.emuledlg->statisticswnd.ShowStatistics(true);

	// End Reset Statistics

}


// Load Statistics
// This used to be integrated in LoadPreferences, but it has been altered
// so that it can be used to load the backup created when the stats are reset.
// Last Modified: 2-22-03 by Khaos

bool CPreferences::LoadStats(int loadBackUp){
	// loadBackUp is 0 by default
	// loadBackUp = 0: Load the stats normally like we used to do in LoadPreferences
	// loadBackUp = 1: Load the stats from statbkup.ini and create a backup of the current stats.  Also, do not initialize session variables.
	// loadBackUp = 2: Load the stats from preferences.ini.old because the version has changed.
	char buffer[200];
	CString sINI;
	//uint64 temp64; moved
	CFileFind findBackUp;

	switch (loadBackUp) {
		case 0:
			sINI.Format("%spreferences.ini", configdir);
			break;
		case 1:
			sINI.Format("%sstatbkup.ini", configdir);
			if (!findBackUp.FindFile(sINI))
				return false;

			SaveStats(2); // Save our temp backup of current values to statbkuptmp.ini, we will be renaming it at the end of this function.
			break;
		case 2:
			sINI.Format("%spreferences.ini.old",configdir);
			break;
	}

	bool fileex=PathFileExists(sINI);
	CIni ini(sINI, "Statistics");

	sprintf(buffer , ini.GetString(			"TotalDownloadedBytes"			, 0 ) );
	prefs->totalDownloadedBytes=			_atoi64( buffer );

	sprintf(buffer , ini.GetString(			"TotalUploadedBytes"			, 0 ) );
	prefs->totalUploadedBytes=				_atoi64( buffer );

	// Load stats for cumulative downline overhead
	sprintf(buffer,ini.GetString(			"DownOverheadTotal"				, 0	) );
	prefs->cumDownOverheadTotal=			_atoi64( buffer );
	sprintf(buffer,ini.GetString(			"DownOverheadFileReq"			, 0	) );
	prefs->cumDownOverheadFileReq=			_atoi64( buffer );
	sprintf(buffer,ini.GetString(			"DownOverheadSrcEx"				, 0	) );
	prefs->cumDownOverheadSrcEx=			_atoi64( buffer );
	sprintf(buffer,ini.GetString(			"DownOverheadServer"			, 0	) );
	prefs->cumDownOverheadServer=			_atoi64( buffer );
	sprintf(buffer,ini.GetString(			"DownOverheadTotalPackets"		, 0 ) );
	prefs->cumDownOverheadTotalPackets=		_atoi64( buffer );
	sprintf(buffer,ini.GetString(			"DownOverheadFileReqPackets"	, 0 ) );
	prefs->cumDownOverheadFileReqPackets=	_atoi64( buffer );
	sprintf(buffer,ini.GetString(			"DownOverheadSrcExPackets"		, 0 ) );
	prefs->cumDownOverheadSrcExPackets=		_atoi64( buffer );
	sprintf(buffer,ini.GetString(			"DownOverheadServerPackets"		, 0 ) );
	prefs->cumDownOverheadServerPackets=	_atoi64( buffer );

	// Load stats for cumulative upline overhead
	sprintf(buffer , ini.GetString(			"UpOverHeadTotal"				, 0 ) );
	prefs->cumUpOverheadTotal=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpOverheadFileReq"				, 0 ) );
	prefs->cumUpOverheadFileReq=			_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpOverheadSrcEx"				, 0 ) );
	prefs->cumUpOverheadSrcEx=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpOverheadServer"				, 0 ) );
	prefs->cumUpOverheadServer=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpOverHeadTotalPackets"		, 0 ) );
	prefs->cumUpOverheadTotalPackets=		_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpOverheadFileReqPackets"		, 0 ) );
	prefs->cumUpOverheadFileReqPackets=		_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpOverheadSrcExPackets"		, 0 ) );
	prefs->cumUpOverheadSrcExPackets=		_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpOverheadServerPackets"		, 0 ) );
	prefs->cumUpOverheadServerPackets=		_atoi64( buffer );

	// Load stats for cumulative upline data
	prefs->cumUpSuccessfulSessions =	ini.GetInt("UpSuccessfulSessions"	, 0 );
	prefs->cumUpFailedSessions =		ini.GetInt("UpFailedSessions"		, 0 );
	prefs->cumUpAvgTime =				ini.GetInt("UpAvgTime"				, 0 );

	// Load cumulative client breakdown stats for sent bytes
	sprintf(buffer , ini.GetString(			"UpData_EDONKEY"				, 0 ) );
	prefs->cumUpData_EDONKEY=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpData_EDONKEYHYBRID"			, 0 ) );
	prefs->cumUpData_EDONKEYHYBRID=			_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpData_EMULE"					, 0 ) );
	prefs->cumUpData_EMULE=					_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpData_MLDONKEY"				, 0 ) );
	prefs->cumUpData_MLDONKEY=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpData_LMULE"					, 0 ) );
	prefs->cumUpData_XMULE=					_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpData_CDONKEY"				, 0 ) );
	prefs->cumUpData_CDONKEY=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpData_SHAREAZA"				, 0 ) );
	prefs->cumUpData_SHAREAZA=				_atoi64( buffer );

	// Load cumulative port breakdown stats for sent bytes
	sprintf(buffer , ini.GetString(			"UpDataPort_4662"				, 0 ) );
	prefs->cumUpDataPort_4662=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpDataPort_OTHER"				, 0 ) );
	prefs->cumUpDataPort_OTHER=				_atoi64( buffer );

	// Load cumulative source breakdown stats for sent bytes
	sprintf(buffer , ini.GetString(			"UpData_File"					, 0 ) );
	prefs->cumUpData_File=					_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"UpData_Partfile"				, 0 ) );
	prefs->cumUpData_Partfile=				_atoi64( buffer );

	// Load stats for cumulative downline data
	prefs->cumDownCompletedFiles =		ini.GetInt("DownCompletedFiles"		, 0 );
	prefs->cumDownSuccessfulSessions=	ini.GetInt("DownSuccessfulSessions"	, 0 );
	prefs->cumDownFailedSessions=		ini.GetInt("DownFailedSessions"		, 0 );
	prefs->cumDownAvgTime=				ini.GetInt("DownAvgTime"			, 0 );

	// Cumulative statistics for saved due to compression/lost due to corruption
	sprintf(buffer , ini.GetString(			"LostFromCorruption"			, 0 ) );
	prefs->cumLostFromCorruption=			_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"SavedFromCompression"			, 0 ) );
	prefs->cumSavedFromCompression=			_atoi64( buffer );
	prefs->cumPartsSavedByICH=				ini.GetInt("PartsSavedByICH"		, 0 );

	// Load cumulative client breakdown stats for received bytes
	sprintf(buffer , ini.GetString(			"DownData_EDONKEY"				, 0 ) );
	prefs->cumDownData_EDONKEY=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"DownData_EDONKEYHYBRID"		, 0 ) );
	prefs->cumDownData_EDONKEYHYBRID=		_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"DownData_EMULE"				, 0 ) );
	prefs->cumDownData_EMULE=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"DownData_MLDONKEY"				, 0 ) );
	prefs->cumDownData_MLDONKEY=			_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"DownData_LMULE"				, 0 ) );
	prefs->cumDownData_XMULE=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"DownData_CDONKEY"				, 0 ) );
	prefs->cumDownData_CDONKEY=				_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"DownData_SHAREAZA"				, 0 ) );
	prefs->cumDownData_SHAREAZA=				_atoi64( buffer );

	// Load cumulative port breakdown stats for received bytes
	sprintf(buffer , ini.GetString(			"DownDataPort_4662"				, 0 ) );
	prefs->cumDownDataPort_4662=			_atoi64( buffer );
	sprintf(buffer , ini.GetString(			"DownDataPort_OTHER"			, 0 ) );
	prefs->cumDownDataPort_OTHER=			_atoi64( buffer );

	// Load stats for cumulative connection data
	prefs->cumConnAvgDownRate =		ini.GetFloat(	"ConnAvgDownRate"		, 0 );
	prefs->cumConnMaxAvgDownRate =	ini.GetFloat(	"ConnMaxAvgDownRate"	, 0 );
	prefs->cumConnMaxDownRate =		ini.GetFloat(	"ConnMaxDownRate"		, 0 );
	prefs->cumConnAvgUpRate =		ini.GetFloat(	"ConnAvgUpRate"			, 0 );
	prefs->cumConnMaxAvgUpRate =	ini.GetFloat(	"ConnMaxAvgUpRate"		, 0 );
	prefs->cumConnMaxUpRate =		ini.GetFloat(	"ConnMaxUpRate"			, 0 );

	sprintf(buffer , ini.GetString(			"ConnRunTime"					, 0 ) );
	prefs->cumConnRunTime=					_atoi64(buffer);

	prefs->cumConnTransferTime=			ini.GetInt(	"ConnTransferTime"			, 0 );
	prefs->cumConnDownloadTime=			ini.GetInt(	"ConnDownloadTime"			, 0 );
	prefs->cumConnUploadTime=			ini.GetInt(	"ConnUploadTime"			, 0 );
	prefs->cumConnServerDuration=		ini.GetInt( "ConnServerDuration"		, 0 );
	prefs->cumConnNumReconnects =		ini.GetInt(	"ConnNumReconnects"			, 0 );
	prefs->cumConnAvgConnections =		ini.GetInt(	"ConnAvgConnections"		, 0 );
	prefs->cumConnMaxConnLimitReached=	ini.GetInt(	"ConnMaxConnLimitReached"	, 0 );
	prefs->cumConnPeakConnections =		ini.GetInt(	"ConnPeakConnections"		, 0 );

	// Load date/time of last reset
	sprintf(buffer , ini.GetString(			"statsDateTimeLastReset"		, 0 ) );
	prefs->stat_datetimeLastReset=			_atoi64( buffer );

	// Smart Load For Restores - Don't overwrite records that are greater than the backed up ones
	if (loadBackUp == 1) {
		uint64 temp64;
		// Load records for servers / network
		if (ini.GetInt("SrvrsMostWorkingServers", 0) > prefs->cumSrvrsMostWorkingServers)
			prefs->cumSrvrsMostWorkingServers = ini.GetInt(		"SrvrsMostWorkingServers"	, 0 );
		if (ini.GetInt("SrvrsMostUsersOnline", 0) > (int)prefs->cumSrvrsMostUsersOnline)
			prefs->cumSrvrsMostUsersOnline =	ini.GetInt(		"SrvrsMostUsersOnline"		, 0 );
		if (ini.GetInt("SrvrsMostFilesAvail", 0) > (int)prefs->cumSrvrsMostFilesAvail)
			prefs->cumSrvrsMostFilesAvail =		ini.GetInt(		"SrvrsMostFilesAvail"		, 0 );

		// Load records for shared files
		if (ini.GetInt("SharedMostFilesShared ", 0, "Statistics") > prefs->cumSharedMostFilesShared)
			prefs->cumSharedMostFilesShared =	ini.GetInt(		"SharedMostFilesShared"		, 0 );

		sprintf(buffer , ini.GetString(	"SharedLargestShareSize" , 0 ) );
		temp64 = _atoi64( buffer );
		if (temp64 > prefs->cumSharedLargestShareSize) prefs->cumSharedLargestShareSize = temp64;
		sprintf(buffer , ini.GetString( "SharedLargestAvgFileSize" , 0 ) );
		temp64 = _atoi64( buffer );
		if (temp64 > prefs->cumSharedLargestAvgFileSize) prefs->cumSharedLargestAvgFileSize = temp64;
		sprintf(buffer , ini.GetString( "SharedLargestFileSize" , 0 ) );
		temp64 = _atoi64( buffer );
		if (temp64 > prefs->cumSharedLargestFileSize) prefs->cumSharedLargestFileSize = temp64;

		// Check to make sure the backup of the values we just overwrote exists.  If so, rename it to the backup file.
		// This allows us to undo a restore, so to speak, just in case we don't like the restored values...
		CString sINIBackUp;
		sINIBackUp.Format("%sstatbkuptmp.ini", configdir);
		if (findBackUp.FindFile(sINIBackUp)) {		
			CFile::Remove(sINI); // Remove the backup that we just restored from
			CFile::Rename(sINIBackUp, sINI); // Rename our temporary backup to the normal statbkup.ini filename.
		}

		// Since we know this is a restore, now we should call ShowStatistics to update the data items to the new ones we just loaded.
		// Otherwise user is left waiting around for the tick counter to reach the next automatic update (Depending on setting in prefs)
		theApp.emuledlg->statisticswnd.ShowStatistics();
		
	}
	// Stupid Load -> Just load the values.
	else {
		// Load records for servers / network
		prefs->cumSrvrsMostWorkingServers = ini.GetInt(		"SrvrsMostWorkingServers"	, 0 );
		prefs->cumSrvrsMostUsersOnline =	ini.GetInt(		"SrvrsMostUsersOnline"		, 0 );
		prefs->cumSrvrsMostFilesAvail =		ini.GetInt(		"SrvrsMostFilesAvail"		, 0 );

		// Load records for shared files
		prefs->cumSharedMostFilesShared =	ini.GetInt(		"SharedMostFilesShared"		, 0 );

		sprintf(buffer , ini.GetString(		"SharedLargestShareSize"					, 0 ) );
		prefs->cumSharedLargestShareSize=	_atoi64( buffer );
		sprintf(buffer , ini.GetString(		"SharedLargestAvgFileSize"					, 0 ) );
		prefs->cumSharedLargestAvgFileSize=	_atoi64( buffer );
		sprintf(buffer , ini.GetString(		"SharedLargestFileSize"						, 0 ) );
		prefs->cumSharedLargestFileSize =	_atoi64( buffer );

		// These are not stats, but they're part of my mod, so we will load them here anyway.
		prefs->statsConnectionsGraphRatio =		ini.GetInt("statsConnectionsGraphRatio"	, 3	, "Statistics");
		sprintf(prefs->statsExpandedTreeItems,"%s",ini.GetString("statsExpandedTreeItems","111000000100000110000010000011110000010010","Statistics"));

		// Initialize new session statistic variables...
		prefs->sesDownCompletedFiles =		0;
		prefs->sesUpData_EDONKEY =			0;
		prefs->sesUpData_EDONKEYHYBRID =	0;
		prefs->sesUpData_EMULE =			0;
		prefs->sesUpData_MLDONKEY =			0;
		prefs->sesUpData_CDONKEY =			0;
		prefs->sesUpDataPort_4662 =			0;
		prefs->sesUpDataPort_OTHER =		0;
		prefs->sesDownData_EDONKEY =		0;
		prefs->sesDownData_EDONKEYHYBRID =	0;
		prefs->sesDownData_EMULE =			0;
		prefs->sesDownData_MLDONKEY =		0;
		prefs->sesDownData_CDONKEY =		0;
		prefs->sesDownData_SHAREAZA =		0;
		prefs->sesDownDataPort_4662 =		0;
		prefs->sesDownDataPort_OTHER =		0;
		prefs->sesDownSuccessfulSessions=	0;
		prefs->sesDownFailedSessions=		0;
		prefs->sesPartsSavedByICH=			0;
	}

	if (!fileex) {time_t	timeNow;time(&timeNow);prefs->stat_datetimeLastReset = (__int64) timeNow;}
	
	return true;

	// End Load Stats
}

// This formats the UCT long value that is saved for stat_datetimeLastReset
// If this value is 0 (Never reset), then it returns Unknown.
CString CPreferences::GetStatsLastResetStr(bool formatLong)
{
	// formatLong dictates the format of the string returned.
	// For example...
	// true: DateTime format from the .ini
	// false: DateTime format from the .ini for the log
	CString	returnStr;
	if (GetStatsLastResetLng()) {
		tm		*statsReset;
		char	strDateReset[128];
		time_t	lastResetDateTime = (time_t) GetStatsLastResetLng();
		statsReset = localtime(&lastResetDateTime);
		strftime(strDateReset, 128, formatLong?GetDateTimeFormat():GetDateTimeFormat4Log(), statsReset);
		returnStr.Format("%s", strDateReset);
	}
	else returnStr = GetResString(IDS_UNKNOWN);
	return returnStr;
}

// <-----khaos-

bool CPreferences::Save(){

	bool error = false;
	char* fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%spreferences.dat",configdir);

	FILE* preffile = fopen(fullpath,"wb");
	prefsExt->version = PREFFILE_VERSION;

	// -khaos--+++> Don't save stats if preferences.ini doesn't exist yet (Results in unhandled exception).
	sprintf(fullpath,"%spreferences.ini",configdir);
	bool bSaveStats = true;
	if (!PathFileExists(fullpath))
		bSaveStats = false;
	// <-----khaos-

	delete[] fullpath;
	if (preffile){
		prefsExt->version=PREFFILE_VERSION;
		prefsExt->EmuleWindowPlacement=prefs->EmuleWindowPlacement;
		md4cpy(&prefsExt->userhash,&prefs->userhash);

		error = fwrite(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (theApp.glob_prefs->GetCommitFiles() >= 2 || (theApp.glob_prefs->GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			fflush(preffile); // flush file stream buffers to disk buffers
			(void)_commit(_fileno(preffile)); // commit disk buffers to disk
		}
		fclose(preffile);
	}
	else
		error = true;

	SavePreferences();
	// -khaos--+++> SaveStats is now called here instead of from SavePreferences...
	if (bSaveStats)
		SaveStats();
	// <-----khaos-
	// khaos::categorymod+ We need to SaveCats() each time we exit eMule.
	SaveCats();
	// khaos::categorymod-

	fullpath = new char[strlen(configdir)+14];
	sprintf(fullpath,"%sshareddir.dat",configdir);
	CStdioFile sdirfile;
	if (sdirfile.Open(fullpath,CFile::modeCreate|CFile::modeWrite))
	{
		try{
			for (POSITION pos = shareddir_list.GetHeadPosition();pos != 0;shareddir_list.GetNext(pos)){
				sdirfile.WriteString(shareddir_list.GetAt(pos).GetBuffer());
				sdirfile.Write("\n",1);
			}
			if (theApp.glob_prefs->GetCommitFiles() >= 2 || (theApp.glob_prefs->GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
				sdirfile.Flush(); // flush file stream buffers to disk buffers
				if (_commit(_fileno(sdirfile.m_pStream)) != 0) // commit disk buffers to disk
					AfxThrowFileException(CFileException::hardIO, GetLastError(), sdirfile.GetFileName());
			}
			sdirfile.Close();
		}
		catch(CFileException* error){
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,ARRSIZE(buffer));
			AddDebugLogLine(true,_T("Failed to save %s - %s"), fullpath, buffer);
			error->Delete();
		}
	}
	else
		error = true;
	delete[] fullpath;
	fullpath=NULL;
	::CreateDirectory(GetIncomingDir(),0);
	::CreateDirectory(GetTempDir(),0);
	return error;
}

void CPreferences::CreateUserHash(){
	for (int i = 0;i != 8; i++){ 
		uint16	random = rand();
		memcpy(&userhash[i*2],&random,2);
	}
	// mark as emule client. that will be need in later version
	userhash[5] = 14;
	userhash[14] = 111;
}
int CPreferences::GetColumnWidth(Table t, int index) const {
	switch(t) {
	case tableDownload:
		return prefs->downloadColumnWidths[index];
	case tableUpload:
		return prefs->uploadColumnWidths[index];
	case tableQueue:
		return prefs->queueColumnWidths[index];
	case tableSearch:
		return prefs->searchColumnWidths[index];
	case tableShared:
		return prefs->sharedColumnWidths[index];
	case tableServer:
		return prefs->serverColumnWidths[index];
	case tableClientList:
		return prefs->clientListColumnWidths[index];
	}
	return 0;
}

void CPreferences::SetColumnWidth(Table t, int index, int width) {
	switch(t) {
	case tableDownload:
		prefs->downloadColumnWidths[index] = width;
		break;
	case tableUpload:
		prefs->uploadColumnWidths[index] = width;
		break;
	case tableQueue:
		prefs->queueColumnWidths[index] = width;
		break;
	case tableSearch:
		prefs->searchColumnWidths[index] = width;
		break;
	case tableShared:
		prefs->sharedColumnWidths[index] = width;
		break;
	case tableServer:
		prefs->serverColumnWidths[index] = width;
		break;
	case tableClientList:
		prefs->clientListColumnWidths[index] = width;
		break;
	}
}

BOOL CPreferences::GetColumnHidden(Table t, int index) const {
	switch(t) {
	case tableDownload:
		return prefs->downloadColumnHidden[index];
	case tableUpload:
		return prefs->uploadColumnHidden[index];
	case tableQueue:
		return prefs->queueColumnHidden[index];
	case tableSearch:
		return prefs->searchColumnHidden[index];
	case tableShared:
		return prefs->sharedColumnHidden[index];
	case tableServer:
		return prefs->serverColumnHidden[index];
	case tableClientList:
		return prefs->clientListColumnHidden[index];
	}
	return FALSE;
}

void CPreferences::SetColumnHidden(Table t, int index, BOOL bHidden) {
	switch(t) {
	case tableDownload:
		prefs->downloadColumnHidden[index] = bHidden;
		break;
	case tableUpload:
		prefs->uploadColumnHidden[index] = bHidden;
		break;
	case tableQueue:
		prefs->queueColumnHidden[index] = bHidden;
		break;
	case tableSearch:
		prefs->searchColumnHidden[index] = bHidden;
		break;
	case tableShared:
		prefs->sharedColumnHidden[index] = bHidden;
		break;
	case tableServer:
		prefs->serverColumnHidden[index] = bHidden;
		break;
	case tableClientList:
		prefs->clientListColumnHidden[index] = bHidden;
		break;
	}
}

int CPreferences::GetColumnOrder(Table t, int index) const {
	switch(t) {
	case tableDownload:
		return prefs->downloadColumnOrder[index];
	case tableUpload:
		return prefs->uploadColumnOrder[index];
	case tableQueue:
		return prefs->queueColumnOrder[index];
	case tableSearch:
		return prefs->searchColumnOrder[index];
	case tableShared:
		return prefs->sharedColumnOrder[index];
	case tableServer:
		return prefs->serverColumnOrder[index];
	case tableClientList:
		return prefs->clientListColumnOrder[index];
	}
	return 0;
}

void CPreferences::SetColumnOrder(Table t, INT *piOrder) {
	switch(t) {
	case tableDownload:
		memcpy(prefs->downloadColumnOrder, piOrder, sizeof(prefs->downloadColumnOrder));
		break;
	case tableUpload:
		memcpy(prefs->uploadColumnOrder, piOrder, sizeof(prefs->uploadColumnOrder));
		break;
	case tableQueue:
		memcpy(prefs->queueColumnOrder, piOrder, sizeof(prefs->queueColumnOrder));
		break;
	case tableSearch:
		memcpy(prefs->searchColumnOrder, piOrder, sizeof(prefs->searchColumnOrder));
		break;
	case tableShared:
		memcpy(prefs->sharedColumnOrder, piOrder, sizeof(prefs->sharedColumnOrder));
		break;
	case tableServer:
		memcpy(prefs->serverColumnOrder, piOrder, sizeof(prefs->serverColumnOrder));
		break;
	case tableClientList:
		memcpy(prefs->clientListColumnOrder, piOrder, sizeof(prefs->clientListColumnOrder));

		break;
	}
}

CPreferences::~CPreferences(){

	Category_Struct* delcat;
	while (!catMap.IsEmpty()) {
		delcat=catMap.GetAt(0); 
		catMap.RemoveAt(0); 
		delete delcat;
	}

//	delete[] appdir;
//	delete[] configdir;
	delete prefs;
	delete prefsExt;
}

int CPreferences::GetRecommendedMaxConnections() {
	int iRealMax = ::GetMaxConnections();
	if(iRealMax == -1 || iRealMax > 520)
		return 500;

	if(iRealMax < 20)
		return iRealMax;

	if(iRealMax <= 256)
		return iRealMax - 10;

	return iRealMax - 20;
}

void CPreferences::SavePreferences(){
	CString buffer;
	char* fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%spreferences.ini",configdir);
	
	CIni ini( fullpath, "eMule" );
	delete[] fullpath;
	fullpath=NULL;
	//---
	ini.WriteString("AppVersion", theApp.m_strCurVersionLong);
	//---
//MORPH START added by Yun.SF3: Ipfilter.dat update
	ini.WriteInt("IPfilterVersion",prefs->m_IPfilterVersion); //added by milobac: Ipfilter.dat update
	ini.WriteBool("AutoUPdateIPFilter",prefs->AutoUpdateIPFilter); //added by milobac: Ipfilter.dat update
//MORPH END added by Yun.SF3: Ipfilter.dat update

	buffer.Format("%s",prefs->nick);
	ini.WriteString("Nick",buffer);

	buffer.Format("%s",prefs->incomingdir);
	ini.WriteString("IncomingDir",buffer );

	buffer.Format("%s",prefs->tempdir);
	ini.WriteString("TempDir",buffer );
//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella
	// Maella -Defeat 0-filled Part Senders- (Idea of xrmb)
	ini.WriteBool("EnableZeroFilledTest", prefs->enableZeroFilledTest);
	// Maella end
//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella

	ini.WriteBool("EnableHighProcess", enableHighProcess); //MORPH - Added by IceCream, high process priority

	ini.WriteBool("EnableDownloadInRed", prefs->enableDownloadInRed); //MORPH - Added by IceCream, show download in red
	ini.WriteBool("EnableChunkAvaibility", prefs->enableChunkAvaibility); //MORPH - Added by IceCream, enable ChunkAvaibility
	ini.WriteBool("EnableAntiLeecher", prefs->enableAntiLeecher); //MORPH - Added by IceCream, enable AntiLeecher
	ini.WriteBool("EnableAntiCreditHack", prefs->enableAntiCreditHack); //MORPH - Added by IceCream, enable AntiCreditHack
	ini.WriteBool("IsBoostLess", prefs->isboostless);//Added by Yun.SF3, boost the less uploaded files
	ini.WriteInt("CreditSystemMode", prefs->creditSystemMode);// EastShare - Added by linekin, ES CreditSystem
	ini.WriteBool("IsUSSLimit", prefs->m_bIsUSSLimit); // EastShare - Added by TAHO, does USS limit
	ini.WriteBool("IsBoostFriends", prefs->isboostfriends);//Added by Yun.SF3, boost friends
	ini.WriteInt("MaxUpload",prefs->maxupload);
	ini.WriteInt("MaxDownload",prefs->maxdownload);
	ini.WriteInt("MaxConnections",prefs->maxconnections);
	ini.WriteInt("RemoveDeadServer",prefs->deadserver);
	ini.WriteInt("Port",prefs->port);
	ini.WriteInt("UDPPort",prefs->udpport);
	ini.WriteInt("ServerUDPPort", prefs->nServerUDPPort);
	ini.WriteInt("KadUDPPort",prefs->kadudpport);
	ini.WriteInt("MaxSourcesPerFile",prefs->maxsourceperfile );
	ini.WriteWORD("Language",prefs->languageID);
	ini.WriteInt("SeeShare",prefs->m_iSeeShares);
	ini.WriteInt("ToolTipDelay",prefs->m_iToolDelayTime);
	ini.WriteInt("StatGraphsInterval",prefs->trafficOMeterInterval);
	ini.WriteInt("StatsInterval",prefs->statsInterval);
	ini.WriteInt("DownloadCapacity",prefs->maxGraphDownloadRate);
	ini.WriteInt("UploadCapacity",prefs->maxGraphUploadRate);
	ini.WriteInt("DeadServerRetry",prefs->deadserverretries);
	ini.WriteInt("ServerKeepAliveTimeout",prefs->m_dwServerKeepAliveTimeout);
	ini.WriteInt("SplitterbarPosition",prefs->splitterbarPosition+2);
	ini.WriteInt("VariousStatisticsMaxValue",prefs->statsMax);
	ini.WriteInt("StatsAverageMinutes",prefs->statsAverageMinutes);
	ini.WriteInt("MaxConnectionsPerFiveSeconds",prefs->MaxConperFive);
	ini.WriteInt("Check4NewVersionDelay",prefs->versioncheckdays);
	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	ini.WriteInt("ReconnectOnLowIdRetries",prefs->LowIdRetries);	// SLUGFILLER: lowIdRetry
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	ini.WriteInt("HideOvershares",prefs->hideOS);
	ini.WriteBool("SelectiveShare",prefs->selectiveShare);
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS
	ini.WriteBool("Reconnect",prefs->reconnect);
	ini.WriteBool("Scoresystem",prefs->scorsystem);
	ini.WriteBool("ICH",prefs->ICH);
	ini.WriteBool("Serverlist",prefs->autoserverlist);
	ini.WriteBool("UpdateNotifyTestClient",prefs->updatenotify);
	ini.WriteBool("MinToTray",prefs->mintotray);
	ini.WriteBool("AddServersFromServer",prefs->addserversfromserver);
	ini.WriteBool("AddServersFromClient",prefs->addserversfromclient);
	ini.WriteBool("Splashscreen",prefs->splashscreen);
	ini.WriteBool("BringToFront",prefs->bringtoforeground);
	ini.WriteBool("TransferDoubleClick",prefs->transferDoubleclick);
	ini.WriteBool("BeepOnError",prefs->beepOnError);
	ini.WriteBool("ConfirmExit",prefs->confirmExit);
	ini.WriteBool("FilterBadIPs",prefs->filterBadIP);
    ini.WriteBool("Autoconnect",prefs->autoconnect);
	ini.WriteBool("OnlineSignature",prefs->onlineSig);
	ini.WriteBool("StartupMinimized",prefs->startMinimized);
	ini.WriteBool("SafeServerConnect",prefs->safeServerConnect);
	ini.WriteBool("ShowRatesOnTitle",prefs->showRatesInTitle);
	ini.WriteBool("IndicateRatings",prefs->indicateratings);
	ini.WriteBool("WatchClipboard4ED2kFilelinks",prefs->watchclipboard);
	ini.WriteInt("SearchMethod",prefs->m_iSearchMethod);
	ini.WriteBool("CheckDiskspace",prefs->checkDiskspace);	// SLUGFILLER: checkDiskspace
	ini.WriteInt("MinFreeDiskSpace",prefs->m_uMinFreeDiskSpace);
	// itsonlyme: hostnameSource
	buffer.Format("%s",prefs->yourHostname);
	ini.WriteString("YourHostname",buffer);
	// itsonlyme: hostnameSource

	ini.WriteBool("AutoDynUpSwitching",prefs->isautodynupswitching);//MORPH - Added by Yun.SF3, Auto DynUp changing
	ini.WriteBool("AutoPowershareNewDownloadFile",prefs->m_bisautopowersharenewdownloadfile); //MORPH - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	ini.WriteInt("FakesDatVersion",prefs->m_FakesDatVersion);
	ini.WriteBool("UpdateFakeStartup",prefs->UpdateFakeStartup);
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating

	//EastShare Start - PreferShareAll by AndCycle
	ini.WriteBool("ShareAll",prefs->shareall);	// SLUGFILLER: preferShareAll
	//EastShare END - PreferShareAll by AndCycle
	// EastShare START - Added by TAHO, .met file control
//	ini.WriteInt("ClientsMetDays", prefs->m_iClientsMetDays);//EastShare - AndCycle, this official setting shoudlnt be change by user
	ini.WriteInt("KnownMetDays", prefs->m_iKnownMetDays);
	// EastShare END - Added by TAHO, .met file control

	ini.WriteBool("AutoClearComplete", prefs->m_bAutoClearComplete);//EastShare - added by AndCycle - AutoClearComplete (NoamSon)

	// Barry - New properties...
    ini.WriteBool("AutoConnectStaticOnly", prefs->autoconnectstaticonly);  
	ini.WriteBool("AutoTakeED2KLinks", prefs->autotakeed2klinks);  
    ini.WriteBool("AddNewFilesPaused", prefs->addnewfilespaused);  
    ini.WriteInt ("3DDepth", prefs->depth3D);  

	ini.WriteBool("NotifyOnDownload",prefs->useDownloadNotifier); // Added by enkeyDEV
	ini.WriteBool("NotifyOnNewDownload",prefs->useNewDownloadNotifier);
	ini.WriteBool("NotifyOnChat",prefs->useChatNotifier);		  
	ini.WriteBool("NotifyOnLog",prefs->useLogNotifier);
	ini.WriteBool("NotifierUseSound",prefs->useSoundInNotifier);
	ini.WriteBool("NotifierPopEveryChatMessage",prefs->notifierPopsEveryChatMsg);
	ini.WriteBool("NotifierPopNewVersion",prefs->notifierNewVersion);

	buffer.Format("%s",prefs->notifierSoundFilePath);
	ini.WriteString("NotifierSoundPath",buffer);
	buffer.Format("%s",prefs->notifierConfiguration);		      // Added by enkeyDEV
	ini.WriteString("NotifierConfiguration",buffer);			  // Added by enkeyDEV
	ini.WriteString("TxtEditor",prefs->TxtEditor);
	ini.WriteString("VideoPlayer",prefs->VideoPlayer);
	ini.WriteString("UpdateURLFakeList",prefs->UpdateURLFakeList);		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	ini.WriteString("UpdateURLIPFilter",prefs->UpdateURLIPFilter);//MORPH START added by Yun.SF3: Ipfilter.dat update

	ini.WriteString("MessageFilter",prefs->messageFilter);
	ini.WriteString("CommentFilter",prefs->commentFilter);
	ini.WriteString("DateTimeFormat",GetDateTimeFormat());
	ini.WriteString("DateTimeFormat4Log",GetDateTimeFormat4Log());
	ini.WriteString("WebTemplateFile",prefs->m_sTemplateFile);
	ini.WriteString("FilenameCleanups",prefs->filenameCleanups);
	ini.WriteInt("ExtractMetaData",prefs->m_iExtractMetaData);

	ini.WriteString("DefaultIRCServer",prefs->m_sircserver);
	ini.WriteString("IRCNick",prefs->m_sircnick);
	ini.WriteBool("IRCAddTimestamp", prefs->m_bircaddtimestamp);
	ini.WriteString("IRCFilterName", prefs->m_sircchannamefilter);
	ini.WriteInt("IRCFilterUser", prefs->m_iircchanneluserfilter);
	ini.WriteBool("IRCUseFilter", prefs->m_bircusechanfilter);
	ini.WriteString("IRCPerformString", prefs->m_sircperformstring);
	ini.WriteBool("IRCUsePerform", prefs->m_bircuseperform);
	ini.WriteBool("IRCListOnConnect", prefs->m_birclistonconnect);
	ini.WriteBool("IRCAcceptLink", prefs->m_bircacceptlinks);
	ini.WriteBool("IRCAcceptLinkFriends", prefs->m_bircacceptlinksfriends);
	ini.WriteBool("IRCIgnoreInfoMessage", prefs->m_bircignoreinfomessage);
	ini.WriteBool("IRCIgnoreEmuleProtoInfoMessage", prefs->m_bircignoreemuleprotoinfomessage);
	ini.WriteBool("IRCHelpChannel", prefs->m_birchelpchannel);
	ini.WriteBool("NotifyOnImportantError", prefs->notifierImportantError);
	ini.WriteBool("SmartIdCheck", prefs->smartidcheck);
	ini.WriteBool("Verbose", prefs->m_bVerbose);
	ini.WriteBool("DebugSourceExchange", prefs->m_bDebugSourceExchange);
	ini.WriteBool("DebugSecuredConnection", prefs->m_bDebugSecuredConnection);   //MORPH - Added by SiRoB, Debug Log option for Secured Connection
	ini.WriteBool("PreviewPrio", prefs->m_bpreviewprio);
	ini.WriteBool("UpdateQueueListPref", prefs->m_bupdatequeuelist);
	ini.WriteBool("ManualHighPrio", prefs->m_bmanualhighprio);
	ini.WriteBool("FullChunkTransfers", prefs->m_btransferfullchunks);
	ini.WriteBool("StartNextFile", prefs->m_bstartnextfile);
	ini.WriteBool("ShowOverhead", prefs->m_bshowoverhead);
	ini.WriteBool("VideoPreviewBackupped", prefs->moviePreviewBackup);
	ini.WriteInt("PreviewSmallBlocks", prefs->m_iPreviewSmallBlocks);
	ini.WriteInt("FileBufferSizePref", prefs->m_iFileBufferSize);
	ini.WriteInt("QueueSizePref", prefs->m_iQueueSize);
	ini.WriteInt("CommitFiles", prefs->m_iCommitFiles);
	ini.WriteBool("DAPPref", prefs->m_bDAP);
	ini.WriteBool("UAPPref", prefs->m_bUAP);
	// khaos::kmod+ Obsolete ini.WriteInt("AllcatType", prefs->allcatType);
	ini.WriteBool("FilterServersByIP",prefs->filterserverbyip);
	ini.WriteBool("DisableKnownClientList",prefs->m_bDisableKnownClientList);
	ini.WriteBool("DisableQueueList",prefs->m_bDisableQueueList);
	ini.WriteBool("UseCreditSystem",prefs->m_bCreditSystem);

	ini.WriteBool("IsPayBackFirst",prefs->m_bPayBackFirst);//EastShare - added by AndCycle, Pay Back First

	ini.WriteBool("SaveLogToDisk",prefs->log2disk);
	ini.WriteBool("SaveDebugToDisk",prefs->debug2disk);
	ini.WriteBool("EnableScheduler",prefs->scheduler);
	ini.WriteBool("MessagesFromFriendsOnly",prefs->msgonlyfriends);
	ini.WriteBool("MessageFromValidSourcesOnly",prefs->msgsecure);
	ini.WriteBool("ShowInfoOnCatTabs",prefs->showCatTabInfos);
	ini.WriteBool("ResumeNextFromSameCat",prefs->resumeSameCat);
	ini.WriteBool("DontRecreateStatGraphsOnResize",prefs->dontRecreateGraphs);
	ini.WriteBool("AutoFilenameCleanup",prefs->autofilenamecleanup);
	ini.WriteBool("ShowExtControls",prefs->m_bExtControls);
	ini.WriteBool("UseAutocompletion",prefs->m_bUseAutocompl);
	ini.WriteBool("NetworkKademlia",prefs->networkkademlia);
	ini.WriteBool("NetworkED2K",prefs->networked2k);

	ini.WriteInt("VersionCheckLastAutomatic", prefs->versioncheckLastAutomatic);
	ini.WriteInt("FilterLevel",prefs->filterlevel);

	ini.WriteBool("SecureIdent", prefs->m_bUseSecureIdent);// change the name in future version to enable it by default
	ini.WriteBool("AdvancedSpamFilter",prefs->m_bAdvancedSpamfilter);
	ini.WriteBool("ShowDwlPercentage",prefs->m_bShowDwlPercentage);		

	// khaos::categorymod+ Save Preferences
	ini.WriteBool("ValidSrcsOnly", prefs->m_bValidSrcsOnly);
	ini.WriteBool("ShowCatName", prefs->m_bShowCatNames);
	ini.WriteBool("ActiveCatDefault", prefs->m_bActiveCatDefault);
	ini.WriteBool("SelCatOnAdd", prefs->m_bSelCatOnAdd);
	ini.WriteBool("AutoSetResumeOrder", prefs->m_bAutoSetResumeOrder);
	ini.WriteBool("SmallFileDLPush", prefs->m_bSmallFileDLPush);
	ini.WriteInt("StartDLInEmptyCats", prefs->m_iStartDLInEmptyCats);
	ini.WriteBool("UseAutoCat", prefs->m_bUseAutoCat);
	// khaos::categorymod-
	// khaos::kmod+
	ini.WriteBool("SmartA4AFSwapping", prefs->m_bSmartA4AFSwapping);
	ini.WriteInt("AdvancedA4AFMode", prefs->m_iAdvancedA4AFMode);
	ini.WriteBool("ShowA4AFDebugOutput", prefs->m_bShowA4AFDebugOutput);
	ini.WriteBool("RespectMaxSources", prefs->m_bRespectMaxSources);
	ini.WriteBool("UseSaveLoadSources", prefs->m_bUseSaveLoadSources);
	// khaos::categorymod-
	// khaos::accuratetimerem+
	ini.WriteInt("TimeRemainingMode", prefs->m_iTimeRemainingMode);
	// khaos::accuratetimerem-
	//MORPH START - Added by SiRoB, (SUC) & (USS)
	ini.WriteInt("MinUpload",prefs->m_iMinUpload);
	//MORPH END   - Added by SiRoB, (SUC) & (USS)
	//MORPH START - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	ini.WriteBool("SUCEnabled",prefs->m_bSUCEnabled);
	ini.WriteInt("SUCLog",prefs->m_bSUCLog);
	ini.WriteInt("SUCHigh",prefs->m_iSUCHigh);
	ini.WriteInt("SUCLow",prefs->m_iSUCLow);
	ini.WriteInt("SUCDrift",prefs->m_iSUCDrift);
	ini.WriteInt("SUCPitch",prefs->m_iSUCPitch);
	//MORPH END - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	ini.WriteInt("MaxConnectionsSwitchBorder",prefs->maxconnectionsswitchborder);//MORPH - Added by Yun.SF3, Auto DynUp changing

	//EastShare Start - Added by Pretender, TBH-AutoBackup
	ini.WriteBool("AutoBackup",prefs->autobackup);
	ini.WriteBool("AutoBackup2",prefs->autobackup2);
	//EastShare End - Added by Pretender, TBH-AutoBackup

	// Toolbar
	ini.WriteString("ToolbarSetting", prefs->m_sToolbarSettings);
	ini.WriteString("ToolbarBitmap", prefs->m_sToolbarBitmap );
	ini.WriteString("ToolbarBitmapFolder", prefs->m_sToolbarBitmapFolder);
	ini.WriteInt("ToolbarLabels", prefs->m_nToolbarLabels);
	ini.WriteString("SkinProfile", prefs->m_szSkinProfile);
	ini.WriteString("SkinProfileDir", prefs->m_szSkinProfileDir);


	ini.SerGet(false, prefs->downloadColumnWidths,
		ELEMENT_COUNT(prefs->downloadColumnWidths), "DownloadColumnWidths");
	ini.SerGet(false, prefs->downloadColumnHidden,
		ELEMENT_COUNT(prefs->downloadColumnHidden), "DownloadColumnHidden");
	ini.SerGet(false, prefs->downloadColumnOrder,
		ELEMENT_COUNT(prefs->downloadColumnOrder), "DownloadColumnOrder");
	ini.SerGet(false, prefs->uploadColumnWidths,
		ELEMENT_COUNT(prefs->uploadColumnWidths), "UploadColumnWidths");
	ini.SerGet(false, prefs->uploadColumnHidden,
		ELEMENT_COUNT(prefs->uploadColumnHidden), "UploadColumnHidden");
	ini.SerGet(false, prefs->uploadColumnOrder,
		ELEMENT_COUNT(prefs->uploadColumnOrder), "UploadColumnOrder");
	ini.SerGet(false, prefs->queueColumnWidths,
		ELEMENT_COUNT(prefs->queueColumnWidths), "QueueColumnWidths");
	ini.SerGet(false, prefs->queueColumnHidden,
		ELEMENT_COUNT(prefs->queueColumnHidden), "QueueColumnHidden");
	ini.SerGet(false, prefs->queueColumnOrder,
		ELEMENT_COUNT(prefs->queueColumnOrder), "QueueColumnOrder");
	ini.SerGet(false, prefs->searchColumnWidths,
		ELEMENT_COUNT(prefs->searchColumnWidths), "SearchColumnWidths");
	ini.SerGet(false, prefs->searchColumnHidden,
		ELEMENT_COUNT(prefs->searchColumnHidden), "SearchColumnHidden");
	ini.SerGet(false, prefs->searchColumnOrder,
		ELEMENT_COUNT(prefs->searchColumnOrder), "SearchColumnOrder");
	ini.SerGet(false, prefs->sharedColumnWidths,
		ELEMENT_COUNT(prefs->sharedColumnWidths), "SharedColumnWidths");
	ini.SerGet(false, prefs->sharedColumnHidden,
		ELEMENT_COUNT(prefs->sharedColumnHidden), "SharedColumnHidden");
	ini.SerGet(false, prefs->sharedColumnOrder,
		ELEMENT_COUNT(prefs->sharedColumnOrder), "SharedColumnOrder");
	ini.SerGet(false, prefs->serverColumnWidths,
		ELEMENT_COUNT(prefs->serverColumnWidths), "ServerColumnWidths");
	ini.SerGet(false, prefs->serverColumnHidden,
		ELEMENT_COUNT(prefs->serverColumnHidden), "ServerColumnHidden");
	ini.SerGet(false, prefs->serverColumnOrder,
		ELEMENT_COUNT(prefs->serverColumnOrder), "ServerColumnOrder");
	ini.SerGet(false, prefs->clientListColumnWidths,
		ELEMENT_COUNT(prefs->clientListColumnWidths), "ClientListColumnWidths");
	ini.SerGet(false, prefs->clientListColumnHidden,
		ELEMENT_COUNT(prefs->clientListColumnHidden), "ClientListColumnHidden");
	ini.SerGet(false, prefs->clientListColumnOrder,
		ELEMENT_COUNT(prefs->clientListColumnOrder), "ClientListColumnOrder");

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	// SLUGFILLER: multiSort - save multiple params
	ini.SerGet(false, prefs->tableSortItemDownload,
		GetColumnSortCount(tableDownload), "TableSortItemDownload");
	ini.SerGet(false, prefs->tableSortItemUpload,
		GetColumnSortCount(tableUpload), "TableSortItemUpload");
	ini.SerGet(false, prefs->tableSortItemQueue,
		GetColumnSortCount(tableQueue), "TableSortItemQueue");
	ini.SerGet(false, prefs->tableSortItemSearch,
		GetColumnSortCount(tableSearch), "TableSortItemSearch");
	ini.SerGet(false, prefs->tableSortItemShared,
		GetColumnSortCount(tableShared), "TableSortItemShared");
	ini.SerGet(false, prefs->tableSortItemServer,
		GetColumnSortCount(tableServer), "TableSortItemServer");
	ini.SerGet(false, prefs->tableSortItemClientList,
		GetColumnSortCount(tableClientList), "TableSortItemClientList");
	ini.SerGet(false, prefs->tableSortAscendingDownload,
		GetColumnSortCount(tableDownload), "TableSortAscendingDownload");
	ini.SerGet(false, prefs->tableSortAscendingUpload,
		GetColumnSortCount(tableUpload), "TableSortAscendingUpload");
	ini.SerGet(false, prefs->tableSortAscendingQueue,
		GetColumnSortCount(tableQueue), "TableSortAscendingQueue");
	ini.SerGet(false, prefs->tableSortAscendingSearch,
		GetColumnSortCount(tableSearch), "TableSortAscendingSearch");
	ini.SerGet(false, prefs->tableSortAscendingShared,
		GetColumnSortCount(tableShared), "TableSortAscendingShared");
	ini.SerGet(false, prefs->tableSortAscendingServer,
		GetColumnSortCount(tableServer), "TableSortAscendingServer");
	ini.SerGet(false, prefs->tableSortAscendingClientList,
		GetColumnSortCount(tableClientList), "TableSortAscendingClientList");
	// SLUGFILLER: multiSort
	ini.WriteBinary("HyperTextFont", (LPBYTE)&prefs->m_lfHyperText, sizeof prefs->m_lfHyperText);

	// deadlake PROXYSUPPORT
	ini.WriteBool("ProxyEnablePassword",prefs->proxy.EnablePassword,"Proxy");
	ini.WriteBool("ProxyEnableProxy",prefs->proxy.UseProxy,"Proxy");
	ini.WriteString("ProxyName",prefs->proxy.name,"Proxy");
	ini.WriteString("ProxyPassword",prefs->proxy.password,"Proxy");
	ini.WriteString("ProxyUser",prefs->proxy.user,"Proxy");
	ini.WriteInt("ProxyPort",prefs->proxy.port,"Proxy");
	ini.WriteInt("ProxyType",prefs->proxy.type,"Proxy");
	ini.WriteBool("ConnectWithoutProxy",prefs->m_bIsASCWOP,"Proxy");
	ini.WriteBool("ShowErrors",prefs->m_bShowProxyErrors,"Proxy");

	CString buffer2;
	for (int i=0;i<15;i++) {
		buffer.Format("%u",GetStatsColor(i));
		buffer2.Format("StatColor%i",i);
		ini.WriteString(buffer2,buffer,"Statistics");
	}

	// -khaos--+++>
	/* Original stat saves from base code now obsolete (KHAOS)
	buffer.Format("%I64u",prefs->totalDownloadedBytes);
	ini.WriteString("TotalDownloadedBytes",buffer ,"Statistics");

	buffer.Format("%I64u",prefs->totalUploadedBytes);
	ini.WriteString("TotalUploadedBytes",buffer ,"Statistics");
	// End original stat saves from base code. */
	// <-----khaos--

	// Web Server
	ini.WriteString("Password", GetWSPass(), "WebServer");
	ini.WriteString("PasswordLow", GetWSLowPass());
	ini.WriteInt("Port", prefs->m_nWebPort);
	ini.WriteBool("Enabled", prefs->m_bWebEnabled);
	ini.WriteBool("UseGzip", prefs->m_bWebUseGzip);
	ini.WriteInt("PageRefreshTime", prefs->m_nWebPageRefresh);
	ini.WriteBool("UseLowRightsUser", prefs->m_bWebLowEnabled);
	
	//mobileMule
	ini.WriteString("Password", GetMMPass(), "MobileMule");
	ini.WriteBool("Enabled", prefs->m_bMMEnabled);
	ini.WriteInt("Port", prefs->m_nMMPort);

	//MORPH START - Added by SiRoB,  ZZ dynamic upload (USS)
	ini.WriteBool("DynUpEnabled", prefs->m_bDynUpEnabled);
	ini.WriteBool("DynUpLog", prefs->m_bDynUpLog);

	ini.WriteInt("DynUpPingLimit", prefs->m_iDynUpPingLimit); // EastShare - Add by TAHO, USS limit

	ini.WriteInt("DynUpPingTolerance", prefs->m_iDynUpPingTolerance);
	ini.WriteInt("DynUpGoingUpDivider", prefs->m_iDynUpGoingUpDivider);
	ini.WriteInt("DynUpGoingDownDivider", prefs->m_iDynUpGoingDownDivider);
	ini.WriteInt("DynUpNumberOfPings", prefs->m_iDynUpNumberOfPings);
	//MORPH END    - Added by SiRoB,  ZZ dynamic upload (USS)
}

void CPreferences::SaveCats(){

	// Cats
	CString catinif,ixStr,buffer;
	catinif.Format("%sCategory.ini",configdir);
	remove(catinif);

		CIni catini( catinif, "Category" );
		catini.WriteInt("Count",catMap.GetCount()-1,"General");
	catini.WriteInt("CategoryVersion", 2, "General"); // khaos::categorymod+
	for (int ix=0;ix<catMap.GetCount();ix++){
			ixStr.Format("Cat#%i",ix);
			catini.WriteString("Title",catMap.GetAt(ix)->title,ixStr);
			catini.WriteString("Incoming",catMap.GetAt(ix)->incomingpath,ixStr);
			catini.WriteString("Comment",catMap.GetAt(ix)->comment,ixStr);
			buffer.Format("%lu",catMap.GetAt(ix)->color,ixStr);
			catini.WriteString("Color",buffer,ixStr);
			catini.WriteInt("Priority",catMap.GetAt(ix)->prio,ixStr);
			// khaos::kmod+ Category Advanced A4AF Mode and Auto Cat
			catini.WriteInt("AdvancedA4AFMode", catMap.GetAt(ix)->iAdvA4AFMode, ixStr);
			//catini.WriteString("AutoCatString", catMap.GetAt(ix)->autocat, ixStr);
			// khaos::kmod-
		// khaos::categorymod+ Save View Filters
		catini.WriteInt("vfFromCats", catMap.GetAt(ix)->viewfilters.nFromCats, ixStr);
		catini.WriteBool("vfVideo", catMap.GetAt(ix)->viewfilters.bVideo, ixStr);
		catini.WriteBool("vfAudio", catMap.GetAt(ix)->viewfilters.bAudio, ixStr);
		catini.WriteBool("vfArchives", catMap.GetAt(ix)->viewfilters.bArchives, ixStr);
		catini.WriteBool("vfImages", catMap.GetAt(ix)->viewfilters.bImages, ixStr);
		catini.WriteBool("vfWaiting", catMap.GetAt(ix)->viewfilters.bWaiting, ixStr);
		catini.WriteBool("vfTransferring", catMap.GetAt(ix)->viewfilters.bTransferring, ixStr);
		catini.WriteBool("vfPaused", catMap.GetAt(ix)->viewfilters.bPaused, ixStr);
		catini.WriteBool("vfStopped", catMap.GetAt(ix)->viewfilters.bStopped, ixStr);
		catini.WriteBool("vfComplete", catMap.GetAt(ix)->viewfilters.bComplete, ixStr);
		catini.WriteBool("vfHashing", catMap.GetAt(ix)->viewfilters.bHashing, ixStr);
		catini.WriteBool("vfErrorUnknown", catMap.GetAt(ix)->viewfilters.bErrorUnknown, ixStr);
		catini.WriteBool("vfCompleting", catMap.GetAt(ix)->viewfilters.bCompleting, ixStr);
		catini.WriteInt("vfFSizeMin", catMap.GetAt(ix)->viewfilters.nFSizeMin, ixStr);
		catini.WriteInt("vfFSizeMax", catMap.GetAt(ix)->viewfilters.nFSizeMax, ixStr);
		catini.WriteInt("vfRSizeMin", catMap.GetAt(ix)->viewfilters.nRSizeMin, ixStr);
		catini.WriteInt("vfRSizeMax", catMap.GetAt(ix)->viewfilters.nRSizeMax, ixStr);
		catini.WriteInt("vfTimeRemainingMin", catMap.GetAt(ix)->viewfilters.nTimeRemainingMin, ixStr);
		catini.WriteInt("vfTimeRemainingMax", catMap.GetAt(ix)->viewfilters.nTimeRemainingMax, ixStr);
		catini.WriteInt("vfSourceCountMin", catMap.GetAt(ix)->viewfilters.nSourceCountMin, ixStr);
		catini.WriteInt("vfSourceCountMax", catMap.GetAt(ix)->viewfilters.nSourceCountMax, ixStr);
		catini.WriteInt("vfAvailSourceCountMin", catMap.GetAt(ix)->viewfilters.nAvailSourceCountMin, ixStr);
		catini.WriteInt("vfAvailSourceCountMax", catMap.GetAt(ix)->viewfilters.nAvailSourceCountMax, ixStr);
		catini.WriteString("AdvancedFilterMask", catMap.GetAt(ix)->viewfilters.sAdvancedFilterMask, ixStr);
		// Save Selection Criteria
		catini.WriteBool("scFileSize", catMap.GetAt(ix)->selectioncriteria.bFileSize, ixStr);
		catini.WriteBool("scAdvancedFilterMask", catMap.GetAt(ix)->selectioncriteria.bAdvancedFilterMask, ixStr);
		// khaos::categorymod-
		}
}

void CPreferences::ResetStatsColor(int index){
	switch(index) {
		case 0 : prefs->statcolors[0]=RGB(0,0,0);break;  //MORPH - HotFix by SiRoB & IceCream, Default Black color for BackGround
		case 1 : prefs->statcolors[1]=RGB(192,192,255);break;
		case 2 : prefs->statcolors[2]=RGB(128, 255, 128);break;
		case 3 : prefs->statcolors[3]=RGB(0, 210, 0);break;
		case 4 : prefs->statcolors[4]=RGB(0, 128, 0);break;
		case 5 : prefs->statcolors[5]=RGB(255, 128, 128);break;
		case 6 : prefs->statcolors[6]=RGB(200, 0, 0);break;
		case 7 : prefs->statcolors[7]=RGB(140, 0, 0);break;
		case 8 : prefs->statcolors[8]=RGB(150, 150, 255);break;
		case 9 : prefs->statcolors[9]=RGB(255, 255, 128);break; //MORPH - Added by Yun.SF3, ZZ Upload System
		case 10 : prefs->statcolors[10]=RGB(0, 255, 0);break;
		case 11 : prefs->statcolors[11]=RGB(0, 0, 0);break; //MORPH - HotFix by SiRoB & IceCream, Default Black color for SystrayBar
		case 12 : prefs->statcolors[12]=RGB(192,   0, 192);break; //MORPH - Added by Yun.SF3, ZZ Upload System
		case 13 : prefs->statcolors[13]=RGB(0, 0, 255);break; //MORPH - Added by Yun.SF3, ZZ Upload System
		case 14 : prefs->statcolors[14]=RGB(200, 200, 0);break;

		default:break;
	}
}

void CPreferences::LoadPreferences(){
	char buffer[200];
	// -khaos--+++> Fix to stats being lost when version changes!
	int loadstatsFromOld = 0;
	// <-----khaos-

	//--- Quick hack to add version tag to preferences.ini-file and solve the issue with the FlatStatusBar tag...
	CString strFileName;
	strFileName.Format("%spreferences.ini", configdir);
	CIni* pIni = new CIni(strFileName, "eMule");

	CString strCurrVersion, strPrefsVersion;

	strCurrVersion = theApp.m_strCurVersionLong;
	strPrefsVersion = pIni->GetString("AppVersion");
	delete pIni;
	prefs->m_bFirstStart = false;

	CFileFind findFileName;
	if (strCurrVersion != strPrefsVersion){
//MORPH START - Added by IceCream, No more wizard at launch if you upgrade your Morph version to an other Morph
		if (!StrStrI(strPrefsVersion,"morph"))
			prefs->m_bFirstStart = true;
//MORPH END  - Added by IceCream, No more wizard at launch if you upgrade your Morph version to an other Morph
		if(findFileName.FindFile(strFileName)){
			CFile file;
			CFileFind findNewName;
			CString strNewName;
			strNewName.Format("%spreferences.ini.old", configdir);
	
			if (findNewName.FindFile(strNewName))
				file.Remove(strNewName);
	
			file.Rename(strFileName, strNewName);
			strFileName = strNewName;
			// -khaos--+++> Set this to 2 so that LoadStats will load 'em from ini.old
			loadstatsFromOld = 2;
			// <-----khaos-
		}
	}
	CIni ini(strFileName, "eMule");
	//--- end Ozon :)

	//MORPH START - Added by IceCream, changed default nickname
	sprintf(prefs->nick,"%s",ini.GetString("Nick","eMule v"+strCurrVersion));
	if(((strstr(prefs->nick, "eMule v"))&&(!StrStrI(prefs->nick, "morph"))) || (strstr(prefs->nick,"emule-project"))) 
		sprintf(prefs->nick,"%s","eMule v"+strCurrVersion);
	//MORPH END   - Added by IceCream, changed default nickname	

	sprintf(buffer,"%sIncoming",appdir);
	sprintf(prefs->incomingdir,"%s",ini.GetString("IncomingDir",buffer ));
	MakeFoldername(prefs->incomingdir);

	sprintf(buffer,"%sTemp",appdir);
	sprintf(prefs->tempdir,"%s",ini.GetString("TempDir",buffer));
	MakeFoldername(prefs->tempdir);

	//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella
	// Maella -Defeat 0-filled Part Senders- (Idea of xrmb)
	prefs->enableZeroFilledTest = ini.GetBool("EnableZeroFilledTest", false);
	// Maella end
	//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella

	prefs->enableDownloadInRed = ini.GetBool("EnableDownloadInRed", true); //MORPH - Added by IceCream, show download in red
	prefs->enableChunkAvaibility = ini.GetBool("EnableChunkAvaibility", true); //MORPH - Added by IceCream, enable ChunkAvaibility
	prefs->enableAntiLeecher = ini.GetBool("EnableAntiLeecher", true); //MORPH - Added by IceCream, enable AntiLeecher
	prefs->enableAntiCreditHack = ini.GetBool("EnableAntiCreditHack", true); //MORPH - Added by IceCream, enable AntiCreditHack
	enableHighProcess = ini.GetBool("EnableHighProcess", false); //MORPH - Added by IceCream, high process priority
	prefs->isboostless = ini.GetBool("IsBoostLess", false);//Added by Yun.SF3, boost the less uploaded files
	prefs->creditSystemMode = (CreditSystemSelection)ini.GetInt("CreditSystemMode", CS_OFFICIAL); // EastShare - Added by linekin, ES CreditSystem
	prefs->m_bIsUSSLimit = ini.GetBool("IsUSSLimit", true); // EastShare - Added by TAHO, does USS limit
	prefs->isboostfriends = ini.GetBool("IsBoostFriends", false);//Added by Yun.SF3, boost friends
	prefs->maxGraphDownloadRate=ini.GetInt("DownloadCapacity",96);
	if (prefs->maxGraphDownloadRate==0) prefs->maxGraphDownloadRate=96;
	prefs->maxGraphUploadRate=ini.GetInt("UploadCapacity",16);
	if (prefs->maxGraphUploadRate==0) prefs->maxGraphUploadRate=16;
	prefs->maxupload=ini.GetInt("MaxUpload",12);
	if (prefs->maxupload>prefs->maxGraphUploadRate) prefs->maxupload=prefs->maxGraphUploadRate*.8;
	prefs->maxdownload=ini.GetInt("MaxDownload",76);
	if (prefs->maxdownload>prefs->maxGraphDownloadRate && prefs->maxdownload!=UNLIMITED) prefs->maxdownload=prefs->maxGraphDownloadRate*.8;
	prefs->maxconnections=ini.GetInt("MaxConnections",GetRecommendedMaxConnections());
	prefs->deadserver=ini.GetInt("RemoveDeadServer",2);
	prefs->port=ini.GetInt("Port",4662);
	prefs->udpport=ini.GetInt("UDPPort",prefs->port+10);
	prefs->nServerUDPPort = ini.GetInt("ServerUDPPort", -1); // 0 = Don't use UDP port for servers, -1 = use a random port (for backward compatibility)
	prefs->kadudpport=ini.GetInt("KadUDPPort",prefs->port+11);
	prefs->maxsourceperfile=ini.GetInt("MaxSourcesPerFile",400 );
	prefs->languageID=ini.GetWORD("Language",0);
	prefs->m_iSeeShares=ini.GetInt("SeeShare",2);
	prefs->m_iToolDelayTime=ini.GetInt("ToolTipDelay",1);
	prefs->trafficOMeterInterval=ini.GetInt("StatGraphsInterval",3);
	prefs->statsInterval=ini.GetInt("statsInterval",5);

	//MORPH START added by Yun.SF3: Ipfilter.dat update
	prefs->m_IPfilterVersion=ini.GetInt("IPfilterVersion",0); //added by milobac: Ipfilter.dat update
	prefs->AutoUpdateIPFilter=ini.GetBool("AutoUPdateIPFilter",false); //added by milobac: Ipfilter.dat update
	//MORPH END added by Yun.SF3: Ipfilter.dat update

	prefs->deadserverretries=ini.GetInt("DeadServerRetry",1);
	prefs->m_dwServerKeepAliveTimeout=ini.GetInt("ServerKeepAliveTimeout",0);
	prefs->splitterbarPosition=ini.GetInt("SplitterbarPosition",75);
	if (prefs->splitterbarPosition>93 || prefs->splitterbarPosition<10) prefs->splitterbarPosition=75;

	prefs->statsMax=ini.GetInt("VariousStatisticsMaxValue",100);
	prefs->statsAverageMinutes=ini.GetInt("StatsAverageMinutes",5);
	prefs->MaxConperFive=ini.GetInt("MaxConnectionsPerFiveSeconds",GetDefaultMaxConperFive());

	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	prefs->LowIdRetries=ini.GetInt("ReconnectOnLowIdRetries",3);	// SLUGFILLER: lowIdRetry
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	prefs->hideOS=ini.GetInt("HideOvershares",0/*5*/);
	prefs->selectiveShare=ini.GetBool("SelectiveShare",false);
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS

	//EastShare - Added by Pretender, TBH-AutoBackup
	prefs->autobackup = ini.GetBool("AutoBackup",true);
	prefs->autobackup2 = ini.GetBool("AutoBackup2",true);
	//EastShare - Added by Pretender, TBH-AutoBackup

	prefs->reconnect=ini.GetBool("Reconnect",true);
	prefs->scorsystem=ini.GetBool("Scoresystem",true);
	prefs->ICH=ini.GetBool("ICH",true);
	prefs->autoserverlist=ini.GetBool("Serverlist",false);
	prefs->isautodynupswitching=ini.GetBool("AutoDynUpSwitching",false);
	prefs->updatenotify=ini.GetBool("UpdateNotifyTestClient",true);
	prefs->mintotray=ini.GetBool("MinToTray",false);
	prefs->addserversfromserver=ini.GetBool("AddServersFromServer",true);
	prefs->addserversfromclient=ini.GetBool("AddServersFromClient",true);
	prefs->splashscreen=ini.GetBool("Splashscreen",true);
	prefs->bringtoforeground=ini.GetBool("BringToFront",true);
	prefs->transferDoubleclick=ini.GetBool("TransferDoubleClick",true);
	prefs->beepOnError=ini.GetBool("BeepOnError",true);
	prefs->confirmExit=ini.GetBool("ConfirmExit",false);
	prefs->filterBadIP=ini.GetBool("FilterBadIPs",true);
	prefs->autoconnect=ini.GetBool("Autoconnect",false);
	prefs->showRatesInTitle=ini.GetBool("ShowRatesOnTitle",false);

	prefs->onlineSig=ini.GetBool("OnlineSignature",false);
	prefs->startMinimized=ini.GetBool("StartupMinimized",false);
	prefs->safeServerConnect =ini.GetBool("SafeServerConnect",false);

	prefs->filterserverbyip=ini.GetBool("FilterServersByIP",false);
	prefs->filterlevel=ini.GetInt("FilterLevel",127);
	prefs->checkDiskspace=ini.GetBool("CheckDiskspace",false);	// SLUGFILLER: checkDiskspace
	prefs->m_uMinFreeDiskSpace=ini.GetInt("MinFreeDiskSpace",0);
	sprintf(prefs->yourHostname,"%s",ini.GetString("YourHostname",""));	// itsonlyme: hostnameSource

	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	prefs->m_bisautopowersharenewdownloadfile=ini.GetBool("AutoPowershareNewDownloadFile",true);
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	prefs->m_FakesDatVersion=ini.GetInt("FakesDatVersion",0);
	prefs->UpdateFakeStartup=ini.GetBool("UpdateFakeStartup",false);
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating
	//MORPH START - Added by SiRoB, (SUC) & (USS)
	prefs->m_iMinUpload = ini.GetInt("MinUpload",/*10*/5*prefs->maxGraphUploadRate/8);
	prefs->m_iMinUpload = min(max(prefs->m_iMinUpload,1),prefs->maxGraphUploadRate);
	//MORPH END   - Added by SiRoB, (SUC) & (USS)
	//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	prefs->m_bSUCEnabled = ini.GetBool("SUCEnabled",false);
	prefs->m_bSUCLog =  ini.GetBool("SUCLog",false);
	prefs->m_iSUCHigh = ini.GetInt("SUCHigh",900);
	prefs->m_iSUCHigh = min(max(prefs->m_iSUCHigh,350),1000);
	prefs->m_iSUCLow = ini.GetInt("SUCLow",600);
	prefs->m_iSUCLow = min(max(prefs->m_iSUCLow,350),prefs->m_iSUCHigh);
	prefs->m_iSUCPitch = ini.GetInt("SUCPitch",3000);
	prefs->m_iSUCPitch = min(max(prefs->m_iSUCPitch,2500),10000);
	prefs->m_iSUCDrift = ini.GetInt("SUCDrift",50);
	prefs->m_iSUCDrift = min(max(prefs->m_iSUCDrift,0),100);
	//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	prefs->maxconnectionsswitchborder = ini.GetInt("MaxConnectionsSwitchBorder",100);//MORPH - Added by Yun.SF3, Auto DynUp changing
	prefs->maxconnectionsswitchborder = min(max(prefs->maxconnectionsswitchborder,50),60000);//MORPH - Added by Yun.SF3, Auto DynUp changing

	//EastShare Start - PreferShareAll by AndCycle
	prefs->shareall=ini.GetBool("ShareAll",true);	// SLUGFILLER: preferShareAll
	//EastShare END - PreferShareAll by AndCycle
	// EastShare START - Added by TAHO, .met file control
//	prefs->m_iClientsMetDays = ini.GetInt("ClientsMetDays", 150);//EastShare - AndCycle, this official setting shoudlnt be change by user
	prefs->m_iKnownMetDays = ini.GetInt("KnownMetDays", 0);
	// EastShare END - Added by TAHO, .met file control

	// Barry - New properties...
	prefs->autoconnectstaticonly = ini.GetBool("AutoConnectStaticOnly",false); 
	prefs->autotakeed2klinks = ini.GetBool("AutoTakeED2KLinks",true); 
	prefs->addnewfilespaused = ini.GetBool("AddNewFilesPaused",false); 
	prefs->depth3D = ini.GetInt("3DDepth", 0);

	// as temporarial converter for previous versions
	if (strPrefsVersion < "0.25a") // before 0.25a
		if (ini.GetBool("FlatStatusBar",false))
			prefs->depth3D = 0;
		else 
			prefs->depth3D = 5;

	prefs->useDownloadNotifier=ini.GetBool("NotifyOnDownload",false);	// Added by enkeyDEV
	prefs->useNewDownloadNotifier=ini.GetBool("NotifyOnNewDownload",false);
	prefs->useChatNotifier=ini.GetBool("NotifyOnChat",false);
	prefs->useLogNotifier=ini.GetBool("NotifyOnLog",false);
	prefs->useSoundInNotifier=ini.GetBool("NotifierUseSound",false);
	prefs->notifierPopsEveryChatMsg=ini.GetBool("NotifierPopEveryChatMessage",false);
	prefs->notifierImportantError=ini.GetBool("NotifyOnImportantError",false);
	prefs->notifierNewVersion=ini.GetBool("NotifierPopNewVersion",false);
	sprintf(prefs->notifierSoundFilePath,"%s",ini.GetString("NotifierSoundPath",""));
	sprintf(prefs->notifierConfiguration,"%s",ini.GetString("NotifierConfiguration","")); // Added by enkeyDEV
	sprintf(prefs->datetimeformat,"%s",ini.GetString("DateTimeFormat","%A, %x, %X"));
	if (strlen(prefs->datetimeformat)==0) strcpy(prefs->datetimeformat,"%A, %x, %X");

	sprintf(prefs->datetimeformat4log,"%s",ini.GetString("DateTimeFormat4Log","%c"));
	if (strlen(prefs->datetimeformat4log)==0) strcpy(prefs->datetimeformat4log,"%c");

	sprintf(prefs->m_sircserver,"%s",ini.GetString("DefaultIRCServer","irc.emule-project.net"));
	sprintf(prefs->m_sircnick,"%s",ini.GetString("IRCNick","eMule"));
	prefs->m_bircaddtimestamp=ini.GetBool("IRCAddTimestamp",true);
	sprintf(prefs->m_sircchannamefilter,"%s",ini.GetString("IRCFilterName", "" ));
	prefs->m_bircusechanfilter=ini.GetBool("IRCUseFilter", false);
	prefs->m_iircchanneluserfilter=ini.GetInt("IRCFilterUser", 0);
	sprintf(prefs->m_sircperformstring,"%s",ini.GetString("IRCPerformString", "" ));
	prefs->m_bircuseperform=ini.GetBool("IRCUsePerform", false);
	prefs->m_birclistonconnect=ini.GetBool("IRCListOnConnect", true);
	prefs->m_bircacceptlinks=ini.GetBool("IRCAcceptLink", true);
	prefs->m_bircacceptlinksfriends=ini.GetBool("IRCAcceptLinkFriends", true);
	prefs->m_bircignoreinfomessage=ini.GetBool("IRCIgnoreInfoMessage", false);
	prefs->m_bircignoreemuleprotoinfomessage=ini.GetBool("IRCIgnoreEmuleProtoInfoMessage", true);
	prefs->m_birchelpchannel=ini.GetBool("IRCHelpChannel",false);
	prefs->smartidcheck=ini.GetBool("SmartIdCheck",true);
	prefs->m_bVerbose=ini.GetBool("Verbose",false);
	prefs->m_bDebugSourceExchange=ini.GetBool("DebugSourceExchange",false);
	prefs->m_bDebugSecuredConnection=ini.GetBool("DebugSecuredConnection",false); //MORPH - Added by SiRoB, Debug Log option for Secured Connection
	prefs->m_dwDebugServerTCP=ini.GetInt("DebugServerTCP",0);
	prefs->m_dwDebugServerUDP=ini.GetInt("DebugServerUDP",0);
	prefs->m_dwDebugServerSources=ini.GetInt("DebugServerSources",0);
	prefs->m_dwDebugServerSearches=ini.GetInt("DebugServerSearches",0);
	prefs->m_bpreviewprio=ini.GetBool("PreviewPrio",false);
	prefs->m_bupdatequeuelist=ini.GetBool("UpdateQueueListPref",false);
	prefs->m_bmanualhighprio=ini.GetBool("ManualHighPrio",false);
	prefs->m_btransferfullchunks=ini.GetBool("FullChunkTransfers",true);
	prefs->m_bstartnextfile=ini.GetBool("StartNextFile",false);
	prefs->m_bshowoverhead=ini.GetBool("ShowOverhead",false);
	prefs->moviePreviewBackup=ini.GetBool("VideoPreviewBackupped",true);
	prefs->m_iPreviewSmallBlocks=ini.GetInt("PreviewSmallBlocks", 0);
	prefs->m_iFileBufferSize=ini.GetInt("FileBufferSizePref",16);
	prefs->m_iQueueSize=ini.GetInt("QueueSizePref",50);
	if (prefs->m_iQueueSize < 20 || prefs->m_iQueueSize > 100) prefs->m_iQueueSize=50; //<--- 
	prefs->m_iCommitFiles=ini.GetInt("CommitFiles", 1); // 1 = "commit" on application shut down; 2 = "commit" on each file saveing
	prefs->versioncheckdays=ini.GetInt("Check4NewVersionDelay",5);
	prefs->m_bDAP=ini.GetBool("DAPPref",true);
	prefs->m_bUAP=ini.GetBool("UAPPref",true);
	prefs->indicateratings=ini.GetBool("IndicateRatings",true);
	// khaos::kmod+ Obsolete prefs->allcatType=ini.GetInt("AllcatType",0);
	prefs->watchclipboard=ini.GetBool("WatchClipboard4ED2kFilelinks",false);
	prefs->m_iSearchMethod=ini.GetInt("SearchMethod",0);

	prefs->log2disk=ini.GetBool("SaveLogToDisk",false);
	prefs->debug2disk=ini.GetBool("SaveDebugToDisk",false);
	prefs->iMaxLogBuff = ini.GetInt("MaxLogBuff",64) * 1024;
	prefs->showCatTabInfos=ini.GetBool("ShowInfoOnCatTabs",false);
	prefs->resumeSameCat=ini.GetBool("ResumeNextFromSameCat",false);
	prefs->dontRecreateGraphs =ini.GetBool("DontRecreateStatGraphsOnResize",false);
	prefs->m_bExtControls =ini.GetBool("ShowExtControls",false);
	
	prefs->versioncheckLastAutomatic=ini.GetInt("VersionCheckLastAutomatic",0);
	prefs->m_bDisableKnownClientList=ini.GetInt("DisableKnownClientList",false);
	prefs->m_bDisableQueueList=ini.GetInt("DisableQueueList",false);
	prefs->m_bCreditSystem=true; //ini.GetInt("UseCreditSystem",true); //MORPH - Changed by SiRoB, CreditSystem allways used

	prefs->m_bPayBackFirst=ini.GetBool("IsPayBackFirst",false);//EastShare - added by AndCycle, Pay Back First
	prefs->m_bAutoClearComplete = ini.GetBool("AutoClearComplete", false );//EastShare - added by AndCycle - AutoClearComplete (NoamSon)

	prefs->scheduler=ini.GetBool("EnableScheduler",false);
	prefs->msgonlyfriends=ini.GetBool("MessagesFromFriendsOnly",false);
	prefs->msgsecure=ini.GetBool("MessageFromValidSourcesOnly",true);
	prefs->autofilenamecleanup=ini.GetBool("AutoFilenameCleanup",false);
	prefs->m_bUseAutocompl=ini.GetBool("UseAutocompletion",true);
	prefs->m_bShowDwlPercentage=ini.GetBool("ShowDwlPercentage",false);
	prefs->networkkademlia=ini.GetBool("NetworkKademlia",true);
	prefs->networked2k=ini.GetBool("NetworkED2K",true);

	prefs->m_iMaxChatHistory=ini.GetInt("MaxChatHistoryLines",100);
	if (prefs->m_iMaxChatHistory<1) prefs->m_iMaxChatHistory=100;

	prefs->maxmsgsessions=ini.GetInt("MaxMessageSessions",50);

	sprintf(prefs->TxtEditor,"%s",ini.GetString("TxtEditor","notepad.exe"));
	sprintf(prefs->VideoPlayer,"%s",ini.GetString("VideoPlayer",""));
	sprintf(prefs->UpdateURLFakeList,"%s",ini.GetString("UpdateURLFakeList","http://www.emuleitor.com/downloads/Morph/fakes.txt"));		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	sprintf(prefs->UpdateURLIPFilter,"%s",ini.GetString("UpdateURLIPFilter","http://www.emuleitor.com/downloads/Morph/ipfilter.txt"));//MORPH START added by Yun.SF3: Ipfilter.dat update

	sprintf(prefs->m_sTemplateFile,"%s",ini.GetString("WebTemplateFile","eMule.tmpl"));

	sprintf(prefs->messageFilter,"%s",ini.GetString("MessageFilter","Your client has an infinite queue"));
	sprintf(prefs->commentFilter,"%s",ini.GetString("CommentFilter","http://|www."));
	sprintf(prefs->filenameCleanups,"%s",ini.GetString("FilenameCleanups","http|www.|.com|shared|powered|sponsored|sharelive|filedonkey|saugstube|eselfilme|eseldownloads|emulemovies|spanishare|eselpsychos.de|saughilfe.de|goldesel.6x.to|freedivx.org|elitedivx|deviance|-ftv|ftv|-flt|flt"));
	prefs->m_iExtractMetaData=ini.GetInt("ExtractMetaData",2); // 0=disable, 1=mp3+avi, 2=MediaDet
		
	prefs->m_bUseSecureIdent=ini.GetBool("SecureIdent",true);
	prefs->m_bAdvancedSpamfilter=ini.GetBool("AdvancedSpamFilter",true);

	// Toolbar
	sprintf(prefs->m_sToolbarSettings,ini.GetString("ToolbarSetting", strDefaultToolbar));
	sprintf(prefs->m_sToolbarBitmap,ini.GetString("ToolbarBitmap", ""));
	sprintf(prefs->m_sToolbarBitmapFolder,ini.GetString("ToolbarBitmapFolder", prefs->incomingdir));
	prefs->m_nToolbarLabels = ini.GetInt("ToolbarLabels",1);
	prefs->m_iStraightWindowStyles=ini.GetInt("StraightWindowStyles",0);
	_sntprintf(prefs->m_szSkinProfile, ARRSIZE(prefs->m_szSkinProfile), "%s", ini.GetString(_T("SkinProfile"), _T("")));
	_sntprintf(prefs->m_szSkinProfileDir, ARRSIZE(prefs->m_szSkinProfileDir), "%s", ini.GetString(_T("SkinProfileDir"), _T("")));

	// khaos::categorymod+ Load Preferences
	prefs->m_bShowCatNames=ini.GetBool("ShowCatName",true);
	prefs->m_bValidSrcsOnly=ini.GetBool("ValidSrcsOnly", false);
	prefs->m_bActiveCatDefault=ini.GetBool("ActiveCatDefault", true);
	prefs->m_bSelCatOnAdd=ini.GetBool("SelCatOnAdd", true);
	prefs->m_bAutoSetResumeOrder=ini.GetBool("AutoSetResumeOrder", true);
	prefs->m_bSmallFileDLPush=ini.GetBool("SmallFileDLPush", true);
	prefs->m_iStartDLInEmptyCats=ini.GetInt("StartDLInEmptyCats", 0);
	prefs->m_bUseAutoCat=ini.GetBool("UseAutoCat", true);
	// khaos::categorymod-
	// khaos::kmod+
	prefs->m_bUseSaveLoadSources=ini.GetBool("UseSaveLoadSources", true);
	prefs->m_bRespectMaxSources=ini.GetBool("RespectMaxSources", true);
	prefs->m_bSmartA4AFSwapping=ini.GetBool("SmartA4AFSwapping", true);
	prefs->m_iAdvancedA4AFMode=ini.GetInt("AdvancedA4AFMode", 1);
	prefs->m_bShowA4AFDebugOutput=ini.GetBool("ShowA4AFDebugOutput", false);
	// khaos::accuratetimerem+
	prefs->m_iTimeRemainingMode=ini.GetInt("TimeRemainingMode", 0);
	// khaos::accuratetimerem-

	//if (prefs->maxGraphDownloadRate<prefs->maxdownload) prefs->maxdownload=UNLIMITED;
	//if (prefs->maxGraphUploadRate<prefs->maxupload) prefs->maxupload=UNLIMITED;

	ini.SerGet(true, prefs->downloadColumnWidths,
		ELEMENT_COUNT(prefs->downloadColumnWidths), "DownloadColumnWidths");
	ini.SerGet(true, prefs->downloadColumnHidden,
		ELEMENT_COUNT(prefs->downloadColumnHidden), "DownloadColumnHidden");
	ini.SerGet(true, prefs->downloadColumnOrder,
		ELEMENT_COUNT(prefs->downloadColumnOrder), "DownloadColumnOrder");
	ini.SerGet(true, prefs->uploadColumnWidths,
		ELEMENT_COUNT(prefs->uploadColumnWidths), "UploadColumnWidths");
	ini.SerGet(true, prefs->uploadColumnHidden,
		ELEMENT_COUNT(prefs->uploadColumnHidden), "UploadColumnHidden");
	ini.SerGet(true, prefs->uploadColumnOrder,
		ELEMENT_COUNT(prefs->uploadColumnOrder), "UploadColumnOrder");
	ini.SerGet(true, prefs->queueColumnWidths,
		ELEMENT_COUNT(prefs->queueColumnWidths), "QueueColumnWidths");
	ini.SerGet(true, prefs->queueColumnHidden,
		ELEMENT_COUNT(prefs->queueColumnHidden), "QueueColumnHidden");
	ini.SerGet(true, prefs->queueColumnOrder,
		ELEMENT_COUNT(prefs->queueColumnOrder), "QueueColumnOrder");
	ini.SerGet(true, prefs->searchColumnWidths,
		ELEMENT_COUNT(prefs->searchColumnWidths), "SearchColumnWidths");
	ini.SerGet(true, prefs->searchColumnHidden,
		ELEMENT_COUNT(prefs->searchColumnHidden), "SearchColumnHidden");
	ini.SerGet(true, prefs->searchColumnOrder,
		ELEMENT_COUNT(prefs->searchColumnOrder), "SearchColumnOrder");
	ini.SerGet(true, prefs->sharedColumnWidths,
		ELEMENT_COUNT(prefs->sharedColumnWidths), "SharedColumnWidths");
	ini.SerGet(true, prefs->sharedColumnHidden,
		ELEMENT_COUNT(prefs->sharedColumnHidden), "SharedColumnHidden");
	ini.SerGet(true, prefs->sharedColumnOrder,
		ELEMENT_COUNT(prefs->sharedColumnOrder), "SharedColumnOrder");
	ini.SerGet(true, prefs->serverColumnWidths,
		ELEMENT_COUNT(prefs->serverColumnWidths), "ServerColumnWidths");
	ini.SerGet(true, prefs->serverColumnHidden,
		ELEMENT_COUNT(prefs->serverColumnHidden), "ServerColumnHidden");
	ini.SerGet(true, prefs->serverColumnOrder,
		ELEMENT_COUNT(prefs->serverColumnOrder), "ServerColumnOrder");
	ini.SerGet(true, prefs->clientListColumnWidths,
		ELEMENT_COUNT(prefs->clientListColumnWidths), "ClientListColumnWidths");
	ini.SerGet(true, prefs->clientListColumnHidden,
		ELEMENT_COUNT(prefs->clientListColumnHidden), "ClientListColumnHidden");
	ini.SerGet(true, prefs->clientListColumnOrder,
		ELEMENT_COUNT(prefs->clientListColumnOrder), "ClientListColumnOrder");

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	// SLUGFILLER: multiSort - load multiple params
	ini.SerGet(true, prefs->tableSortItemDownload,
		ELEMENT_COUNT(prefs->tableSortItemDownload), "TableSortItemDownload", NULL, -1);
	ini.SerGet(true, prefs->tableSortItemUpload,
		ELEMENT_COUNT(prefs->tableSortItemUpload), "TableSortItemUpload", NULL, -1);
	ini.SerGet(true, prefs->tableSortItemQueue,
		ELEMENT_COUNT(prefs->tableSortItemQueue), "TableSortItemQueue", NULL, -1);
	ini.SerGet(true, prefs->tableSortItemSearch,
		ELEMENT_COUNT(prefs->tableSortItemSearch), "TableSortItemSearch", NULL, -1);
	ini.SerGet(true, prefs->tableSortItemShared,
		ELEMENT_COUNT(prefs->tableSortItemShared), "TableSortItemShared", NULL, -1);
	ini.SerGet(true, prefs->tableSortItemServer,
		ELEMENT_COUNT(prefs->tableSortItemServer), "TableSortItemServer", NULL, -1);
	ini.SerGet(true, prefs->tableSortItemClientList,
		ELEMENT_COUNT(prefs->tableSortItemClientList), "TableSortItemClientList", NULL, -1);
	ini.SerGet(true, prefs->tableSortAscendingDownload,
		ELEMENT_COUNT(prefs->tableSortAscendingDownload), "TableSortAscendingDownload");
	ini.SerGet(true, prefs->tableSortAscendingUpload,
		ELEMENT_COUNT(prefs->tableSortAscendingUpload), "TableSortAscendingUpload");
	ini.SerGet(true, prefs->tableSortAscendingQueue,
		ELEMENT_COUNT(prefs->tableSortAscendingQueue), "TableSortAscendingQueue");
	ini.SerGet(true, prefs->tableSortAscendingSearch,
		ELEMENT_COUNT(prefs->tableSortAscendingSearch), "TableSortAscendingSearch");
	ini.SerGet(true, prefs->tableSortAscendingShared,
		ELEMENT_COUNT(prefs->tableSortAscendingShared), "TableSortAscendingShared");
	ini.SerGet(true, prefs->tableSortAscendingServer,
		ELEMENT_COUNT(prefs->tableSortAscendingServer), "TableSortAscendingServer");
	ini.SerGet(true, prefs->tableSortAscendingClientList,
		ELEMENT_COUNT(prefs->tableSortAscendingClientList), "TableSortAscendingClientList");
	// topmost must be valid
	if (prefs->tableSortItemDownload[0] == -1) {
		prefs->tableSortItemDownload[0] = 0;
		prefs->tableSortAscendingDownload[0] = true;
	}
	if (prefs->tableSortItemUpload[0] == -1) {
		prefs->tableSortItemUpload[0] = 0;
		prefs->tableSortAscendingUpload[0] = true;
	}
	if (prefs->tableSortItemQueue[0] == -1) {
		prefs->tableSortItemQueue[0] = 0;
		prefs->tableSortAscendingQueue[0] = true;
	}
	if (prefs->tableSortItemSearch[0] == -1) {
		prefs->tableSortItemSearch[0] = 0;
		prefs->tableSortAscendingSearch[0] = true;
	}
	if (prefs->tableSortItemShared[0] == -1) {
		prefs->tableSortItemShared[0] = 0;
		prefs->tableSortAscendingShared[0] = true;
	}
	if (prefs->tableSortItemServer[0] == -1) {
		prefs->tableSortItemServer[0] = 0;
		prefs->tableSortAscendingServer[0] = true;
	}
	if (prefs->tableSortItemClientList[0] == -1) {
		prefs->tableSortItemClientList[0] = 0;
		prefs->tableSortAscendingClientList[0] = true;
	}
	// SLUGFILLER: multiSort

	LPBYTE pData = NULL;
	UINT uSize = sizeof prefs->m_lfHyperText;
	if (ini.GetBinary("HyperTextFont", &pData, &uSize) && uSize == sizeof prefs->m_lfHyperText)
		memcpy(&prefs->m_lfHyperText, pData, sizeof prefs->m_lfHyperText);
	else
		memset(&prefs->m_lfHyperText, 0, sizeof prefs->m_lfHyperText);
	delete[] pData;

	if (prefs->statsAverageMinutes<1) prefs->statsAverageMinutes=5;

	// deadlake PROXYSUPPORT
	prefs->proxy.EnablePassword = ini.GetBool("ProxyEnablePassword",false,"Proxy");
	prefs->proxy.UseProxy = ini.GetBool("ProxyEnableProxy",false,"Proxy");
	sprintf(buffer,"");
	sprintf(prefs->proxy.name,"%s",ini.GetString("ProxyName",buffer,"Proxy"));
	sprintf(prefs->proxy.password,"%s",ini.GetString("ProxyPassword",buffer,"Proxy"));
	sprintf(prefs->proxy.user,"%s",ini.GetString("ProxyUser",buffer,"Proxy"));
	prefs->proxy.port = ini.GetInt("ProxyPort",1080,"Proxy");
	prefs->proxy.type = ini.GetInt("ProxyType",PROXYTYPE_NOPROXY,"Proxy");
	prefs->m_bIsASCWOP = ini.GetBool("ConnectWithoutProxy",false,"Proxy");
	prefs->m_bShowProxyErrors = ini.GetBool("ShowErrors",false,"Proxy");

	CString buffer2;
	for (int i=0;i<15;i++) {
		buffer2.Format("StatColor%i",i);
		sprintf(buffer,ini.GetString(buffer2,"0","Statistics"));
		prefs->statcolors[i]=_atoi64(buffer);
		if (prefs->statcolors[i]==0) ResetStatsColor(i);
	}

	// -khaos--+++> Load Stats
	// I changed this to a seperate function because it is now also used
	// to load the stats backup and to load stats from preferences.ini.old.
	LoadStats(loadstatsFromOld);
	// <-----khaos-

	// Web Server
	sprintf(prefs->m_sWebPassword,"%s",ini.GetString("Password", "","WebServer"));
	sprintf(prefs->m_sWebLowPassword,"%s",ini.GetString("PasswordLow", ""));
	prefs->m_nWebPort=ini.GetInt("Port", 4711);
	prefs->m_bWebEnabled=ini.GetBool("Enabled", false);
	prefs->m_bWebUseGzip=ini.GetBool("UseGzip", true);
	prefs->m_bWebLowEnabled=ini.GetBool("UseLowRightsUser", false);
	prefs->m_nWebPageRefresh=ini.GetInt("PageRefreshTime", 120);

	prefs->dontcompressavi=ini.GetBool("DontCompressAvi",false);

	// mobilemule
	sprintf(prefs->m_sMMPassword,"%s",ini.GetString("Password", "","MobileMule"));
	prefs->m_bMMEnabled = ini.GetBool("Enabled", false);
	prefs->m_nMMPort = ini.GetInt("Port", 80);

	//MORPH START - Added by SiRoB,  ZZ dynamic upload (USS)
	if (!prefs->m_bSUCEnabled) prefs->m_bDynUpEnabled = ini.GetBool("DynUpEnabled", false);
	prefs->m_bDynUpLog = ini.GetBool("DynUpLog", false);

	prefs->m_iDynUpPingLimit = ini.GetInt("DynUpPingLimit", 200); // EastShare - Added by TAHO, USS limit

	prefs->m_iDynUpPingTolerance = ini.GetInt("DynUpPingTolerance", 800);
	prefs->m_iDynUpGoingUpDivider = ini.GetInt("DynUpGoingUpDivider", 1000);
	prefs->m_iDynUpGoingDownDivider = ini.GetInt("DynUpGoingDownDivider", 1000);
	prefs->m_iDynUpNumberOfPings = ini.GetInt("DynUpNumberOfPings", 1);
	//MORPH END   - Added by SiRoB,  ZZ dynamic upload (USS)

	LoadCats();
	//MORPH - Khaos Obsolete //if (GetCatCount()==1) SetAllcatType(0);

	SetLanguage();
	if (loadstatsFromOld == 2) SavePreferences();
}

// khaos::categorymod+
void CPreferences::LoadCats() {
	CString ixStr,catinif;//,cat_a,cat_b,cat_c;
	//char buffer[100];

	catinif.Format("%sCategory.ini", configdir);
	CIni catini;
	
	bool bCreateDefault = false;
	bool bSkipLoad = false;
	if (!PathFileExists(catinif))
	{
		bCreateDefault = true;
		bSkipLoad = true;
	}
	else
	{
		catini.SetFileName(catinif);
		catini.SetSection("General");
		if (catini.GetInt("CategoryVersion") == 0)
			bCreateDefault = true;
	}

	if (bCreateDefault)
	{
		Category_Struct* defcat=new Category_Struct;

		sprintf(defcat->title,"Default");
		defcat->prio=0;
		defcat->iAdvA4AFMode = 0;
		sprintf(defcat->incomingpath, prefs->incomingdir);
		sprintf(defcat->comment, "The default category.  It can't be merged or deleted.");
		defcat->color = 0;
		defcat->viewfilters.bArchives = true;
		defcat->viewfilters.bAudio = true;
		defcat->viewfilters.bComplete = true;
		defcat->viewfilters.bCompleting = true;
		defcat->viewfilters.bErrorUnknown = true;
		defcat->viewfilters.bHashing = true;
		defcat->viewfilters.bImages = true;
		defcat->viewfilters.bPaused = true;
		defcat->viewfilters.bStopped = true;
		defcat->viewfilters.bSuspendFilters = false;
		defcat->viewfilters.bTransferring = true;
		defcat->viewfilters.bVideo = true;
		defcat->viewfilters.bWaiting = true;
		defcat->viewfilters.nAvailSourceCountMax = 0;
		defcat->viewfilters.nAvailSourceCountMin = 0;
		defcat->viewfilters.nFromCats = 2;
		defcat->viewfilters.nFSizeMax = 0;
		defcat->viewfilters.nFSizeMin = 0;
		defcat->viewfilters.nRSizeMax = 0;
		defcat->viewfilters.nRSizeMin = 0;
		defcat->viewfilters.nSourceCountMax = 0;
		defcat->viewfilters.nSourceCountMin = 0;
		defcat->viewfilters.nTimeRemainingMax = 0;
		defcat->viewfilters.nTimeRemainingMin = 0;
		defcat->viewfilters.sAdvancedFilterMask = "";
		defcat->selectioncriteria.bAdvancedFilterMask = true;
		defcat->selectioncriteria.bFileSize = true;
		AddCat(defcat);
		if (bSkipLoad)
		{
			SaveCats();
			return;
		}
	}

	int max = catini.GetInt("Count", 0, "General");

	for (int ix = bCreateDefault ? 1 : 0; ix <= max; ix++)
	{
		ixStr.Format("Cat#%i",ix);
        catini.SetSection(ixStr);

		Category_Struct* newcat = new Category_Struct;

		sprintf(newcat->title, catini.GetString("Title", ix == 0 ? "Default" : "?"));
		sprintf(newcat->incomingpath, catini.GetString("Incoming", ix == 0 ? CString(prefs->incomingdir) : ""));
		MakeFoldername(newcat->incomingpath);
		sprintf(newcat->comment, catini.GetString("Comment", "", ixStr));
		newcat->prio = catini.GetInt("Priority", 0, ixStr);
		CString sBuff = catini.GetString("Color", "0", ixStr);
		newcat->color = _atoi64(sBuff.GetBuffer());
		// khaos::kmod+ Category Advanced A4AF Mode
		newcat->iAdvA4AFMode = catini.GetInt("AdvancedA4AFMode", 0);
		//newcat->autocat = catini.GetString("AutoCatString","",ixStr);
		// khaos::kmod-
		// Load View Filters
		newcat->viewfilters.nFromCats = catini.GetInt("vfFromCats", ix==0?0:2);
		newcat->viewfilters.bSuspendFilters = false;
		newcat->viewfilters.bVideo = catini.GetBool("vfVideo", true);
		newcat->viewfilters.bAudio = catini.GetBool("vfAudio", true);
		newcat->viewfilters.bArchives = catini.GetBool("vfArchives", true);
		newcat->viewfilters.bImages = catini.GetBool("vfImages", true);
		newcat->viewfilters.bWaiting = catini.GetBool("vfWaiting", true);
		newcat->viewfilters.bTransferring = catini.GetBool("vfTransferring", true);
		newcat->viewfilters.bPaused = catini.GetBool("vfPaused", true);
		newcat->viewfilters.bStopped = catini.GetBool("vfStopped", true);
		newcat->viewfilters.bComplete = catini.GetBool("vfComplete", true);
		newcat->viewfilters.bHashing = catini.GetBool("vfHashing", true);
		newcat->viewfilters.bErrorUnknown = catini.GetBool("vfErrorUnknown", true);
		newcat->viewfilters.bCompleting = catini.GetBool("vfCompleting", true);
		newcat->viewfilters.nFSizeMin = catini.GetInt("vfFSizeMin", 0);
		newcat->viewfilters.nFSizeMax = catini.GetInt("vfFSizeMax", 0);
		newcat->viewfilters.nRSizeMin = catini.GetInt("vfRSizeMin", 0);
		newcat->viewfilters.nRSizeMax = catini.GetInt("vfRSizeMax", 0);
		newcat->viewfilters.nTimeRemainingMin = catini.GetInt("vfTimeRemainingMin", 0);
		newcat->viewfilters.nTimeRemainingMax = catini.GetInt("vfTimeRemainingMax", 0);
		newcat->viewfilters.nSourceCountMin = catini.GetInt("vfSourceCountMin", 0);
		newcat->viewfilters.nSourceCountMax = catini.GetInt("vfSourceCountMax", 0);
		newcat->viewfilters.nAvailSourceCountMin = catini.GetInt("vfAvailSourceCountMin", 0);
		newcat->viewfilters.nAvailSourceCountMax = catini.GetInt("vfAvailSourceCountMax", 0);
		newcat->viewfilters.sAdvancedFilterMask = catini.GetString("vfAdvancedFilterMask", "");
		// Load Selection Criteria
		newcat->selectioncriteria.bFileSize = catini.GetBool("scFileSize", true);
		newcat->selectioncriteria.bAdvancedFilterMask = catini.GetBool("scAdvancedFilterMask", true);

		AddCat(newcat);
		if (!PathFileExists(newcat->incomingpath)) ::CreateDirectory(newcat->incomingpath, 0);
	}
}
// khaos::categorymod-

WORD CPreferences::GetWindowsVersion(){
	static bool bWinVerAlreadyDetected = false;
	if(!bWinVerAlreadyDetected)
	{	
		bWinVerAlreadyDetected = true;
		m_wWinVer = DetectWinVersion();	
	}	
	return m_wWinVer;
}

uint16 CPreferences::GetDefaultMaxConperFive(){
	switch (GetWindowsVersion()){
		case _WINVER_98_:
			return 5;
		case _WINVER_95_:	
		case _WINVER_ME_:
			return MAXCON5WIN9X;
		case _WINVER_2K_:
		case _WINVER_XP_:
			return MAXCONPER5SEC;
		default:
			return MAXCONPER5SEC;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
int CPreferences::GetColumnSortItem(Table t, int column) const	// SLUGFILLER: multiSort
{
	switch(t) 
	{
		case tableDownload:
			return prefs->tableSortItemDownload[column];	// SLUGFILLER: multiSort
		case tableUpload:
			return prefs->tableSortItemUpload[column];	// SLUGFILLER: multiSort
		case tableQueue:
			return prefs->tableSortItemQueue[column];	// SLUGFILLER: multiSort
		case tableSearch:
			return prefs->tableSortItemSearch[column];	// SLUGFILLER: multiSort
		case tableShared:
			return prefs->tableSortItemShared[column];	// SLUGFILLER: multiSort
		case tableServer:
			return prefs->tableSortItemServer[column];	// SLUGFILLER: multiSort
		case tableClientList:
			return prefs->tableSortItemClientList[column];	// SLUGFILLER: multiSort
	}
	return 0;
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
bool CPreferences::GetColumnSortAscending(Table t, int column) const	// SLUGFILLER: multiSort
{
	switch(t) 
	{
		case tableDownload:
			return prefs->tableSortAscendingDownload[column];	// SLUGFILLER: multiSort
		case tableUpload:
			return prefs->tableSortAscendingUpload[column];	// SLUGFILLER: multiSort
		case tableQueue:
			return prefs->tableSortAscendingQueue[column];	// SLUGFILLER: multiSort
		case tableSearch:
			return prefs->tableSortAscendingSearch[column];	// SLUGFILLER: multiSort
		case tableShared:
			return prefs->tableSortAscendingShared[column];	// SLUGFILLER: multiSort
		case tableServer:
			return prefs->tableSortAscendingServer[column];	// SLUGFILLER: multiSort
		case tableClientList:
			return prefs->tableSortAscendingClientList[column];	// SLUGFILLER: multiSort
	}
	return true;
}

// SLUGFILLER: multiSort
int CPreferences::GetColumnSortCount(Table t) const
{
	int i;
	switch(t)
	{
		case tableDownload:
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemDownload); i++)
				if (prefs->tableSortItemDownload[i] == -1)
					break;
			return i;
		case tableUpload:
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemUpload); i++)
				if (prefs->tableSortItemUpload[i] == -1)
					break;
			return i;
		case tableQueue:
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemQueue); i++)
				if (prefs->tableSortItemQueue[i] == -1)
					break;
			return i;
		case tableSearch:
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemSearch); i++)
				if (prefs->tableSortItemSearch[i] == -1)
					break;
			return i;
		case tableShared:
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemShared); i++)
				if (prefs->tableSortItemShared[i] == -1)
					break;
			return i;
		case tableServer:
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemServer); i++)
				if (prefs->tableSortItemServer[i] == -1)
					break;
			return i;
		case tableClientList:
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemClientList); i++)
				if (prefs->tableSortItemClientList[i] == -1)
					break;
			return i;
	}
	return 0;
}
// SLUGFILLER: multiSort

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortItem(Table t, int sortItem)
{
	int i;	// SLUGFILLER: multiSort - roll params
	switch(t) 
	{
		case tableDownload:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemDownload)-1; i++)
				if (prefs->tableSortItemDownload[i] == sortItem ||
					prefs->tableSortItemDownload[i] == -1)
					break;
			for (; i > 0; i--) {
				prefs->tableSortItemDownload[i] = prefs->tableSortItemDownload[i-1];
				prefs->tableSortAscendingDownload[i] = prefs->tableSortAscendingDownload[i-1];
			}
			prefs->tableSortItemDownload[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableUpload:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemUpload)-1; i++)
				if (prefs->tableSortItemUpload[i] == sortItem ||
					prefs->tableSortItemUpload[i] == -1)
					break;
			for (; i > 0; i--) {
				prefs->tableSortItemUpload[i] = prefs->tableSortItemUpload[i-1];
				prefs->tableSortAscendingUpload[i] = prefs->tableSortAscendingUpload[i-1];
			}
			prefs->tableSortItemUpload[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableQueue:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemQueue)-1; i++)
				if (prefs->tableSortItemQueue[i] == sortItem ||
					prefs->tableSortItemQueue[i] == -1)
					break;
			for (; i > 0; i--) {
				prefs->tableSortItemQueue[i] = prefs->tableSortItemQueue[i-1];
				prefs->tableSortAscendingQueue[i] = prefs->tableSortAscendingQueue[i-1];
			}
			prefs->tableSortItemQueue[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableSearch:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemSearch)-1; i++)
				if (prefs->tableSortItemSearch[i] == sortItem ||
					prefs->tableSortItemSearch[i] == -1)
					break;
			for (; i > 0; i--) {
				prefs->tableSortItemSearch[i] = prefs->tableSortItemSearch[i-1];
				prefs->tableSortAscendingSearch[i] = prefs->tableSortAscendingSearch[i-1];
			}
			prefs->tableSortItemSearch[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableShared:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemShared)-1; i++)
				if (prefs->tableSortItemShared[i] == sortItem ||
					prefs->tableSortItemShared[i] == -1)
					break;
			for (; i > 0; i--) {
				prefs->tableSortItemShared[i] = prefs->tableSortItemShared[i-1];
				prefs->tableSortAscendingShared[i] = prefs->tableSortAscendingShared[i-1];
			}
			prefs->tableSortItemShared[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableServer:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemServer)-1; i++)
				if (prefs->tableSortItemServer[i] == sortItem ||
					prefs->tableSortItemServer[i] == -1)
					break;
			for (; i > 0; i--) {
				prefs->tableSortItemServer[i] = prefs->tableSortItemServer[i-1];
				prefs->tableSortAscendingServer[i] = prefs->tableSortAscendingServer[i-1];
			}
			prefs->tableSortItemServer[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableClientList:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ELEMENT_COUNT(prefs->tableSortItemClientList)-1; i++)
				if (prefs->tableSortItemClientList[i] == sortItem ||
					prefs->tableSortItemClientList[i] == -1)
					break;
			for (; i > 0; i--) {
				prefs->tableSortItemClientList[i] = prefs->tableSortItemClientList[i-1];
				prefs->tableSortAscendingClientList[i] = prefs->tableSortAscendingClientList[i-1];
			}
			prefs->tableSortItemClientList[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortAscending(Table t, bool sortAscending)
{
	switch(t) 
	{
		case tableDownload:
			prefs->tableSortAscendingDownload[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableUpload:
			prefs->tableSortAscendingUpload[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableQueue:
			prefs->tableSortAscendingQueue[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableSearch:
			prefs->tableSortAscendingSearch[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableShared:
			prefs->tableSortAscendingShared[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableServer:
			prefs->tableSortAscendingServer[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableClientList:
			prefs->tableSortAscendingClientList[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
	}
}

void CPreferences::RemoveCat(int index)	{
	if (index>=0 && index<catMap.GetCount()) { 
		Category_Struct* delcat;
		delcat=catMap.GetAt(index); 
		catMap.RemoveAt(index); 
		delete delcat;
	}
}

bool CPreferences::MoveCat(UINT from, UINT to){
	if (from>=(UINT)catMap.GetCount() || to >=(UINT)catMap.GetCount()+1 || from==to) return false;

	Category_Struct* tomove;

	tomove=catMap.GetAt(from);

	if (from < to) {
		catMap.RemoveAt(from);
		catMap.InsertAt(to-1,tomove);
	} else {
		catMap.InsertAt(to,tomove);
		catMap.RemoveAt(from+1);
	}
	
	SaveCats();

	return true;
}

bool CPreferences::IsInstallationDirectory(const CString& rstrDir) const
{
	CString strFullPath;
	if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
		strFullPath.ReleaseBuffer();
	else
		strFullPath = rstrDir;
	
	// skip sharing of several special eMule folders
	if (!CompareDirectories(strFullPath, GetAppDir()))			// ".\eMule"
		return true;
	if (!CompareDirectories(strFullPath, GetConfigDir()))		// ".\eMule\config"
		return true;
	if (!CompareDirectories(strFullPath, GetWebServerDir()))	// ".\eMule\webserver"
		return true;
	if (!CompareDirectories(strFullPath, GetLangDir()))			// ".\eMule\lang"
		return true;

	return false;
}

bool CPreferences::IsShareableDirectory(const CString& rstrDir) const
{
	if (IsInstallationDirectory(rstrDir))
		return false;

	CString strFullPath;
	if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
		strFullPath.ReleaseBuffer();
	else
		strFullPath = rstrDir;
	
	// skip sharing of several special eMule folders
	if (!CompareDirectories(strFullPath, GetTempDir()))			// ".\eMule\temp"
		return false;

	return true;
}

//MORPH START - Added by IceCream, high process priority
void CPreferences::SetEnableHighProcess(bool enablehigh) 			
{
	enableHighProcess = enablehigh;
	if (enablehigh)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
}
//MORPH END   - Added by IceCream, high process priority
