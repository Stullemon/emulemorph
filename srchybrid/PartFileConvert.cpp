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

// PartFileConvert.cpp : implementation file
// created by Ornis

#include "stdafx.h"
#include "PartFileConvert.h"
#include "ResizableLib\ResizableDialog.h"
#include <io.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

int CPartFileConvert::ScanFolderToAdd(CString folder,bool deletesource) {
	int count=0;
	CFileFind finder;
	BOOL bWorking;

	bWorking = finder.FindFile(folder+"\\*.part.met");
	if (bWorking) {
		while (bWorking) {
			bWorking=finder.FindNextFile();
			ConvertToeMule(finder.GetFilePath(),deletesource);
			count++;
		}
	} else {
		bWorking = finder.FindFile(folder+"\\*.*");
		while (bWorking) {
            bWorking = finder.FindNextFile();
			CString test=finder.GetFilePath();
			if (finder.IsDirectory() && finder.GetFileName().Left(1)!=".")
				ScanFolderToAdd(finder.GetFilePath(),deletesource);
		}
	}

	return count;
}

void CPartFileConvert::ConvertToeMule(CString folder,bool deletesource)
{
	if (!PathFileExists(folder))
		return;
	
	//if ( folder.Left(strlen(theApp.glob_prefs->GetTempDir())).CompareNoCase(theApp.glob_prefs->GetTempDir()) ==0 ) return;

	ConvertJob* newjob=new ConvertJob ();
	newjob->folder=folder;
	newjob->removeSource=deletesource;
	newjob->state=CONV_QUEUE;
	m_jobs.AddTail(newjob);

	if (m_convertgui)
		m_convertgui->AddJob(newjob);
	
	StartThread();
}

void CPartFileConvert::StartThread() {
	if (convertPfThread==NULL)
		convertPfThread=AfxBeginThread(run, NULL);
}

UINT AFX_CDECL CPartFileConvert::run(LPVOID lpParam)
{
	DbgSetThreadName("Partfile-Converter");

	while (true) {

		// search next queued job and start it
		pfconverting=NULL;
		for(POSITION pos = m_jobs.GetHeadPosition(); pos != NULL; m_jobs.GetNext(pos)){
			pfconverting=m_jobs.GetAt(pos);
			if (pfconverting->state==CONV_QUEUE) break; else pfconverting=NULL;
		}
		if (pfconverting!=NULL) {
			pfconverting->state=CONV_INPROGRESS;
			UpdateGUI(pfconverting);
			pfconverting->state=performConvertToeMule(pfconverting->folder);
			UpdateGUI(pfconverting);
			AddLogLine(true,GetResString(IDS_IMP_STATUS),pfconverting->folder,GetReturncodeText(pfconverting->state));
		} else
			break;// nothing more to do now
	}

	// clean up
	UpdateGUI(NULL);

	convertPfThread=NULL;
	return 0;
}

int CPartFileConvert::performConvertToeMule(CString folder)
{
	BOOL bWorking;
	CString filepartindex,newfilename;
	CString buffer;
	int fileindex;
	CFileFind finder;
	
	CString partfile=folder;
	folder.Delete(folder.ReverseFind('\\'),folder.GetLength());
	partfile=partfile.Mid(partfile.ReverseFind('\\')+1,partfile.GetLength());


	UpdateGUI(0,GetResString(IDS_IMP_STEPREADPF),true);

	filepartindex=partfile.Left(partfile.Find('.'));
	int pos=filepartindex.ReverseFind('\\');
	if (pos>-1) filepartindex=filepartindex.Mid(pos+1,filepartindex.GetLength()-pos);

	UpdateGUI(4,GetResString(IDS_IMP_STEPBASICINF));

	CPartFile* file=new CPartFile();
	pfconverting->versiontag=file->LoadPartFile(folder,filepartindex+".part.met",true);

	if (pfconverting->versiontag==0) {
		delete file;
		return CONV_FAILED;
	}

	pfconverting->size=file->GetFileSize();
	pfconverting->filename=file->GetFileName();
	pfconverting->filehash= EncodeBase16( file->GetFileHash() ,16);
	UpdateGUI(pfconverting);

	if (theApp.downloadqueue->GetFileByID(file->GetFileHash())!=0) {
		delete file;
		return CONV_ALREADYEXISTS;
	}
	
	if (pfconverting->versiontag==PARTFILE_SPLITTEDVERSION) {
		try {
			CByteArray ba;
			ba.SetSize(PARTSIZE);

			CFile inputfile;
			int pos1,pos2;
			CString filename;

			// just count
			uint16 maxindex=0;
			uint16 partfilecount=0;
			bWorking = finder.FindFile(folder+"\\"+filepartindex+".*.part");
			while (bWorking)
			{
				bWorking = finder.FindNextFile();
				++partfilecount;
				buffer=finder.GetFileName();
				pos1=buffer.Find('.');
				pos2=buffer.Find('.',pos1+1);
				fileindex=atoi(buffer.Mid(pos1+1,pos2-pos1) );
				if (fileindex==0) continue;
				if (fileindex>maxindex) maxindex=fileindex;
			}
			float stepperpart;
			if (partfilecount>0) {
				stepperpart=(80.0f / partfilecount );
				if (maxindex*PARTSIZE<=pfconverting->size) pfconverting->spaceneeded=maxindex*PARTSIZE;
					else pfconverting->spaceneeded=((pfconverting->size / PARTSIZE) * PARTSIZE)+(pfconverting->size % PARTSIZE);
			} else {
				stepperpart=80.0f;
				pfconverting->spaceneeded=0;
			}
			
			UpdateGUI(pfconverting);

			if (GetFreeDiskSpaceX(theApp.glob_prefs->GetTempDir()) < (maxindex*PARTSIZE) ) {
				delete file;
				return CONV_OUTOFDISKSPACE;
			}

			// create new partmetfile, and remember the new name
			file->CreatePartFile();
			newfilename=file->GetFullName();

			UpdateGUI(8,GetResString(IDS_IMP_STEPCRDESTFILE));
			file->m_hpartfile.SetLength( pfconverting->spaceneeded );

			uint16 curindex=0;
			bWorking = finder.FindFile(folder+"\\"+filepartindex+".*.part");
			while (bWorking)
			{
				bWorking = finder.FindNextFile();
				
				//stats
				++curindex;
				buffer.Format(GetResString(IDS_IMP_LOADDATA),curindex,partfilecount);
				UpdateGUI( 10+(curindex*stepperpart) ,buffer);

				filename=finder.GetFileName();
				pos1=filename.Find('.');
				pos2=filename.Find('.',pos1+1);
				fileindex=atoi(filename.Mid(pos1+1,pos2-pos1) );
				if (fileindex==0) continue;

				uint32 chunkstart=(fileindex-1) * PARTSIZE ;

				// open, read data of the part-part-file into buffer, close file
				inputfile.Open(finder.GetFilePath(),CFile::modeRead);
				uint32 readed=inputfile.Read( ba.GetData() ,PARTSIZE);
				inputfile.Close();

				buffer.Format(GetResString(IDS_IMP_SAVEDATA),curindex,partfilecount);
				UpdateGUI( 10+(curindex*stepperpart) ,buffer);

				// write the buffered data
				file->m_hpartfile.Seek(chunkstart, CFile::begin );
				file->m_hpartfile.Write(ba.GetData(),readed);
			}
		}
		catch(CFileException* error) {
			OUTPUT_DEBUG_TRACE();
			CString strError(GetResString(IDS_IMP_IOERROR));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (error->GetErrorMessage(szError, ELEMENT_COUNT(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			AddLogLine(false, _T("%s"), strError);
			error->Delete();
			delete file;
			return CONV_IOERROR;
		}
		file->m_hpartfile.Close();
	}

	// import an external common format partdownload
	else if (pfconverting->versiontag==PARTFILE_VERSION) {
		

		if (!pfconverting->removeSource) 
		pfconverting->spaceneeded=GetDiskFileSize(folder+"\\"+partfile.Left(partfile.GetLength()-4));

		UpdateGUI(pfconverting);

		if (!pfconverting->removeSource && (GetFreeDiskSpaceX(theApp.glob_prefs->GetTempDir()) < pfconverting->spaceneeded) ) {
			delete file;
			return CONV_OUTOFDISKSPACE;
		}

		file->CreatePartFile();
		newfilename=file->GetFullName();

		if (pfconverting->removeSource)
			MoveFile(folder+"\\"+partfile,newfilename);
		else CopyFile(folder+"\\"+partfile,newfilename,false);
		
		file->m_hpartfile.Close();

		if (pfconverting->removeSource)
			MoveFile(folder+"\\"+partfile.Left(partfile.GetLength()-4), newfilename.Left(newfilename.GetLength()-4) );
		else CopyFile(folder+"\\"+partfile.Left(partfile.GetLength()-4), newfilename.Left(newfilename.GetLength()-4) ,false);
	}


	UpdateGUI( 94 ,GetResString(IDS_IMP_GETPFINFO));
	CopyFile(folder+"\\"+filepartindex+".part.met",newfilename,false);

	for (int i = 0; i < file->hashlist.GetSize(); i++)
		delete[] file->hashlist[i];
	file->hashlist.RemoveAll();
	if (file->gaplist.GetCount()>0 ) {
		delete file->gaplist.GetAt(file->gaplist.GetHeadPosition());
		file->gaplist.RemoveAt(file->gaplist.GetHeadPosition());
	}

	if (!file->LoadPartFile(theApp.glob_prefs->GetTempDir(),file->GetPartMetFileName(),false)) {
		delete file;
		return CONV_FAILED;
	}

	if (pfconverting->versiontag==PARTFILE_SPLITTEDVERSION) {
		file->completedsize=file->transfered;
		file->m_iGainDueToCompression = 0;
		file->m_iLostDueToCorruption = 0;
	}

	UpdateGUI( 100 ,GetResString(IDS_IMP_ADDDWL));

	theApp.downloadqueue->AddDownload(file);
	file->SavePartFile();

	if (pfconverting->removeSource) {

		bWorking = finder.FindFile(folder+"\\"+filepartindex+".*");
		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			unlink(finder.GetFilePath());
		}

		if (pfconverting->versiontag==PARTFILE_SPLITTEDVERSION)
			RemoveDirectory(folder+"\\");
	}

	return CONV_OK;
}

void CPartFileConvert::UpdateGUI(float percent,CString text, bool fullinfo) {
	
	if (m_convertgui==NULL) return;

	m_convertgui->pb_current.SetPos(percent);
	CString buffer;
	buffer.Format("%.2f %%",percent);
	m_convertgui->SetDlgItemText(IDC_CONV_PROZENT,buffer);

	if (!text.IsEmpty())
		m_convertgui->SetDlgItemText(IDC_CONV_PB_LABEL,text);

	if (fullinfo) {
		m_convertgui->SetDlgItemText(IDC_CURJOB,pfconverting->folder);
	}
}

void CPartFileConvert::UpdateGUI(ConvertJob* job){
	
	if (m_convertgui==NULL) return;

	m_convertgui->UpdateJobInfo(job);
}


void CPartFileConvert::ShowGUI(){

	if (m_convertgui)
		m_convertgui->SetForegroundWindow();
	else {
		m_convertgui= new CModeless();
		m_convertgui->Create( IDD_CONVERTPARTFILES , CWnd::GetDesktopWindow() );//,  );
		m_convertgui->ShowWindow(SW_SHOW);

		m_convertgui->AddAnchor(IDC_CONV_PB_CURRENT, TOP_LEFT, TOP_RIGHT);
		m_convertgui->AddAnchor(IDC_CURJOB, TOP_LEFT, TOP_RIGHT);
		m_convertgui->AddAnchor(IDC_CONV_PB_LABEL, TOP_LEFT, TOP_RIGHT);
		m_convertgui->AddAnchor(IDC_CONV_PROZENT, TOP_RIGHT);
		m_convertgui->AddAnchor(IDC_JOBLIST, TOP_LEFT, BOTTOM_RIGHT);
		m_convertgui->AddAnchor(IDC_ADDITEM, BOTTOM_LEFT);
		m_convertgui->AddAnchor(IDC_RETRY, BOTTOM_LEFT);
		m_convertgui->AddAnchor(IDC_CONVREMOVE, BOTTOM_LEFT);
		m_convertgui->AddAnchor(IDC_HIDECONVDLG, BOTTOM_RIGHT);

		HICON importicon;
		importicon=theApp.LoadIcon("CONVERT",16,16);
		//importicon=theApp.LoadIcon(IDI_IMPORT);
		m_convertgui->SetIcon(importicon,FALSE);
		
		// init gui
		m_convertgui->pb_current.SetRange(0,100);
		m_convertgui->joblist.SetExtendedStyle(LVS_EX_FULLROWSELECT);
		m_convertgui->joblist.ModifyStyle(LVS_SINGLESEL,0);
		m_convertgui->joblist.InsertColumn(0, GetResString(IDS_DL_FILENAME) ,LVCFMT_LEFT,350,0);
		m_convertgui->joblist.InsertColumn(1,GetResString(IDS_STATUS),LVCFMT_LEFT,110,1);
		m_convertgui->joblist.InsertColumn(2,GetResString(IDS_DL_SIZE),LVCFMT_LEFT,150,2);
		m_convertgui->joblist.InsertColumn(3,GetResString(IDS_FILEHASH),LVCFMT_LEFT,150,3);

		if (!pfconverting==NULL)  { 
			UpdateGUI(pfconverting);
			UpdateGUI(50,GetResString(IDS_IMP_FETCHSTATUS),true);
		}

		// set gui labels
		m_convertgui->SetDlgItemText(IDC_ADDITEM,GetResString(IDS_IMP_ADDBTN));
		m_convertgui->SetDlgItemText(IDC_RETRY,GetResString(IDS_IMP_RETRYBTN));
		m_convertgui->SetDlgItemText(IDC_CONVREMOVE,GetResString(IDS_IMP_REMOVEBTN));
		m_convertgui->SetDlgItemText(IDC_HIDECONVDLG,GetResString(IDS_FD_CLOSE));		

		
		// fill joblist
		ConvertJob* job;
		for(POSITION pos = m_jobs.GetHeadPosition(); pos != NULL; m_jobs.GetNext(pos)){
			job=m_jobs.GetAt(pos);
			m_convertgui->AddJob(job);
			UpdateGUI(job);
		}
	}
}

void CPartFileConvert::CloseGUI(){

	if (m_convertgui==NULL) return;

	m_convertgui->DestroyWindow();
	ClosedGUI();
	
}

void CPartFileConvert::ClosedGUI() {
	m_convertgui=NULL;
}

void CPartFileConvert::RemoveAllJobs(){
	for(POSITION pos = m_jobs.GetHeadPosition(); pos != NULL; m_jobs.GetNext(pos)){
		if (m_convertgui) m_convertgui->RemoveJob(m_jobs.GetAt(pos));
		delete m_jobs.GetAt(pos);
	}
	m_jobs.RemoveAll();
}

void CPartFileConvert::RemoveJob(ConvertJob* job){
	for(POSITION pos = m_jobs.GetHeadPosition(); pos != NULL; m_jobs.GetNext(pos)){
		ConvertJob* del=m_jobs.GetAt(pos);
		if (del==job) {
			if (m_convertgui) m_convertgui->RemoveJob(del);
			m_jobs.RemoveAt(pos);
			delete del;
			if (m_jobs.GetCount()==0) return;
		}
	}
}

void CPartFileConvert::RemoveAllSuccJobs(){
	for(POSITION pos = m_jobs.GetHeadPosition(); pos != NULL; m_jobs.GetNext(pos)){
		ConvertJob* del=m_jobs.GetAt(pos);
		if (del->state==CONV_OK) {
			if (m_convertgui) m_convertgui->RemoveJob(del);
			m_jobs.RemoveAt(pos);
			delete del;	
		}
	}
}

CString CPartFileConvert::GetReturncodeText(int ret) {
	switch (ret) {
		case CONV_OK				: return GetResString(IDS_DL_TRANSFCOMPL);
		case CONV_INPROGRESS		: return GetResString(IDS_IMP_INPROGR);
		case CONV_OUTOFDISKSPACE	: return GetResString(IDS_IMP_ERR_DISKSP);
		case CONV_PARTMETNOTFOUND	: return GetResString(IDS_IMP_ERR_PARTMETIO);
		case CONV_IOERROR			: return GetResString(IDS_IMP_ERR_IO);
		case CONV_FAILED			: return GetResString(IDS_IMP_ERR_FAILED);
		case CONV_QUEUE				: return GetResString(IDS_IMP_STATUSQUEUED);
		case CONV_ALREADYEXISTS		: return GetResString(IDS_IMP_ALRDWL);
		default: return "?";
	}
}









// Modless Dialog Implementation
// CModeless dialog

IMPLEMENT_DYNAMIC(CModeless, CDialog)
CModeless::CModeless(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CModeless::IDD, pParent)
{
	m_pParent = pParent;
}

CModeless::~CModeless()
{
}

void CModeless::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_CONV_PB_CURRENT, pb_current);
	DDX_Control(pDX, IDC_JOBLIST, joblist);
}


BEGIN_MESSAGE_MAP(CModeless, CResizableDialog)
	ON_BN_CLICKED(IDC_HIDECONVDLG, OnBnClickedOk)
	ON_BN_CLICKED(IDC_ADDITEM, OnAddFolder)
	ON_BN_CLICKED(IDC_RETRY,RetrySel)
	ON_BN_CLICKED(IDC_CONVREMOVE,RemoveSel)
END_MESSAGE_MAP()


// CModeless message handlers

void CModeless::OnBnClickedOk()
{
    DestroyWindow();
}

void CModeless::OnCancel() {
    DestroyWindow();
}

void CModeless::PostNcDestroy()
{
	CPartFileConvert::ClosedGUI();

	CResizableDialog::PostNcDestroy();
	delete this;
}

void CModeless::OnAddFolder() {
	// browse...
	
	LPMALLOC pMalloc;
	SHGetMalloc(&pMalloc);

	// buffer - a place to hold the file system pathname
	char buffer[MAX_PATH];

	LPITEMIDLIST pidlRoot;

	// This struct holds the various options for the dialog
	BROWSEINFO bi;
	bi.hwndOwner = this->m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = buffer;
	CString title=GetResString(IDS_IMP_SELFOLDER);
	bi.lpszTitle = title.GetBuffer(title.GetLength());
	bi.ulFlags =  BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_SHAREABLE ;
	bi.lpfn = NULL;

	// Now cause the dialog to appear.
	if((pidlRoot = SHBrowseForFolder(&bi)) == NULL){
		// User hit cancel - do whatever
		return;
	}

	bool removesrc;
	int antw=MessageBox(GetResString(IDS_IMP_DELSRC), GetResString(IDS_REMOVE) ,MB_YESNOCANCEL);
	if (antw==IDCANCEL) return;
	else removesrc=(antw==IDYES);

	//
	// Again, almost undocumented.  How to get a ASCII pathname
	// from the LPITEMIDLIST struct.  I guess you just have to
	// "know" this stuff.
	//
	if(SHGetPathFromIDList(pidlRoot, buffer)){
		// Do something with the converted string.
		CPartFileConvert::ScanFolderToAdd(CString(buffer), removesrc);
	}

	// Free the returned item identifier list using the
	// shell's task allocator!Arghhhh.
	pMalloc->Free(pidlRoot);
}

void CModeless::UpdateJobInfo(ConvertJob* job) {

	if (job==NULL) {
		SetDlgItemText(IDC_CURJOB, GetResString(IDS_FSTAT_WAITING) );
		SetDlgItemText(IDC_CONV_PROZENT, "" );
		pb_current.SetPos(0);
		SetDlgItemText(IDC_CONV_PB_LABEL,"");
		return;
	}
	
	CString buffer;

	// search jobitem in listctrl
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)job;
	sint32 itemnr = joblist.FindItem(&find);
	if (itemnr != (-1)) {
		joblist.SetItemText(itemnr,0, job->filename.IsEmpty()?job->folder:job->filename  );
		joblist.SetItemText(itemnr,1, CPartFileConvert::GetReturncodeText(job->state) );
		buffer="";
		if (job->size>0)
			buffer.Format(GetResString(IDS_IMP_SIZE),CastItoXBytes(job->size),CastItoXBytes(job->spaceneeded));
		joblist.SetItemText(itemnr,2, buffer );
		joblist.SetItemText(itemnr,3, job->filehash);

	} else {
//		AddJob(job);	why???
	}
}

void CModeless::RemoveJob(ConvertJob* job) {
	// search jobitem in listctrl
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)job;
	sint32 itemnr = joblist.FindItem(&find);
	if (itemnr != (-1)) {
		joblist.DeleteItem(itemnr);
	}
}

void CModeless::AddJob(ConvertJob* job) {
    int ix=joblist.InsertItem(LVIF_TEXT|LVIF_PARAM,joblist.GetItemCount(),job->folder,0,0,0,(LPARAM)job);
	joblist.SetItemText(ix,1,CPartFileConvert::GetReturncodeText(job->state));
}

void CModeless::RemoveSel() {
	if (joblist.GetSelectedCount()==0) return;

	ConvertJob* job;
	POSITION pos = joblist.GetFirstSelectedItemPosition(); 
	while(pos != NULL) 
	{ 
		int index = joblist.GetNextSelectedItem(pos);
		if(index > -1) 
		{
			job=(ConvertJob*)joblist.GetItemData(index);
			if (job->state != CONV_INPROGRESS) {
				RemoveJob(job); // from list
				CPartFileConvert::RemoveJob(job);
				pos = joblist.GetFirstSelectedItemPosition();
			}
		} 
	}
}

void CModeless::RetrySel(){

	if (joblist.GetSelectedCount()==0) return;

	ConvertJob* job;
	POSITION pos = joblist.GetFirstSelectedItemPosition(); 
	while(pos != NULL) 
	{ 
		int index = joblist.GetNextSelectedItem(pos); 
		if(index > -1) 
		{
			job=(ConvertJob*)joblist.GetItemData(index);
			if (job->state != CONV_OK && job->state !=CONV_INPROGRESS) {
				UpdateJobInfo(job);
				job->state=CONV_QUEUE;
			}
		} 
	} 

	CPartFileConvert::StartThread();
}
