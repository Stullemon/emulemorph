// [!output WTL_FRAME_FILE].h : interface of the [!output WTL_FRAME_CLASS] class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class [!output WTL_FRAME_CLASS] : public [!output WTL_FRAME_BASE_CLASS]<[!output WTL_FRAME_CLASS]>, public CUpdateUI<[!output WTL_FRAME_CLASS]>,
		public CMessageFilter, public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

[!if WTL_APPTYPE_SDI || WTL_APPTYPE_MTSDI]
[!if WTL_USE_VIEW]
	[!output WTL_VIEW_CLASS] m_view;

[!endif]
[!endif]
[!if WTL_USE_CMDBAR]
[!if WTL_APPTYPE_MDI]
	CMDICommandBarCtrl m_CmdBar;

[!else]
	CCommandBarCtrl m_CmdBar;

[!endif]
[!endif]
[!if WTL_USE_CPP_FILES]
	virtual BOOL PreTranslateMessage(MSG* pMsg);
[!else]
	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
[!if WTL_APPTYPE_MDI]
		if([!output WTL_FRAME_BASE_CLASS]<[!output WTL_FRAME_CLASS]>::PreTranslateMessage(pMsg))
			return TRUE;

		HWND hWnd = MDIGetActive();
		if(hWnd != NULL)
			return (BOOL)::SendMessage(hWnd, WM_FORWARDMSG, 0, (LPARAM)pMsg);

		return FALSE;
[!else]
[!if WTL_USE_VIEW]
		if([!output WTL_FRAME_BASE_CLASS]<[!output WTL_FRAME_CLASS]>::PreTranslateMessage(pMsg))
			return TRUE;

		return m_view.PreTranslateMessage(pMsg);
[!else]
		return [!output WTL_FRAME_BASE_CLASS]<[!output WTL_FRAME_CLASS]>::PreTranslateMessage(pMsg);
[!endif]
[!endif]
	}

[!endif]
[!if WTL_USE_CPP_FILES]
	virtual BOOL OnIdle();
[!else]
	virtual BOOL OnIdle()
	{
[!if WTL_USE_TOOLBAR]
		UIUpdateToolBar();
[!endif]
		return FALSE;
	}
[!endif]

	BEGIN_UPDATE_UI_MAP([!output WTL_FRAME_CLASS])
[!if WTL_USE_TOOLBAR]
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
[!endif]
[!if WTL_USE_STATUSBAR]
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
[!endif]
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP([!output WTL_FRAME_CLASS])
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
[!if WTL_COM_SERVER]
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
[!endif]
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
[!if WTL_APPTYPE_MTSDI]
		COMMAND_ID_HANDLER(ID_FILE_NEW_WINDOW, OnFileNewWindow)
[!endif]
[!if WTL_USE_TOOLBAR]
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
[!endif]
[!if WTL_USE_STATUSBAR]
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
[!endif]
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
[!if WTL_APPTYPE_MDI]
		COMMAND_ID_HANDLER(ID_WINDOW_CASCADE, OnWindowCascade)
		COMMAND_ID_HANDLER(ID_WINDOW_TILE_HORZ, OnWindowTile)
		COMMAND_ID_HANDLER(ID_WINDOW_ARRANGE, OnWindowArrangeIcons)
[!endif]
		CHAIN_MSG_MAP(CUpdateUI<[!output WTL_FRAME_CLASS]>)
		CHAIN_MSG_MAP([!output WTL_FRAME_BASE_CLASS]<[!output WTL_FRAME_CLASS]>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

[!if WTL_USE_CPP_FILES]
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
[!if WTL_USE_TOOLBAR]
[!if WTL_USE_REBAR]
[!if WTL_USE_CMDBAR]
		// create command bar window
		HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
		// attach menu
		m_CmdBar.AttachMenu(GetMenu());
		// load command bar images
		m_CmdBar.LoadImages(IDR_MAINFRAME);
		// remove old menu
		SetMenu(NULL);

[!endif]
		HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

		CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
[!if WTL_USE_CMDBAR]
		AddSimpleReBarBand(hWndCmdBar);
		AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
[!else]
		AddSimpleReBarBand(hWndToolBar);
[!endif]
[!else]
		CreateSimpleToolBar();
[!endif]
[!endif]
[!if WTL_USE_STATUSBAR]

		CreateSimpleStatusBar();
[!endif]
[!if WTL_APPTYPE_MDI]

		CreateMDIClient();
[!if WTL_USE_CMDBAR]
		m_CmdBar.SetMDIClient(m_hWndMDIClient);
[!endif]
[!endif]
[!if WTL_APPTYPE_SDI || WTL_APPTYPE_MTSDI]
[!if WTL_USE_VIEW]
[!if WTL_VIEWTYPE_FORM]

		m_hWndClient = m_view.Create(m_hWnd);
[!else]
[!if WTL_VIEWTYPE_HTML]

		//TODO: Replace with a URL of your choice
		m_hWndClient = m_view.Create(m_hWnd, rcDefault, _T("http://www.microsoft.com"), [!output WTL_VIEW_STYLES], [!output WTL_VIEW_EX_STYLES]);
[!else]

		m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, [!output WTL_VIEW_STYLES], [!output WTL_VIEW_EX_STYLES]);
[!endif]
[!endif]
[!endif]
[!endif]
[!if WTL_USE_TOOLBAR]
[!if WTL_USE_REBAR]

		UIAddToolBar(hWndToolBar);
[!else]

		UIAddToolBar(m_hWndToolBar);
[!endif]
		UISetCheck(ID_VIEW_TOOLBAR, 1);
[!endif]
[!if WTL_USE_STATUSBAR]
		UISetCheck(ID_VIEW_STATUS_BAR, 1);
[!endif]

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		return 0;
	}

[!endif]
[!if WTL_COM_SERVER]
[!if WTL_USE_CPP_FILES]
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);
		// if UI is the last thread, no need to wait
		if(_Module.GetLockCount() == 1)
		{
			_Module.m_dwTimeOut = 0L;
			_Module.m_dwPause = 0L;
		}
		_Module.Unlock();
		return 0;
	}

[!endif]
[!endif]
[!if WTL_USE_CPP_FILES]
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}

[!endif]
[!if WTL_USE_CPP_FILES]
	LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
[!if WTL_APPTYPE_MDI]
		[!output WTL_CHILD_FRAME_CLASS]* pChild = new [!output WTL_CHILD_FRAME_CLASS];
		pChild->CreateEx(m_hWndClient);

[!endif]
		// TODO: add code to initialize document

		return 0;
	}

[!endif]
[!if WTL_APPTYPE_MTSDI]
[!if WTL_USE_CPP_FILES]
	LRESULT OnFileNewWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnFileNewWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		::PostThreadMessage(_Module.m_dwMainThreadID, WM_USER, 0, 0L);
		return 0;
	}

[!endif]
[!endif]
[!if WTL_USE_TOOLBAR]
[!if WTL_USE_CPP_FILES]
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
[!if WTL_USE_REBAR]
		static BOOL bVisible = TRUE;	// initially visible
		bVisible = !bVisible;
		CReBarCtrl rebar = m_hWndToolBar;
[!if WTL_USE_CMDBAR]
		int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
[!else]
		int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST);	// toolbar is first 1st band
[!endif]
		rebar.ShowBand(nBandIndex, bVisible);
[!else]
		BOOL bVisible = !::IsWindowVisible(m_hWndToolBar);
		::ShowWindow(m_hWndToolBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
[!endif]
		UISetCheck(ID_VIEW_TOOLBAR, bVisible);
		UpdateLayout();
		return 0;
	}

[!endif]
[!endif]
[!if WTL_USE_STATUSBAR]
[!if WTL_USE_CPP_FILES]
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
		::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
		UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
		UpdateLayout();
		return 0;
	}

[!endif]
[!endif]
[!if WTL_USE_CPP_FILES]
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}
[!endif]
[!if WTL_APPTYPE_MDI]

[!if WTL_USE_CPP_FILES]
	LRESULT OnWindowCascade(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnWindowCascade(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		MDICascade();
		return 0;
	}

[!endif]
[!if WTL_USE_CPP_FILES]
	LRESULT OnWindowTile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnWindowTile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		MDITile();
		return 0;
	}

[!endif]
[!if WTL_USE_CPP_FILES]
	LRESULT OnWindowArrangeIcons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
[!else]
	LRESULT OnWindowArrangeIcons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		MDIIconArrange();
		return 0;
	}
[!endif]
[!endif]
};
