#pragma once
class CColorEdit : public CEdit
{
	// Construction
public:
	CColorEdit();
	virtual ~CColorEdit();
	// Attributes
public:

	// Operations
public:

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CColorEdit)
		//}}AFX_VIRTUAL

	// Implementation
public:
	void SetTextColor(COLORREF rgb);
	void SetBackColor(COLORREF rgb);
	// Generated message map functions
protected:
	//text and text background colors
	COLORREF m_crText;
	COLORREF m_crBackGnd;
	//background brush
	CBrush m_brBackGnd;
	//{{AFX_MSG(CEditEx)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
