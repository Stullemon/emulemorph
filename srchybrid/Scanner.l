%{
#include "stdafx.h"
#include "resource.h"
#include <io.h>
#include "SearchExpr.h"
#include "scanner.h"
#include "parser.hpp"
#include "OtherFunctions.h"
#include "ED2KLink.h"
#include "StringConversion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif //_DEBUG

#define YY_NEVER_INTERACTIVE 1

extern int yyerror(const char* errstr);
#ifdef _UNICODE
extern int yyerror(LPCTSTR errstr);
#endif

#define	ECHO				TRACE
#define	YY_INPUT			ReadLexBuff
#define	YY_FATAL_ERROR		FatalLexError

static void ReadLexBuff(char* pcBuff, int& riResult, size_t uMaxSize);
static void FatalLexError(yyconst char msg[]);

static CStringA _strInputBuff;
static LPCSTR _pszLexBuff;
%}

%option noyywrap

keywordchar		[^ \"()]

%%

[ ]				{ /* Skip blanks. */ }
"OR"			{ return TOK_OR; }
"AND"			{ return TOK_AND; }
"NOT"			{ return TOK_NOT; }

"ed2k://|file|"[^|]+"|"[0-9]+"|"[a-fA-F0-9]{32}"|/"	{
					CString strError;
					try
					{
						USES_CONVERSION;
						CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(A2T(yytext));
						if (pLink && pLink->GetKind() == CED2KLink::kFile)
						{
							CED2KFileLink* pFileLink = pLink->GetFileLink();
							if (pFileLink)
							{
								yylval.pstr = new CStringA;
								yylval.pstr->Format("ed2k::%s", md4strA(pFileLink->GetHashKey()));
								delete pLink;
								return TOK_ED2K_LINK;
							}
						}
						delete pLink;
					}
					catch(CString strEd2kLinkError)
					{
						strError = strEd2kLinkError;
					}
					if (strError.IsEmpty())
						strError = _T("Invalid eD2K file link");
					yyerror(strError);
					yylval.pstr = new CStringA(yytext);
					return TOK_STRING;
				}

"ed2k::"[a-fA-F0-9]{32}	{
					yylval.pstr = new CStringA(yytext);
					return TOK_ED2K_LINK;
				}

{keywordchar}*  {
					yylval.pstr = new CStringA(yytext);
					return TOK_STRING;
                }

"\""			{
					int l = 128;
					char* psz = (char*)malloc(l);
					int i = 0;
					int c;
					while ((c = yyinput()) != '\"')
					{
						if (c == EOF || c == '\n'){
							unput(c);
							yyerror(GetResString(IDS_SEARCH_UNTERMSTRING));
							break;
						}
						if (c == '\\'){		/*Escape sequence*/
							switch (c = yyinput())
							{
							case '\n':
								continue;
							case 't':		/*Tab*/
								c = '\t';
								break;
							case 'n':		/*Linefeed*/
								c = '\n';
								break;
							case 'f':		/*Formfeed*/
								c = '\f';
								break;
							case 'r':		/*Carriage return*/
								c = '\r';
								break;
							case '\\':		/*Backslash*/
								c = '\\';
								break;
							case '"':		/*Double quotation mark*/
								c = '\"';
								break;
							case '\'':		/*Single quotation mark*/
								c = '\'';
								break;
							case '?':		/*Question mark*/
								c = '\?';
								break;
							case 'v':		/*Vertical Tab*/
								c = '\v';
								break;
							case 'a':		/*Alert*/
								c = '\a';
								break;
							case 'b':		/*Backspace*/
								c = '\b';
								break;
							case 'x':		/*Hexadecimal number*/
								{
									int n, octv;
									for (n = 1, octv = 0; n <= 3; n++) {
										if ((c = yyinput()) >= '0' && c <= '9')
											c -= '0';
										else if (c >= 'a' && c <= 'f')
											c = (c - 'a') + 10;
										else if (c >= 'A' && c <= 'F')
											c = (c - 'A') + 10;
										else
											break;
										octv = octv * 16 + c;
									}
									unput(c);
									if (n == 1)
										c = 'x';
									else
										c = octv;
								}
								break;
							}
						}
						else{
							if ((unsigned char)c >= 0x80 && IsDBCSLeadByte(yytext[0])){
								psz[i++] = (unsigned char)c;
								if (i >= l){
									psz = (char*)realloc(psz, l += 128);
									if (psz == NULL){
										yyerror("Less memory for string");
										break;
									}
								}
								c = yyinput();
							}
						}

						psz[i++] = (unsigned char)c;
						if (i >= l){
							psz = (char*)realloc(psz, l += 128);
							if (psz == NULL){
								yyerror("Less memory for string");
								break;
							}
						}
					}
					psz[i] = '\0';
					yylval.pstr = new CStringA(psz);
					free(psz);
					return TOK_STRING;
				}

.				{ return yytext[0]; }

%%

static void ReadLexBuff(char* pcBuff, int& riResult, size_t uMaxSize)
{
	ASSERT( _pszLexBuff != NULL );
	if (_pszLexBuff == NULL)
		YY_FATAL_ERROR("Input in flex scanner failed");

	ASSERT( sizeof(YY_CHAR) == sizeof(char) );
	size_t uCharsInBuff = strlen(_pszLexBuff);
	size_t uCharsRead = min(uMaxSize, uCharsInBuff);
	riResult = uCharsRead;
	memcpy(pcBuff, _pszLexBuff, uCharsRead);
	_pszLexBuff += uCharsRead;
}

static void FatalLexError(yyconst char msg[])
{
#ifdef _CONSOLE
	printf("Fatal error in flex scanner: %s\n", msg);
#else
	USES_CONVERSION;
	AfxMessageBox(CString(_T("Fatal error in flex scanner: ")) + A2CT(msg), MB_ICONSTOP);
#endif
}

void LexInit(LPCTSTR pszInput)
{
	_strInputBuff = StrToUtf8(pszInput);
	_pszLexBuff = (LPCSTR)_strInputBuff;
}

void LexFree()
{
	yytext = NULL;
	yyleng = 0;
	yyin = NULL;
	yyout = NULL;
	yy_delete_buffer(yy_current_buffer);
	yy_hold_char = '\0';
	yy_n_chars = 0;
	yy_c_buf_p = NULL;
	yy_init = 1;
	yy_start = 0;
	yy_did_buffer_switch_on_eof = 0;
	yy_last_accepting_state = 0;
	yy_last_accepting_cpos = NULL;
#if YY_STACK_USED
	yy_start_stack_ptr = 0;
	yy_start_stack_depth = 0;
	yy_start_stack = NULL;
#endif
	_strInputBuff.Empty();
	_pszLexBuff = NULL;
}
