// XMessageBox.cpp
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// Some parts of this software are from information in the
// Microsoft SDK.
//
// This software is released into the public domain.
// You are free to use it in any way you like.
//
// This software is provided "as is" with no expressed
// or implied warranty.  I accept no liability for any
// damage or loss of business that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMessageBox.h"

// if you change the value for MAXITEMS, make sure that the code
// in CDialogTemp remains consistent with your changes.

#define MAXITEMS 20		// max no. of items in the dialog

///////////////////////////////////////////////////////////////////////////////
//
// Class definitions
//

class CDialogItem
{
public:
	DLGITEMTEMPLATE	m_dlgItemTemplate;
	enum			controltype {ICON = 0x7F, BUTTON, EDITCONTROL, STATICTEXT, CHECKBOX};
	controltype		m_controltype;
	_TCHAR			m_szCaption[1024];

public:
	CDialogItem(enum controltype cType);	// default constructor will fill in default values
	CDialogItem() {};						// default constructor, not to be called directly

	void AddItem(enum controltype cType,
				 UINT nID, 
				 CRect* prect = NULL, 
				 LPCTSTR pszCaption = NULL);
};

class CDialogTemp
{
public:
	CDialogTemp(LPCTSTR lpszMessage, UINT nStyle);
	virtual ~CDialogTemp();
	int Display(LPCTSTR lpszCaption);
	void AddItem(CDialogItem::controltype cType,
				 UINT nID, 
				 CRect* prect = NULL, 
				 LPCTSTR pszCaption = NULL);

protected:
	CDialogItem* m_pDlgItemArray[MAXITEMS];
};


///////////////////////////////////////////////////////////////////////////////
// internal definitions for XMessageBox()

#define MSGBOXEX_SPACING                8
#define MSGBOXEX_BUTTONWIDTH            82
#define MSGBOXEX_BUTTONHEIGHT           23
#define MSGBOXEX_BUTTONSPACING          6
#define MSGBOXEX_BOTTOMMARGIN           10
#define MSGBOXEX_MINHEIGHT              70
#define MSGBOXEX_DONOTASKAGAIN_HEIGHT   16

#define IDEXHELP                        300
#define IDDONOTASKAGAIN                 5555

#define MAXBUTTONSTRING 100

static _TCHAR szOK[MAXBUTTONSTRING];
static _TCHAR szCancel[MAXBUTTONSTRING];
static _TCHAR szIgnore[MAXBUTTONSTRING];
static _TCHAR szRetry[MAXBUTTONSTRING];
static _TCHAR szAbort[MAXBUTTONSTRING];
static _TCHAR szHelp[MAXBUTTONSTRING];
static _TCHAR szYes[MAXBUTTONSTRING];
static _TCHAR szNo[MAXBUTTONSTRING];
static _TCHAR szContinue[MAXBUTTONSTRING];
static _TCHAR szDoNotAskAgain[MAXBUTTONSTRING];
static _TCHAR szDoNotTellAgain[MAXBUTTONSTRING];
static _TCHAR szYesToAll[MAXBUTTONSTRING];
static _TCHAR szNoToAll[MAXBUTTONSTRING];

static void LoadButtonStrings();
static void LoadButtonStringsFromResources();

///////////////////////////////////////////////////////////////////////////////
// global data
static HICON        ghIcon;             // Handle of icon
static int          gnDefButton;        // Default button
static HWND         ghWnd;              // handle of owner window
static HINSTANCE    ghInst;             // handle to app instance
static DLGTEMPLATE  gdlgTempl;          // message box dialog template
static HANDLE       ghFont;             // handle to font for the message box
static int          gnMaxID;            // max control id
static int          gnDoNotAskID;       // control id for Do Not Ask checkbox
static BOOL         gbDoNotAskAgain;    // TRUE = include Do Not Ask checkbox
static BOOL         gbDoNotTellAgain;   // TRUE = include Do Not Tell checkbox
static BOOL         gbCancelButton;     // TRUE = include Cancel button
static BOOL         gbOkButton;         // TRUE = MB_OK used
static int          gnButton;           // current button no.
static UINT         gnDefId;            // button number of default button
static UINT         gnHelpId;           // help context id



/*
///////////////////////////////////////////////////////////////////////////////

XMessageBox

The XMessageBox function creates, displays, and operates a message box.
The message box contains an application-defined message and title, plus
any combination of predefined icons, push buttons, and checkboxes.

int XMessageBox(HWND hwnd,            // handle of owner window
                LPCTSTR lpszMessage,  // address of text in message box
                LPCTSTR lpszCaption,  // address of title of message box
                UINT nStyle,          // style of message box
                UINT nHelpId)         // help context id

PARAMETERS

    hwnd -        Identifies the owner window of the message box to be created.
                  If this parameter is NULL, the message box has no owner window.

    lpszMessage - Pointer to a null-terminated string containing the
                  message to be displayed.

    lpszCaption - Pointer to a null-terminated string used for the dialog
                  box title. If this parameter is NULL, the default title Error
                  is used.

    nStyle -      Specifies a set of bit flags that determine the contents and
                  behavior of the dialog box. This parameter can be a combination
                  of flags from the following groups of flags.

    nHelpId -     Specifies help context ID.  If 0, when the user chooses the help key,
                  the help file is opened at the index entry.  If non-zero, the help
                  file will be opened at that entry.  Currently XMessageBox() uses


    Flags for nStyle
    ----------------
    Specify one of the following flags to indicate the buttons contained
    in the message box:

    Flag                 Meaning
    ----                 -------
    MB_ABORTRETRYIGNORE  The message box contains three push buttons: Abort, Retry, and Ignore.

    MB_OK                The message box contains one push button: OK. This is the default.

    MB_OKCANCEL          The message box contains two push buttons: OK and Cancel.

    MB_RETRYCANCEL       The message box contains two push buttons: Retry and Cancel.

    MB_YESNO             The message box contains two push buttons: Yes and No.

    MB_YESNOCANCEL       The message box contains three push buttons: Yes, No, and Cancel.

    MB_CONTINUEABORT     The message box contains two push buttons: Continue and Abort


    Specify one of the following flags to display an icon in the message box:

    MB_ICONEXCLAMATION,
    MB_ICONWARNING       An exclamation-point icon appears in the message box.

    MB_ICONINFORMATION,
    MB_ICONASTERISK      An icon consisting of a lowercase letter i in a
                         circle appears in the message box.

    MB_ICONQUESTION      A question-mark icon appears in the message box.

    MB_ICONSTOP,
    MB_ICONERROR,
    MB_ICONHAND          A stop-sign icon appears in the message box.


    Specify one of the following flags to indicate the default button:

    MB_DEFBUTTON1        The first button is the default button. MB_DEFBUTTON1 is
                         the default unless MB_DEFBUTTON2, MB_DEFBUTTON3, or
                         MB_DEFBUTTON4 is specified.

    MB_DEFBUTTON2        The second button is the default button.

    MB_DEFBUTTON3        The third button is the default button.

    MB_DEFBUTTON4        The fourth button is the default button.

    MB_DEFBUTTON5        The fifth button is the default button.

    MB_DEFBUTTON6        The sixth button is the default button.


    In addition, you can specify the following flags:

    MB_SYSTEMMODAL       Use system-modal message boxes to notify the user of
                         serious, potentially damaging errors that require
                         immediate attention (for example, running out of memory).
                         This flag has no effect on the user's ability to interact
                         with windows other than those associated with hwnd.

                         NOTE:  use of MB_SYSTEMMODAL along with MB_ICONHAND
                         will cause the standard Windows MessageBox() function
                         to be called.

    MB_HELP              Adds a Help button to the message box. Choosing the Help
                         button or pressing F1 generates a Help event, according
                         to the value of nHelpId.

    MB_DONOTASKAGAIN     Adds a checkbox with the caption "Don't ask me again"
                         If the user clicks on the checkbox, the result code
                         returned by XMessageBox will be OR'd with the bit flag
                         MB_DONOTASKAGAIN.

    MB_DONOTTELLAGAIN    Adds a checkbox with the caption "Don't tell me again"
                         If the user clicks on the checkbox, the result code
                         returned by XMessageBox will be OR'd with the bit flag
                         MB_DONOTTELLAGAIN.

    MB_YESTOALL          Adds a button with the caption "Yes to All".  If the user
                         clicks the button, the result code will be IDYESTOALL.

                         Note:  This bit flag must be used in conjunction with
                         either MB_YESNO or MB_YESNOCANCEL.

    MB_NOTOALL           Adds a button with the caption "No to All".  If the user
                         clicks the button, the result code will be IDNOTOALL.

                         Note:  This bit flag must be used in conjunction with
                         either MB_YESNO or MB_YESNOCANCEL.

    MB_NORESOURCE        Do not try to load button strings from resources.  (See
                         XMessageBox.h for string resource id numbers.)  If this
                         bit flag is used, English strings will be used for buttons
                         and checkboxes.  If this bit flag is not used, XMessageBox() 
                         will attempt to load the strings for buttons and checkboxes 
                         from string resources first, and then use English strings 
                         if that fails.

    MB_NOSOUND           Do not play sounds when message box is displayed.


RETURN VALUES

    If the function succeeds, the return value is one of the following values,
    depending on which buttons are present:

         Value           Meaning
         -----           -------
         IDABORT         Abort button was selected.
         IDCANCEL        Cancel button was selected.
         IDIGNORE        Ignore button was selected.
         IDNO            No button was selected.
         IDOK            OK button was selected.
         IDRETRY         Retry button was selected.
         IDYES           Yes button was selected.
         IDCONTINUE      Continue button was selected

    If a message box has a Cancel button, the function returns the IDCANCEL value if
    the Esc key is pressed or the Cancel button is selected or the close button on the
    caption bar is pressed. If the message box has no Cancel button, pressing Esc has
    no effect, and the close button on the caption bar is disabled.


NOT IMPLEMENTED

    The following MessageBox() flags have not yet been implemented:

         MB_APPLMODAL
         MB_DEFAULT_DESKTOP_ONLY
         MB_RIGHT
         MB_RTLREADING
         MB_SETFOREGROUND
         MB_SERVICE_NOTIFICATION
         MB_SERVICE_NOTIFICATION_NT3X
         MB_TASKMODAL
         MB_TOPMOST

    If you decide to implement any of these please send me the changes 
    and I will post the new code to CodeProject.com.


///////////////////////////////////////////////////////////////////////////////
*/

int XMessageBox(HWND hwnd,
				LPCTSTR lpszMessage,
				LPCTSTR lpszCaption /* = NULL */,
				UINT nStyle /* = MB_OK|MB_ICONEXCLAMATION */,
				UINT nHelpId /* = 0 */)
{
	ASSERT(lpszMessage);

	gbDoNotAskAgain  = FALSE;
	gbDoNotTellAgain = FALSE;
	gbCancelButton   = FALSE;
	gbOkButton       = FALSE;
	gnButton         = 0;
	gnDefId          = 1;
	gnHelpId         = nHelpId;
	ghFont           = NULL;


	// get font for message box
/*
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	_tcscpy(lf.lfFaceName, _T("MS Sans Serif"));
	lf.lfHeight = -12;
	lf.lfWeight = FW_NORMAL;
	ghFont = ::CreateFontIndirect(&lf);
*/

	NONCLIENTMETRICS ncm;
	memset(&ncm, 0, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
	ghFont = ::CreateFontIndirect(&ncm.lfMessageFont);

	if (hwnd == NULL)
		hwnd = ::GetDesktopWindow();

	ghWnd = hwnd;

	ghInst = AfxGetInstanceHandle();
	ASSERT(ghInst);

	_TCHAR szTitle[1024];

	if (lpszCaption == NULL || lpszCaption[0] == 0)
	{
		if (::LoadString(ghInst, AFX_IDS_APP_TITLE, szTitle, sizeof(szTitle)-1) == 0)
			_tcscpy(szTitle, _T("Error"));
	}
	else
	{
		_tcscpy(szTitle, lpszCaption);
	}
	szTitle[sizeof(szTitle)-1] = _T('\0');

	if ((nStyle & MB_ICONHAND) && (nStyle & MB_SYSTEMMODAL))
	{
		// NOTE: When an application calls MessageBox and specifies the
		// MB_ICONHAND and MB_SYSTEMMODAL flags for the nStyle parameter,
		// the system displays the resulting message box regardless of
		// available memory.

		return ::MessageBox(hwnd, lpszMessage, szTitle, nStyle);
	}

	ghIcon = NULL;

	if (nStyle & MB_NORESOURCE)
		LoadButtonStrings();                // use English strings
	else
		LoadButtonStringsFromResources();   // try to load from resource strings

	CDialogTemp dlg(lpszMessage, nStyle);

	if ((nStyle & MB_NOSOUND) == 0)
		::MessageBeep(nStyle & MB_ICONMASK);

	int rc = dlg.Display(szTitle);

	if (ghFont)
		::DeleteObject(ghFont);
	ghFont = NULL;

	ghIcon = NULL;      // shared icon, do not call DestroyIcon

	return rc;
}

///////////////////////////////////////////////////////////////////////////////
// IconProc
LONG CALLBACK IconProc(HWND hwnd, UINT message, WPARAM, LPARAM)
{
	if (message == WM_PAINT)
	{
		PAINTSTRUCT ps;
		HDC hdc;

		hdc = BeginPaint(hwnd, &ps);
		DrawIcon(hdc, 0, 0, ghIcon);
		EndPaint(hwnd, &ps);
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// LoadButtonStrings
static void LoadButtonStrings()
{
	_tcscpy(szOK, _T("OK"));
	_tcscpy(szCancel, _T("Cancel"));
	_tcscpy(szIgnore, _T("&Ignore"));
	_tcscpy(szRetry, _T("&Retry"));
	_tcscpy(szAbort, _T("&Abort"));
	_tcscpy(szHelp, _T("&Help"));
	_tcscpy(szYes, _T("&Yes"));
	_tcscpy(szNo, _T("&No"));
	_tcscpy(szContinue, _T("&Continue"));
	_tcscpy(szDoNotAskAgain, _T("Don't ask me again"));
	_tcscpy(szDoNotTellAgain, _T("Don't tell me again"));
	_tcscpy(szYesToAll, _T("Yes to &All"));
	_tcscpy(szNoToAll, _T("No to A&ll"));
}

///////////////////////////////////////////////////////////////////////////////
// LoadButtonStringsFromResources
static void LoadButtonStringsFromResources()
{
	if (::LoadString(ghInst, IDS_XMBOK, szOK, MAXBUTTONSTRING) == 0)
		_tcscpy(szOK, _T("OK"));
	szOK[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBCANCEL, szCancel, MAXBUTTONSTRING) == 0)
		_tcscpy(szCancel, _T("Cancel"));
	szCancel[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBIGNORE, szIgnore, MAXBUTTONSTRING) == 0)
		_tcscpy(szIgnore, _T("&Ignore"));
	szIgnore[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBRETRY, szRetry, MAXBUTTONSTRING) == 0)
		_tcscpy(szRetry, _T("&Retry"));
	szRetry[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBABORT, szAbort, MAXBUTTONSTRING) == 0)
		_tcscpy(szAbort, _T("&Abort"));
	szAbort[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBHELP, szHelp, MAXBUTTONSTRING) == 0)
		_tcscpy(szHelp, _T("&Help"));
	szHelp[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBYES, szYes, MAXBUTTONSTRING) == 0)
		_tcscpy(szYes, _T("&Yes"));
	szYes[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBNO, szNo, MAXBUTTONSTRING) == 0)
		_tcscpy(szNo, _T("&No"));
	szNo[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBCONTINUE, szContinue, MAXBUTTONSTRING) == 0)
		_tcscpy(szContinue, _T("&Continue"));
	szContinue[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBDONOTASKAGAIN, szDoNotAskAgain, MAXBUTTONSTRING) == 0)
		_tcscpy(szDoNotAskAgain, _T("Don't ask me again"));
	szDoNotAskAgain[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBDONOTTELLAGAIN, szDoNotTellAgain, MAXBUTTONSTRING) == 0)
		_tcscpy(szDoNotTellAgain, _T("Don't tell me again"));
	szDoNotTellAgain[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBYESTOALL, szYesToAll, MAXBUTTONSTRING) == 0)
		_tcscpy(szYesToAll, _T("Yes to &All"));
	szYesToAll[MAXBUTTONSTRING-1] = _T('\0');

	if (::LoadString(ghInst, IDS_XMBNOTOALL, szNoToAll, MAXBUTTONSTRING) == 0)
		_tcscpy(szNoToAll, _T("No to A&ll"));
	szNoToAll[MAXBUTTONSTRING-1] = _T('\0');
}

///////////////////////////////////////////////////////////////////////////////
// MsgBoxDlgProc
BOOL CALLBACK MsgBoxDlgProc(HWND hwnd,
							UINT message,
							WPARAM wParam,
							LPARAM)
{
	HWND hwndChild;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			HDC hdc = ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
			ASSERT(hdc);

			::SelectObject(hdc, ghFont);
			::DeleteDC(hdc);
			int nID;

			for (nID = 1001; nID <= gnMaxID; nID++)
			{
				hwndChild = ::GetDlgItem(hwnd, nID);
				if (::IsWindow(hwndChild))
					::SendMessage(hwndChild, WM_SETFONT, (WPARAM)ghFont, 0);
				else
					break;

			}
			for (nID = 1; nID < 18; nID++)
			{
				hwndChild = ::GetDlgItem(hwnd, nID);
				if (hwndChild && ::IsWindow(hwndChild))
				{
					::SendMessage(hwndChild, WM_SETFONT, (WPARAM)ghFont, 0);
				}
			}

			hwndChild = ::GetDlgItem(hwnd, IDEXHELP);
			if (hwndChild && ::IsWindow(hwndChild))
				::SendMessage(hwndChild, WM_SETFONT, (WPARAM)ghFont, 0);

			hwndChild = ::GetDlgItem(hwnd, IDDONOTASKAGAIN);
			if (hwndChild && ::IsWindow(hwndChild))
			{
				::SendMessage(hwndChild, WM_SETFONT, (WPARAM)ghFont, 0);
				CheckDlgButton(hwnd, IDDONOTASKAGAIN, 0);
			}

			hwndChild = ::GetDlgItem(hwnd, gnDefId);
			if (hwndChild && ::IsWindow(hwndChild))
			{
				::SetFocus(hwndChild);
			}

			// disable close button just like real MessageBox
			if (!gbCancelButton && !gbOkButton)
				EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_GRAYED);

			if (ghIcon)
			{
				HWND hwndIcon;

				hwndIcon = ::GetDlgItem(hwnd, 1000);
				::SetWindowLong(hwndIcon, GWL_WNDPROC, (LONG)IconProc);
			}

			return FALSE;
		}

		case WM_COMMAND:
			if (wParam == IDCLOSE)
			{
			    return TRUE;
			}
			if (wParam == IDCANCEL)
			{
                if (gbCancelButton)
					::EndDialog(hwnd, IDCANCEL);
				else if (gbOkButton)
					::EndDialog(hwnd, IDOK);
				return TRUE;
			}
			if (wParam != IDEXHELP && wParam != IDDONOTASKAGAIN)
			{
				hwndChild = ::GetDlgItem(hwnd, IDDONOTASKAGAIN);
				BOOL bFlag = FALSE;
				if (hwndChild && ::IsWindow(hwndChild))
					bFlag = ::SendMessage(hwndChild, BM_GETCHECK, 0, 0);
				if (gbDoNotAskAgain)
					wParam |= bFlag ? MB_DONOTASKAGAIN : 0;
				else if (gbDoNotTellAgain)
					wParam |= bFlag ? MB_DONOTTELLAGAIN : 0;
				::EndDialog(hwnd, wParam);
			}
			else if (wParam == IDEXHELP)
			{
				_TCHAR szBuf[_MAX_PATH*2];
				szBuf[0] = 0;
				GetModuleFileName(AfxGetInstanceHandle(), szBuf, sizeof(szBuf) - 1);
				if (strlen(szBuf) > 0)
				{
					_TCHAR *cp = _tcsrchr(szBuf, _T('.'));
					if (cp)
					{
						_tcscpy(cp, _T(".hlp"));
						::WinHelp(hwnd,
								  szBuf,
								  (gnHelpId == 0) ? HELP_PARTIALKEY : HELP_CONTEXT,
								  gnHelpId);
					}
				}
			}
			break;

		default:
			return FALSE;
	}

	return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// CDialogTemp class

///////////////////////////////////////////////////////////////////////////////
// CDialogTemp ctor
CDialogTemp::CDialogTemp(LPCTSTR lpszMessage, UINT nStyle)
{
	UINT nID = 1001;

	HDC hdc = ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	ASSERT(hdc);

	HFONT hOldFont = (HFONT)::SelectObject(hdc, ghFont);

	int nMaxWidth = (::GetSystemMetrics(SM_CXSCREEN) / 2) + 100;
	if (nStyle & MB_ICONMASK)
		nMaxWidth -= GetSystemMetrics(SM_CXICON) + 2*MSGBOXEX_SPACING;

	CRect msgrect;
	msgrect.SetRect(0, 0, nMaxWidth, nMaxWidth);

	::DrawText(hdc, lpszMessage, -1, &msgrect, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
	msgrect.right += 12;
	msgrect.bottom += 5;

	msgrect.left    = 2 * MSGBOXEX_SPACING;
	msgrect.top     = 2 * MSGBOXEX_SPACING;
	msgrect.right  += 2 * MSGBOXEX_SPACING;
	msgrect.bottom += 2 * MSGBOXEX_SPACING;

	// client rect
	CRect mbrect;
	mbrect.SetRect(0, 0,
				   msgrect.Width() + (2 * MSGBOXEX_SPACING),
				   msgrect.Height() + (2 * MSGBOXEX_SPACING));

	if (mbrect.Height() < MSGBOXEX_MINHEIGHT)
		mbrect.bottom = MSGBOXEX_MINHEIGHT;

	// now initialize the DLGTEMPLATE structure
	gdlgTempl.x    = 0;
	gdlgTempl.y    = 0;

	gdlgTempl.cdit = 0;

	gdlgTempl.style = WS_CAPTION | WS_VISIBLE | WS_SYSMENU | 
					  WS_POPUP | DS_MODALFRAME | DS_CENTER;

	gdlgTempl.dwExtendedStyle = 0;

	if (nStyle & MB_SYSTEMMODAL)
		gdlgTempl.style |= DS_SYSMODAL;

	for (int j = 0; j < MAXITEMS; j++)
		m_pDlgItemArray[j] = NULL;

	int x, y;

	CRect iconrect;
	iconrect.SetRect(0, 0, 0, 0);
	CRect rect;

	if (nStyle & MB_ICONMASK)
	{
		int cxIcon;
		int cyIcon;
		LPSTR lpIcon = NULL;

		switch (nStyle & MB_ICONMASK)
		{
			case MB_ICONEXCLAMATION:
				lpIcon = (LPSTR)IDI_EXCLAMATION;
				break;

			case MB_ICONHAND:
				lpIcon = (LPSTR)IDI_HAND;
				break;

			case MB_ICONQUESTION:
				lpIcon = (LPSTR)IDI_QUESTION;
				break;

			case MB_ICONASTERISK:
				lpIcon = (LPSTR)IDI_ASTERISK;
				break;
		}

		if (lpIcon)
		{
			ghIcon = ::LoadIcon(NULL, lpIcon);

			cxIcon = GetSystemMetrics(SM_CXICON);
			cyIcon = GetSystemMetrics(SM_CYICON);

			int icon_x = MSGBOXEX_SPACING;
			int icon_y = MSGBOXEX_SPACING;

			msgrect.left += cxIcon + icon_x;
			msgrect.right += cxIcon + icon_x;

			mbrect.right = msgrect.right + MSGBOXEX_SPACING;

			iconrect.SetRect(icon_x, icon_y, icon_x + cxIcon + 2, icon_y + cyIcon + 2);
			AddItem(CDialogItem::STATICTEXT, 1000, &iconrect, _T(""));
		}
	}

	AddItem(CDialogItem::STATICTEXT, nID++, &msgrect, lpszMessage);

	int cItems = 0;

	switch (nStyle & MB_TYPEMASK)
	{
		case MB_OK               : cItems = 1; break;
		case MB_OKCANCEL         : cItems = 2; break;
		case MB_YESNO            : cItems = 2; break;
		case MB_YESNOCANCEL      : cItems = 3; break;
		case MB_ABORTRETRYIGNORE : cItems = 3; break;
		case MB_RETRYCANCEL      : cItems = 2; break;
		case MB_CONTINUEABORT    : cItems = 2; break;
	}

	if (nStyle & MB_HELP)
		cItems++;

	if (nStyle & MB_YESTOALL)
	{
		if ((nStyle & MB_YESNO) || (nStyle & MB_YESNOCANCEL))
		{
			cItems++;
		}
		else
		{
			ASSERT(FALSE);	// must have either MB_YESNO or MB_YESNOCANCEL
		}
	}

	if (nStyle & MB_NOTOALL)
	{
		if ((nStyle & MB_YESNO) || (nStyle & MB_YESNOCANCEL))
		{
			cItems++;
		}
		else
		{
			ASSERT(FALSE);	// must have either MB_YESNO or MB_YESNOCANCEL
		}
	}

	if (nStyle & MB_DONOTASKAGAIN)
		gbDoNotAskAgain = TRUE;
	else if (nStyle & MB_DONOTTELLAGAIN)
		gbDoNotTellAgain = TRUE;

	ASSERT(cItems > 0);

	CRect buttonrow;
	y = (msgrect.bottom > iconrect.bottom) ? msgrect.bottom : iconrect.bottom;
	y += MSGBOXEX_SPACING;
	int w = MSGBOXEX_BUTTONWIDTH * cItems + (MSGBOXEX_BUTTONSPACING * (cItems - 1));

	buttonrow.SetRect(0,
					  y,
					  w,
					  y + MSGBOXEX_BUTTONHEIGHT);

	switch (nStyle & MB_DEFMASK)
	{
		case MB_DEFBUTTON1 : gnDefButton = 1; break;
		case MB_DEFBUTTON2 : gnDefButton = 2; break;
		case MB_DEFBUTTON3 : gnDefButton = 3; break;
		case MB_DEFBUTTON4 : gnDefButton = 4; break;
		case MB_DEFBUTTON5 : gnDefButton = 5; break;
		case MB_DEFBUTTON6 : gnDefButton = 6; break;
		default:             gnDefButton = 1; break;
	}

	if (gnDefButton > cItems)
		gnDefButton = 1;

	x = (mbrect.Width() - buttonrow.Width()) / 2;

	mbrect.bottom = buttonrow.bottom + MSGBOXEX_BOTTOMMARGIN;

	int bw = buttonrow.Width();
	int bleft = 2 * MSGBOXEX_SPACING;
	int bright = bleft + bw;

	if (mbrect.right <= (bright + (2 * MSGBOXEX_SPACING)))
	{
		mbrect.right = bright + (2 * MSGBOXEX_SPACING);
	}

	x = (mbrect.Width() - bw) / 2;
	y = buttonrow.top;

	switch (nStyle & MB_TYPEMASK)
	{
		case MB_OK:
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDOK, &rect, szOK);
			gbOkButton = TRUE;
			break;

		case MB_OKCANCEL:
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDOK, &rect, szOK);

			x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDCANCEL, &rect, szCancel);
			gbCancelButton = TRUE;
			break;

		case MB_YESNO:
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDYES, &rect, szYes);

			if (nStyle & MB_YESTOALL)
			{
				x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
				rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
				AddItem(CDialogItem::BUTTON, IDYESTOALL, &rect, szYesToAll);
			}

			x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDNO, &rect, szNo);

			if (nStyle & MB_NOTOALL)
			{
				x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
				rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
				AddItem(CDialogItem::BUTTON, IDNOTOALL, &rect, szNoToAll);
			}
			break;

		case MB_YESNOCANCEL:
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDYES, &rect, szYes);

			if (nStyle & MB_YESTOALL)
			{
				x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
				rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
				AddItem(CDialogItem::BUTTON, IDYESTOALL, &rect, szYesToAll);
			}

			x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDNO, &rect, szNo);

			if (nStyle & MB_NOTOALL)
			{
				x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
				rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
				AddItem(CDialogItem::BUTTON, IDNOTOALL, &rect, szNoToAll);
			}

			x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDCANCEL, &rect, szCancel);
			gbCancelButton = TRUE;
			break;

		case MB_ABORTRETRYIGNORE:
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDABORT, &rect, szAbort);

			x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDRETRY, &rect, szRetry);

			x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDIGNORE, &rect, szIgnore);
			break;

		case MB_RETRYCANCEL:
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDRETRY, &rect, szRetry);

			x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDCANCEL, &rect, szCancel);
			gbCancelButton = TRUE;
			break;

		case MB_CONTINUEABORT:
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDCONT, &rect, szContinue);

			x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDABORT, &rect, szAbort);
			break;

		default:
			rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
			AddItem(CDialogItem::BUTTON, IDOK, &rect, szOK);
			break;

	}

	if (nStyle & MB_HELP)
	{
		x += MSGBOXEX_BUTTONWIDTH + MSGBOXEX_BUTTONSPACING;
		rect.SetRect(x, y, x + MSGBOXEX_BUTTONWIDTH, y + MSGBOXEX_BUTTONHEIGHT);
		AddItem(CDialogItem::BUTTON, IDEXHELP, &rect, szHelp);
	}

	nMaxWidth = ::GetSystemMetrics(SM_CXSCREEN) / 3;

	CRect checkboxrect;
	checkboxrect.SetRect(0, 0, nMaxWidth, MSGBOXEX_DONOTASKAGAIN_HEIGHT);

	if (nStyle & MB_DONOTASKAGAIN)
	{
		x = 2 * MSGBOXEX_BUTTONSPACING + 5;
		y += MSGBOXEX_BUTTONHEIGHT + (2 * MSGBOXEX_BUTTONSPACING);

		::DrawText(hdc, szDoNotAskAgain, -1, &checkboxrect, 
			DT_LEFT | DT_NOPREFIX | DT_CALCRECT | DT_SINGLELINE);

		int w = checkboxrect.right - checkboxrect.left;
		w += w/3;

		//rect.SetRect(x, y, x + MSGBOXEX_DONOTASKAGAIN_WIDTH, y + MSGBOXEX_DONOTASKAGAIN_HEIGHT);
		rect.SetRect(x, y, x + w, y + MSGBOXEX_DONOTASKAGAIN_HEIGHT);
		AddItem(CDialogItem::CHECKBOX, IDDONOTASKAGAIN, &rect, szDoNotAskAgain);
		buttonrow.bottom = y + MSGBOXEX_DONOTASKAGAIN_HEIGHT;
		mbrect.bottom = buttonrow.bottom + MSGBOXEX_SPACING;
		if (mbrect.Width() < (x + w))
			mbrect.right = mbrect.left + x + w;
	}
	else if (nStyle & MB_DONOTTELLAGAIN)
	{
		x = 2 * MSGBOXEX_BUTTONSPACING + 5;
		y += MSGBOXEX_BUTTONHEIGHT + (2 * MSGBOXEX_BUTTONSPACING);

		::DrawText(hdc, szDoNotTellAgain, -1, &checkboxrect, 
			DT_LEFT | DT_NOPREFIX | DT_CALCRECT | DT_SINGLELINE);

		int w = checkboxrect.right - checkboxrect.left;
		w += w/3;

		//rect.SetRect(x, y, x + MSGBOXEX_DONOTASKAGAIN_WIDTH, y + MSGBOXEX_DONOTASKAGAIN_HEIGHT);
		rect.SetRect(x, y, x + w, y + MSGBOXEX_DONOTASKAGAIN_HEIGHT);

		AddItem(CDialogItem::CHECKBOX, IDDONOTASKAGAIN, &rect, szDoNotTellAgain);
		buttonrow.bottom = y + MSGBOXEX_DONOTASKAGAIN_HEIGHT;
		mbrect.bottom = buttonrow.bottom + MSGBOXEX_SPACING;
		if (mbrect.Width() < (x + w))
			mbrect.right = mbrect.left + x + w;
	}

	if (buttonrow.bottom >= mbrect.bottom)
		mbrect.bottom = buttonrow.bottom + (2 * MSGBOXEX_SPACING);

	if (mbrect.right < (buttonrow.right + (2 * MSGBOXEX_SPACING)))
	{
		mbrect.right = buttonrow.right + (2 * MSGBOXEX_SPACING);
	}

	short hidbu = HIWORD(GetDialogBaseUnits());
	short lodbu = LOWORD(GetDialogBaseUnits());

	gdlgTempl.x = 0;
	gdlgTempl.y = 0;
	gdlgTempl.cx = (short)((mbrect.Width() * 4) / lodbu);
	gdlgTempl.cy = (short)((mbrect.Height() * 8) / hidbu);

	gnMaxID = nID - 1;

	::SelectObject(hdc, hOldFont);

	::DeleteDC(hdc);
}

///////////////////////////////////////////////////////////////////////////////
// CDialogTemp dtor
CDialogTemp::~CDialogTemp()
{
	for (int i = 0; i < MAXITEMS; i++)
	{
		if (m_pDlgItemArray[i])
		{
			delete m_pDlgItemArray[i];
			m_pDlgItemArray[i] = NULL;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// CDialogTemp::AddItem
void CDialogTemp::AddItem(CDialogItem::controltype cType, UINT nID, CRect* prect, LPCTSTR pszCaption)
{
	ASSERT(m_pDlgItemArray[gdlgTempl.cdit] == NULL);

	CDialogItem::controltype ct = cType;
	if (ct == CDialogItem::CHECKBOX)
	{
		ct = CDialogItem::BUTTON;
	}
	m_pDlgItemArray[gdlgTempl.cdit] = new CDialogItem(ct);
	ASSERT(m_pDlgItemArray[gdlgTempl.cdit]);

	m_pDlgItemArray[gdlgTempl.cdit]->AddItem(cType, nID, prect, pszCaption);

	gdlgTempl.cdit++;
	ASSERT(gdlgTempl.cdit < MAXITEMS);
}

///////////////////////////////////////////////////////////////////////////////
// CDialogTemp::Display
int CDialogTemp::Display(LPCTSTR lpszCaption)
{
	// The first step is to allocate memory to define the dialog.  The information to be
	// stored in the allocated buffer is the following:
	//
	// 1.  DLGTEMPLATE structure
	//			typedef struct
	//			{
	//					DWORD style;
	//					DWORD dwExtendedStyle;
	//					WORD  cdit;
	//					short x;
	//					short y;
	//					short cx;
	//					short cy;
	//			} DLGTEMPLATE;
	// 2.    0x0000 (Word) indicating the dialog has no menu
	// 3.    0x0000 (Word) Let windows assign default class to the dialog
	// 4.    (Caption)  Null terminated unicode string
	// 5.	 0x000B  (size of the font to be used)
	// 6.    "MS Sans Serif"  (name of the typeface to be used)
	// 7.  DLGITEMTEMPLATE structure for the button	 (HAS TO BE DWORD ALIGNED)
	//			typedef struct
	//			{
	//				DWORD style;
	//				DWORD dwExtendedStyle;
	//				short x;
	//				short y;
	//				short cx;
	//				short cy;
	//				WORD  id;
	//			} DLGITEMTEMPLATE;
	// 8.	 0x0080  to indicate the control is a button
	// 9.    (Title). Unicode null terminated string with the caption
	// 10.    0x0000   0 extra bytes of data for this control
	// 11.  DLGITEMTEMPLATE structure for the Static Text  (HAS TO BE DWORD ALIGNED)
	// 12.    0x0081 to indicate the control is static text
	// 13.   (Title). Unicode null terminated string with the text
	// 14     0x0000.  0 extra bytes of data for this control


	int rc = IDCANCEL;

	_TCHAR szTitle[1024];
	_tcsncpy(szTitle, lpszCaption, sizeof(szTitle)-1);
	szTitle[sizeof(szTitle)-1] = _T('\0');
	int nTitleLen = _tcslen(szTitle);

	int i;

	TRY  // catch memory exceptions and don't worry about allocation failures
	{
		int nBufferSize = sizeof(DLGTEMPLATE) +
							(2 * sizeof(WORD)) +				// menu and class
							((nTitleLen + 1) * sizeof(WCHAR));

		// NOTE - font is set in MsgBoxDlgProc

		nBufferSize = (nBufferSize + 3) & ~3;           // adjust size to make
														// first control DWORD aligned

		// loop to calculate size of buffer we need - 
		// add size of each control:
		//    sizeof(DLGITEMTEMPLATE) +
		//    sizeof(WORD) +				// atom value flag 0xFFFF
		//    sizeof(WORD) +				// ordinal value of control's class
		//    sizeof(WORD) +				// no. of bytes in creation data array
		//    sizeof title in WCHARs
		for (i = 0; i < gdlgTempl.cdit; i++)
		{
			int nItemLength = sizeof(DLGITEMTEMPLATE) + 3 * sizeof(WORD);
			int nChars = _tcslen(m_pDlgItemArray[i]->m_szCaption) + 1;
			nItemLength += nChars * sizeof(WCHAR);

			if (i != gdlgTempl.cdit - 1)                // the last control does not need extra bytes
				nItemLength = (nItemLength + 3) & ~3;   // take into account gap
														// so next control is DWORD aligned

			nBufferSize += nItemLength;
		}

		HLOCAL hLocal = LocalAlloc(LHND, nBufferSize);
		if (hLocal == NULL)
			AfxThrowMemoryException();

		BYTE* pBuffer = (BYTE*)LocalLock(hLocal);
		if (pBuffer == NULL)
		{
			LocalFree(hLocal);
			AfxThrowMemoryException();
		}

		BYTE* pdest = pBuffer;

		// transfer DLGTEMPLATE structure to the buffer
		MEMCOPY(pdest, &gdlgTempl, sizeof(DLGTEMPLATE));
		pdest += sizeof(DLGTEMPLATE);

		*(WORD*)pdest = 0;                              // no menu

		*(WORD*)(pdest + 1) = 0;                        // use default window class
		pdest += 2 * sizeof(WORD);

		// transfer title
		WCHAR* pchCaption;
		int nActualChars;

		pchCaption = new WCHAR[nTitleLen + 1];
		nActualChars = MultiByteToWideChar(CP_ACP, 0, szTitle, -1, pchCaption, nTitleLen + 1);
		ASSERT(nActualChars > 0);
		MEMCOPY(pdest, pchCaption, nActualChars * sizeof(WCHAR));
		pdest += nActualChars * sizeof(WCHAR);
		delete pchCaption;

		// will now transfer the information for each one of the item templates
		for (i = 0; i < gdlgTempl.cdit; i++)
		{
			pdest = (BYTE*)(((DWORD)pdest + 3) & ~3);  // make the pointer DWORD aligned
			MEMCOPY(pdest, (void *)&m_pDlgItemArray[i]->m_dlgItemTemplate, sizeof(DLGITEMTEMPLATE));
			pdest += sizeof(DLGITEMTEMPLATE);
			*(WORD*)pdest = 0xFFFF;                     // indicating atom value
			pdest += sizeof(WORD);
			*(WORD*)pdest = (WORD)m_pDlgItemArray[i]->m_controltype;     // atom value for the control
			pdest += sizeof(WORD);

			// transfer the caption even when it is an empty string
			WCHAR*  pchCaption;

			int nChars = _tcslen(m_pDlgItemArray[i]->m_szCaption) + 1;
			pchCaption = new WCHAR[nChars];
			int nActualChars = MultiByteToWideChar(CP_ACP, 0,
											   m_pDlgItemArray[i]->m_szCaption, -1, pchCaption, nChars);
			ASSERT(nActualChars > 0);
			MEMCOPY(pdest, pchCaption, nActualChars * sizeof(WCHAR));
			pdest += nActualChars * sizeof(WCHAR);
			delete pchCaption;

			*(WORD*)pdest = 0;                          // How many bytes in data for control
			pdest += sizeof(WORD);
		}
		ASSERT(pdest - pBuffer == nBufferSize);         // just make sure we did not overrun the heap

		rc = ::DialogBoxIndirect((HINSTANCE) ghInst, (LPDLGTEMPLATE)pBuffer,
								 ghWnd, MsgBoxDlgProc);

		LocalUnlock(hLocal);
		LocalFree(hLocal);
	}
	CATCH (CMemoryException, e)
	{
		::MessageBox(NULL, _T("Memory allocation for dialog template failed."),
						   _T("Allocation Failure"), 
						   MB_ICONEXCLAMATION | MB_OK);
		rc = IDCANCEL;
	}
	END_CATCH

	return rc;
}


///////////////////////////////////////////////////////////////////////////////
// CDialogItem class

///////////////////////////////////////////////////////////////////////////////
// CDialogItem ctor
CDialogItem::CDialogItem(enum CDialogItem::controltype ctrlType)
{
	m_controltype = ctrlType;
}

///////////////////////////////////////////////////////////////////////////////
// CDialogItem::AddItem
void CDialogItem::AddItem(enum controltype ctrltype, UINT nID, CRect* prect, LPCTSTR lpszCaption)
{
	short hidbu = HIWORD(GetDialogBaseUnits());
	short lodbu = LOWORD(GetDialogBaseUnits());

	// first fill in the type, location and size of the control
	m_controltype = ctrltype;
	if (m_controltype == CHECKBOX)
		m_controltype = BUTTON;

	if (prect != NULL)
	{
		m_dlgItemTemplate.x  = (short)((prect->left * 4) / lodbu);
		m_dlgItemTemplate.y  = (short)((prect->top * 8) / hidbu);
		m_dlgItemTemplate.cx = (short)((prect->Width() * 4) / lodbu);
		m_dlgItemTemplate.cy = (short)((prect->Height() * 8) / hidbu);
	}
	else
	{
		m_dlgItemTemplate.x  = 0;
		m_dlgItemTemplate.y  = 0;
		m_dlgItemTemplate.cx = 10;  // some useless default
		m_dlgItemTemplate.cy = 10;
	}

	m_dlgItemTemplate.dwExtendedStyle = 0;
	m_dlgItemTemplate.id = (WORD)nID;

	switch (ctrltype)
	{
		case ICON:
			m_dlgItemTemplate.style = WS_CHILD | SS_ICON | WS_VISIBLE;
			break;

		case BUTTON:
			gnButton++;
			m_dlgItemTemplate.style = WS_VISIBLE | WS_CHILD | WS_TABSTOP;
			if (gnButton == gnDefButton)
			{
				m_dlgItemTemplate.style |= BS_DEFPUSHBUTTON;
				gnDefId = nID;
			}
			else
			{
				m_dlgItemTemplate.style |= BS_PUSHBUTTON;
			}
			break;

		case CHECKBOX:
			m_dlgItemTemplate.style = WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX;
			break;

		case EDITCONTROL:
			m_dlgItemTemplate.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_LEFT;
			break;

		case STATICTEXT:
			m_dlgItemTemplate.style = WS_CHILD | WS_VISIBLE | SS_LEFT;
			break;

		default:
			ASSERT(FALSE);                // should never get here
	}

	_tcsncpy(m_szCaption,  (lpszCaption != NULL) ? lpszCaption : _T(""), sizeof(m_szCaption)-1);
	m_szCaption[sizeof(m_szCaption)-1] = _T('\0');
}
