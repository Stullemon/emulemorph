class CEditView : public CWindowImpl<CEditView, CRichEditCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(NULL, CRichEditCtrl::GetWndClassName())

	enum
	{
		cchTAB = 8,
		nSearchStringLen = 128,
		nMaxBufferLen = 4000000
	};

	CFont m_font;
	int m_nRow, m_nCol;
	TCHAR m_strFilePath[MAX_PATH];
	TCHAR m_strFileName[MAX_PATH];
	CFindReplaceDialog* m_pFindDlg;
	TCHAR m_strFind[nSearchStringLen];
	BOOL m_bMatchCase;
	BOOL m_bWholeWord;
	BOOL m_bFindOnly;
	BOOL m_bWordWrap;

	CEditView() : 
		m_nRow(0), m_nCol(0), 
		m_pFindDlg(NULL), m_bMatchCase(FALSE), m_bWholeWord(FALSE),
		m_bFindOnly(TRUE), m_bWordWrap(FALSE)
	{
		m_font = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
		m_strFilePath[0] = 0;
		m_strFileName[0] = 0;
		m_strFind[0] = 0;
	}

	void Sorry()
	{
		MessageBox(_T("Sorry, not yet implemented"), _T("MTPad"), MB_OK);
	}

	BEGIN_MSG_MAP(CEditView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKey)
		MESSAGE_HANDLER(WM_KEYUP, OnKey)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnKey)
		MESSAGE_HANDLER(CFindReplaceDialog::GetFindReplaceMsg(), OnFindReplaceCmd)
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_EDIT_UNDO, OnEditUndo)
		COMMAND_ID_HANDLER(ID_EDIT_CUT, OnEditCut)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
		COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnEditPaste)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR, OnEditClear)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, OnEditSelectAll)
		COMMAND_ID_HANDLER(ID_EDIT_WORD_WRAP, OnEditWordWrap)
		COMMAND_ID_HANDLER(ID_EDIT_FIND, OnEditFind)
		COMMAND_ID_HANDLER(ID_EDIT_REPEAT, OnEditFindNext)
		COMMAND_ID_HANDLER(ID_EDIT_REPLACE, OnEditReplace)
		COMMAND_ID_HANDLER(ID_FORMAT_FONT, OnViewFormatFont)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

		LimitText(nMaxBufferLen);
		SetFont(m_font);

		CClientDC dc(m_hWnd);
		HFONT hOldFont = dc.SelectFont(m_font);
		TEXTMETRIC tm;
		dc.GetTextMetrics(&tm);
		int nLogPix = dc.GetDeviceCaps(LOGPIXELSX);
		dc.SelectFont(hOldFont);
		int cxTab = ::MulDiv(tm.tmAveCharWidth * cchTAB, 1440, nLogPix);	// 1440 twips = 1 inch
		if(cxTab != -1)
		{
			PARAFORMAT pf;
			pf.cbSize = sizeof(PARAFORMAT);
			pf.dwMask = PFM_TABSTOPS;
			pf.cTabCount = MAX_TAB_STOPS;
			for(int i = 0; i < MAX_TAB_STOPS; i++)
				pf.rgxTabs[i] = (i + 1) * cxTab;
			SetParaFormat(pf);
		}
		dc.SelectFont(hOldFont);

		SetModify(FALSE);

		return lRet;
	}

	LRESULT OnKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

		// calc caret position
		long nStartPos, nEndPos;
		GetSel(nStartPos, nEndPos);
		m_nRow = LineFromChar(nEndPos);
		m_nCol = 0;
		int nChar = nEndPos - LineIndex();
		if(nChar > 0)
		{
			LPTSTR lpstrLine = (LPTSTR)_alloca(max(2, (nChar + 1) * sizeof(TCHAR)));	// min = WORD for length
			nChar = GetLine(m_nRow, lpstrLine, nChar);
			for(int i = 0; i < nChar; i++)
			{
				if(lpstrLine[i] == _T('\t'))
					m_nCol = ((m_nCol / cchTAB) + 1) * cchTAB;
				else
					m_nCol++;
			}
		}

		::SendMessage(GetParent(), WM_UPDATEROWCOL, 0, 0L);

		return lRet;
	}

	LRESULT OnEditUndo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Undo();
		return 0;
	}

	LRESULT OnEditCut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Cut();
		return 0;
	}

	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Copy();
		return 0;
	}

	LRESULT OnEditPaste(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PasteSpecial(CF_TEXT);
		return 0;
	}

	LRESULT OnEditClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Clear();
		return 0;
	}

	LRESULT OnEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		SetSel(0, -1);
		return 0;
	}

	LRESULT OnEditWordWrap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_bWordWrap = !m_bWordWrap;
		int nLine = m_bWordWrap ? 0 : 1;
		SetTargetDevice(NULL, nLine);

		return 0;
	}

	LRESULT OnEditFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(m_pFindDlg != NULL)
		{
			if(m_bFindOnly)
			{
				_ASSERTE(::IsWindow(m_pFindDlg->m_hWnd));
				m_pFindDlg->SetFocus();
				return 1;
			}
			else
				m_pFindDlg->DestroyWindow();
		}

		m_pFindDlg = new CFindReplaceDialog;

		if(m_pFindDlg == NULL)
		{
			_ASSERTE(FALSE);
			::MessageBeep((UINT)-1);
			return 1;
		}

		m_bFindOnly = TRUE;
		DWORD dwFlags = FR_HIDEUPDOWN;
		if(m_bMatchCase)
			dwFlags |= FR_MATCHCASE;
		if(m_bWholeWord)
			dwFlags |= FR_WHOLEWORD;

		if(!m_pFindDlg->Create(TRUE, m_strFind, NULL, dwFlags, m_hWnd))
		{
			delete m_pFindDlg;
			m_pFindDlg = NULL;
			::MessageBeep((UINT)-1);
			return 1;
		}

		m_pFindDlg->ShowWindow(SW_NORMAL);
		return 0;
	}

	LRESULT OnEditFindNext(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if(m_strFind[0] == 0)
			return OnEditFind(wNotifyCode, wID, hWndCtl, bHandled);

		BOOL bRet = DoFindText();
		if(!bRet)
			::MessageBeep((UINT)-1);

		return 0;
	}

	LRESULT OnFindReplaceCmd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		CFindReplaceDialog* pDlg = CFindReplaceDialog::GetNotifier(lParam);
		if(pDlg == NULL)
		{
			::MessageBeep((UINT)-1);
			return 1;
		}
		_ASSERTE(pDlg == m_pFindDlg);

		if(pDlg->FindNext())
		{
			lstrcpyn(m_strFind, pDlg->m_fr.lpstrFindWhat, nSearchStringLen);
			m_bMatchCase = pDlg->MatchCase();
			m_bWholeWord = pDlg->MatchWholeWord();

			BOOL bRet = DoFindText();
			if(!bRet)
				::MessageBeep((UINT)-1);
		}
		else if(pDlg->ReplaceCurrent())
		{
			lstrcpyn(m_strFind, pDlg->m_fr.lpstrFindWhat, nSearchStringLen);
			m_bMatchCase = pDlg->MatchCase();
			m_bWholeWord = pDlg->MatchWholeWord();

			CHARRANGE chrg;
			GetSel(chrg);

			if(chrg.cpMin != chrg.cpMax)
			{
				USES_CONVERSION;
				LPSTR lpstrTextA = (LPSTR)_alloca(chrg.cpMax - chrg.cpMin + 2);
				GetSelText(lpstrTextA);
				LPTSTR lpstrText = A2T(lpstrTextA);
				int nRet;
				if(m_bMatchCase)
					nRet = lstrcmp(lpstrText, m_strFind);
				else
					nRet = lstrcmpi(lpstrText, m_strFind);
				if(nRet == 0)
					ReplaceSel(pDlg->GetReplaceString(), TRUE);
			}

			BOOL bRet = DoFindText();
			if(!bRet)
				::MessageBeep((UINT)-1);

		}
		else if(pDlg->ReplaceAll())
		{
			lstrcpyn(m_strFind, pDlg->m_fr.lpstrFindWhat, nSearchStringLen);
			m_bMatchCase = pDlg->MatchCase();
			m_bWholeWord = pDlg->MatchWholeWord();

			HCURSOR hOldCursor = NULL;
			SetRedraw(FALSE);
			BOOL bRet = DoFindText(FALSE);
			if(!bRet)
				::MessageBeep((UINT)-1);
			else
			{
				hOldCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
				do
				{
					ReplaceSel(pDlg->GetReplaceString(), TRUE);
				}
				while(DoFindText(FALSE));
			}
			SetRedraw(TRUE);
			Invalidate();
			UpdateWindow();
			if(hOldCursor != NULL)
				::SetCursor(hOldCursor);
		}
		else if(pDlg->IsTerminating())
			m_pFindDlg = NULL;

		return 0;
	}

	BOOL DoFindText(BOOL bNext = TRUE)
	{
		DWORD dwFlags = FR_DOWN;
		if(m_bMatchCase)
			dwFlags |= FR_MATCHCASE;
		if(m_bWholeWord)
			dwFlags |= FR_WHOLEWORD;

		CHARRANGE chrg;
		GetSel(chrg);

		FINDTEXTEX ft;
		ft.chrg.cpMin = bNext ? chrg.cpMax : chrg.cpMin;
		ft.chrg.cpMax = -1;
		ft.lpstrText = m_strFind;

		if(FindText(dwFlags, ft) == -1)
			return FALSE;

		SetSel(ft.chrgText);

		return TRUE;
	}

	LRESULT OnEditReplace(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(m_pFindDlg != NULL)
		{
			if(!m_bFindOnly)
			{
				_ASSERTE(::IsWindow(m_pFindDlg->m_hWnd));
				m_pFindDlg->SetFocus();
				return 1;
			}
			else
				m_pFindDlg->DestroyWindow();
		}

		m_pFindDlg = new CFindReplaceDialog;

		if(m_pFindDlg == NULL)
		{
			_ASSERTE(FALSE);
			::MessageBeep((UINT)-1);
			return 1;
		}

		m_bFindOnly = FALSE;
		DWORD dwFlags = FR_HIDEUPDOWN;
		if(m_bMatchCase)
			dwFlags |= FR_MATCHCASE;
		if(m_bWholeWord)
			dwFlags |= FR_WHOLEWORD;

		if(!m_pFindDlg->Create(FALSE, m_strFind, NULL, dwFlags, m_hWnd))
		{
			delete m_pFindDlg;
			m_pFindDlg = NULL;
			::MessageBeep((UINT)-1);
			return 1;
		}

		m_pFindDlg->ShowWindow(SW_NORMAL);
		return 0;
	}

	LRESULT OnViewFormatFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CFontDialog dlg;
		m_font.GetLogFont(&dlg.m_lf);
		dlg.m_cf.Flags |= CF_INITTOLOGFONTSTRUCT;
		if (dlg.DoModal() == IDOK)
		{
			m_font.DeleteObject();
			m_font.CreateFontIndirect(&dlg.m_lf);
			SetFont(m_font);
		}
		return 0;
	}

	BOOL LoadFile(LPCTSTR lpstrFilePath)
	{
		_ASSERTE(lpstrFilePath != NULL);

		HANDLE hFile = ::CreateFile(lpstrFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		EDITSTREAM es;
		es.dwCookie = (DWORD)hFile;
		es.dwError = 0;
		es.pfnCallback = StreamReadCallback;
		StreamIn(SF_TEXT, es);

		::CloseHandle(hFile);

		return !(BOOL)es.dwError;
	}

	static DWORD CALLBACK StreamReadCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR *pcb)
	{
		_ASSERTE(dwCookie != 0);
		_ASSERTE(pcb != NULL);

		return !::ReadFile((HANDLE)dwCookie, pbBuff, cb, (LPDWORD)pcb, NULL);
	}

	BOOL SaveFile(LPTSTR lpstrFilePath)
	{
		_ASSERTE(lpstrFilePath != NULL);

		HANDLE hFile = ::CreateFile(lpstrFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		EDITSTREAM es;
		es.dwCookie = (DWORD)hFile;
		es.dwError = 0;
		es.pfnCallback = StreamWriteCallback;
		StreamOut(SF_TEXT, es);

		::CloseHandle(hFile);

		return !(BOOL)es.dwError;
	}

	static DWORD CALLBACK StreamWriteCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR *pcb)
	{
		_ASSERTE(dwCookie != 0);
		_ASSERTE(pcb != NULL);

		return !::WriteFile((HANDLE)dwCookie, pbBuff, cb, (LPDWORD)pcb, NULL);
	}

	void Init(LPCTSTR lpstrFilePath, LPCTSTR lpstrFileName)
	{
		lstrcpy(m_strFilePath, lpstrFilePath);
		lstrcpy(m_strFileName, lpstrFileName);
		SetModify(FALSE);
	}

	BOOL QueryClose()
	{
		if(!GetModify())
			return TRUE;

		CWindow wndMain(GetParent());
		TCHAR szBuff[MAX_PATH + 30];
		wsprintf(szBuff, _T("Save changes to %s ?"), m_strFileName);
		int nRet = wndMain.MessageBox(szBuff, _T("MTPad"), MB_YESNOCANCEL | MB_ICONEXCLAMATION);

		if(nRet == IDCANCEL)
			return FALSE;
		else if(nRet == IDYES)
		{
			if(!DoFileSaveAs())
				return FALSE;
		}

		return TRUE;
	}

	BOOL DoFileSaveAs()
	{
		BOOL bRet = FALSE;

		CFileDialog dlg(FALSE, NULL, m_strFilePath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpcstrFilter);
		int nRet = dlg.DoModal();

		if(nRet == IDOK)
		{
			ATLTRACE(_T("File path: %s\n"), dlg.m_ofn.lpstrFile);
			bRet = SaveFile(dlg.m_ofn.lpstrFile);
			if(bRet)
				Init(dlg.m_ofn.lpstrFile, dlg.m_ofn.lpstrFileTitle);
			else
				MessageBox(_T("Error writing file!\n"));
		}

		return bRet;
	}
};
