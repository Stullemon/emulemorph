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
#pragma once


// finestra di dialogo CPPgNotify

class CPPgNotify : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgNotify)

public:
	CPPgNotify();
	virtual ~CPPgNotify();	
	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }
	void Localize(void);

	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

// Dialog Data
	enum { IDD = IDD_PPG_NOTIFY };

protected:
	CPreferences* app_prefs;
	void LoadSettings(void);

	virtual void DoDataExchange(CDataExchange* pDX);
	
	afx_msg void OnSettingsChange() { SetModified(); }
	afx_msg void OnBnClickedCbTbnOnchat();
	afx_msg void OnBnClickedBtnBrowseWav();
	afx_msg void OnBnClickedUseSound();

	DECLARE_MESSAGE_MAP()
};
