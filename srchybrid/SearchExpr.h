#pragma once

enum ESearchOperators
{
	SEARCHOP_AND,
	SEARCHOP_OR,
	SEARCHOP_NOT
};

#define	SEARCHOPTOK_AND	_T("\255AND")
#define	SEARCHOPTOK_OR	_T("\255OR")
#define	SEARCHOPTOK_NOT	_T("\255NOT")

class CSearchExpr
{
public:
	CSearchExpr(){}
	CSearchExpr(LPCTSTR pszString)
	{
		Add(pszString);
	}
	CSearchExpr(const CString* pstrString)
	{
		Add(*pstrString);
	}
	
	void Add(ESearchOperators eOperator)
	{
		if (eOperator == SEARCHOP_AND)
			m_aExpr.Add(SEARCHOPTOK_AND);
		if (eOperator == SEARCHOP_OR)
			m_aExpr.Add(SEARCHOPTOK_OR);
		if (eOperator == SEARCHOP_NOT)
			m_aExpr.Add(SEARCHOPTOK_NOT);
	}
	void Add(LPCTSTR pszString)
	{
		m_aExpr.Add(pszString);
	}
	void Add(const CString* pstrString)
	{
		m_aExpr.Add(*pstrString);
	}
	void Add(const CSearchExpr* pexpr)
	{
		m_aExpr.Append(pexpr->m_aExpr);
	}
	void Concatenate(const CString* pstrString)
	{
		ASSERT( m_aExpr.GetSize() == 1 );
		m_aExpr[0] += _T(' ');
		m_aExpr[0] += *pstrString;
	}
	
	CStringArray m_aExpr;
};
