// InputBox.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "InputBox.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// InputBox dialog

IMPLEMENT_DYNAMIC(InputBox, CDialog)
InputBox::InputBox(CWnd* pParent /*=NULL*/)
	: CDialog(InputBox::IDD, pParent)
{
	m_label="";
	m_title="";
	m_default="";
	m_return="";
	m_cancel=true;
	m_bFilenameMode=false;
	// khaos::categorymod+
	isNumber=false;
	// khaos::categorymod-
}

InputBox::~InputBox()
{
}

void InputBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(InputBox, CDialog)
	ON_BN_CLICKED(IDC_CLEANFILENAME, OnCleanFilename)
END_MESSAGE_MAP()

void InputBox::OnOK()
{	char buffer[510];
	m_cancel=false;
	CWnd* textBox;
	if (!isNumber) textBox = GetDlgItem(IDC_TEXT);
	else textBox = GetDlgItem(IDC_TEXTNUM);

	if(textBox->GetWindowTextLength())
	{ 
		textBox->GetWindowText(buffer,510);
		m_return.Format("%s",buffer);
	}
	CDialog::OnOK();
}

// khaos::categorymod+
void InputBox::OnCancel()
{
	if (isNumber) m_return = "-1";
	else m_return = "0";
	
	CDialog::OnCancel();
}
// khaos::categorymod-

void InputBox::SetLabels(CString title,CString label,CString defaultStr){
	m_label=label;
	m_title=title;
	m_default=defaultStr;
}

CString InputBox::GetInput(){
	return m_return;
}

// khaos::categorymod+
int InputBox::GetInputInt(){
	return atoi(m_return);
}
// khaos::categorymod-

BOOL InputBox::OnInitDialog(){
	CDialog::OnInitDialog();
	InitWindowStyles(this);

	GetDlgItem(IDC_IBLABEL)->SetWindowText( m_label);
	// khaos::categorymod+
	if (!isNumber)
		GetDlgItem(IDC_TEXT)->SetWindowText(m_default);
	else {
		GetDlgItem(IDC_TEXTNUM)->SetWindowText(m_default);
		GetDlgItem(IDC_TEXT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_TEXTNUM)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_TEXTNUM)->SetFocus();
	}
	// khaos::categorymod-
	SetWindowText(m_title);

	GetDlgItem(IDCANCEL)->SetWindowText(GetResString(IDS_CANCEL));
	GetDlgItem(IDC_CLEANFILENAME)->ShowWindow( m_bFilenameMode?SW_NORMAL:SW_HIDE);

	return TRUE;
}
void InputBox::OnCleanFilename() {
	CString filename;
	GetDlgItem(IDC_TEXT)->GetWindowText(filename);
	GetDlgItem(IDC_TEXT)->SetWindowText( CleanupFilename(filename) );
}
