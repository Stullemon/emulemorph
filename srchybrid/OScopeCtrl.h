#ifndef __OScopeCtrl_H__
#define __OScopeCtrl_H__

#pragma once

/////////////////////////////////////////////////////////////////////////////
// COScopeCtrl window

class COScopeCtrl : public CWnd
{
	// Construction
public:
	COScopeCtrl(int NTrends = 1);
	
	// Attributes
	// -khaos--+++> Added parameters: bool bUseTrendRatio = true (AppendPoints and AppendEmptyPoints)
	//				Added public function: SetTrendRatio
	void AppendPoints(double dNewPoint[], bool bInvalidate = true, bool bAdd2List = true, bool bUseTrendRatio = true);
	void AppendEmptyPoints(double dNewPoint[], bool bInvalidate = true, bool bAdd2List = true, bool bUseTrendRatio = true);
	void SetTrendRatio(int iTrend, unsigned int iRatio = 1);
	// <-----khaos-
	//MORPH START - Added by SiRoB, Legend Graph
	void SetLegendLabel(CString string,int iTrend);
	void SetBarsPlot(bool BarsPlot,int iTrend);
	//MORPH END  - Added by SiRoB, Legend Graph
	void SetRange(double dLower, double dUpper, int iTrend = 0);
	void SetRanges(double dLower, double dUpper);
	void SetXUnits(CString string, CString XMin = "", CString XMax = "");
	void SetYUnits(CString string, CString YMin = "", CString YMax = "");
	void SetGridColor(COLORREF color);
	void SetPlotColor(COLORREF color, int iTrend = 0);
	COLORREF GetPlotColor(int iTrend = 0);
	void SetBackgroundColor(COLORREF color);
	void InvalidateCtrl(bool deleteGraph = true);
	void DrawPoint();
	void Reset();
	bool ready;
	
	// Operations
	
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COScopeCtrl)
public:
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = NULL);
	//}}AFX_VIRTUAL
	
	// Implementation

	bool drawBars;
	bool autofitYscale;
	int m_nXGrids;
	int m_nYGrids;
	int m_nShiftPixels;         // amount to shift with each new point 
	int m_nTrendPoints;			// when you set this to > 0, then plot will
	int m_nMaxPointCnt;
	// contain that much points (regardless of the
	// Trend/control) width drawn on screen !!!
	
	// Otherwise, if this is -1 (which is default),
	// m_nShiftPixels will be in use
	int m_nYDecimals;
	
	typedef struct m_str_struct 
	{	
		CString XUnits, XMin, XMax;
		CString YUnits, YMin, YMax;
	} m_str_t;
	m_str_t m_str;
	
	COLORREF m_crBackColor;        // background color
	COLORREF m_crGridColor;        // grid color
	
	typedef struct PlotDataStruct 
	{
		COLORREF crPlotColor;       // data plot color  
		CPen   penPlot;
		double dCurrentPosition;    // current position
		double dPreviousPosition;   // previous position
		int nPrevY;
		double dLowerLimit;         // lower bounds
		double dUpperLimit;         // upper bounds
		double dRange;				// = UpperLimit - LowerLimit
		double dVerticalFactor;
		// -khaos--+++> Optional variable to set a ratio for a given "trend".
		//				The purpose here is to better implement the customizable
		//				active connections ratio, so that points are redrawn correctly
		//				when the ratio is changed, rather than having all previous points
		//				no longer match up with the new ratio.
		int		iTrendRatio;
		// <-----khaos-
		//MORPH START - Added by SiRoB
		CString LegendLabel;
		bool BarsPlot;
		//MORPH END - Added by SiRoB
		CList<double,double> lstPoints;
	} PlotData_t;
	
	virtual ~COScopeCtrl();
	
	// Generated message map functions
protected:
	int m_NTrends;
	
	//{{AFX_MSG(COScopeCtrl)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy); 
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
		
		struct CustShiftStruct 
	{		// when m_nTrendPoints > 0, this structure will contain needed vars
		int m_nRmndr;				// reminder after dividing m_nWidthToDo/m_nPointsToDo
		int m_nWidthToDo;
		int m_nPointsToDo;
	} CustShift;
	
	PlotData_t *m_PlotData; // !!! !!!
	
	int m_nClientHeight;
	int m_nClientWidth;
	int m_nPlotHeight;
	int m_nPlotWidth;
	
	CRect  m_rectClient;
	CRect  m_rectPlot;
	CBrush m_brushBack;
	
	CDC     m_dcGrid;
	CDC     m_dcPlot;
	CBitmap *m_pbitmapOldGrid;
	CBitmap *m_pbitmapOldPlot;
	CBitmap m_bitmapGrid;
	CBitmap m_bitmapPlot;

	bool m_bDoUpdate;
	UINT m_nRedrawTimer;
public:
	int ReCreateGraph(void);
	afx_msg void OnTimer(UINT nIDEvent);

	void GetPlotRect(CRect& rPlotRect)
	{
		rPlotRect = m_rectPlot;
	}
};

/////////////////////////////////////////////////////////////////////////////
#endif
