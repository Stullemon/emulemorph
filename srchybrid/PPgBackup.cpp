// PPgBackup.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgBackup.h"
#include "XMessageBox.h"
#include "string.h"



// CPPgBackup dialog

IMPLEMENT_DYNAMIC(CPPgBackup, CPropertyPage)
CPPgBackup::CPPgBackup()
: CPropertyPage(CPPgBackup::IDD)
{
}

CPPgBackup::~CPPgBackup()
{
}

void CPPgBackup::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}



BOOL CPPgBackup::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	LoadSettings();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}



BEGIN_MESSAGE_MAP(CPPgBackup, CPropertyPage)
	ON_BN_CLICKED(IDC_BACKUPNOW, OnBnClickedBackupnow)
	ON_BN_CLICKED(IDC_DAT, OnBnClickedDat)
	ON_BN_CLICKED(IDC_MET, OnBnClickedMet)
	ON_BN_CLICKED(IDC_INI, OnBnClickedIni)
	ON_BN_CLICKED(IDC_PART, OnBnClickedPart)
	ON_BN_CLICKED(IDC_PARTMET, OnBnClickedPartMet)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDC_AUTOBACKUP, OnBnClickedAutobackup)
	ON_BN_CLICKED(IDC_AUTOBACKUP2, OnBnClickedAutobackup2) //EastShare - Added by Pretender, Double Backup
END_MESSAGE_MAP()

void CPPgBackup::OnBnClickedDat()
{
	if ((bool)IsDlgButtonChecked(IDC_DAT)) {
		GetDlgItem(IDC_BACKUPNOW)->EnableWindow(IsDlgButtonChecked(IDC_DAT));
	} else {
		if (!(bool)IsDlgButtonChecked(IDC_DAT) && !(bool)IsDlgButtonChecked(IDC_MET) && !(bool)IsDlgButtonChecked(IDC_INI) && !(bool)IsDlgButtonChecked(IDC_PART) && !(bool)IsDlgButtonChecked(IDC_PARTMET))
			GetDlgItem(IDC_BACKUPNOW)->EnableWindow(false);
	}

}

void CPPgBackup::OnBnClickedMet()
{
	if ((bool)IsDlgButtonChecked(IDC_MET)) {
		GetDlgItem(IDC_BACKUPNOW)->EnableWindow(IsDlgButtonChecked(IDC_MET));
	} else {
		if (!(bool)IsDlgButtonChecked(IDC_DAT) && !(bool)IsDlgButtonChecked(IDC_MET) && !(bool)IsDlgButtonChecked(IDC_INI) && !(bool)IsDlgButtonChecked(IDC_PART) && !(bool)IsDlgButtonChecked(IDC_PARTMET))
			GetDlgItem(IDC_BACKUPNOW)->EnableWindow(false);
	}

}

void CPPgBackup::OnBnClickedIni()
{
	if ((bool)IsDlgButtonChecked(IDC_INI)) {
		GetDlgItem(IDC_BACKUPNOW)->EnableWindow(IsDlgButtonChecked(IDC_INI));
	} else {
		if (!(bool)IsDlgButtonChecked(IDC_DAT) && !(bool)IsDlgButtonChecked(IDC_MET) && !(bool)IsDlgButtonChecked(IDC_INI) && !(bool)IsDlgButtonChecked(IDC_PART) && !(bool)IsDlgButtonChecked(IDC_PARTMET))
			GetDlgItem(IDC_BACKUPNOW)->EnableWindow(false);
	}

}

void CPPgBackup::OnBnClickedPart()
{
	if ((bool)IsDlgButtonChecked(IDC_PART)) {
		GetDlgItem(IDC_BACKUPNOW)->EnableWindow(IsDlgButtonChecked(IDC_PART));
	} else {
		if (!(bool)IsDlgButtonChecked(IDC_DAT) && !(bool)IsDlgButtonChecked(IDC_MET) && !(bool)IsDlgButtonChecked(IDC_INI) && !(bool)IsDlgButtonChecked(IDC_PART) && !(bool)IsDlgButtonChecked(IDC_PARTMET))
			GetDlgItem(IDC_BACKUPNOW)->EnableWindow(false);
	}

}

void CPPgBackup::OnBnClickedPartMet()
{
	if ((bool)IsDlgButtonChecked(IDC_PARTMET)) {
		GetDlgItem(IDC_BACKUPNOW)->EnableWindow(IsDlgButtonChecked(IDC_PARTMET));
	} else {
		if (!(bool)IsDlgButtonChecked(IDC_DAT) && !(bool)IsDlgButtonChecked(IDC_MET) && !(bool)IsDlgButtonChecked(IDC_INI) && !(bool)IsDlgButtonChecked(IDC_PART) && !(bool)IsDlgButtonChecked(IDC_PARTMET))
			GetDlgItem(IDC_BACKUPNOW)->EnableWindow(false);
	}

}


void CPPgBackup::OnBnClickedBackupnow()
{
	char buffer[200];
	y2All = FALSE;
	if ((bool)IsDlgButtonChecked(IDC_DAT))
	{
		Backup("*.dat", true);
		CheckDlgButton(IDC_DAT,BST_UNCHECKED);
	}

	if ((bool)IsDlgButtonChecked(IDC_MET))
	{
		Backup("*.met", true);
		CheckDlgButton(IDC_MET,BST_UNCHECKED);
	}

	if ((bool)IsDlgButtonChecked(IDC_INI))
	{
		Backup("*.ini", true);
		CheckDlgButton(IDC_INI,BST_UNCHECKED);
	}

	if ((bool)IsDlgButtonChecked(IDC_PARTMET))
	{
		Backup2("*.part.met");
		CheckDlgButton(IDC_PARTMET,BST_UNCHECKED);
	}

	if ((bool)IsDlgButtonChecked(IDC_PART))
	{
		sprintf(buffer,"Because of their size, backing up *.part files may take a few minutes.\nAre you sure you want to do this?");
		if(MessageBox(buffer,"Are you sure?",MB_ICONQUESTION|MB_YESNO)== IDYES)
			Backup2("*.part");
		CheckDlgButton(IDC_PART,BST_UNCHECKED);

	}

	MessageBox("File(s) Copied Successfully.", "BackUp complete.", MB_OK);
	y2All = FALSE;
}



void CPPgBackup::Backup(LPCSTR extensionToBack, BOOL conFirm)  
{
	WIN32_FIND_DATA FileData; 
	HANDLE hSearch; 
	char buffer[200];
	//CString szDirPath = CString(theApp.glob_prefs->GetAppDir());
	CString szDirPath = CString(theApp.glob_prefs->GetConfigDir());
	char szNewPath[MAX_PATH]; 

	SetCurrentDirectory(szDirPath);
	BOOL error = FALSE;
	BOOL OverWrite = TRUE;
	szDirPath +="Backup\\";

	BOOL fFinished = FALSE; 

	// Create a new directory if one does not exist
	if(!PathFileExists(szDirPath))
		CreateDirectory(szDirPath, NULL);

	// Start searching for files in the current directory. 

	hSearch = FindFirstFile(extensionToBack, &FileData); 
	if (hSearch == INVALID_HANDLE_VALUE) 
	{ 
		error = TRUE;
	} 

	// Copy each file to the new directory 
	CString str;
	while (!fFinished && !error) 
	{ 
		lstrcpy(szNewPath, szDirPath); 
		lstrcat(szNewPath, FileData.cFileName); 

		if(PathFileExists(szNewPath))
		{
			if (conFirm)
			{
				if (y2All == FALSE)
				{
					sprintf(buffer, "File %s Already Exists. OverWrite It?", FileData.cFileName);
					int rc = ::XMessageBox(m_hWnd,buffer,"OverWrite?",MB_YESNO|MB_YESTOALL|MB_ICONQUESTION);
					if (rc == IDYES)
						OverWrite = TRUE;
					else if (rc == IDYESTOALL)
					{
						OverWrite = TRUE;
						y2All = TRUE;
					}
					else 
						OverWrite = FALSE;
				} else
					OverWrite = TRUE;
			} 
			else
				OverWrite = TRUE;
		}	
		if(OverWrite)
			CopyFile(FileData.cFileName, szNewPath, FALSE);

		if (!FindNextFile(hSearch, &FileData)) 
		{
			if (GetLastError() == ERROR_NO_MORE_FILES) 
			{ 
				//MessageBox("File Copied Successfully.", "BackUp complete", MB_OK); 
				fFinished = TRUE; 

			} 
			else 
			{ 
				error = TRUE;
			} 
		}

	} 


	// Close the search handle. 
	if (!FindClose(hSearch)) 
	{ 
		error = TRUE;
	} 
	if (error)
		MessageBox("Error encountered during backup","Error",MB_OK);
}


void CPPgBackup::Backup2(LPCSTR extensionToBack)  
{

	WIN32_FIND_DATA FileData;   
	HANDLE hSearch;   
	char buffer[200];  


	//CString szDirPath = CString(theApp.glob_prefs->GetAppDir());  
	CString szDirPath = CString(theApp.glob_prefs->GetConfigDir());
	CString szTempPath = CString(theApp.glob_prefs->GetTempDir());  
	char szNewPath[MAX_PATH]; 

	BOOL fFinished = FALSE;     
	BOOL error = FALSE;  
	BOOL OverWrite = TRUE;  
	szDirPath +="Backup\\";

	if(!PathFileExists(szDirPath))  
		CreateDirectory(szDirPath, NULL);  

	szDirPath+="Temp\\";  

	if(!PathFileExists(szDirPath))  
		CreateDirectory(szDirPath, NULL);  


	// Start searching for files in the current directory.   
	SetCurrentDirectory(szTempPath);  

	hSearch = FindFirstFile(extensionToBack, &FileData);   

	if (hSearch == INVALID_HANDLE_VALUE)   
	{   
		error = TRUE;
	}   

	// Copy each file to the new directory   
	while (!fFinished && !error)   
	{   
		lstrcpy(szNewPath, szDirPath);   
		lstrcat(szNewPath, FileData.cFileName);   

		//MessageBox(szNewPath,"New Path",MB_OK);  
		if(PathFileExists(szNewPath))  
		{  
				if (y2All == FALSE)
				{
					sprintf(buffer, "File %s Already Exists. OverWrite It?", FileData.cFileName);
					int rc = ::XMessageBox(m_hWnd,buffer,"OverWrite?",MB_YESNO|MB_YESTOALL|MB_ICONQUESTION);
					if (rc == IDYES)
						OverWrite = TRUE;
					else if (rc == IDYESTOALL)
					{
						OverWrite = TRUE;
						y2All = TRUE;
					}
					else 
						OverWrite = FALSE;
				} else
					OverWrite = TRUE;  
		}  

		if(OverWrite)  
			CopyFile(FileData.cFileName, szNewPath, FALSE);  

		if (!FindNextFile(hSearch, &FileData))   
		{  
			if (GetLastError() == ERROR_NO_MORE_FILES)   
			{   

				fFinished = TRUE;   
			}   
			else   
			{   
				error = TRUE;  
			}   
		}  

	}   

	// Close the search handle.   
	if (!FindClose(hSearch))   
	{   
		error = TRUE;  
	}   
	SetCurrentDirectory(CString(theApp.glob_prefs->GetConfigDir()));  

	if (error)  
		MessageBox("Error encountered during backup","Error",MB_OK);  

} 

void CPPgBackup::OnBnClickedSelectall()
{
	CheckDlgButton(IDC_DAT,BST_CHECKED);
	CheckDlgButton(IDC_MET,BST_CHECKED);
	CheckDlgButton(IDC_INI,BST_CHECKED);
	CheckDlgButton(IDC_PART,BST_CHECKED);
	CheckDlgButton(IDC_PARTMET,BST_CHECKED);
	GetDlgItem(IDC_BACKUPNOW)->EnableWindow(true);
}

void CPPgBackup::OnBnClickedAutobackup()
{
	SetModified();
}

//EastShare - Added by Pretender, Double Backup
void CPPgBackup::OnBnClickedAutobackup2()
{
	SetModified();
}
//EastShare - Added by Pretender, Double Backup

void CPPgBackup::LoadSettings(void)
{
	if(m_hWnd)
	{
		if(theApp.glob_prefs->GetAutoBackup())
			CheckDlgButton(IDC_AUTOBACKUP,1);
		else
			CheckDlgButton(IDC_AUTOBACKUP,0);
		//EastShare Start - Added by Pretender, Double Backup
		if(theApp.glob_prefs->GetAutoBackup2())
			CheckDlgButton(IDC_AUTOBACKUP2,1);
		else
			CheckDlgButton(IDC_AUTOBACKUP2,0);
		//EastShare End - Added by Pretender, Double Backup
	}
}

BOOL CPPgBackup::OnApply()
{
	theApp.glob_prefs->SetAutoBackup(IsDlgButtonChecked(IDC_AUTOBACKUP));
	
	//EastShare Start - Added by Pretender, Double Backup
	theApp.glob_prefs->SetAutoBackup2(IsDlgButtonChecked(IDC_AUTOBACKUP2)); 

	LoadSettings();
	SetModified();
	//EastShare End - Added by Pretender, Double Backup

	return CPropertyPage::OnApply();
}

//eastshare start - added by linekin, backup backup
void CPPgBackup::Backup3()
{
	WIN32_FIND_DATA FileData; 
	HANDLE hSearch; 
	CString szDirPath = CString(theApp.glob_prefs->GetConfigDir())+"Backup\\";
	if(!PathFileExists(szDirPath)) return;
	char szNewPath[MAX_PATH]; 

	SetCurrentDirectory(szDirPath);
	BOOL error = FALSE;
	szDirPath = CString(theApp.glob_prefs->GetConfigDir())+"Backup2\\";

	BOOL fFinished = FALSE; 

	// Create a new directory if one does not exist
	if(!PathFileExists(szDirPath))
		CreateDirectory(szDirPath, NULL);

	// Start searching for files in the current directory. 

	hSearch = FindFirstFile("*.*", &FileData); 
	if (hSearch == INVALID_HANDLE_VALUE) 
	{ 
		error = TRUE;
	} 

	// Copy each file to the new directory 
	while (!fFinished && !error) 
	{ 
		lstrcpy(szNewPath, szDirPath); 
		lstrcat(szNewPath, FileData.cFileName); 

		CopyFile(FileData.cFileName, szNewPath, FALSE);

		if (!FindNextFile(hSearch, &FileData)) 
		{
			if (GetLastError() == ERROR_NO_MORE_FILES) 
			{ 
				//MessageBox("File Copied Successfully.", "BackUp complete", MB_OK); 
				fFinished = TRUE; 

			} 
			else 
			{ 
				error = TRUE;
			} 
		}

	} 


	// Close the search handle. 
	if (!FindClose(hSearch)) 
	{ 
		error = TRUE;
	} 
	if (error)
		MessageBox("Error encountered during backup","Error",MB_OK);
}
//eastshare end