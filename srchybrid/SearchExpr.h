#pragma once

enum ESearchOperators
{
	SEARCHOP_AND,
	SEARCHOP_OR,
	SEARCHOP_NOT
};

#define	SEARCHOPTOK_AND	"\255AND"
#define	SEARCHOPTOK_OR	"\255OR"
#define	SEARCHOPTOK_NOT	"\255NOT"

class CSearchExpr
{
public:
	CSearchExpr(){}
	CSearchExpr(LPCSTR pszString)
	{
		Add(pszString);
	}
	CSearchExpr(const CStringA* pstrString)
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
	void Add(LPCSTR pszString)
	{
		m_aExpr.Add(pszString);
	}
	void Add(const CStringA* pstrString)
	{
		m_aExpr.Add(*pstrString);
	}
	void Add(const CSearchExpr* pexpr)
	{
		m_aExpr.Append(pexpr->m_aExpr);
	}
	void Concatenate(const CStringA* pstrString)
	{
		ASSERT( m_aExpr.GetSize() == 1 );
		m_aExpr[0] += ' ';
		m_aExpr[0] += *pstrString;
	}
	
	CStringAArray m_aExpr;
};
