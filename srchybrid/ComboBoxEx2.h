#pragma once

class CComboBoxEx2 : public CComboBoxEx
{
	DECLARE_DYNAMIC(CComboBoxEx2)
public:
	CComboBoxEx2();
	virtual ~CComboBoxEx2();

	int AddItem(LPCTSTR pszText, int iImage);

	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	DECLARE_MESSAGE_MAP()
};

void UpdateHorzExtent(CComboBox &rctlComboBox, int iIconWidth);
