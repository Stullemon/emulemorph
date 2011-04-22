//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "CreditsThread.h"
#include "OtherFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// define mask color
#define MASK_RGB	(COLORREF)0xFFFFFF

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CCreditsThread, CGDIThread)

BEGIN_MESSAGE_MAP(CCreditsThread, CGDIThread)
	//{{AFX_MSG_MAP(CCreditsThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CCreditsThread::CCreditsThread(CWnd* pWnd, HDC hDC, CRect rectScreen)
    : CGDIThread(pWnd,hDC)
{
	m_rectScreen = rectScreen;
	m_rgnScreen.CreateRectRgnIndirect(m_rectScreen);
	m_nScrollPos = 0;
	m_pbmpOldBk = NULL;
	m_pbmpOldCredits = NULL;
	m_pbmpOldScreen = NULL;
	m_pbmpOldMask = NULL;
	m_nCreditsBmpWidth = 0;
	m_nCreditsBmpHeight = 0;
}

CCreditsThread::~CCreditsThread() 
{
}

BOOL CCreditsThread::InitInstance()
{
	InitThreadLocale();
	BOOL bResult = CGDIThread::InitInstance();

	// NOTE: Because this is a separate thread, we have to delete our GDI objects here (while
	// the handle maps are still available.)
	if(m_dcBk.m_hDC != NULL && m_pbmpOldBk != NULL)
	{
		m_dcBk.SelectObject(m_pbmpOldBk);
		m_pbmpOldBk = NULL;
		m_bmpBk.DeleteObject();
	}

	if(m_dcScreen.m_hDC != NULL && m_pbmpOldScreen != NULL)
	{
		m_dcScreen.SelectObject(m_pbmpOldScreen);
		m_pbmpOldScreen = NULL;
		m_bmpScreen.DeleteObject();
	}

	if(m_dcCredits.m_hDC != NULL && m_pbmpOldCredits != NULL)
	{
		m_dcCredits.SelectObject(m_pbmpOldCredits);
		m_pbmpOldCredits = NULL;
		m_bmpCredits.DeleteObject();
	}

	if(m_dcMask.m_hDC != NULL && m_pbmpOldMask != NULL)
	{
		m_dcMask.SelectObject(m_pbmpOldMask);
		m_pbmpOldMask = NULL;
		m_bmpMask.DeleteObject();
	}

	// clean up the fonts we created
	for(int n = 0; n < m_arFonts.GetSize(); n++)
	{
		m_arFonts.GetAt(n)->DeleteObject();
		delete m_arFonts.GetAt(n);
	}
	m_arFonts.RemoveAll();

	return bResult;
}

// wait for vertical retrace
// makes scrolling smoother, especially at fast speeds
// NT does not like this at all
void waitvrt(void)
{
	__asm {
			mov	dx,3dah
	VRT:
			in		al,dx
			test	al,8
			jnz		VRT
	NoVRT:
			in		al,dx
			test	al,8
			jz		NoVRT
	}
}

void CCreditsThread::SingleStep()
{
	// if this is our first time, initialize the credits
	if(m_dcCredits.m_hDC == NULL)
	{
		CreateCredits();
	}

	// track scroll position
	static int nScrollY = 0;

	// timer variables
	LARGE_INTEGER nFrequency;
	LARGE_INTEGER nStart;
	LARGE_INTEGER nEnd;
	int nTimeInMilliseconds;
	BOOL bTimerValid;

	nStart.QuadPart = 0;

	if(!QueryPerformanceFrequency(&nFrequency))
	{
		bTimerValid = FALSE;
	}
	else
	{
		bTimerValid = TRUE;

		// get start time
		QueryPerformanceCounter(&nStart);
	}

	CGDIThread::m_csGDILock.Lock();
	{
		PaintBk(&m_dcScreen);

		m_dcScreen.BitBlt(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, &m_dcCredits, 0, nScrollY, SRCINVERT);
		m_dcScreen.BitBlt(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, &m_dcMask, 0, nScrollY, SRCAND);
		m_dcScreen.BitBlt(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, &m_dcCredits, 0, nScrollY, SRCINVERT);

		// wait for vertical retrace
		if(m_bWaitVRT) waitvrt();

		m_dc.BitBlt(m_rectScreen.left, m_rectScreen.top, m_rectScreen.Width(), m_rectScreen.Height(), &m_dcScreen, 0, 0, SRCCOPY);

		GdiFlush();
	}
	CGDIThread::m_csGDILock.Unlock();

	// continue scrolling
	nScrollY += m_nScrollInc;
	if(nScrollY >= m_nCreditsBmpHeight) nScrollY = 0;	// scrolling up
	if(nScrollY < 0) nScrollY = m_nCreditsBmpHeight;	// scrolling down

	// delay scrolling by the specified time
	if(bTimerValid)
	{
		QueryPerformanceCounter(&nEnd);
		nTimeInMilliseconds = (int)((nEnd.QuadPart - nStart.QuadPart) * 1000 / nFrequency.QuadPart);

		if(nTimeInMilliseconds < m_nDelay)
		{
			Sleep(m_nDelay - nTimeInMilliseconds);
		}
	}
	else
	{
		Sleep(m_nDelay);
	}
}

void CCreditsThread::PaintBk(CDC* pDC)
{
	//save background the first time
	if (m_dcBk.m_hDC == NULL)
	{
		m_dcBk.CreateCompatibleDC(&m_dc);
		m_bmpBk.CreateCompatibleBitmap(&m_dc, m_rectScreen.Width(), m_rectScreen.Height());
		m_pbmpOldBk = m_dcBk.SelectObject(&m_bmpBk);
		m_dcBk.BitBlt(0, 0, m_rectScreen.Width(), m_rectScreen.Height(), &m_dc, m_rectScreen.left, m_rectScreen.top, SRCCOPY);
	}

	pDC->BitBlt(0, 0, m_rectScreen.Width(), m_rectScreen.Height(), &m_dcBk, 0, 0, SRCCOPY);
}

void CCreditsThread::CreateCredits()
{
	InitFonts();
	InitColors();
	InitText();

	m_dc.SelectClipRgn(&m_rgnScreen);

	m_dcScreen.CreateCompatibleDC(&m_dc);
	m_bmpScreen.CreateCompatibleBitmap(&m_dc, m_rectScreen.Width(), m_rectScreen.Height());
	m_pbmpOldScreen = m_dcScreen.SelectObject(&m_bmpScreen);

	m_nCreditsBmpWidth = m_rectScreen.Width();
	m_nCreditsBmpHeight = CalcCreditsHeight();

	m_dcCredits.CreateCompatibleDC(&m_dc);
	m_bmpCredits.CreateCompatibleBitmap(&m_dc, m_nCreditsBmpWidth, m_nCreditsBmpHeight);
	m_pbmpOldCredits = m_dcCredits.SelectObject(&m_bmpCredits);

	m_dcCredits.FillSolidRect(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, MASK_RGB);

	CFont* pOldFont;

	pOldFont  = m_dcCredits.SelectObject(m_arFonts.GetAt(0));

	m_dcCredits.SetBkMode(TRANSPARENT);

	int y = 0;

	int nFont;
	int nColor;

	int nLastFont = -1;
	int nLastColor = -1;

	int nTextHeight = m_dcCredits.GetTextExtent(_T("Wy")).cy;

	for(int n = 0; n < m_arCredits.GetSize(); n++)
	{
		CString sType = m_arCredits.GetAt(n).Left(1);

		if(sType == 'B')
		{
			// it's a bitmap

			CBitmap bmp;
			if(! bmp.LoadBitmap(m_arCredits.GetAt(n).Mid(2)))
			{
				CString str; 
				str.Format(_T("Could not find bitmap resource \"%s\". Be sure to assign the bitmap a QUOTED resource name"), m_arCredits.GetAt(n).Mid(2)); 
				AfxMessageBox(str); 
				return; 
			}

			BITMAP bmInfo;
			bmp.GetBitmap(&bmInfo);

			CDC dc;
			dc.CreateCompatibleDC(&m_dcCredits);
			CBitmap* pOldBmp = dc.SelectObject(&bmp);

			// draw the bitmap
			m_dcCredits.BitBlt((m_rectScreen.Width() - bmInfo.bmWidth) / 2, y, bmInfo.bmWidth, bmInfo.bmHeight, &dc, 0, 0, SRCCOPY);

			dc.SelectObject(pOldBmp);
			bmp.DeleteObject();

			y += bmInfo.bmHeight;
		}
		else if(sType == 'S')
		{
			// it's a vertical space

			y += _ttoi(m_arCredits.GetAt(n).Mid(2));
		}
		else
		{
			// it's a text string

			nFont = _ttoi(m_arCredits.GetAt(n).Left(2));
			nColor = _ttoi(m_arCredits.GetAt(n).Mid(3,2));

			if(nFont != nLastFont)
			{
				m_dcCredits.SelectObject(m_arFonts.GetAt(nFont));
				nTextHeight = m_arFontHeights.GetAt(nFont);
			}

			if(nColor != nLastColor)
			{
				m_dcCredits.SetTextColor(m_arColors.GetAt(nColor));
			}

			CRect rect(0, y, m_rectScreen.Width(), y + nTextHeight);

			m_dcCredits.DrawText(m_arCredits.GetAt(n).Mid(6), &rect, DT_CENTER);

			y += nTextHeight;
		}
	}

	m_dcCredits.SetBkColor(MASK_RGB);
	m_dcCredits.SelectObject(pOldFont);

	// create the mask bitmap
	m_dcMask.CreateCompatibleDC(&m_dcScreen);
	m_bmpMask.CreateBitmap(m_nCreditsBmpWidth, m_nCreditsBmpHeight, 1, 1, NULL);

	// select the mask bitmap into the appropriate dc
	m_pbmpOldMask = m_dcMask.SelectObject(&m_bmpMask);

	// build mask based on transparent color
	m_dcMask.BitBlt(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, &m_dcCredits, 0, 0, SRCCOPY);
}

void CCreditsThread::InitFonts()
{
	// create each font we'll need and add it to the fonts array

	CDC dcMem;
	dcMem.CreateCompatibleDC(&m_dc);
	CFont* pOldFont;
	int nTextHeight;

	LOGFONT lf;

	// font 0
	// SMALL ARIAL
	CFont* font0 = new CFont;
	memset((void*)&lf, 0, sizeof(lf));
	lf.lfHeight = 12;
	lf.lfWeight = 500;
	lf.lfQuality = NONANTIALIASED_QUALITY;
	_tcscpy(lf.lfFaceName, _T("Arial"));
	font0->CreateFontIndirect(&lf);
	m_arFonts.Add(font0);

	pOldFont = dcMem.SelectObject(font0);
	nTextHeight = dcMem.GetTextExtent(_T("Wy")).cy;
	m_arFontHeights.Add(nTextHeight);

	// font 1
	// MEDIUM BOLD ARIAL
	CFont* font1 = new CFont;
	memset((void*)&lf, 0, sizeof(lf));
	lf.lfHeight = 14;
	lf.lfWeight = 600;
	lf.lfQuality = NONANTIALIASED_QUALITY;
	_tcscpy(lf.lfFaceName, _T("Arial"));
	font1->CreateFontIndirect(&lf);
	m_arFonts.Add(font1);

	dcMem.SelectObject(font1);
	nTextHeight = dcMem.GetTextExtent(_T("Wy")).cy;
	m_arFontHeights.Add(nTextHeight);

	// font 2
	// LARGE ITALIC HEAVY BOLD TIMES ROMAN
	CFont* font2 = new CFont;
	memset((void*)&lf, 0, sizeof(lf));
	lf.lfHeight = 16;
	lf.lfWeight = 700;
	//lf.lfItalic = TRUE;
	/* MORPH win95 is not supported, vs2008 does not support .win95
	lf.lfQuality = afxIsWin95() ? NONANTIALIASED_QUALITY : ANTIALIASED_QUALITY;
	*/
	lf.lfQuality = ANTIALIASED_QUALITY;
	// MORPH END
	_tcscpy(lf.lfFaceName, _T("Arial"));
	font2->CreateFontIndirect(&lf);
	m_arFonts.Add(font2);

	dcMem.SelectObject(font2);
	nTextHeight = dcMem.GetTextExtent(_T("Wy")).cy;
	m_arFontHeights.Add(nTextHeight);

	// font 3
	CFont* font3 = new CFont;
	memset((void*)&lf, 0, sizeof(lf));
	lf.lfHeight = 25;
	lf.lfWeight = 900;
	/* MORPH
	lf.lfQuality = afxIsWin95() ? NONANTIALIASED_QUALITY : ANTIALIASED_QUALITY;
	 */
	lf.lfQuality = ANTIALIASED_QUALITY;
	// MORPH END
	_tcscpy(lf.lfFaceName, _T("Arial"));
	font3->CreateFontIndirect(&lf);
	m_arFonts.Add(font3);

	dcMem.SelectObject(font3);
	nTextHeight = dcMem.GetTextExtent(_T("Wy")).cy;
	m_arFontHeights.Add(nTextHeight);

	dcMem.SelectObject(pOldFont);
}

void CCreditsThread::InitColors()
{
	// define each color we'll be using

	m_arColors.Add(PALETTERGB(0, 0, 0));	// 0 = BLACK
	m_arColors.Add(PALETTERGB(90, 90, 90));	// 1 = very dark gray
	m_arColors.Add(PALETTERGB(128, 128, 128));		// 2 = DARK GRAY
	m_arColors.Add(PALETTERGB(192, 192, 192));	// 3 = LIGHT GRAY
	m_arColors.Add(PALETTERGB(200, 50, 50));	// 4 = very light gray
	m_arColors.Add(PALETTERGB(255, 255, 128));	// 5 white
	m_arColors.Add(PALETTERGB(0, 0, 128));	// 6 dark blue
	m_arColors.Add(PALETTERGB(128, 128, 255));	// 7 light blue
	m_arColors.Add(PALETTERGB(0, 106, 0));	// 8 dark green
}

void CCreditsThread::InitText()
{
	// 1st pair of digits identifies the font to use
	// 2nd pair of digits identifies the color to use
	// B = Bitmap
	// S = Space (moves down the specified number of pixels)

	CString sTmp;
	
	/*
		You may NOT modify this copyright message. You may add your name, if you
		changed or improved this code, but you mot not delete any part of this message,
		make it invisible etc.
	*/

	// start at the bottom of the screen
	sTmp.Format(_T("S:%d"), m_rectScreen.Height());
	m_arCredits.Add(sTmp);

	m_arCredits.Add(_T("03:00:eMule"));
	sTmp.Format(_T("02:01:Version %s"),theApp.m_strCurVersionLong);
	//MORPH START - Added by SiRoB, [itsonlyme: -modname-]
	sTmp += _T(" [") + theApp.m_strModLongVersion + _T("]");
	//MORPH END   - Added by SiRoB, [itsonlyme: -modname-]

	m_arCredits.Add(sTmp);
	m_arCredits.Add(_T("01:06:Copyright (C) 2002-2009 Merkur"));
	m_arCredits.Add(_T("S:50"));

    //Commander - Added: MorphTeam - Start
	m_arCredits.Add(_T("02:04:Morph Developers"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:AndCycle (semi-inactive)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Commander (inactive)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:IceCream (inactive)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:JLEE (inactive)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Mighty Knife (inactive)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Milobac (inactive)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Morpheus (founder)(retired)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:SiRoB (retired)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Wistily (inactive)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Yun.SF3 (inactive)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:leuk_he (inactive)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Stulle"));
	m_arCredits.Add(_T("S:5"));
			// as required by the upnp library licence:
	m_arCredits.Add(_T("01:06:morph contains some code that is "));
	m_arCredits.Add(_T("01:06:(c) 2000-2003 Intel Corporation "));
	m_arCredits.Add(_T("S:50"));
    //Commander - Added: MorphTeam - End

	m_arCredits.Add(_T("02:04:Official Developers")); //Commander - Modified: Make clear who belongs to the official Developers and who to the Mod ones
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Unknown1"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Ornis"));

	m_arCredits.Add(_T("S:50"));

	m_arCredits.Add(_T("02:04:Official Testers")); //Commander - Modified: Make clear who belongs to the official Developers and who to the Mod ones
		m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Monk"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Daan"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Elandal"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Frozen_North"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:kayfam"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Khandurian"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Masta2002"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:mrLabr"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Nesi-San"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:SeveredCross"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Skynetman"));


	m_arCredits.Add(_T("S:50"));
	m_arCredits.Add(_T("02:04:Official Retired Members")); //Commander - Modified: Make clear who belongs to the official Developers and who to the Mod ones
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Merkur (the Founder)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:tecxx"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Pach2"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Juanjo"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Barry"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Dirus"));
	

	m_arCredits.Add(_T("S:50"));
	m_arCredits.Add(_T("02:04:Thanks to these programmers"));
	m_arCredits.Add(_T("02:04:for publishing useful codeparts"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Paolo Messina (ResizableDialog class)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:6:PJ Naughter (HttpDownload Dialog)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Jim Connor (Scrolling Credits)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Yury Goltsman (extended Progressbar)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Magomed G. Abdurakhmanov (Hyperlink ctrl)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Arthur Westerman (Titled menu)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Tim Kosse (AsyncSocket-Proxysupport)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Keith Rule (Memory DC)"));
	m_arCredits.Add(_T("S:50"));

	m_arCredits.Add(_T("02:07:And thanks to the following"));
	m_arCredits.Add(_T("02:07:people for translating eMule"));
	m_arCredits.Add(_T("02:07:into different languages:"));
	m_arCredits.Add(_T("S:20"));
	
	
	m_arCredits.Add(_T("01:06:Arabic: Dody"));
	m_arCredits.Add(_T("S:05"));	
	m_arCredits.Add(_T("01:06:Albanian: Besmir"));
	m_arCredits.Add(_T("S:05"));	
	m_arCredits.Add(_T("01:06:Basque: TXiKi"));
	m_arCredits.Add(_T("S:05"));	
	m_arCredits.Add(_T("01:06:Breton: KAD-Korvigello� an Drouizig"));
	m_arCredits.Add(_T("S:05"));	
	m_arCredits.Add(_T("01:06:Bulgarian: DapKo, Dumper"));
	m_arCredits.Add(_T("S:05"));	
	m_arCredits.Add(_T("01:06:Catalan: LeChuck"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Chinese simplyfied: Tim Chen, Qilu T."));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Chinese Traditional: CML, Donlong, Ryan"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Czech: Patejl"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Danish: Tiede, Cirrus, Itchy"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Estonian: Symbio"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Dutch: Mr.Bean"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Finnish: Nikerabbit"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:French: Motte, Emzc, Lalrobin"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Galician: Juan, Emilio R."));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Greek: Michael Papadakis"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Italian: Trevi, FrankyFive"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Japanese: DukeDog, Shinro T."));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Hebrew: Avi-3k"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Hungarian: r0ll3r"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Korean: pooz"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Latvian: Zivs"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Lithuanian: Daan"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Maltese: Reuben"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Norwegian (Bokmal): Iznogood"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Norwegian (Nynorsk): Hallvor"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Polish: Tomasz \"TMouse\" Broniarek"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Portugese: Filipe, Lu�s Claro"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Portugese Brasilian: DarthMaul,Brasco"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Romanian: Dragos"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Russian: T-Mac, BRMAIL"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Slowenian: Rok Kralj"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Spanish Castellano: Azuredraco, Javier L., |_Hell_|"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Swedish: Andre"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Turkish: Burak Y."));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Ukrainian: Kex"));
	m_arCredits.Add(_T("S:05"));
	m_arCredits.Add(_T("01:06:Vietnamese: Paul Tran HQ Loc"));

	m_arCredits.Add(_T("S:50"));
	m_arCredits.Add(_T("02:04:Part of eMule is based on Kademlia:"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("02:04:Peer-to-peer routing based on the XOR metric."));
	m_arCredits.Add(_T("S:10"));
	m_arCredits.Add(_T("01:06:Copyright (C) 2002 Petar Maymounkov"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:http://kademlia.scs.cs.nyu.edu"));

	// pause before repeating
	m_arCredits.Add(_T("S:100"));
}

int CCreditsThread::CalcCreditsHeight()
{
	int nHeight = 0;

	for(int n = 0; n < m_arCredits.GetSize(); n++)
	{
		CString sType = m_arCredits.GetAt(n).Left(1);

		if(sType == 'B')
		{
			// it's a bitmap

			CBitmap bmp;
			if (! bmp.LoadBitmap(m_arCredits.GetAt(n).Mid(2)))
			{
				CString str; 
				str.Format(_T("Could not find bitmap resource \"%s\". Be sure to assign the bitmap a QUOTED resource name"), m_arCredits.GetAt(n).Mid(2)); 
				AfxMessageBox(str); 
				return -1; 
			}

			BITMAP bmInfo;
			bmp.GetBitmap(&bmInfo);

			nHeight += bmInfo.bmHeight;
		}
		else if(sType == 'S')
		{
			// it's a vertical space

			nHeight += _ttoi(m_arCredits.GetAt(n).Mid(2));
		}
		else
		{
			// it's a text string

			int nFont = _ttoi(m_arCredits.GetAt(n).Left(2));
			nHeight += m_arFontHeights.GetAt(nFont);
		}
	}

	return nHeight;
}
