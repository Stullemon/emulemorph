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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "stdafx.h" 
#include "emule.h"
#include "ED2kLinkDlg.h"
#include "KnownFile.h"
#include "partfile.h"
#include "OtherFunctions.h"
#include "preferences.h"
#include "shahashset.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CED2kLinkDlg, CResizablePage) 

BEGIN_MESSAGE_MAP(CED2kLinkDlg, CResizablePage) 
	ON_BN_CLICKED(IDC_LD_CLIPBOARDBUT, OnBnClickedClipboard)
	ON_BN_CLICKED(IDC_LD_SOURCECHE, OnSettingsChange)
	ON_BN_CLICKED(IDC_LD_EMULEHASHCHE, OnSettingsChange)
	ON_BN_CLICKED(IDC_LD_HTMLCHE, OnSettingsChange)
	ON_BN_CLICKED(IDC_LD_HOSTNAMECHE, OnSettingsChange)
	ON_BN_CLICKED(IDC_LD_HASHSETCHE, OnSettingsChange)
END_MESSAGE_MAP() 

CED2kLinkDlg::CED2kLinkDlg() 
   : CResizablePage(CED2kLinkDlg::IDD, IDS_CMT_READALL) 
{ 
	m_strCaption = GetResString(IDS_FILELINK);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
} 

CED2kLinkDlg::~CED2kLinkDlg() 
{ 
} 

void CED2kLinkDlg::DoDataExchange(CDataExchange* pDX) 
{ 
	CResizablePage::DoDataExchange(pDX); 
	DDX_Control(pDX, IDC_LD_LINKEDI, m_ctrlLinkEdit);
} 


BOOL CED2kLinkDlg::OnInitDialog()
{ 
	CResizablePage::OnInitDialog(); 
	InitWindowStyles(this);

	AddAnchor(IDC_LD_LINKGROUP,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_LD_LINKEDI,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_LD_CLIPBOARDBUT,BOTTOM_RIGHT);

	AddAnchor(IDC_LD_BASICGROUP,BOTTOM_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_LD_SOURCECHE,BOTTOM_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_LD_EMULEHASHCHE,BOTTOM_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_LD_ADVANCEDGROUP,BOTTOM_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_LD_HTMLCHE,BOTTOM_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_LD_HASHSETCHE,BOTTOM_LEFT,BOTTOM_LEFT);
	//AddAnchor(IDC_LD_KADLOWIDCHE,BOTTOM_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_LD_HOSTNAMECHE,BOTTOM_LEFT,BOTTOM_LEFT);

	// enabled/disable checkbox depending on situation
	if (theApp.IsConnected() && !theApp.IsFirewalled())
		GetDlgItem(IDC_LD_SOURCECHE)->EnableWindow(TRUE);
	else{
		GetDlgItem(IDC_LD_SOURCECHE)->EnableWindow(FALSE);
		((CButton*)GetDlgItem(IDC_LD_SOURCECHE))->SetCheck(BST_UNCHECKED);
	}
	if (theApp.IsConnected() && !theApp.IsFirewalled() && !CString(thePrefs.GetYourHostname()).IsEmpty() && CString(thePrefs.GetYourHostname()).Find(_T(".")) != -1)
		GetDlgItem(IDC_LD_HOSTNAMECHE)->EnableWindow(TRUE);
	else{
		GetDlgItem(IDC_LD_HOSTNAMECHE)->EnableWindow(FALSE);
		((CButton*)GetDlgItem(IDC_LD_HOSTNAMECHE))->SetCheck(BST_UNCHECKED);
	}
	//hashsetlink - check if at least one file has a hasset
	BOOL bShow = FALSE;
	for (int i = 0; i != m_paFiles->GetSize(); i++){
		if (!((*m_paFiles)[i]->GetHashCount() > 0 && (*m_paFiles)[i]->GetHashCount() == (*m_paFiles)[i]->GetED2KPartHashCount())){
			continue;
		}
		bShow = TRUE;
		break;
	}
	GetDlgItem(IDC_LD_HASHSETCHE)->EnableWindow(bShow);
	if (!bShow)
		((CButton*)GetDlgItem(IDC_LD_HASHSETCHE))->SetCheck(BST_UNCHECKED);

	//aich hash - check if at least one file has a valid hash
	bShow = FALSE;
	for (int i = 0; i != m_paFiles->GetSize(); i++){
		if ((*m_paFiles)[i]->GetAICHHashset()->HasValidMasterHash() 
			&& ((*m_paFiles)[i]->GetAICHHashset()->GetStatus() == AICH_VERIFIED || (*m_paFiles)[i]->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE))
		{	
			bShow = TRUE;
			break;
		}
	}
	GetDlgItem(IDC_LD_EMULEHASHCHE)->EnableWindow(bShow);
	if (!bShow)
		((CButton*)GetDlgItem(IDC_LD_EMULEHASHCHE))->SetCheck(BST_UNCHECKED);
	else
		((CButton*)GetDlgItem(IDC_LD_EMULEHASHCHE))->SetCheck(BST_CHECKED);

	Localize(); 
	UpdateLink();

	return TRUE; 
} 

void CED2kLinkDlg::Localize(void)
{ 
	GetDlgItem(IDC_LD_BASICGROUP)->SetWindowText(GetResString(IDS_LD_BASICOPT));
	GetDlgItem(IDC_LD_SOURCECHE)->SetWindowText(GetResString(IDS_LD_ADDSOURCE)); 
	GetDlgItem(IDC_LD_EMULEHASHCHE)->SetWindowText(GetResString(IDS_LD_EMULEHASH)); 
	GetDlgItem(IDC_LD_ADVANCEDGROUP)->SetWindowText(GetResString(IDS_LD_ADVANCEDOPT)); 
	GetDlgItem(IDC_LD_HTMLCHE)->SetWindowText(GetResString(IDS_LD_ADDHTML)); 
	GetDlgItem(IDC_LD_HASHSETCHE)->SetWindowText(GetResString(IDS_LD_ADDHASHSET)); 
	//GetDlgItem(IDC_LD_KADLOWIDCHE)->SetWindowText(GetResString(IDS_LD_PREFERKAD)); 
	GetDlgItem(IDC_LD_LINKGROUP)->SetWindowText(GetResString(IDS_LD_ED2KLINK)); 
	GetDlgItem(IDC_LD_CLIPBOARDBUT)->SetWindowText(GetResString(IDS_LD_COPYCLIPBOARD));
	GetDlgItem(IDC_LD_HOSTNAMECHE)->SetWindowText(GetResString(IDS_LD_HOSTNAME)); 
}

void CED2kLinkDlg::UpdateLink(){
	CString strLinks;
	CString strBuffer;
	const bool bHashset = ((CButton*)GetDlgItem(IDC_LD_HASHSETCHE))->GetCheck() == BST_CHECKED;
	const bool bHTML = ((CButton*)GetDlgItem(IDC_LD_HTMLCHE))->GetCheck() == BST_CHECKED;
	const bool bSource = ((CButton*)GetDlgItem(IDC_LD_SOURCECHE))->GetCheck() == BST_CHECKED && theApp.IsConnected() && !theApp.IsFirewalled();
	const bool bHostname = ((CButton*)GetDlgItem(IDC_LD_HOSTNAMECHE))->GetCheck() == BST_CHECKED && theApp.IsConnected() && !theApp.IsFirewalled()
		&& !CString(thePrefs.GetYourHostname()).IsEmpty() && CString(thePrefs.GetYourHostname()).Find(_T(".")) != -1;
	const bool bEMHash = ((CButton*)GetDlgItem(IDC_LD_EMULEHASHCHE))->GetCheck() == BST_CHECKED;

	for (int i = 0; i != m_paFiles->GetSize(); i++){
		if (!strLinks.IsEmpty())
			strLinks += _T("\r\n\r\n");

		if (bHTML)
			strLinks += _T("<a href=\"");
		
		strLinks += CreateED2kLink((*m_paFiles)[i], false);
		
		if (bHashset && (*m_paFiles)[i]->GetHashCount() > 0 && (*m_paFiles)[i]->GetHashCount() == (*m_paFiles)[i]->GetED2KPartHashCount()){
			strLinks += _T("p=");
			for (int j = 0; j < (*m_paFiles)[i]->GetHashCount(); j++)
			{
				if (j > 0)
					strLinks += _T(':');
				strLinks += EncodeBase16((*m_paFiles)[i]->GetPartHash(j), 16);
			}
			strLinks += _T('|');
		}

		if (bEMHash && (*m_paFiles)[i]->GetAICHHashset()->HasValidMasterHash() && 
			((*m_paFiles)[i]->GetAICHHashset()->GetStatus() == AICH_VERIFIED || (*m_paFiles)[i]->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE))
		{
			strBuffer.Format(_T("h=%s|"), (*m_paFiles)[i]->GetAICHHashset()->GetMasterHash().GetString() );
			strLinks += strBuffer;			
		}

		strLinks += _T('/');
		if (bHostname){
			strBuffer.Format(_T("|sources,%s:%i|/"), thePrefs.GetYourHostname(), thePrefs.GetPort() );
			strLinks += strBuffer;
		}
		else if(bSource){
			uint32 dwID = theApp.GetID();
			strBuffer.Format(_T("|sources,%i.%i.%i.%i:%i|/"),(uint8)dwID,(uint8)(dwID>>8),(uint8)(dwID>>16),(uint8)(dwID>>24), thePrefs.GetPort() );
			strLinks += strBuffer;
		}

		if (bHTML)
			strLinks += _T("\">") + StripInvalidFilenameChars((*m_paFiles)[i]->GetFileName(), true) + _T("</a>");
	}
	m_ctrlLinkEdit.SetWindowText(strLinks);

}
void CED2kLinkDlg::OnBnClickedClipboard()
{
	CString strBuffer;
	m_ctrlLinkEdit.GetWindowText(strBuffer);
	theApp.CopyTextToClipboard(strBuffer);
}

void CED2kLinkDlg::OnSettingsChange()
{
	UpdateLink();
}
