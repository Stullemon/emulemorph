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
#include <math.h>
#include "emule.h"
#include "ClientCredits.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "Opcodes.h"
#include "Sockets.h"
#include <crypto51/base64.h>
#include <crypto51/osrng.h>
#include <crypto51/files.h>
#include <crypto51/sha.h>
#include "emuledlg.h"
#include "Log.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define CLIENTS_MET_FILENAME	_T("clients.met")

CClientCredits::CClientCredits(CreditStruct* in_credits)
{
	// Moonlight: Dynamic ClientStruct - adjust structure element offsets.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	m_pCredits = in_credits;
	InitalizeIdent();
	m_dwUnSecureWaitTime = 0;
	m_dwSecureWaitTime = 0;
	m_dwWaitTimeIP = 0;

	//Morph Start - Added by AndCycle, reduce a little CPU usage for ratio count
	m_bCheckScoreRatio = true;
	//Removed by SiRoB , for speedup creditfile load
	/*
	m_fLastScoreRatio = 0;
	m_cssCurrentCreditSystem = thePrefs.GetCreditSystem();
	*/
	//Morph End - Added by AndCycle, reduce a little CPU usage for ratio count

	InitPayBackFirstStatus();//EastShare - added by AndCycle, Pay Back First
}

CClientCredits::CClientCredits(const uchar* key)
{
	m_pCredits = new CreditStruct;
	memset(m_pCredits, 0, sizeof(CreditStruct));
	md4cpy(m_pCredits->abyKey, key);
	InitalizeIdent();

	// EastShare START - Modified by TAHO, modified SUQWT
	//m_dwUnSecureWaitTime = ::GetTickCount();
	//m_dwSecureWaitTime = ::GetTickCount();
	m_dwUnSecureWaitTime = 0;
	m_dwSecureWaitTime = 0;
	// EastShare END - Modified by TAHO, modified SUQWT

	m_dwWaitTimeIP = 0;

	//Morph Start - Added by AndCycle, reduce a little CPU usage for ratio count
	m_bCheckScoreRatio = true;
	//Removed by SiRoB , for speedup creditfile load
	/*
	m_fLastScoreRatio = 0;
	m_cssCurrentCreditSystem = thePrefs.GetCreditSystem();
	*/
	//Morph End - Added by AndCycle, reduce a little CPU usage for ratio count

	InitPayBackFirstStatus();//EastShare - added by AndCycle, Pay Back First
}

CClientCredits::~CClientCredits()
{
	delete m_pCredits;
}

void CClientCredits::AddDownloaded(uint32 bytes, uint32 dwForIP) {
	if ( ( GetCurrentIdentState(dwForIP) == IS_IDFAILED || GetCurrentIdentState(dwForIP) == IS_IDBADGUY || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable() ){
		return;
	}
	//encode
	uint64 current=m_pCredits->nDownloadedHi;
	current=(current<<32)+ m_pCredits->nDownloadedLo + bytes ;

	//recode
	m_pCredits->nDownloadedLo=(uint32)current;
	m_pCredits->nDownloadedHi=(uint32)(current>>32);

	//is it good to refresh PayBackFirst status here?
	TestPayBackFirstStatus();//EastShare - added by AndCycle, Pay Back First

	m_bCheckScoreRatio = true;//Morph - Added by AndCycle, reduce a little CPU usage for ratio count
}

void CClientCredits::AddUploaded(uint32 bytes, uint32 dwForIP) {
	if ( ( GetCurrentIdentState(dwForIP) == IS_IDFAILED || GetCurrentIdentState(dwForIP) == IS_IDBADGUY || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable() ){
		return;
	}
	//encode
	uint64 current=m_pCredits->nUploadedHi;
	current=(current<<32)+ m_pCredits->nUploadedLo + bytes ;

	//recode
	m_pCredits->nUploadedLo=(uint32)current;
	m_pCredits->nUploadedHi=(uint32)(current>>32);

	//is it good to refresh PayBackFirst status here?
	TestPayBackFirstStatus();//EastShare - added by AndCycle, Pay Back First

	m_bCheckScoreRatio = true;//Morph - Added by AndCycle, reduce a little CPU usage for ratio count
}

uint64	CClientCredits::GetUploadedTotal() const{
	return ( (uint64)m_pCredits->nUploadedHi<<32)+m_pCredits->nUploadedLo;
}

uint64	CClientCredits::GetDownloadedTotal() const{
	return ( (uint64)m_pCredits->nDownloadedHi<<32)+m_pCredits->nDownloadedLo;
}

float CClientCredits::GetScoreRatio(uint32 dwForIP) /*const*/
{
	// check the client ident status
	if ( ( GetCurrentIdentState(dwForIP) == IS_IDFAILED || GetCurrentIdentState(dwForIP) == IS_IDBADGUY || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable() ){
		// bad guy - no credits for you
		return 1;
	}

	//Morph Start - Added by AndCycle, reduce a little CPU usage for ratio count
	if(m_cssCurrentCreditSystem != thePrefs.GetCreditSystem()){
		m_cssCurrentCreditSystem = thePrefs.GetCreditSystem();
	}else if(m_bCheckScoreRatio == false){//only refresh ScoreRatio when really need
		return m_fLastScoreRatio;
	}
	m_bCheckScoreRatio = false;
	//Morph End - Added by AndCycle, reduce a little CPU usage for ratio count

	//Morph Start - Modified by AndCycle, reduce a little CPU usage for ratio count

	float result = 0;//everybody share one result.
    //EastShare START - Added by linekin, CreditSystem 
	switch (m_cssCurrentCreditSystem)	{

		// EastShare - Added by linekin, lovelace Credit
		case CS_LOVELACE:{
			// new creditsystem by [lovelace]
			double cl_up,cl_down; 

			cl_up = GetUploadedTotal()/(double)1048576;
			cl_down = GetDownloadedTotal()/(double)1048576;
			result=(float)(3.0 * cl_down * cl_down - cl_up * cl_up);
			if (fabs(result)>20000.0f) 
				result*=20000.0f/fabs(result);
			result=100.0f*powf((float)(1-1/(1.0f+expf(result*0.001))),6.6667f);
			if (result<0.1f) 
				result=0.1f;
			if (result>10.0f && IdentState == IS_NOTAVAILABLE)
				result=10.0f;
			// end new creditsystem by [lovelace]
		}break;

		//EastShare Start - added by AndCycle, Pawcio credit
		case CS_PAWCIO:{	
			//Pawcio: Credits
			if ((GetDownloadedTotal() < 1000000)&&(GetUploadedTotal() > 1000000)){
				result = 1.0f;
				break;
			}
			else if ((GetDownloadedTotal() < 1000000)&&(GetUploadedTotal()<1000000)) {
				result = 3.0f;
				break;
			}
			result = 0;
			if (GetUploadedTotal()<1000000)
				result = 10.0f * GetDownloadedTotal()/1000000.0f;
			else
				result = (float)(GetDownloadedTotal()*3)/GetUploadedTotal();
			if ((GetDownloadedTotal() > 100000000)&&(GetUploadedTotal()<GetDownloadedTotal()+8000000)&&(result<50)) result=50;
			else if ((GetDownloadedTotal() > 50000000)&&(GetUploadedTotal()<GetDownloadedTotal()+5000000)&&(result<25)) result=25;
			else if ((GetDownloadedTotal() > 25000000)&&(GetUploadedTotal()<GetDownloadedTotal()+3000000)&&(result<12)) result=12;
			else if ((GetDownloadedTotal() > 10000000)&&(GetUploadedTotal()<GetDownloadedTotal()+2000000)&&(result<5)) result=5;
			if (result > 100.0f){
				result = 100.0f;
				break;
			}
			if (result < 1.0f){
				result = 1.0f;
				break;
			}
		}break;
		//EastShare End - added by AndCycle, Pawcio credit

		// EastShare START - Added by TAHO, new Credit System //Modified by Pretender
		case CS_EASTSHARE:{
			result = (IdentState == IS_NOTAVAILABLE) ? 80 : 100;
			
			result += (float)((double)GetDownloadedTotal()/174762.67 - (double)GetUploadedTotal()/524288); //Modefied by Pretender - 20040120
			
			if ((double)GetDownloadedTotal() > 1048576) {
				result += 100; 
				if (result<50 && ((double)GetDownloadedTotal()*10 > (double)GetUploadedTotal())) result=50;
				} //Modefied by Pretender - 20040330

			if ( result < 10 ) {
				result = 10;
			}else if ( result > 5000 ) {
				result = 5000;
			}
			result = result / 100;

		}break;
		// EastShare END - Added by TAHO, new Credit System

		case CS_OFFICIAL:
		default:{
			if (GetDownloadedTotal() < 1000000){
				result = 1;
				break;
			}
			if (!GetUploadedTotal())
				result = 10;
			else
				result = (float)(((double)GetDownloadedTotal()*2.0)/(double)GetUploadedTotal());
			float result2 = 0;
			result2 = (float)GetDownloadedTotal()/1048576.0;
			result2 += 2;
			result2 = (double)sqrt((double)result2);

			if (result > result2)
				result = result2;

			if (result < 1){
				result = 1;
				break;
			}else if (result > 10){
				result = 10;
				break;
			}
		}break;
	}

	return m_fLastScoreRatio = result;
	//EastShare END - Added by linekin, CreditSystem 

	//EastShare END - Added by linekin, CreditSystem 
}

//MORPH START - Added by IceCream, VQB: ownCredits
float CClientCredits::GetMyScoreRatio(uint32 dwForIP) const
{
	// check the client ident status
	if ( ( GetCurrentIdentState(dwForIP) == IS_IDFAILED  || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable() ){
		// bad guy - no credits for... me?
		return 1;
	}

	if (GetUploadedTotal() < 1000000)
		return 1;
	float result = 0;
	if (!GetDownloadedTotal())
		result = 10;
	else
		result = (float)(((double)GetUploadedTotal()*2.0)/(double)GetDownloadedTotal());
	float result2 = 0;
	result2 = (float)GetUploadedTotal()/1048576.0;
	result2 += 2;
	result2 = (double)sqrt((double)result2);

	if (result > result2)
		result = result2;

	if (result < 1)
		return 1;
	else if (result > 10)
		return 10;
	return result;
}
//MORPH END   - Added by IceCream, VQB: ownCredits

//Morph Start - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
// Moonlight: SUQWT - Conditions to determine an active record.
// Returns true if the client has been seen recently
bool CClientCredits::IsActive(uint32 dwExpired) {
	return (GetUploadedTotal() || GetDownloadedTotal() || m_pCredits->nSecuredWaitTime || m_pCredits->nUnSecuredWaitTime) &&
			(m_pCredits->nLastSeen >= dwExpired);
}
//Morph End - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)

CClientCreditsList::CClientCreditsList()
{
	m_nLastSaved = ::GetTickCount();
	LoadList();
	
	InitalizeCrypting();
}

CClientCreditsList::~CClientCreditsList()
{
	SaveList();
	CClientCredits* cur_credit;
	CCKey tmpkey(0);
	POSITION pos = m_mapClients.GetStartPosition();
	while (pos){
		m_mapClients.GetNextAssoc(pos, tmpkey, cur_credit);
		delete cur_credit;
	}
		delete m_pSignkey;
}

// Moonlight: SUQWT: Change the file import 0.30c format.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
void CClientCreditsList::LoadList()
{
	CString strFileName = thePrefs.GetConfigDir() + CString(CLIENTS_MET_FILENAME);
	const int iOpenFlags = CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite;
	CSafeBufferedFile file;
	CFileException fexp;

	m_bSaveUploadQueueWaitTime = thePrefs.SaveUploadQueueWaitTime();//Morph - added by AndCycle, Save Upload Queue Wait Time (SUQWT)
	//Morph Start - added by AndCycle, choose .met to load

	CSafeBufferedFile	loadFile;

	const int	totalLoadFile = 9;

	CString		loadFileName[totalLoadFile];
	CFileStatus	loadFileStatus[totalLoadFile];
	bool		successLoadFile[totalLoadFile];

	int	countFile = 0;

	//SUQWTv2.met must have bigger number than original clients.met to have higher prio
	loadFileName[countFile++].Format(_T("%s") CLIENTS_MET_FILENAME, thePrefs.GetConfigDir());
	loadFileName[countFile++].Format(_T("%s") CLIENTS_MET_FILENAME _T(".bak"), thePrefs.GetConfigDir());
	loadFileName[countFile++].Format(_T("%s") CLIENTS_MET_FILENAME _T(".MSUQWT"), thePrefs.GetConfigDir());//Pawcio
	loadFileName[countFile++].Format(_T("%s") CLIENTS_MET_FILENAME _T(".SUQWTv2.met"), thePrefs.GetConfigDir());
	loadFileName[countFile++].Format(_T("%s") CLIENTS_MET_FILENAME _T(".SUQWTv2.met.bak"), thePrefs.GetConfigDir());
	loadFileName[countFile++].Format(_T("%s") CLIENTS_MET_FILENAME, thePrefs.GetConfigDir()+_T("Backup\\"));
	loadFileName[countFile++].Format(_T("%s") CLIENTS_MET_FILENAME _T(".SUQWTv2.met"), thePrefs.GetConfigDir()+_T("Backup\\"));
	loadFileName[countFile++].Format(_T("%s") CLIENTS_MET_FILENAME, thePrefs.GetConfigDir()+_T("Backup2\\"));
	loadFileName[countFile++].Format(_T("%s") CLIENTS_MET_FILENAME _T(".SUQWTv2.met"), thePrefs.GetConfigDir()+_T("Backup2\\"));
	//totalLoadFile = 9;
	uint8 prioOrderfile[totalLoadFile];

	int	index = 0;
	for(int curFile = 0; curFile < totalLoadFile; curFile++){
		//check clients.met status
		successLoadFile[curFile] = loadFile.Open(loadFileName[curFile], iOpenFlags, &fexp);
		if (successLoadFile[curFile]){
			loadFile.GetStatus(loadFileStatus[curFile]);
			prioOrderfile[index++]=curFile;
			loadFile.Close();
		}
	}
	uint8 tmpprioOrderfile;
	uint8 maxavailablefile = index;
	while (index-- > 0){
		for (uint8 i=1; i<index;i++)
		{
			if(m_bSaveUploadQueueWaitTime)
				if(loadFileStatus[prioOrderfile[i-1]].m_mtime > loadFileStatus[prioOrderfile[i]].m_mtime)
					continue;
			else
				if(loadFileStatus[prioOrderfile[i-1]].m_mtime >= loadFileStatus[prioOrderfile[i]].m_mtime)
					continue;
			tmpprioOrderfile = prioOrderfile[i-1];
			prioOrderfile[i-1] = prioOrderfile[i];
			prioOrderfile[i] = tmpprioOrderfile;
		}
	}

	for (uint8 i=0;i<maxavailablefile;i++)
	{
		strFileName = loadFileName[prioOrderfile[i]];
		AddLogLine(false, _T("Load %s"), strFileName);
	//Morph End - added by AndCycle, choose .met to load
	//MORPH END  - Changed by SiRoB, Allternative choose .met to load

		if (!file.Open(strFileName, iOpenFlags, &fexp)){
			if (fexp.m_cause != CFileException::fileNotFound){
				CString strError(GetResString(IDS_ERR_LOADCREDITFILE));
				TCHAR szError[MAX_CFEXP_ERRORMSG];
				if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
					strError += _T(" - ");
					strError += szError;
				}
			LogError(LOG_STATUSBAR, _T("%s"), strError);
			}
			//MORPH START - Changed by SiRoB, Allternative choose .met to load
			/*
            return;
			*/
			continue;
			//MORPH END  - Changed by SiRoB, Allternative choose .met to load
		}
		setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
		
		try{
			uint8 version = file.ReadUInt8();
			//Morph Start - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
			/*
			if (version != CREDITFILE_VERSION && version != CREDITFILE_VERSION_29){
				LogWarning(GetResString(IDS_ERR_CREDITFILEOLD));
				file.Close();
				return;
			}
			*/
			// Moonlight: SUQWT - Import CreditStruct from 0.30c and SUQWTv1
			if (version != CREDITFILE_VERSION_30_SUQWTv1 && version != CREDITFILE_VERSION_30_SUQWTv2 &&
				version != CREDITFILE_VERSION_30 && version != CREDITFILE_VERSION_29){
				LogWarning(GetResString(IDS_ERR_CREDITFILEOLD));
				file.Close();
				//MORPH START - Changed by SiRoB, Allternative choose .met to load
				/*
				return;
				*/
				continue;
				//MORPH END  - Changed by SiRoB, Allternative choose .met to load
			}
			//Morph End - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)

			// everything is ok, lets see if the backup exist...
			CString strBakFileName;
			//Morph start - modify by AndCycle, backup loaded file
			/*
			strBakFileName.Format(_T("%s") CLIENTS_MET_FILENAME _T(".BAK"), thePrefs.GetConfigDir());
			*/
			strBakFileName.Format(_T("%s") _T(".BAK"), strFileName);
			//Morph end - modify by AndCycle, backup loaded file

			DWORD dwBakFileSize = 0;
			BOOL bCreateBackup = TRUE;

			HANDLE hBakFile = ::CreateFile(strBakFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
											OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hBakFile != INVALID_HANDLE_VALUE)
			{
				// Ok, the backup exist, get the size
				dwBakFileSize = ::GetFileSize(hBakFile, NULL); //debug
				if (dwBakFileSize > (DWORD)file.GetLength())
				{
					// the size of the backup was larger then the org. file, something is wrong here, don't overwrite old backup..
					bCreateBackup = FALSE;
				}
				//else: backup is smaller or the same size as org. file, proceed with copying of file
				::CloseHandle(hBakFile);
			}
			//else: the backup doesn't exist, create it

			if (bCreateBackup)
			{
				file.Close(); // close the file before copying

				if (!::CopyFile(strFileName, strBakFileName, FALSE))
					LogError(GetResString(IDS_ERR_MAKEBAKCREDITFILE));

				// reopen file
				CFileException fexp;
				if (!file.Open(strFileName, iOpenFlags, &fexp)){
					CString strError(GetResString(IDS_ERR_LOADCREDITFILE));
					TCHAR szError[MAX_CFEXP_ERRORMSG];
					if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
						strError += _T(" - ");
						strError += szError;
					}
					LogError(LOG_STATUSBAR, _T("%s"), strError);
					continue; //MORPH - Continue loading files
				}
				setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
				file.Seek(1, CFile::begin); //set filepointer behind file version byte
			}

			UINT count = file.ReadUInt32();
			m_mapClients.InitHashTable(count+5000); // TODO: should be prime number... and 20% larger

			const uint32 dwExpired = time(NULL) - 12960000; // today - 150 day
			uint32 cDeleted = 0;
			//Morph Start - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
			CreditStruct* newcstruct;
			//Morph End   - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)

			for (UINT i = 0; i < count; i++){
				//Morph Start - Changed by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
				/*
				CreditStruct* newcstruct = new CreditStruct;
				*/
				newcstruct = new CreditStruct;
				//Morph End   - Changed by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
				
				memset(newcstruct, 0, sizeof(CreditStruct));
				//Morph Start - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
				/*
				if (version == CREDITFILE_VERSION_29)
					file.Read(newcstruct, sizeof(CreditStruct_29a));
				else
					file.Read(newcstruct, sizeof(CreditStruct));
				*/
				// --> Moonlight: SUQWT - import 0.30c and 30c-SUQWTv1 structures.
				if (version == CREDITFILE_VERSION)
					file.Read(newcstruct, sizeof(CreditStruct_30c_SUQWTv2));
				else if (version == CREDITFILE_VERSION_30_SUQWTv1) {
					file.Read(((uint8*)newcstruct) + 8, sizeof(CreditStruct_30c_SUQWTv1) - 8);
					file.Read(((uint8*)newcstruct), 8);
				}
				else if (version == CREDITFILE_VERSION_30)
					file.Read(((uint8*)newcstruct) + 8, sizeof(CreditStruct_30c));
				else
					file.Read(((uint8*)newcstruct) + 8, sizeof(CreditStruct_29a));
				// <-- Moonlight: SUQWT			
				//Morph End - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
			
				if (newcstruct->nLastSeen < dwExpired){
					cDeleted++;
					delete newcstruct;
					continue;
				}

				CClientCredits* newcredits = new CClientCredits(newcstruct);
				m_mapClients.SetAt(CCKey(newcredits->GetKey()), newcredits);
			}
			file.Close();

			if (cDeleted>0)
				AddLogLine(false, GetResString(IDS_CREDITFILELOADED) + GetResString(IDS_CREDITSEXPIRED), count-cDeleted,cDeleted);
			else
				AddLogLine(false, GetResString(IDS_CREDITFILELOADED), count);
			
			//We got a valide Credit file so exit now
			break; //Added by SiRoB
		}
		catch(CFileException* error){
			if (error->m_cause == CFileException::endOfFile)
			LogError(LOG_STATUSBAR, GetResString(IDS_CREDITFILECORRUPT));
			else{
				TCHAR buffer[MAX_CFEXP_ERRORMSG];
				error->GetErrorMessage(buffer, ARRSIZE(buffer));
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_CREDITFILEREAD), buffer);
			}
			error->Delete();
		}
	}//MORPH - Added by SiRoB, Alternative choose .met to load
}

// Moonlight: SUQWT - Save the wait times before saving the list.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
void CClientCreditsList::SaveList()
{
	if (thePrefs.GetLogFileSaving())
		AddDebugLogLine(false, _T("Saving clients credit list file \"%s\""), CLIENTS_MET_FILENAME);
	m_nLastSaved = ::GetTickCount();

	CString name = thePrefs.GetConfigDir() + CString(CLIENTS_MET_FILENAME);
	CFile file;// no buffering needed here since we swap out the entire array
	CFileException fexp;
	//Morph - modified by AndCycle, SUQWT save in client.met.SUQWTv2.met
	/*
	if (!file.Open(name, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
	*/
	if (!file.Open(name+_T(".SUQWTv2.met"), CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		CString strError(GetResString(IDS_ERR_FAILED_CREDITSAVE));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		return;
	}

	//Morph Start - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	CFile fileBack; // Moonlight: SUQWT - Also open a file to save original 30c format.
	if (!fileBack.Open(name, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){//Morph - modified by AndCycle, SUQWT save in client.met.SUQWTv2.met
		CString strError(GetResString(IDS_ERR_FAILED_CREDITSAVE));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		return;
	}
	//Morph End - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)

	uint32 count = m_mapClients.GetCount();
	BYTE* pBuffer = new BYTE[count*sizeof(CreditStruct)];
	CClientCredits* cur_credit;
	CCKey tempkey(0);
	POSITION pos = m_mapClients.GetStartPosition();
	//Morph Start - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	BYTE* pBuffer30c = new BYTE[count*sizeof(CreditStruct_30c)];
	const uint32 dwExpired = time(NULL) - 12960000; // today - 150 day
	//Morph End   - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	count = 0;
	while (pos)
	{
		m_mapClients.GetNextAssoc(pos, tempkey, cur_credit);
		//Morph Start - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
		/*
		if (cur_credit->GetUploadedTotal() || cur_credit->GetDownloadedTotal())
		{
			memcpy(pBuffer+(count*sizeof(CreditStruct)), cur_credit->GetDataStruct(), sizeof(CreditStruct));
			count++; 
		}
		*/
		if(theApp.clientcredits->IsSaveUploadQueueWaitTime()){
			if (cur_credit->IsActive(dwExpired))	// Moonlight: SUQWT - Also save records if there is wait time.
			{
				cur_credit->SaveUploadQueueWaitTime();	// Moonlight: SUQWT
				memcpy(pBuffer+(count*sizeof(CreditStruct)), cur_credit->GetDataStruct(), sizeof(CreditStruct));
				memcpy(pBuffer30c+(count*sizeof(CreditStruct_30c)), (uint8 *)cur_credit->GetDataStruct() + 8, sizeof(CreditStruct_30c));	// Moonlight: SUQWT - Save 0.30c CreditStruct
				count++; 
			}
		}else{
			//official way to clean out client
			if (cur_credit->GetUploadedTotal() || cur_credit->GetDownloadedTotal())
			{
				cur_credit->ClearUploadQueueWaitTime();//fair to all client
				memcpy(pBuffer+(count*sizeof(CreditStruct)), cur_credit->GetDataStruct(), sizeof(CreditStruct));
				memcpy(pBuffer30c+(count*sizeof(CreditStruct_30c)), (uint8 *)cur_credit->GetDataStruct() + 8, sizeof(CreditStruct_30c));	// Moonlight: SUQWT - Save 0.30c CreditStruct
				count++; 
			}
		}
		//Morph End - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	}

	try{
		//Morph Start - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
		uint8 version30c = CREDITFILE_VERSION_30;//Morph - modified by AndCycle, corrected version
		fileBack.SeekToBegin();
		fileBack.Write(&version30c, 1);
		fileBack.Write(&count, 4);
		fileBack.Write(pBuffer30c, count*sizeof(CreditStruct_30c));
		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning()))
			fileBack.Flush();
		fileBack.Close();
		delete[] pBuffer30c;
		//Morph End - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)

		uint8 version = CREDITFILE_VERSION;
		file.Write(&version, 1);
		file.Write(&count, 4);
		file.Write(pBuffer, count*sizeof(CreditStruct));
		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning()))
			file.Flush();
		file.Close();
	}
	catch(CFileException* error){
		CString strError(GetResString(IDS_ERR_FAILED_CREDITSAVE));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		error->Delete();
	}

	delete[] pBuffer;
}

CClientCredits* CClientCreditsList::GetCredit(const uchar* key)
{
	CClientCredits* result;
	CCKey tkey(key);
	if (!m_mapClients.Lookup(tkey, result)){
		result = new CClientCredits(key);
		m_mapClients.SetAt(CCKey(result->GetKey()), result);
	}
	result->SetLastSeen();
	return result;
}

void CClientCreditsList::Process()
{
	if (::GetTickCount() - m_nLastSaved > MIN2MS(13))
		SaveList();
}

void CClientCredits::InitalizeIdent(){
	if (m_pCredits->nKeySize == 0 ){
		memset(m_abyPublicKey,0,80); // for debugging
		m_nPublicKeyLen = 0;
		IdentState = IS_NOTAVAILABLE;
	}
	else{
		m_nPublicKeyLen = m_pCredits->nKeySize;
		memcpy(m_abyPublicKey, m_pCredits->abySecureIdent, m_nPublicKeyLen);
		IdentState = IS_IDNEEDED;
	}
	m_dwCryptRndChallengeFor = 0;
	m_dwCryptRndChallengeFrom = 0;
	m_dwIdentIP = 0;
}

void CClientCredits::Verified(uint32 dwForIP){
	m_dwIdentIP = dwForIP;
	// client was verified, copy the keyto store him if not done already
	if (m_pCredits->nKeySize == 0){
		m_pCredits->nKeySize = m_nPublicKeyLen; 
		memcpy(m_pCredits->abySecureIdent, m_abyPublicKey, m_nPublicKeyLen);
		if (GetDownloadedTotal() > 0){
			// for security reason, we have to delete all prior credits here
			m_pCredits->nDownloadedHi = 0;
			m_pCredits->nDownloadedLo = 1;
			m_pCredits->nUploadedHi = 0;
			m_pCredits->nUploadedLo = 1; // in order to safe this client, set 1 byte
			if (thePrefs.GetVerbose())
				DEBUG_ONLY(AddDebugLogLine(false, _T("Credits deleted due to new SecureIdent")));
		}
	}
	IdentState = IS_IDENTIFIED;
}

bool CClientCredits::SetSecureIdent(uchar* pachIdent, uint8 nIdentLen){ // verified Public key cannot change, use only if there is not public key yet
	if (MAXPUBKEYSIZE < nIdentLen || m_pCredits->nKeySize != 0 )
		return false;
	memcpy(m_abyPublicKey,pachIdent, nIdentLen);
	m_nPublicKeyLen = nIdentLen;
	IdentState = IS_IDNEEDED;
	return true;
}

EIdentState	CClientCredits::GetCurrentIdentState(uint32 dwForIP) const{
	if (IdentState != IS_IDENTIFIED)
		return IdentState;
	else{
		if (dwForIP == m_dwIdentIP)
			return IS_IDENTIFIED;
		else
			return IS_IDBADGUY; 
			// mod note: clients which just reconnected after an IP change and have to ident yet will also have this state for 1-2 seconds
			//		 so don't try to spam such clients with "bad guy" messages (besides: spam messages are always bad)
	}
}

using namespace CryptoPP;

void CClientCreditsList::InitalizeCrypting(){
	m_nMyPublicKeyLen = 0;
	memset(m_abyMyPublicKey,0,80); // not really needed; better for debugging tho
	m_pSignkey = NULL;
	if (!thePrefs.IsSecureIdentEnabled())
		return;
	// check if keyfile is there
	bool bCreateNewKey = false;
	HANDLE hKeyFile = ::CreateFile(thePrefs.GetConfigDir() + CString("cryptkey.dat"), GENERIC_READ, FILE_SHARE_READ, NULL,
										OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hKeyFile != INVALID_HANDLE_VALUE)
	{
		if (::GetFileSize(hKeyFile, NULL) == 0)
			bCreateNewKey = true;
		::CloseHandle(hKeyFile);
	}
	else
		bCreateNewKey = true;
	if (bCreateNewKey)
		CreateKeyPair();
	
	// load key
	try{
		// load private key
		FileSource filesource(CStringA(thePrefs.GetConfigDir() + _T("cryptkey.dat")), true,new Base64Decoder);
		m_pSignkey = new RSASSA_PKCS1v15_SHA_Signer(filesource);
		// calculate and store public key
		RSASSA_PKCS1v15_SHA_Verifier pubkey(*m_pSignkey);
		ArraySink asink(m_abyMyPublicKey, 80);
		pubkey.DEREncode(asink);
		m_nMyPublicKeyLen = asink.TotalPutLength();
		asink.MessageEnd();
	}
	catch(...)
	{
		if (m_pSignkey){
			delete m_pSignkey;
			m_pSignkey = NULL;
		}
		LogError(LOG_STATUSBAR, GetResString(IDS_CRYPT_INITFAILED));
		ASSERT(0);
	}
	//Debug_CheckCrypting();
}

bool CClientCreditsList::CreateKeyPair(){
	try{
		AutoSeededRandomPool rng;
		InvertibleRSAFunction privkey;
		privkey.Initialize(rng,RSAKEYSIZE);

		Base64Encoder privkeysink(new FileSink(CStringA(thePrefs.GetConfigDir() + _T("cryptkey.dat"))));
		privkey.DEREncode(privkeysink);
		privkeysink.MessageEnd();

		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("Created new RSA keypair"));
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Failed to create new RSA keypair"));
		ASSERT ( false );
		return false;
	}
	return true;
}

uint8 CClientCreditsList::CreateSignature(CClientCredits* pTarget, uchar* pachOutput, uint8 nMaxSize, uint32 ChallengeIP, uint8 byChaIPKind, CryptoPP::RSASSA_PKCS1v15_SHA_Signer* sigkey){
	// sigkey param is used for debug only
	if (sigkey == NULL)
		sigkey = m_pSignkey;

	// create a signature of the public key from pTarget
	ASSERT( pTarget );
	ASSERT( pachOutput );
	uint8 nResult;
	if ( !CryptoAvailable() )
		return 0;
	try{
		
		SecByteBlock sbbSignature(sigkey->SignatureLength());
		AutoSeededRandomPool rng;
		byte abyBuffer[MAXPUBKEYSIZE+9];
		uint32 keylen = pTarget->GetSecIDKeyLen();
		memcpy(abyBuffer,pTarget->GetSecureIdent(),keylen);
		// 4 additional bytes random data send from this client
		uint32 challenge = pTarget->m_dwCryptRndChallengeFrom;
		ASSERT ( challenge != 0 );
		PokeUInt32(abyBuffer+keylen, challenge);
		uint16 ChIpLen = 0;
		if ( byChaIPKind != 0){
			ChIpLen = 5;
			PokeUInt32(abyBuffer+keylen+4, ChallengeIP);
			PokeUInt8(abyBuffer+keylen+4+4, byChaIPKind);
		}
		sigkey->SignMessage(rng, abyBuffer ,keylen+4+ChIpLen , sbbSignature.begin());
		ArraySink asink(pachOutput, nMaxSize);
		asink.Put(sbbSignature.begin(), sbbSignature.size());
		nResult = asink.TotalPutLength();			
	}
	catch(...)
	{
		ASSERT ( false );
		nResult = 0;
	}
	return nResult;
}

bool CClientCreditsList::VerifyIdent(CClientCredits* pTarget, uchar* pachSignature, uint8 nInputSize, uint32 dwForIP, uint8 byChaIPKind){
	ASSERT( pTarget );
	ASSERT( pachSignature );
	if ( !CryptoAvailable() ){
		pTarget->IdentState = IS_NOTAVAILABLE;
		return false;
	}
	bool bResult;
	try{
		StringSource ss_Pubkey((byte*)pTarget->GetSecureIdent(),pTarget->GetSecIDKeyLen(),true,0);
		RSASSA_PKCS1v15_SHA_Verifier pubkey(ss_Pubkey);
		// 4 additional bytes random data send from this client +5 bytes v2
		byte abyBuffer[MAXPUBKEYSIZE+9];
		memcpy(abyBuffer,m_abyMyPublicKey,m_nMyPublicKeyLen);
		uint32 challenge = pTarget->m_dwCryptRndChallengeFor;
		ASSERT ( challenge != 0 );
		PokeUInt32(abyBuffer+m_nMyPublicKeyLen, challenge);
		
		// v2 security improvments (not supported by 29b, not used as default by 29c)
		uint8 nChIpSize = 0;
		if (byChaIPKind != 0){
			nChIpSize = 5;
			uint32 ChallengeIP = 0;
			switch (byChaIPKind){
				case CRYPT_CIP_LOCALCLIENT:
					ChallengeIP = dwForIP;
					break;
				case CRYPT_CIP_REMOTECLIENT:
					if (theApp.serverconnect->GetClientID() == 0 || theApp.serverconnect->IsLowID()){
						if (thePrefs.GetLogSecureIdent())
							AddDebugLogLine(false, _T("Warning: Maybe SecureHash Ident fails because LocalIP is unknown"));
						ChallengeIP = theApp.serverconnect->GetLocalIP();
					}
					else
						ChallengeIP = theApp.serverconnect->GetClientID();
					break;
				case CRYPT_CIP_NONECLIENT: // maybe not supported in future versions
					ChallengeIP = 0;
					break;
			}
			PokeUInt32(abyBuffer+m_nMyPublicKeyLen+4, ChallengeIP);
			PokeUInt8(abyBuffer+m_nMyPublicKeyLen+4+4, byChaIPKind);
		}
		//v2 end

		bResult = pubkey.VerifyMessage(abyBuffer, m_nMyPublicKeyLen+4+nChIpSize, pachSignature, nInputSize);
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Error: Unknown exception in %hs"), __FUNCTION__);
		//ASSERT(0);
		bResult = false;
	}
	if (!bResult){
		if (pTarget->IdentState == IS_IDNEEDED)
			pTarget->IdentState = IS_IDFAILED;
	}
	else{
		pTarget->Verified(dwForIP);
	}
	return bResult;
}

bool CClientCreditsList::CryptoAvailable(){
	return (m_nMyPublicKeyLen > 0 && m_pSignkey != 0 && thePrefs.IsSecureIdentEnabled() );
}


#ifdef _DEBUG
bool CClientCreditsList::Debug_CheckCrypting(){
	// create random key
	AutoSeededRandomPool rng;

	RSASSA_PKCS1v15_SHA_Signer priv(rng, 384);
	RSASSA_PKCS1v15_SHA_Verifier pub(priv);

	byte abyPublicKey[80];
	ArraySink asink(abyPublicKey, 80);
	pub.DEREncode(asink);
	uint8 PublicKeyLen = asink.TotalPutLength();
	asink.MessageEnd();
	uint32 challenge = rand();
	// create fake client which pretends to be this emule
	CreditStruct* newcstruct = new CreditStruct;
	memset(newcstruct, 0, sizeof(CreditStruct));
	CClientCredits* newcredits = new CClientCredits(newcstruct);
	newcredits->SetSecureIdent(m_abyMyPublicKey,m_nMyPublicKeyLen);
	newcredits->m_dwCryptRndChallengeFrom = challenge;
	// create signature with fake priv key
	uchar pachSignature[200];
	memset(pachSignature,200,0);
	uint8 sigsize = CreateSignature(newcredits,pachSignature,200,0,false, &priv);


	// next fake client uses the random created public key
	CreditStruct* newcstruct2 = new CreditStruct;
	memset(newcstruct2, 0, sizeof(CreditStruct));
	CClientCredits* newcredits2 = new CClientCredits(newcstruct2);
	newcredits2->m_dwCryptRndChallengeFor = challenge;

	// if you uncomment one of the following lines the check has to fail
	//abyPublicKey[5] = 34;
	//m_abyMyPublicKey[5] = 22;
	//pachSignature[5] = 232;

	newcredits2->SetSecureIdent(abyPublicKey,PublicKeyLen);

	//now verify this signature - if it's true everything is fine
	bool bResult = VerifyIdent(newcredits2,pachSignature,sigsize,0,0);

	delete newcredits;
	delete newcredits2;

	return bResult;
}
#endif

//EastShare START - Modified by TAHO, modified SUQWT
/*
uint32 CClientCredits::GetSecureWaitStartTime(uint32 dwForIP){
*/
sint64 CClientCredits::GetSecureWaitStartTime(uint32 dwForIP){
//EastShare END - Modified by TAHO, modified SUQWT
	if (m_dwUnSecureWaitTime == 0 || m_dwSecureWaitTime == 0)
		SetSecWaitStartTime(dwForIP);

	if (m_pCredits->nKeySize != 0){	// this client is a SecureHash Client
		if (GetCurrentIdentState(dwForIP) == IS_IDENTIFIED){ // good boy
			return m_dwSecureWaitTime;
		}
		else{	// not so good boy
			if (dwForIP == m_dwWaitTimeIP){
				return m_dwUnSecureWaitTime;
			}
			else{	// bad boy
				// this can also happen if the client has not identified himself yet, but will do later - so maybe he is not a bad boy :) .
				CString buffer2, buffer;
				/*for (uint16 i = 0;i != 16;i++){
					buffer2.Format("%02X",this->m_pCredits->abyKey[i]);
					buffer+=buffer2;
				}
				if (thePrefs.GetLogSecureIdent())
					AddDebugLogLine(false,"Warning: WaitTime resetted due to Invalid Ident for Userhash %s",buffer.GetBuffer());*/
				if(theApp.clientcredits->IsSaveUploadQueueWaitTime()){
						//EastShare START - Modified by TAHO, modified SUQWT
						//m_dwUnSecureWaitTime = ::GetTickCount() - m_pCredits->nUnSecuredWaitTime;	// Moonlight: SUQWT//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
						m_dwUnSecureWaitTime = ::GetTickCount() - ((sint64) m_pCredits->nUnSecuredWaitTime);
						//EastShare END - Modified by TAHO, modified SUQWT
				}
				else{
					m_dwUnSecureWaitTime = ::GetTickCount();//original
				}
				m_dwWaitTimeIP = dwForIP;
				return m_dwUnSecureWaitTime;
			}	
		}
	}
	else{	// not a SecureHash Client - handle it like before for now (no security checks)
		return m_dwUnSecureWaitTime;
	}
}

//Morph Start - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
// Moonlight: SUQWT - Save the wait times.
void CClientCredits::SaveUploadQueueWaitTime(int iKeepPct) {
	if (m_dwUnSecureWaitTime) m_pCredits->nUnSecuredWaitTime = ((GetTickCount() - m_dwUnSecureWaitTime) / 100) * iKeepPct;
	if (m_dwSecureWaitTime) m_pCredits->nSecuredWaitTime = ((GetTickCount() - m_dwSecureWaitTime) / 100) * iKeepPct;
	// EastShare START - Marked by TAHO, modified SUQWT
	// SetSecWaitStartTime(m_dwWaitTimeIP);
	// EastShare END - Marked by TAHO, modified SUQWT
}
// Moonlight: SUQWT - Clear the wait times.
void CClientCredits::ClearUploadQueueWaitTime() {
	m_pCredits->nUnSecuredWaitTime = 0;
	m_pCredits->nSecuredWaitTime = 0;
	// Doing SaveUploadQueueWaitTime(0) should be reduced to something equivalent during compile.
}
//Morph End - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)

//EastShare START - Added by TAHO, modified SUQWT
void CClientCredits::SetSecWaitStartTime() {
	SetSecWaitStartTime(m_dwWaitTimeIP);
}
//EastShare END - Added by TAHO, modified SUQWT

// Moonlight: SUQWT: Adjust to take previous wait time into account.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
void CClientCredits::SetSecWaitStartTime(uint32 dwForIP){
	//Morph Start - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	if(theApp.clientcredits->IsSaveUploadQueueWaitTime()){
		//EastShare START - Added by TAHO, modified SUQWT
		//m_dwUnSecureWaitTime = ::GetTickCount() - m_pCredits->nUnSecuredWaitTime - 1;	// Moonlight: SUQWT
		//m_dwSecureWaitTime = ::GetTickCount() - m_pCredits->nSecuredWaitTime - 1;		// Moonlight: SUQWT
		m_dwUnSecureWaitTime = ::GetTickCount() - ((sint64) m_pCredits->nUnSecuredWaitTime) - 1;
		m_dwSecureWaitTime = ::GetTickCount() - ((sint64) m_pCredits->nSecuredWaitTime) - 1;
		//EastShare END - Added by TAHO, modified SUQWT
	}
	else{
		//original
		m_dwUnSecureWaitTime = ::GetTickCount()-1;
		m_dwSecureWaitTime = ::GetTickCount()-1;
	}
	//Morph End - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	m_dwWaitTimeIP = dwForIP;
}

void CClientCredits::ClearWaitStartTime(){
	m_dwUnSecureWaitTime = 0;
	m_dwSecureWaitTime = 0;
}

//EastShare Start - added by AndCycle, Pay Back First

//init will be triggered at 
//1. client credit create, 
//2. when reach 10MB transfered, between first time remove check and second time remove check
//anyway, this just make a check at "check point" :p

void CClientCredits::InitPayBackFirstStatus(){
	//MORPH START - Changed by SiRoB, Pay Back First Tweak
	m_bPayBackFirst = false;
	TestPayBackFirstStatus();
	//MORPH END   - Changed by SiRoB, Pay Back First Tweak
}

//test will be triggered at client have up/down transfered
void CClientCredits::TestPayBackFirstStatus(){

	if(GetDownloadedTotal() < 9728000){
		m_bPayBackFirst = false;
	}else
	{
		uint64 clientUpload = GetDownloadedTotal();
		uint64 clientDownload = GetUploadedTotal();
		//MORPH START - Changed by SiRoB, Pay Back First Tweak
		if(clientUpload > clientDownload+((uint64)thePrefs.GetPayBackFirstLimit()<<20)){
		//MORPH END   - Changed by SiRoB, Pay Back First Tweak
			m_bPayBackFirst = true;
		}
		else if(clientUpload < clientDownload){
			m_bPayBackFirst = false;
		}
	}
}
//EastShare End - added by AndCycle, Pay Back First Tweak