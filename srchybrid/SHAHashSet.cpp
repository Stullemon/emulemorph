//this file is part of eMule
//Copyright (C)2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "StdAfx.h"
#include "shahashset.h"
#include "opcodes.h"
#include "otherfunctions.h"
#include "emule.h"
#include "safefile.h"
#include "knownfile.h"
#include "preferences.h"
#include "sha.h"
#include "updownclient.h"
#include "DownloadQueue.h"
#include "partfile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// for this version the limits are set very high, they might be lowered later
// to make a hash trustworthy, at least 10 unique Ips (255.255.128.0) must have send it
// and if we have received more than one hash  for the file, one hash has to be send by more than 95% of all unique IPs
#define MINUNIQUEIPS_TOTRUST		10	// how many unique IPs most have send us a hash to make it trustworthy
#define	MINPERCENTAGE_TOTRUST		92  // how many percentage of clients most have sent the same hash to make it trustworthy

CList<CAICHRequestedData, CAICHRequestedData&> CAICHHashSet::m_liRequestedData;

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHash
CString CAICHHash::GetString() const{
	return EncodeBase32(m_abyBuffer, HASHSIZE);
}

void CAICHHash::Read(CFileDataIO* file)	{ 
	file->Read(m_abyBuffer,HASHSIZE);
}

void CAICHHash::Write(CFileDataIO* file) const{ 
	file->Write(m_abyBuffer,HASHSIZE);
}
/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHashTree

CAICHHashTree::CAICHHashTree(uint32 nDataSize, bool bLeftBranch, uint32 nBaseSize){
	m_nDataSize = nDataSize;
	m_nBaseSize = nBaseSize;
	m_bIsLeftBranch = bLeftBranch;
	m_pLeftTree = NULL;
	m_pRightTree = NULL;
	m_bHashValid = false;
}

CAICHHashTree::~CAICHHashTree(){
	if (m_pLeftTree)
		delete m_pLeftTree;
	if (m_pRightTree)
		delete m_pRightTree;
}

// recursive
CAICHHashTree* CAICHHashTree::FindHash(uint32 nStartPos, uint32 nSize, uint8* nLevel){
	(*nLevel)++;
	if (*nLevel > 16){ // sanity
		ASSERT( false );
		return false;
	}
	if (nStartPos + nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return NULL;
	}
	if (nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return NULL;
	}

	if (nStartPos == 0 && nSize == m_nDataSize){
		// this is the searched hash
		return this;
	}
	else if (m_nDataSize <= m_nBaseSize){ // sanity
		// this is already the last level, cant go deeper
		ASSERT( false );
		return NULL;
	}
	else{
		uint32 nBlocks = m_nDataSize / m_nBaseSize + ((m_nDataSize % m_nBaseSize != 0 )? 1:0); 
		uint32 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* m_nBaseSize;
		uint32 nRight = m_nDataSize - nLeft;
		if (nStartPos < nLeft){
			if (nStartPos + nSize > nLeft){ // sanity
				ASSERT ( false );
				return NULL;
			}
			if (m_pLeftTree == NULL)
				m_pLeftTree = new CAICHHashTree(nLeft, true, (nLeft <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				ASSERT( m_pLeftTree->m_nDataSize == nLeft );
			}
			return m_pLeftTree->FindHash(nStartPos, nSize, nLevel);
		}
		else{
			nStartPos -= nLeft;
			if (nStartPos + nSize > nRight){ // sanity
				ASSERT ( false );
				return NULL;
			}
			if (m_pRightTree == NULL)
				m_pRightTree = new CAICHHashTree(nRight, false, (nRight <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				ASSERT( m_pRightTree->m_nDataSize == nRight ); 
			}
			return m_pRightTree->FindHash(nStartPos, nSize, nLevel);
		}
	}
}

// recursive
// calculates missing hash fromt he existing ones
// overwrites existing hashs
// fails if no hash is found for any branch
bool CAICHHashTree::ReCalculateHash(CAICHHashAlgo* hashalg, bool bDontReplace){
	ASSERT ( !( (m_pLeftTree != NULL) ^ (m_pRightTree != NULL)) ); 
	if (m_pLeftTree && m_pRightTree){
		if ( !m_pLeftTree->ReCalculateHash(hashalg, bDontReplace) || !m_pRightTree->ReCalculateHash(hashalg, bDontReplace) )
			return false;
		if (bDontReplace && m_bHashValid)
			return true;
		if (m_pRightTree->m_bHashValid && m_pLeftTree->m_bHashValid){
			hashalg->Reset();
			hashalg->Add(m_pLeftTree->m_Hash.GetRawHash(), HASHSIZE);
			hashalg->Add(m_pRightTree->m_Hash.GetRawHash(), HASHSIZE);
			hashalg->Finish(m_Hash);
			m_bHashValid = true;
			return true;
		}
		else
			return m_bHashValid;
	}
	else
		return true;
}

bool CAICHHashTree::VerifyHashTree(CAICHHashAlgo* hashalg, bool bDeleteBadTrees){
	if (!m_bHashValid){
		ASSERT ( false );
		if (bDeleteBadTrees){
			if (m_pLeftTree){
				delete m_pLeftTree;
				m_pLeftTree = NULL;
			}
			if (m_pRightTree){
				delete m_pRightTree;
				m_pRightTree = NULL;
			}
		}
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		theApp.QueueDebugLogLine(/*DLP_HIGH,*/ false, _T("VerifyHashTree - No masterhash available"));
		return false;
	}
	
		// calculated missing hashs without overwriting anything
		if (m_pLeftTree && !m_pLeftTree->m_bHashValid)
			m_pLeftTree->ReCalculateHash(hashalg, true);
		if (m_pRightTree && !m_pRightTree->m_bHashValid)
			m_pRightTree->ReCalculateHash(hashalg, true);
		
		if ((m_pRightTree && m_pRightTree->m_bHashValid) ^ (m_pLeftTree && m_pLeftTree->m_bHashValid)){
			// one branch can never be verified
			if (bDeleteBadTrees){
				if (m_pLeftTree){
					delete m_pLeftTree;
					m_pLeftTree = NULL;
				}
				if (m_pRightTree){
					delete m_pRightTree;
					m_pRightTree = NULL;
				}
			}
// WebCache ////////////////////////////////////////////////////////////////////////////////////
			if(thePrefs.GetLogICHEvents()) //JP log ICH events
			theApp.QueueDebugLogLine(/*DLP_HIGH,*/ false, _T("VerifyHashSet failed - Hashtree incomplete"));
			return false;
		}
	if ((m_pRightTree && m_pRightTree->m_bHashValid) && (m_pLeftTree && m_pLeftTree->m_bHashValid)){			
	    // check verify the hashs of both child nodes against my hash 
	
		CAICHHash CmpHash;
		hashalg->Reset();
		hashalg->Add(m_pLeftTree->m_Hash.GetRawHash(), HASHSIZE);
		hashalg->Add(m_pRightTree->m_Hash.GetRawHash(), HASHSIZE);
		hashalg->Finish(CmpHash);
		
		if (m_Hash != CmpHash){
			if (bDeleteBadTrees){
				if (m_pLeftTree){
					delete m_pLeftTree;
					m_pLeftTree = NULL;
				}
				if (m_pRightTree){
					delete m_pRightTree;
					m_pRightTree = NULL;
				}
			}
			return false;
		}
		return m_pLeftTree->VerifyHashTree(hashalg, bDeleteBadTrees) && m_pRightTree->VerifyHashTree(hashalg, bDeleteBadTrees);
	}
	else
		// last hash in branch - nothing below to verify
		return true;

}

void CAICHHashTree::SetBlockHash(uint32 nSize, uint32 nStartPos, CAICHHashAlgo* pHashAlg){
	ASSERT ( nSize <= EMBLOCKSIZE );
	CAICHHashTree* pToInsert = FindHash(nStartPos, nSize);
	if (pToInsert == NULL){ // sanity
		ASSERT ( false );
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Critical Error: Failed to Insert SHA-HashBlock, FindHash() failed!"));
		return;
	}
	
	//sanity
	if (pToInsert->m_nBaseSize != EMBLOCKSIZE || pToInsert->m_nDataSize != nSize){
		ASSERT ( false );
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Critical Error: Logical error on values in SetBlockHashFromData"));
		return;
	}

	pHashAlg->Finish(pToInsert->m_Hash);
	pToInsert->m_bHashValid = true;
	//DEBUG_ONLY(theApp.QueueDebugLogLine(/*DLP_VERYLOW,*/ false, _T("Set ShaHash for block %u - %u (%u Bytes) to %s"), nStartPos, nStartPos + nSize, nSize, pToInsert->m_Hash.GetString()) );
	
}

bool CAICHHashTree::CreatePartRecoveryData(uint32 nStartPos, uint32 nSize, CFileDataIO* fileDataOut, uint16 wHashIdent){
	if (nStartPos + nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return false;
	}
	if (nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return false;
	}

	if (nStartPos == 0 && nSize == m_nDataSize){
		// this is the searched part, now write all blocks of this part
		// hashident for this level will be adjsuted by WriteLowestLevelHash
		return WriteLowestLevelHashs(fileDataOut, wHashIdent);
	}
	else if (m_nDataSize <= m_nBaseSize){ // sanity
		// this is already the last level, cant go deeper
		ASSERT( false );
		return false;
	}
	else{
		wHashIdent <<= 1;
		wHashIdent |= (m_bIsLeftBranch) ? 1: 0;
		
		uint32 nBlocks = m_nDataSize / m_nBaseSize + ((m_nDataSize % m_nBaseSize != 0 )? 1:0); 
		uint32 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* m_nBaseSize;
		uint32 nRight = m_nDataSize - nLeft;
		if (m_pLeftTree == NULL || m_pRightTree == NULL){
			ASSERT( false );
			return false;
		}
		if (nStartPos < nLeft){
			if (nStartPos + nSize > nLeft || !m_pRightTree->m_bHashValid){ // sanity
				ASSERT ( false );
				return false;
			}
			m_pRightTree->WriteHash(fileDataOut, wHashIdent);
			return m_pLeftTree->CreatePartRecoveryData(nStartPos, nSize, fileDataOut, wHashIdent);
		}
		else{
			nStartPos -= nLeft;
			if (nStartPos + nSize > nRight || !m_pLeftTree->m_bHashValid){ // sanity
				ASSERT ( false );
				return false;
			}
			m_pLeftTree->WriteHash(fileDataOut, wHashIdent);
			return m_pRightTree->CreatePartRecoveryData(nStartPos, nSize, fileDataOut, wHashIdent);

		}
	}
}

void CAICHHashTree::WriteHash(CFileDataIO* fileDataOut, uint16 wHashIdent) const{
	ASSERT( m_bHashValid );
	wHashIdent <<= 1;
	wHashIdent |= (m_bIsLeftBranch) ? 1: 0;
	fileDataOut->WriteUInt16(wHashIdent);
	m_Hash.Write(fileDataOut);
}

// write lowest level hashs into file, ordered from left to right optional without identifier
bool CAICHHashTree::WriteLowestLevelHashs(CFileDataIO* fileDataOut, uint16 wHashIdent, bool bNoIdent) const{
	wHashIdent <<= 1;
	wHashIdent |= (m_bIsLeftBranch) ? 1: 0;
	if (m_pLeftTree == NULL && m_pRightTree == NULL){
		if (m_nDataSize <= m_nBaseSize && m_bHashValid ){
			if (!bNoIdent)
				fileDataOut->WriteUInt16(wHashIdent);
			m_Hash.Write(fileDataOut);
			//AddDebugLogLine(false,_T("%s"),m_Hash.GetString(), wHashIdent, this);
			return true;
		}
		else{
			ASSERT( false );
			return false;
		}
	}
	else if (m_pLeftTree == NULL || m_pRightTree == NULL){
		ASSERT( false );
		return false;
	}
	else{
		return m_pLeftTree->WriteLowestLevelHashs(fileDataOut, wHashIdent, bNoIdent)
				&& m_pRightTree->WriteLowestLevelHashs(fileDataOut, wHashIdent, bNoIdent);
	}
}

// recover all low level hashs from given data. hashs are assumed to be ordered in left to right - no identifier used
bool CAICHHashTree::LoadLowestLevelHashs(CFileDataIO* fileInput){
	if (m_nDataSize <= m_nBaseSize){ // sanity
		// lowest level, read hash
		m_Hash.Read(fileInput);
		//AddDebugLogLine(false,m_Hash.GetString());
		m_bHashValid = true; 
		return true;
	}
	else{
		uint32 nBlocks = m_nDataSize / m_nBaseSize + ((m_nDataSize % m_nBaseSize != 0 )? 1:0); 
		uint32 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* m_nBaseSize;
		uint32 nRight = m_nDataSize - nLeft;
		if (m_pLeftTree == NULL)
			m_pLeftTree = new CAICHHashTree(nLeft, true, (nLeft <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
		else{
			ASSERT( m_pLeftTree->m_nDataSize == nLeft );
		}
		if (m_pRightTree == NULL)
			m_pRightTree = new CAICHHashTree(nRight, false, (nRight <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
		else{
			ASSERT( m_pRightTree->m_nDataSize == nRight ); 
		}
		return m_pLeftTree->LoadLowestLevelHashs(fileInput)
				&& m_pRightTree->LoadLowestLevelHashs(fileInput);
	}
}


// write the hash, specified by wHashIdent, with Data from fileInput.
bool CAICHHashTree::SetHash(CFileDataIO* fileInput, uint16 wHashIdent, sint8 nLevel, bool bAllowOverwrite){
	if (nLevel == (-1)){
		// first call, check how many level we need to go
		for (uint8 i = 0; i != 16 && (wHashIdent & 0x8000) == 0; i++){
			wHashIdent <<= 1;
		}
		if (i > 15){
// WebCache ////////////////////////////////////////////////////////////////////////////////////
			if(thePrefs.GetLogICHEvents()) //JP log ICH events
			theApp.QueueDebugLogLine(/*DLP_HIGH,*/ false, _T("CAICHHashTree::SetHash - found invalid HashIdent (0)"));
			return false;
		}
		else{
			nLevel = 15 - i;
		}
	}
	if (nLevel == 0){
		// this is the searched hash
		if (m_bHashValid && !bAllowOverwrite){
			// not allowed to overwrite this hash, however move the filepointer by reading a hash
			CAICHHash(file);
			return true;
		}
		m_Hash.Read(fileInput);
		m_bHashValid = true; 
		return true;
	}
	else if (m_nDataSize <= m_nBaseSize){ // sanity
		// this is already the last level, cant go deeper
		ASSERT( false );
		return false;
	}
	else{
		// adjust ident to point the path to the next node
		wHashIdent <<= 1;
		nLevel--;
		uint32 nBlocks = m_nDataSize / m_nBaseSize + ((m_nDataSize % m_nBaseSize != 0 )? 1:0); 
		uint32 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* m_nBaseSize;
		uint32 nRight = m_nDataSize - nLeft;
		if ((wHashIdent & 0x8000) > 0){
			if (m_pLeftTree == NULL)
				m_pLeftTree = new CAICHHashTree(nLeft, true, (nLeft <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				ASSERT( m_pLeftTree->m_nDataSize == nLeft );
			}
			return m_pLeftTree->SetHash(fileInput, wHashIdent, nLevel);
		}
		else{
			if (m_pRightTree == NULL)
				m_pRightTree = new CAICHHashTree(nRight, false, (nRight <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				ASSERT( m_pRightTree->m_nDataSize == nRight ); 
			}
			return m_pRightTree->SetHash(fileInput, wHashIdent, nLevel);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
///CAICHUntrustedHash
bool CAICHUntrustedHash::AddSigningIP(uint32 dwIP){
	dwIP &= 0x00F0FFFF; // we use only the 20 most significant bytes for unique IPs
	for (int i=0; i < m_adwIpsSigning.GetCount(); i++){
		if (m_adwIpsSigning[i] == dwIP)
			return false;
	}
	m_adwIpsSigning.Add(dwIP);
	return true;
}



/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHashSet
CAICHHashSet::CAICHHashSet(CKnownFile* pOwner)
	: m_pHashTree(0, true, PARTSIZE)
{
	m_eStatus = AICH_EMPTY;
	m_pOwner = pOwner;
}

CAICHHashSet::~CAICHHashSet(void)
{
	FreeHashSet();
}

bool CAICHHashSet::CreatePartRecoveryData(uint32 nPartStartPos, CFileDataIO* fileDataOut, bool bDbgDontLoad){
	ASSERT( m_pOwner );
	if (m_pOwner->IsPartFile() || m_eStatus != AICH_HASHSETCOMPLETE){
		ASSERT( false );
		return false;
	}
	if (m_pHashTree.m_nDataSize <= EMBLOCKSIZE){
		ASSERT( false );
		return false;
	}
	if (!bDbgDontLoad){
		if (!LoadHashSet()){
// WebCache ////////////////////////////////////////////////////////////////////////////////////
			if(thePrefs.GetLogICHEvents()) //JP log ICH events
			theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Created RecoveryData error: failed to load hashset (file: %s)"), m_pOwner->GetFileName() );
			SetStatus(AICH_ERROR);
			return false;
		}
	}
	bool bResult;
	uint8 nLevel = 0;
	uint32 nPartSize = min(PARTSIZE, m_pOwner->GetFileSize()-nPartStartPos);
	m_pHashTree.FindHash(nPartStartPos, nPartSize,&nLevel);
	uint16 nHashsToWrite = (nLevel-1) + nPartSize/EMBLOCKSIZE + ((nPartSize % EMBLOCKSIZE != 0 )? 1:0);
	fileDataOut->WriteUInt16(nHashsToWrite);
	uint32 nCheckFilePos = fileDataOut->GetPosition();
	if (m_pHashTree.CreatePartRecoveryData(nPartStartPos, nPartSize, fileDataOut, 0)){
		if (nHashsToWrite*(HASHSIZE+2) != fileDataOut->GetPosition() - nCheckFilePos){
			ASSERT( false );
// WebCache ////////////////////////////////////////////////////////////////////////////////////
			if(thePrefs.GetLogICHEvents()) //JP log ICH events
			theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Created RecoveryData has wrong length (file: %s)"), m_pOwner->GetFileName() );
			bResult = false;
			SetStatus(AICH_ERROR);
		}
		else
			bResult = true;
	}
	else{
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to create RecoveryData for %s"), m_pOwner->GetFileName() );
		bResult = false;
		SetStatus(AICH_ERROR);
	}
	if (!bDbgDontLoad){
		FreeHashSet();
	}
	return bResult;
}

bool CAICHHashSet::ReadRecoveryData(uint32 nPartStartPos, CSafeMemFile* fileDataIn){
	if (/*TODO !m_pOwner->IsPartFile() ||*/ !(m_eStatus == AICH_VERIFIED || m_eStatus == AICH_TRUSTED) ){
		ASSERT( false );
		return false;
	}
	// at this time we check the recoverydata for the correct ammounts of hashs only
	// all hash are then taken into the tree, depending on there hashidentifier (except the masterhash)

	uint8 nLevel = 0;
	uint32 nPartSize = min(PARTSIZE, m_pOwner->GetFileSize()-nPartStartPos);
	m_pHashTree.FindHash(nPartStartPos, nPartSize,&nLevel);
	uint16 nHashsToRead = (nLevel-1) + nPartSize/EMBLOCKSIZE + ((nPartSize % EMBLOCKSIZE != 0 )? 1:0);
	uint16 nHashsAvailable = fileDataIn->ReadUInt16();
	if (fileDataIn->GetLength()-fileDataIn->GetPosition() < nHashsToRead*(HASHSIZE+2) || nHashsToRead != nHashsAvailable){
		// this check is redunant, CSafememfile would catch such an error too
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Received datasize/amounts of hashs was invalid"), m_pOwner->GetFileName() );
		return false;
	}
	for (uint32 i = 0; i != nHashsToRead; i++){
		uint16 wHashIdent = fileDataIn->ReadUInt16();
		if (wHashIdent == 1 /*never allow masterhash to be overwritten*/
			|| !m_pHashTree.SetHash(fileDataIn, wHashIdent,(-1), false))
		{
// WebCache ////////////////////////////////////////////////////////////////////////////////////
			if(thePrefs.GetLogICHEvents()) //JP log ICH events
			theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Error when trying to read hash into tree"), m_pOwner->GetFileName() );
			VerifyHashTree(true); // remove invalid hashs which we have already written
			return false;
		}
	}
	if (VerifyHashTree(true)){
		// some final check if all hashs we wanted are there
		for (uint32 nPartPos = 0; nPartPos < nPartSize; nPartPos += EMBLOCKSIZE){
			CAICHHashTree* phtToCheck = m_pHashTree.FindHash(nPartStartPos+nPartPos, min(EMBLOCKSIZE, nPartSize-nPartPos));
			if (phtToCheck == NULL || !phtToCheck->m_bHashValid){
// WebCache ////////////////////////////////////////////////////////////////////////////////////
				if(thePrefs.GetLogICHEvents()) //JP log ICH events
				theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Error while verifying presence of all lowest level hashs"), m_pOwner->GetFileName() );
				return false;
			}
		}
		// all done
		return true;
	}
	else{
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Verifying received hashtree failed"), m_pOwner->GetFileName() );
		return false;
	}
}

// this function is only allowed to be called right after successfully calculating the hashset (!)
// will delete the hashset, after saving to free the memory
bool CAICHHashSet::SaveHashSet(){
	if (m_eStatus != AICH_HASHSETCOMPLETE){
		ASSERT( false );
		return false;
	}
	if ( !m_pHashTree.m_bHashValid || m_pHashTree.m_nDataSize != m_pOwner->GetFileSize()){
		ASSERT( false );
		return false;
	}
	CString fullpath=thePrefs.GetConfigDir();
	fullpath.Append(KNOWN2_MET_FILENAME);
	CSafeFile file;
	CFileException fexp;
	if (!file.Open(fullpath,CFile::modeCreate|CFile::modeReadWrite|CFile::modeNoTruncate|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") KNOWN2_MET_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			theApp.QueueLogLine(true, _T("%s"), strError);
		}
		return false;
	}
	try {
		//setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

		// first we check if the hashset we want to write is already stored
		CAICHHash CurrentHash;
		uint32 nExistingSize = file.GetLength();
		uint16 nHashCount;
		while (file.GetPosition() < nExistingSize){
			CurrentHash.Read(&file);
			if (m_pHashTree.m_Hash == CurrentHash){
				// this hashset if already available, no need to save it again
				return true;
			}
			nHashCount = file.ReadUInt16();
			if (file.GetPosition() + nHashCount*HASHSIZE > nExistingSize){
				AfxThrowFileException(CFileException::endOfFile, 0, file.GetFileName());
			}
			// skip the rest of this hashset
			file.Seek(nHashCount*HASHSIZE, CFile::current);
		}
		// write hashset
		m_pHashTree.m_Hash.Write(&file);
		nHashCount = (PARTSIZE/EMBLOCKSIZE + ((PARTSIZE % EMBLOCKSIZE != 0)? 1 : 0)) * (m_pHashTree.m_nDataSize/PARTSIZE);
		if (m_pHashTree.m_nDataSize % PARTSIZE != 0)
			nHashCount += (m_pHashTree.m_nDataSize % PARTSIZE)/EMBLOCKSIZE + (((m_pHashTree.m_nDataSize % PARTSIZE) % EMBLOCKSIZE != 0)? 1 : 0);
		file.WriteUInt16(nHashCount);
		if (!m_pHashTree.WriteLowestLevelHashs(&file, 0, true)){
			// thats bad... really
			file.SetLength(nExistingSize);
// WebCache ////////////////////////////////////////////////////////////////////////////////////
			if(thePrefs.GetLogICHEvents()) //JP log ICH events
			theApp.QueueDebugLogLine(true, _T("Failed to save HashSet: WriteLowestLevelHashs() failed!"));
			return false;
		}
		if (file.GetLength() != nExistingSize + (nHashCount+1)*HASHSIZE + 2){
			// thats even worse
			file.SetLength(nExistingSize);
// WebCache ////////////////////////////////////////////////////////////////////////////////////
			if(thePrefs.GetLogICHEvents()) //JP log ICH events
			theApp.QueueDebugLogLine(true, _T("Failed to save HashSet: Calculated and real size of hashset differ!"));
			return false;
		}
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		theApp.QueueDebugLogLine(false, _T("Sucessfully saved eMuleAC Hashset, %u Hashs + 1 Masterhash written"), nHashCount);
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			theApp.QueueLogLine(true,GetResString(IDS_ERR_SERVERMET_BAD));
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			theApp.QueueLogLine(true,GetResString(IDS_ERR_SERVERMET_UNKNOWN),buffer);
		}
		error->Delete();
		return false;
	}
	FreeHashSet();
	return true;
}


bool CAICHHashSet::LoadHashSet(){
	if (m_eStatus != AICH_HASHSETCOMPLETE){
		ASSERT( false );
		return false;
	}
	if ( !m_pHashTree.m_bHashValid || m_pHashTree.m_nDataSize != m_pOwner->GetFileSize() || m_pHashTree.m_nDataSize == 0){
		ASSERT( false );
		return false;
	}
	CString fullpath=thePrefs.GetConfigDir();
	fullpath.Append(KNOWN2_MET_FILENAME);
	CSafeFile file;
	CFileException fexp;
	if (!file.Open(fullpath,CFile::modeCreate|CFile::modeRead|CFile::modeNoTruncate|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyNone, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") KNOWN2_MET_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			theApp.QueueLogLine(true, _T("%s"), strError);
		}
		return false;
	}
	try {
		//setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
		CAICHHash CurrentHash;
		uint32 nExistingSize = file.GetLength();
		uint16 nHashCount;
		while (file.GetPosition() < nExistingSize){
			CurrentHash.Read(&file);
			if (m_pHashTree.m_Hash == CurrentHash){
				// found Hashset
				uint32 nExpectedCount =	(PARTSIZE/EMBLOCKSIZE + ((PARTSIZE % EMBLOCKSIZE != 0)? 1 : 0)) * (m_pHashTree.m_nDataSize/PARTSIZE);
				if (m_pHashTree.m_nDataSize % PARTSIZE != 0)
					nExpectedCount += (m_pHashTree.m_nDataSize % PARTSIZE)/EMBLOCKSIZE + (((m_pHashTree.m_nDataSize % PARTSIZE) % EMBLOCKSIZE != 0)? 1 : 0);
				nHashCount = file.ReadUInt16();
				if (nHashCount != nExpectedCount){
// WebCache ////////////////////////////////////////////////////////////////////////////////////
					if(thePrefs.GetLogICHEvents()) //JP log ICH events
					theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: Available Hashs and expected hashcount differ!"));
					return false;
				}
				uint32 dbgPos = file.GetPosition();
				if (!m_pHashTree.LoadLowestLevelHashs(&file)){
// WebCache ////////////////////////////////////////////////////////////////////////////////////
					if(thePrefs.GetLogICHEvents()) //JP log ICH events
					theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: LoadLowestLevelHashs failed!"));
					return false;
				}
				uint32 dbgHashRead = (file.GetPosition()-dbgPos)/HASHSIZE;
				if (!ReCalculateHash(false)){
// WebCache ////////////////////////////////////////////////////////////////////////////////////
					if(thePrefs.GetLogICHEvents()) //JP log ICH events
					theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: Calculating loaded hashs failed!"));
					return false;
				}
				if (CurrentHash != m_pHashTree.m_Hash){
// WebCache ////////////////////////////////////////////////////////////////////////////////////
					if(thePrefs.GetLogICHEvents()) //JP log ICH events
					theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: Calculated Masterhash differs from given Masterhash - hashset corrupt!"));
					return false;
				}
				return true;
			}
			nHashCount = file.ReadUInt16();
			if (file.GetPosition() + nHashCount*HASHSIZE > nExistingSize){
				AfxThrowFileException(CFileException::endOfFile, 0, file.GetFileName());
			}
			// skip the rest of this hashset
			file.Seek(nHashCount*HASHSIZE, CFile::current);
		}
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: HashSet not found!"));
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			theApp.QueueLogLine(true,GetResString(IDS_ERR_SERVERMET_BAD));
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			theApp.QueueLogLine(true,GetResString(IDS_ERR_SERVERMET_UNKNOWN),buffer);
		}
		error->Delete();
	}
	return false;
}

// delete the hashset except the masterhash (we dont keep aich hashsets in memory to save ressources)
void CAICHHashSet::FreeHashSet(){
	if (m_pHashTree.m_pLeftTree){
		delete m_pHashTree.m_pLeftTree;
		m_pHashTree.m_pLeftTree = NULL;
	}
	if (m_pHashTree.m_pRightTree){
		delete m_pHashTree.m_pRightTree;
		m_pHashTree.m_pRightTree = NULL;
	}
}

void CAICHHashSet::SetMasterHash(const CAICHHash& Hash, EAICHStatus eNewStatus){
	m_pHashTree.m_Hash = Hash;
	m_pHashTree.m_bHashValid = true;
	SetStatus(eNewStatus);
}

CAICHHashAlgo*	CAICHHashSet::GetNewHashAlgo(){
	return new CSHA();
}

bool CAICHHashSet::ReCalculateHash(bool bDontReplace){
	CAICHHashAlgo* hashalg = GetNewHashAlgo();
	bool bResult = m_pHashTree.ReCalculateHash(hashalg, bDontReplace);
	delete hashalg;
	return bResult;
}

bool CAICHHashSet::VerifyHashTree(bool bDeleteBadTrees){
	CAICHHashAlgo* hashalg = GetNewHashAlgo();
	bool bResult = m_pHashTree.VerifyHashTree(hashalg, bDeleteBadTrees);
	delete hashalg;
	return bResult;
}

void CAICHHashSet::SetFileSize(uint32 nSize){
	m_pHashTree.m_nDataSize = nSize;
	m_pHashTree.m_nBaseSize = (nSize <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE;	
}

void CAICHHashSet::UntrustedHashReceived(const CAICHHash& Hash, uint32 dwFromIP){
	switch(GetStatus()){
		case AICH_EMPTY:
		case AICH_UNTRUSTED:
		case AICH_TRUSTED:
			break;
		default:
			return;
	}
	bool bFound = false;
	bool bAdded = false;
	for (int i = 0; i < m_aUntrustedHashs.GetCount(); i++){
		if (m_aUntrustedHashs[i].m_Hash == Hash){
			bAdded = m_aUntrustedHashs[i].AddSigningIP(dwFromIP);
			bFound = true;
			break;
		}
	}
	if (!bFound){
		bAdded = true;
		CAICHUntrustedHash uhToAdd;
		uhToAdd.m_Hash = Hash;
		uhToAdd.AddSigningIP(dwFromIP);
		m_aUntrustedHashs.Add(uhToAdd);
	}

	uint32 nSigningIPsTotal = 0;	// unique clients who send us a hash
	sint32 nMostTrustedPos = (-1);  // the hash which most clients send us
	uint32 nMostTrustedIPs = 0;
	for (uint32 i = 0; i < (uint32)m_aUntrustedHashs.GetCount(); i++){
		nSigningIPsTotal += m_aUntrustedHashs[i].m_adwIpsSigning.GetCount();
		if ((uint32)m_aUntrustedHashs[i].m_adwIpsSigning.GetCount() > nMostTrustedIPs){
			nMostTrustedIPs = m_aUntrustedHashs[i].m_adwIpsSigning.GetCount();
			nMostTrustedPos = i;
		}
	}
	if (nMostTrustedPos == (-1) || nSigningIPsTotal == 0){
		ASSERT( false );
		return;
	}
	// the check if we trust any hash
	if ( thePrefs.IsTrustingEveryHash() ||
		(nMostTrustedIPs >= MINUNIQUEIPS_TOTRUST && (100 * nMostTrustedIPs)/nSigningIPsTotal >= MINPERCENTAGE_TOTRUST)){
		//trusted
			//, Hash.GetString(), bAdded? _T(""):_T("not "), m_aUntrustedHashs.GetCount(), nSigningIPsTotal, m_aUntrustedHashs[nMostTrustedPos].m_Hash.GetString()
			//, nMostTrustedIPs, (100 * nMostTrustedIPs)/nSigningIPsTotal, ipstr(dwFromIP & 0x00F0FFFF), m_pOwner->GetFileName());
		
		SetStatus(AICH_TRUSTED);
		if (!HasValidMasterHash() || GetMasterHash() != m_aUntrustedHashs[nMostTrustedPos].m_Hash){
			SetMasterHash(m_aUntrustedHashs[nMostTrustedPos].m_Hash, AICH_TRUSTED);
			FreeHashSet();
		}
	}
	else{
		// untrusted
		//theApp.QueueDebugLogLine(false, _T("AICH Hash received: %s (%sadded), We have now %u hash from %u unique IPs. Best Hash (%s) is from %u clients (%u%%) - but we dont trust it yet. Added IP:%s, file: %s")
		//	, Hash.GetString(), bAdded? _T(""):_T("not "), m_aUntrustedHashs.GetCount(), nSigningIPsTotal, m_aUntrustedHashs[nMostTrustedPos].m_Hash.GetString()
		//	, nMostTrustedIPs, (100 * nMostTrustedIPs)/nSigningIPsTotal, ipstr(dwFromIP & 0x00F0FFFF), m_pOwner->GetFileName());
		
		SetStatus(AICH_UNTRUSTED);
		if (!HasValidMasterHash() || GetMasterHash() != m_aUntrustedHashs[nMostTrustedPos].m_Hash){
			SetMasterHash(m_aUntrustedHashs[nMostTrustedPos].m_Hash, AICH_UNTRUSTED);
			FreeHashSet();
		}
	}
}

void CAICHHashSet::ClientAICHRequestFailed(CUpDownClient* pClient){
	pClient->SetReqFileAICHHash(NULL);
	CAICHRequestedData data = GetAICHReqDetails(pClient);
	RemoveClientAICHRequest(pClient);
	if (data.m_pClient != pClient)
		return;
	if(theApp.downloadqueue->IsPartFile(data.m_pPartFile)){
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		theApp.QueueDebugLogLine(false, _T("AICH Request failed, Trying to ask another client (file %s, Part: %u,  Client%s)"), data.m_pPartFile->GetFileName(), data.m_nPart, pClient->DbgGetClientInfo());
		data.m_pPartFile->RequestAICHRecovery(data.m_nPart);
	}
}

void CAICHHashSet::RemoveClientAICHRequest(const CUpDownClient* pClient){
	for (POSITION pos = m_liRequestedData.GetHeadPosition();pos != 0; m_liRequestedData.GetNext(pos))
	{
		if (m_liRequestedData.GetAt(pos).m_pClient == pClient){
			m_liRequestedData.RemoveAt(pos);
			return;
		}
	}
	ASSERT( false );
}

bool CAICHHashSet::IsClientRequestPending(const CPartFile* pForFile, uint16 nPart){
	for (POSITION pos = m_liRequestedData.GetHeadPosition();pos != 0; m_liRequestedData.GetNext(pos))
	{
		if (m_liRequestedData.GetAt(pos).m_pPartFile == pForFile && m_liRequestedData.GetAt(pos).m_nPart == nPart){
			return true;
		}
	}
	return false;
}

CAICHRequestedData CAICHHashSet::GetAICHReqDetails(const  CUpDownClient* pClient){
	for (POSITION pos = m_liRequestedData.GetHeadPosition();pos != 0; m_liRequestedData.GetNext(pos))
	{
		if (m_liRequestedData.GetAt(pos).m_pClient == pClient){
			return m_liRequestedData.GetAt(pos);
		}
	}	
	ASSERT( false );
	CAICHRequestedData empty;
	return empty;
}

bool CAICHHashSet::IsPartDataAvailable(uint32 nPartStartPos){
	if (!(m_eStatus == AICH_VERIFIED || m_eStatus == AICH_TRUSTED || m_eStatus == AICH_HASHSETCOMPLETE) ){
		ASSERT( false );
		return false;
	}
	uint32 nPartSize = min(PARTSIZE, m_pOwner->GetFileSize()-nPartStartPos);
	for (uint32 nPartPos = 0; nPartPos < nPartSize; nPartPos += EMBLOCKSIZE){
		CAICHHashTree* phtToCheck = m_pHashTree.FindHash(nPartStartPos+nPartPos, min(EMBLOCKSIZE, nPartSize-nPartPos));
		if (phtToCheck == NULL || !phtToCheck->m_bHashValid){
			return false;
		}
	}
	return true;
}

void CAICHHashSet::DbgTest(){
#ifdef _DEBUG
	//define TESTSIZE 4294567295
	uint8 maxLevel = 0;
	uint32 cHash = 1;
	uint8 curLevel = 0;
	uint32 cParts = 0;
	maxLevel = 0;
/*	CAICHHashTree* pTest = new CAICHHashTree(TESTSIZE, true, 9728000);
	for (uint64 i = 0; i+9728000 < TESTSIZE; i += 9728000){
		CAICHHashTree* pTest2 = new CAICHHashTree(9728000, true, EMBLOCKSIZE);
		pTest->ReplaceHashTree(i, 9728000, &pTest2);
		cParts++;
	}
	CAICHHashTree* pTest2 = new CAICHHashTree(TESTSIZE-i, true, EMBLOCKSIZE);
	pTest->ReplaceHashTree(i, (TESTSIZE-i), &pTest2);
	cParts++;
*/
#define TESTSIZE m_pHashTree.m_nDataSize
	if (m_pHashTree.m_nDataSize <= EMBLOCKSIZE)
		return;
	CAICHHashSet TestHashSet(m_pOwner);
	TestHashSet.SetFileSize(m_pOwner->GetFileSize());
	TestHashSet.SetMasterHash(GetMasterHash(), AICH_VERIFIED);
	CSafeMemFile file;
	for (uint64 i = 0; i+9728000 < TESTSIZE; i += 9728000){
		VERIFY( CreatePartRecoveryData(i, &file) );
		
		/*uint32 nRandomCorruption = (rand() * rand()) % (file.GetLength()-4);
		file.Seek(nRandomCorruption, CFile::begin);
		file.Write(&nRandomCorruption, 4);*/

		file.SeekToBegin();
		VERIFY( TestHashSet.ReadRecoveryData(i, &file) );
		file.SeekToBegin();
		TestHashSet.FreeHashSet();
		for (uint32 j = 0; j+EMBLOCKSIZE < 9728000; j += EMBLOCKSIZE){
			VERIFY( m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel) );
			//TRACE(_T("%u - %s\r\n"), cHash, m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel)->m_Hash.GetString());
			maxLevel = max(curLevel, maxLevel);
			curLevel = 0;
			cHash++;
		}
		VERIFY( m_pHashTree.FindHash(i+j, 9728000-j, &curLevel) );
		//TRACE(_T("%u - %s\r\n"), cHash, m_pHashTree.FindHash(i+j, 9728000-j, &curLevel)->m_Hash.GetString());
		maxLevel = max(curLevel, maxLevel);
		curLevel = 0;
		cHash++;

	}
	VERIFY( CreatePartRecoveryData(i, &file) );
	file.SeekToBegin();
	VERIFY( TestHashSet.ReadRecoveryData(i, &file) );
	file.SeekToBegin();
	TestHashSet.FreeHashSet();
	for (uint64 j = 0; j+EMBLOCKSIZE < TESTSIZE-i; j += EMBLOCKSIZE){
		VERIFY( m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel) );
		//TRACE(_T("%u - %s\r\n"), cHash,m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel)->m_Hash.GetString());
		maxLevel = max(curLevel, maxLevel);
		curLevel = 0;
		cHash++;
	}
	//VERIFY( m_pHashTree.FindHash(i+j, (TESTSIZE-i)-j, &curLevel) );
	TRACE(_T("%u - %s\r\n"), cHash,m_pHashTree.FindHash(i+j, (TESTSIZE-i)-j, &curLevel)->m_Hash.GetString());
	maxLevel = max(curLevel, maxLevel);
#endif
}
