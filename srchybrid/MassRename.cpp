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

// Original file: Written by Mighty Knife, EMule Morph Team

#include "stdafx.h"
#include "resource.h"
#include "MassRename.h"
#include "OtherFunctions.h"
#include ".\massrename.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define IDC_FILENAMEMASKEDIT 100

// CMassRenameEdit edit control

BEGIN_MESSAGE_MAP(CMassRenameEdit, CEdit)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_MESSAGE(WM_PASTE,OnPaste)
END_MESSAGE_MAP()


IMPLEMENT_DYNAMIC(CMassRenameEdit, CEdit)

IMPLEMENT_DYNAMIC(CMassRenameDialog, CDialog)
CMassRenameDialog::CMassRenameDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CMassRenameDialog::IDD, pParent)
{
}

void CMassRenameEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Get the current string before edit and the selection - then default message processing
	GetSel (Start1,End1);
	GetWindowText (m_BeforeEdit);
	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

void CMassRenameEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Get the current string before edit and the selection - then default message processing
	GetSel (Start1,End1);
	GetWindowText (m_BeforeEdit);
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

LRESULT CMassRenameEdit::OnPaste(WPARAM wParam, LPARAM lParam)
{
	// Get the current string before edit and the selection - then default message processing
	GetSel (Start1,End1);
	GetWindowText (m_BeforeEdit);
	return Default();
}

// CMassRenameDialog dialog

CMassRenameDialog::~CMassRenameDialog()
{
}

void CMassRenameDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMassRenameDialog, CDialog)
	ON_EN_VSCROLL(IDC_OLDFILENAMESEDIT, OnEnVscrollOldfilenamesedit)
	ON_EN_VSCROLL(IDC_NEWFILENAMESEDITLEFT, OnEnVscrollNewfilenameseditLeft)
	ON_EN_VSCROLL(IDC_NEWFILENAMESEDITRIGHT, OnEnVscrollNewfilenameseditRight)
	ON_BN_CLICKED(ID_MASSRENAMEOK, OnBnClickedMassrenameok)
	ON_BN_CLICKED(ID_MASSRENAMECANCEL, OnBnClickedMassrenamecancel)
	ON_EN_SETFOCUS(IDC_FILENAMEMASKEDIT, OnEnSetfocusFilenamemaskedit)
	ON_BN_CLICKED(IDC_FILENAMELEFT, OnBnClickedFilenameleft)
	ON_BN_CLICKED(IDC_FILENAMERIGHT, OnBnClickedFilenameright)
	ON_BN_CLICKED(IDC_RESETBUTTON, OnBnClickedReset)
	ON_WM_SHOWWINDOW()
	ON_WM_CHAR()
	ON_EN_CHANGE(IDC_FILENAMEMASKEDIT, OnEnChangeFilenamemaskedit)
END_MESSAGE_MAP()

void CMassRenameDialog::OnOK() {
}

void CMassRenameDialog::OnCancel() {
}

BOOL CMassRenameDialog::OnInitDialog() {
	CDialog::OnInitDialog();
	InitWindowStyles(this);

	// Go through the list of files, collect all filenames to one string and
	// show it in the list of the old filenames
	POSITION pos = m_FileList.GetHeadPosition();
	CString FileListString;
	while (pos != NULL) {
		CKnownFile* file = m_FileList.GetAt (pos);
		if (FileListString=="") FileListString = file->GetFileName ();
		else FileListString = FileListString + "\r\n" + file->GetFileName ();
		m_FileList.GetNext (pos);
	}
	OldFN = (CRichEditCtrl*) GetDlgItem(IDC_OLDFILENAMESEDIT);
	NFNLeft = (CRichEditCtrl*) GetDlgItem(IDC_NEWFILENAMESEDITLEFT);
	NFNRight = (CRichEditCtrl*) GetDlgItem(IDC_NEWFILENAMESEDITRIGHT);
	
	OldFN->SetWindowText (FileListString);
	NFNLeft->SetWindowText (FileListString);
	NFNRight->SetWindowText (FileListString);

	// Show the left justify edit control
	NFNLeft->ModifyStyle (0,WS_VISIBLE);
	NFNRight->ModifyStyle (WS_VISIBLE,0);

	CheckDlgButton (IDC_FILENAMELEFT,BST_CHECKED);

	// Create a new CEdit to replace the IDC_FILENAMEMASKEDIT edit control
	CEdit* FNEdit = (CEdit*) GetDlgItem(IDC_FILENAMEMASKEDITTEMPLATE);
	int Style = FNEdit->GetStyle ();
	int ExStyle = FNEdit->GetExStyle ();
	// Hide the old control and show the new one instead
	FNEdit->ModifyStyle (WS_VISIBLE,0);
	MassRenameEdit = new CMassRenameEdit;
	MassRenameEdit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
      CRect (6,26,474,49), this, IDC_FILENAMEMASKEDIT);
	// Take the font of the old control
	MassRenameEdit->SetFont (FNEdit->GetFont ());

	m_DontTrackKeys = false;

	// activate windows messages for scrolling events in the filename window(s)
	NFNLeft->SetEventMask(NFNLeft->GetEventMask() | ENM_SCROLL);
	NFNRight->SetEventMask(NFNRight->GetEventMask() | ENM_SCROLL);
	OldFN->SetEventMask(OldFN->GetEventMask() | ENM_SCROLL);

	return TRUE;
}

void CMassRenameDialog::OnEnVscrollOldfilenamesedit()
{
	// Scroll the "new filename" windows to the correct position
	int VDiff = OldFN->GetFirstVisibleLine ()-NFNLeft->GetFirstVisibleLine();
	if (VDiff != 0) NFNLeft->LineScroll (VDiff,0);
	VDiff = OldFN->GetFirstVisibleLine ()-NFNRight->GetFirstVisibleLine();
	if (VDiff != 0) NFNRight->LineScroll (VDiff,0);
}

void CMassRenameDialog::OnEnVscrollNewfilenameseditLeft()
{
	// Scroll the "old filename" windows to the correct position
	int VDiff = NFNLeft->GetFirstVisibleLine()-OldFN->GetFirstVisibleLine ();
	if (VDiff != 0) OldFN->LineScroll (VDiff,0);
}

void CMassRenameDialog::OnEnVscrollNewfilenameseditRight()
{
	// Scroll the "old filename" windows to the correct position
	int VDiff = NFNRight->GetFirstVisibleLine()-OldFN->GetFirstVisibleLine ();
	if (VDiff != 0) OldFN->LineScroll (VDiff,0);
}

//typedef vector<bool> fnvector;

void CMassRenameDialog::OnBnClickedMassrenameok()
{
	// First check for duplicate and invalid filenames - otherwise the dialog
	// won't close !
	
	// Create a sorted list, add all filenames with path uppercase. If there are
	// duplicate filenames, that's not correct. Also if there's an empty filename
	// or if there are not enough filenames in the window, this is not correct.

	bool RightJustify = !IsDlgButtonChecked (IDC_FILENAMELEFT);
	CRichEditCtrl* NFNEdit = NFNLeft;
	if (RightJustify) {
		NFNEdit = NFNRight;
	}

	if (NFNEdit->GetLineCount () < m_FileList.GetCount()) {
		AfxMessageBox ("Not enough filenames in the list of new filenames. Rename not possible.",
					   MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	m_NewFilenames.clear(); // Clear the list of target filenames and rebuild it

	// Create new strings and insert them into a sList
	std::vector<CString> sList;
	POSITION fpos = m_FileList.GetHeadPosition();
	int i = 0;
	while (fpos != NULL) {
		CString FName;
		NFNEdit->GetLine (i,FName.GetBuffer (MAX_PATH),MAX_PATH);
		FName.ReleaseBuffer ();
		FName.Trim ('\r');
		FName.Trim (' ');
		if ((FName=="") || (FName==".") || (FName=="..") || (FName.FindOneOf (":\\?*") >= 0)){
			CString er;
			er.Format ("Invalid filename in line %d. Rename not possible.",i+1);
			AfxMessageBox (er,MB_OK|MB_ICONEXCLAMATION);
			return;
		}
		CString path;
		CKnownFile* file = m_FileList.GetAt (fpos);
		PathCombine (path.GetBuffer (MAX_PATH),file->GetPath(),FName);

		// Add to the list of new filenames. This will be the result for the
		// caller of this dialog if all checks are ok.
		m_NewFilenames.push_back (FName);
		m_NewFilePaths.push_back (path);

		path.ReleaseBuffer ();
		path.MakeUpper ();
		sList.push_back (path);

		m_FileList.GetNext (fpos);
		i++;
	}

	// Sort the stringlist to check for duplicate items
	std::sort (sList.begin(),sList.end());

	// Check for duplicate filenames
	for (int i=1; i < (int) sList.size(); i++) {
		if (sList.at (i-1) == sList.at (i)) {
			CString er;
			er.Format ("Two or more equal filenames within the same directory are not allowed (Line %d). Rename not possible.",i+1);
			AfxMessageBox (er,MB_OK|MB_ICONEXCLAMATION);
			return;
		}
	}

	// Everything is ok, the caller can take m_NewFilenames to rename the files.
	EndDialog (IDOK);
}

void CMassRenameDialog::OnBnClickedMassrenamecancel()
{
	EndDialog (IDCANCEL);
}

void CMassRenameDialog::OnEnSetfocusFilenamemaskedit()
{
	m_DontTrackKeys = true;
	// Take the first line of the "New filenames" edit control as a mask for all filenames
	CString FirstLine;
	NFNLeft->GetWindowText (FirstLine);
	int i = FirstLine.Find ('\r');
	if (i != -1) {
		FirstLine = FirstLine.Left (i);
	}
	GetDlgItem (IDC_FILENAMEMASKEDIT)->SetWindowText (FirstLine);
	m_DontTrackKeys = false;
}

void CMassRenameDialog::OnBnClickedFilenameleft()
{
	// Show the left justify edit control
	NFNLeft->ModifyStyle (0,WS_VISIBLE);
	NFNRight->ModifyStyle (WS_VISIBLE,0);
	CString txt;
	NFNRight->GetWindowText (txt);
	NFNLeft->SetWindowText (txt);

	Invalidate();
}

void CMassRenameDialog::OnBnClickedFilenameright()
{
	// Show the right justify edit control
	NFNLeft->ModifyStyle (WS_VISIBLE,0);
	NFNRight->ModifyStyle (0,WS_VISIBLE);
	CString txt;
	NFNLeft->GetWindowText (txt);
	NFNRight->SetWindowText (txt);

	Invalidate();
}

void CMassRenameDialog::OnBnClickedReset()
{
	CString txt;
	OldFN->GetWindowText (txt);
	NFNLeft->SetWindowText (txt);
	NFNRight->SetWindowText (txt);
	OnEnSetfocusFilenamemaskedit();
}

void CMassRenameDialog::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (bShow) GetDlgItem(IDC_FILENAMEMASKEDIT)->SetFocus ();
}


void CMassRenameDialog::OnEnChangeFilenamemaskedit()
{
	if (m_DontTrackKeys) return;
	
	CString AfterEdit;
	int Start1,End1,Start2,End2;
	int StartR1,EndR1,StartR2,EndR2;
	
	MassRenameEdit->GetSel (Start2,End2);
	MassRenameEdit->GetWindowText (AfterEdit);
	Start1 = MassRenameEdit->Start1;
	End1 = MassRenameEdit->End1;
	bool RightJustify = !IsDlgButtonChecked (IDC_FILENAMELEFT);

	int BeforeEditLen = MassRenameEdit->m_BeforeEdit.GetLength ();

	StartR1 = BeforeEditLen-Start1;
	StartR2 = BeforeEditLen-Start2;
	EndR1 = BeforeEditLen-End1;
	EndR2 = BeforeEditLen-End2;

	CRichEditCtrl* NFNEdit = NFNLeft;
	if (RightJustify) {
		NFNEdit = NFNRight;
	}

	CString allFNText;
	NFNEdit->GetWindowText (allFNText);
	if ((MassRenameEdit->Start1==End1) && (Start2==End2) && (Start2 <= Start1)) {
		// DEL or ENTF pressed; remove 1 character on position Start2
		for (int i=0; i < NFNEdit->GetLineCount (); i++) {
			int lstart = NFNEdit->LineIndex (i);
			int llen = NFNEdit->LineLength (lstart);
			bool nochange=false;  // Don't edit if position is not in the current line
			if (!RightJustify) {
				NFNEdit->SetSel (lstart+Start2,lstart+Start2+1);
				if (lstart+Start2 >= lstart+llen) nochange = true;
			} else {
				NFNEdit->SetSel (lstart+llen-StartR2,lstart+llen-StartR2+1);
				if (llen-StartR2 < 0) nochange = true;
			}
			if (!nochange) NFNEdit->ReplaceSel ("");
		}
	} else {
		// Remove some characters Start1..End1 and replace them by Start1..End2
		CString NewChars = AfterEdit.Mid (Start1,End2-Start1);
		for (int i=0; i < NFNEdit->GetLineCount (); i++) {
			int lstart = NFNEdit->LineIndex (i);
			int llen = NFNEdit->LineLength (lstart);
			bool nochange=false;  // Don't edit if position is not in the current line
			if (!RightJustify) {
				NFNEdit->SetSel (lstart+Start1,lstart+End1);
				if (lstart+Start1 > lstart+llen) nochange = true;
			} else {
				NFNEdit->SetSel (lstart+llen-StartR1,lstart+llen-EndR1);
				if (llen-StartR1 < 0) nochange = true;
			}
			if (!nochange) NFNEdit->ReplaceSel (NewChars);
		}
	}
}
