// XMessageBox.h
//
// This software is released into the public domain.  
// You are free to use it in any way you like.
//
// This software is provided "as is" with no expressed 
// or implied warranty.  I accept no liability for any 
// damage or loss of business that this software may cause. 
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XMESSAGEBOX_H
#define XMESSAGEBOX_H

#define MB_DEFBUTTON5		0x00000400L
#define MB_DEFBUTTON6		0x00000500L

#define MB_CONTINUEABORT	0x00000008L     // adds two buttons, "Continue"  and "Abort"
#define MB_DONOTASKAGAIN	0x01000000L     // add checkbox "Do not ask me again"
#define MB_DONOTTELLAGAIN	0x02000000L     // add checkbox "Do not tell me again"
#define MB_YESTOALL			0x04000000L     // must be used with either MB_YESNO or MB_YESNOCANCEL
#define MB_NOTOALL			0x08000000L     // must be used with either MB_YESNO or MB_YESNOCANCEL
#define MB_NORESOURCE		0x10000000L		// do not try to load button strings from resources
#define MB_NOSOUND			0x80000000L     // do not play sound when mb is displayed

#define IDCONT				12
#define IDYESTOALL			13
#define IDNOTOALL			14

#define IDS_XMBOK				9001
#define IDS_XMBCANCEL			9002
#define IDS_XMBIGNORE			9003
#define IDS_XMBRETRY			9004
#define IDS_XMBABORT			9005
#define IDS_XMBHELP			9006
#define IDS_XMBYES				9007
#define IDS_XMBNO				9008
#define IDS_XMBCONTINUE		9009
#define IDS_XMBDONOTASKAGAIN	9010
#define IDS_XMBDONOTTELLAGAIN	9011
#define IDS_XMBYESTOALL		9012
#define IDS_XMBNOTOALL			9013



int XMessageBox(HWND hwnd, 
				LPCTSTR lpszMessage,
				LPCTSTR lpszCaption = NULL, 
				UINT uStyle = MB_OK|MB_ICONEXCLAMATION,
				UINT uHelpId = 0);

#endif //XMESSAGEBOX_H
