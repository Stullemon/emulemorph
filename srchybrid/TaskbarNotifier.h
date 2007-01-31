// CTaskbarNotifier Header file
// By John O'Byrne - 15 July 2002
// Modified by kei-kun
#pragma once

#define TN_TEXT_NORMAL			0x0000
#define TN_TEXT_BOLD			0x0001
#define TN_TEXT_ITALIC			0x0002
#define TN_TEXT_UNDERLINE		0x0004

//START - enkeyDEV(kei-kun) -TaskbarNotifier-
enum TbnMsg {
	TBN_NONOTIFY,
	TBN_NULL,
	TBN_CHAT,
	TBN_DOWNLOADFINISHED,
	TBN_LOG,
	TBN_IMPORTANTEVENT,
	TBN_NEWVERSION,
	TBN_DOWNLOADADDED,
	TBN_NEWMVERSION //MORPH - Added by SiRoB, New Version Check
};
//END - enkeyDEV(kei-kun) -TaskbarNotifier-


///////////////////////////////////////////////////////////////////////////////
// CTaskbarNotifierHistory

class CTaskbarNotifierHistory : public CObject
{
public:
	CTaskbarNotifierHistory() {};
	virtual ~CTaskbarNotifierHistory() {};

	CString m_strMessage;
	int m_nMessageType;
	CString m_strLink;
};


///////////////////////////////////////////////////////////////////////////////
// CTaskbarNotifier

class CTaskbarNotifier : public CWnd
{
	DECLARE_DYNAMIC(CTaskbarNotifier)
public:
	CTaskbarNotifier();
	virtual ~CTaskbarNotifier();

	int Create(CWnd *pWndParent);
	void Show(LPCTSTR szCaption, int nMsgType, LPCTSTR pszLink, BOOL bAutoClose = TRUE);
	void ShowLastHistoryMessage();
	int GetMessageType();
	void Hide();
	BOOL SetBitmap(UINT nBitmapID, int red=-1, int green=-1, int blue=-1);
	BOOL SetBitmap(LPCTSTR szFileName,int red=-1, int green=-1, int blue=-1);
	BOOL SetBitmap(CBitmap* Bitmap, int red, int green, int blue);
	void SetTextFont(LPCTSTR szFont,int nSize,int nNormalStyle,int nSelectedStyle);
	void SetTextDefaultFont();
	void SetTextColor(COLORREF crNormalTextColor,COLORREF crSelectedTextColor);
	void SetTextRect(RECT rcText);
	void SetCloseBtnRect(RECT rcCloseBtn);
	void SetHistoryBtnRect(RECT rcHistoryBtn);
	void SetTextFormat(UINT uTextFormat);
	BOOL LoadConfiguration(LPCTSTR szFileName);
	void SetAutoClose(BOOL autoClose);

protected:
	CString m_strConfigFilePath;
	CWnd *m_pWndParent;
	CFont m_myNormalFont;
	CFont m_mySelectedFont;
	COLORREF m_crNormalTextColor;
	COLORREF m_crSelectedTextColor;
	HCURSOR m_hCursor;
	CBitmap m_bitmapBackground;
	HRGN m_hBitmapRegion;
	int m_nBitmapWidth;
	int m_nBitmapHeight;
	CString m_strCaption;
	CString m_strLink;
	CRect  m_rcText;
	CRect  m_rcCloseBtn;
	CRect  m_rcHistoryBtn;
	CPoint m_ptMousePosition;
	UINT m_uTextFormat;
	BOOL m_bMouseIsOver;
	BOOL m_bTextSelected;
	BOOL m_bAutoClose;
	int m_nAnimStatus;
	int m_nTaskbarPlacement;
	DWORD m_dwTimerPrecision;
	//DWORD m_dwTimeToStay;
	UINT m_dwTimeToStay; //MORPH SetTime uses UINT. ( simulate permanent message becomes possible) 
	DWORD m_dwShowEvents;
	DWORD m_dwHideEvents;
	DWORD m_dwTimeToShow;
	DWORD m_dwTimeToHide;
	int m_nCurrentPosX;
	int m_nCurrentPosY;
	int m_nCurrentWidth;
	int m_nCurrentHeight;
	int m_nIncrementShow;
	int m_nIncrementHide;
	int m_nHistoryPosition;		  //<<--enkeyDEV(kei-kun) -TaskbarNotifier-
	int m_nActiveMessageType;	  //<<--enkeyDEV(kei-kun) -TaskbarNotifier-
	CObList m_MessageHistory;	  //<<--enkeyDEV(kei-kun) -TaskbarNotifier-

	HRGN CreateRgnFromBitmap(HBITMAP hBmp, COLORREF color);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseHover(WPARAM w, LPARAM l);
	afx_msg LRESULT OnMouseLeave(WPARAM w, LPARAM l);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSysColorChange();
};
