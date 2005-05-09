// TitleMenu.h: interface for the CTitleMenu class.
// Based on the code of Per Fikse(1999/06/16) on codeguru.earthweb.com
// Author: Arthur Westerman
// Bug reports by : Brian Pearson 
//////////////////////////////////////////////////////////////////////
#pragma once

typedef struct tagMENUINFO
{
    DWORD   cbSize;
    DWORD   fMask;
    DWORD   dwStyle;
    UINT    cyMax;
    HBRUSH  hbrBack;
    DWORD   dwContextHelpID;
    ULONG_PTR dwMenuData;
}   MENUINFO, FAR *LPMENUINFO;
typedef MENUINFO CONST FAR *LPCMENUINFO;

typedef BOOL (WINAPI* TSetMenuInfo)(
  HMENU hmenu,       // handle to menu
  LPCMENUINFO lpcmi  // menu information
);
typedef BOOL (WINAPI* TGetMenuInfo)(
  HMENU hmenu,            // handle to menu
  LPCMENUINFO lpcmi       // menu information
);

class CTitleMenu : public CMenu
{
	typedef UINT (CALLBACK* LPFNDLLFUNC1)(HDC,CONST PTRIVERTEX,DWORD,CONST PVOID,DWORD,DWORD);
// Construction
public:
	CTitleMenu();

// Attributes
protected:
	CString m_strTitle;

	LPFNDLLFUNC1 dllfunc_GradientFill;
	HINSTANCE hinst_msimg32;
	long clRight;
	long clLeft;
	long clText;
	bool bDrawEdge;
	bool m_bIconMenu;
	UINT flag_edge;

private:
	static HMODULE		m_hUSER32_DLL;
	CImageList  m_ImageList;
	CMap<int, int, int, int> m_mapIconPos;
// Operations
public:
	void AddMenuTitle(LPCTSTR lpszTitle, bool bIsIconMenu = false);
	void EnableIcons();

	static  void	FreeAPI();
	static	void	Init();

protected:
	bool m_bCanDoGradientFill;

	BOOL GradientFill(	HDC hdc,
						CONST PTRIVERTEX pVertex,
						DWORD dwNumVertex,
						CONST PVOID pMesh,
						DWORD dwNumMesh,
						DWORD dwMode);

	void	DrawMonoIcon(int nIconPos, CPoint nDrawPos, CDC *dc);
	static  TSetMenuInfo SetMenuInfo;
	static  TGetMenuInfo GetMenuInfo;
	static	bool	LoadAPI();
	// Implementation
public:
	void SetColor(long cl) {clLeft=cl;};
	void SetGradientColor(long cl) {clRight=cl;};
	void SetTextColor(long cl) {clText=cl;};
	// See ::DrawEdge for flag values
	void SetEdge(bool shown,UINT remove=0,UINT add=0) {bDrawEdge=shown; (flag_edge^=remove)|=add;};

	long GetColor() {return clLeft;};
	long GetGradientColor() {return clRight;};
	long GetTextColor() {return clText;};
	long GetEdge() {return flag_edge;};

	virtual ~CTitleMenu();
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);

	BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = NULL, LPCTSTR lpszIconName = NULL);

};
