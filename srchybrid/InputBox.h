#pragma once


// InputBox dialog

class InputBox : public CDialog
{
	DECLARE_DYNAMIC(InputBox)

public:
	InputBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~InputBox();
	virtual BOOL OnInitDialog();
	afx_msg void OnOK();
	// khaos::categorymod+
	afx_msg void OnCancel();
	// khaos::categorymod-
// Dialog Data
	enum { IDD = IDD_INPUTBOX };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnCleanFilename();
	DECLARE_MESSAGE_MAP()
public:
	// khaos::categorymod+
	void	SetNumber(bool isNum = false) { isNumber = isNum; }
	int	GetInputInt();
	// khaos::categorymod-
	void	SetLabels(CString title, CString label, CString defaultStr);
	CString	GetInput();
	bool	WasCancelled() { return m_cancel;}
	void	SetEditFilenameMode(bool isfilenamemode=true) {m_bFilenameMode=isfilenamemode;}
private:
	CString m_label;
	CString m_title;
	CString m_default;
	CString m_return;
	bool	m_cancel;
	bool	m_bFilenameMode;
	// khaos::categorymod+
	bool	isNumber;
	// khaos::categorymod-
};