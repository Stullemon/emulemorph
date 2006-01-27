/* A Bison parser, made by GNU Bison 1.875b.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_STRING = 258,
     TOK_NUMBER = 259,
     TOK_SIZE = 260,
     TOK_TYPE = 261,
     TOK_EXT = 262,
     TOK_SOURCES = 263,
     TOK_COMPLETE = 264,
     TOK_BITRATE = 265,
     TOK_LENGTH = 266,
     TOK_CODEC = 267,
     TOK_RATING = 268,
     TOK_TITLE = 269,
     TOK_ALBUM = 270,
     TOK_ARTIST = 271,
     TOK_TYPEVAL = 272,
     TOK_AND = 273,
     TOK_OR = 274,
     TOK_NOT = 275,
     TOK_OPR_EQ = 276,
     TOK_OPR_LT = 277,
     TOK_OPR_LE = 278,
     TOK_OPR_GT = 279,
     TOK_OPR_GE = 280,
     TOK_OPR_NE = 281,
     TOK_ED2K_LINK = 282,
     TOK_EOF = 283
   };
#endif
#define TOK_STRING 258
#define TOK_NUMBER 259
#define TOK_SIZE 260
#define TOK_TYPE 261
#define TOK_EXT 262
#define TOK_SOURCES 263
#define TOK_COMPLETE 264
#define TOK_BITRATE 265
#define TOK_LENGTH 266
#define TOK_CODEC 267
#define TOK_RATING 268
#define TOK_TITLE 269
#define TOK_ALBUM 270
#define TOK_ARTIST 271
#define TOK_TYPEVAL 272
#define TOK_AND 273
#define TOK_OR 274
#define TOK_NOT 275
#define TOK_OPR_EQ 276
#define TOK_OPR_LT 277
#define TOK_OPR_LE 278
#define TOK_OPR_GT 279
#define TOK_OPR_GE 280
#define TOK_OPR_NE 281
#define TOK_ED2K_LINK 282
#define TOK_EOF 283




/* Copy the first part of user declarations.  */


#include "stdafx.h"
#include "resource.h"
#include "OtherFunctions.h"
#include "SearchExpr.h"
#include "scanner.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CStringArray _astrParserErrors;

void ParsedSearchExpression(const CSearchExpr* pexpr);
int yyerror(const char* errstr);
int yyerror(LPCTSTR errstr);
int yyerrorf(LPCTSTR errstr, ...);

#pragma warning(disable:4065) // switch statement contains 'default' but no 'case' labels
#pragma warning(disable:4102) // 'yyerrlab1' : unreferenced label
#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)

typedef union YYSTYPE {
	uint64			num;
	int				iopr;
	CStringA*		pstr;
	CSearchExpr*	pexpr;
	CSearchAttr*	pattr;
} YYSTYPE;
/* Line 191 of yacc.c.  */

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */


#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  54
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   242

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  31
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  6
/* YYNRULES -- Number of rules. */
#define YYNRULES  60
/* YYNRULES -- Number of states. */
#define YYNSTATES  93

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   283

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      29,    30,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     6,     9,    12,    14,    17,    19,    23,
      27,    31,    35,    39,    43,    47,    50,    54,    56,    60,
      64,    68,    72,    76,    80,    84,    88,    92,    96,   100,
     104,   106,   110,   113,   117,   120,   124,   127,   131,   134,
     138,   141,   145,   148,   152,   155,   159,   162,   166,   169,
     173,   176,   180,   183,   187,   190,   192,   194,   196,   198,
     200
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      32,     0,    -1,    33,    28,    -1,    27,    28,    -1,    33,
       1,    -1,    34,    -1,    34,    33,    -1,    35,    -1,    34,
      18,    34,    -1,    34,    19,    34,    -1,    34,    20,    34,
      -1,    29,    33,    30,    -1,    34,    18,     1,    -1,    34,
      19,     1,    -1,    34,    20,     1,    -1,    29,     1,    -1,
      29,    33,     1,    -1,     3,    -1,     5,    36,     4,    -1,
       6,    21,    17,    -1,     7,    21,     3,    -1,     8,    36,
       4,    -1,     9,    36,     4,    -1,    13,    36,     4,    -1,
      10,    36,     4,    -1,    11,    36,     4,    -1,    12,    21,
       3,    -1,    14,    21,     3,    -1,    15,    21,     3,    -1,
      16,    21,     3,    -1,    36,    -1,     5,    36,     1,    -1,
       5,     1,    -1,     6,    21,     1,    -1,     6,     1,    -1,
       7,    21,     1,    -1,     7,     1,    -1,     8,    36,     1,
      -1,     8,     1,    -1,     9,    36,     1,    -1,     9,     1,
      -1,    13,    36,     1,    -1,    13,     1,    -1,    10,    36,
       1,    -1,    10,     1,    -1,    11,    36,     1,    -1,    11,
       1,    -1,    12,    21,     1,    -1,    12,     1,    -1,    14,
      21,     1,    -1,    14,     1,    -1,    15,    21,     1,    -1,
      15,     1,    -1,    16,    21,     1,    -1,    16,     1,    -1,
      21,    -1,    24,    -1,    22,    -1,    25,    -1,    23,    -1,
      26,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,    56,    56,    77,    86,   103,   108,   120,   125,   135,
     145,   155,   160,   166,   172,   178,   183,   191,   196,   200,
     205,   210,   214,   218,   222,   226,   230,   235,   240,   245,
     252,   255,   257,   260,   262,   265,   267,   270,   272,   275,
     277,   280,   282,   285,   287,   290,   292,   295,   297,   300,
     302,   305,   307,   310,   312,   316,   318,   320,   322,   324,
     326
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_STRING", "TOK_NUMBER", "TOK_SIZE", 
  "TOK_TYPE", "TOK_EXT", "TOK_SOURCES", "TOK_COMPLETE", "TOK_BITRATE", 
  "TOK_LENGTH", "TOK_CODEC", "TOK_RATING", "TOK_TITLE", "TOK_ALBUM", 
  "TOK_ARTIST", "TOK_TYPEVAL", "TOK_AND", "TOK_OR", "TOK_NOT", 
  "TOK_OPR_EQ", "TOK_OPR_LT", "TOK_OPR_LE", "TOK_OPR_GT", "TOK_OPR_GE", 
  "TOK_OPR_NE", "TOK_ED2K_LINK", "TOK_EOF", "'('", "')'", "$accept", 
  "action", "and_searchexpr", "searchexpr", "attribute", "int_opr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,    40,
      41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    31,    32,    32,    32,    33,    33,    34,    34,    34,
      34,    34,    34,    34,    34,    34,    34,    35,    35,    35,
      35,    35,    35,    35,    35,    35,    35,    35,    35,    35,
      35,    35,    35,    35,    35,    35,    35,    35,    35,    35,
      35,    35,    35,    35,    35,    35,    35,    35,    35,    35,
      35,    35,    35,    35,    35,    36,    36,    36,    36,    36,
      36
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     2,     2,     2,     1,     2,     1,     3,     3,
       3,     3,     3,     3,     3,     2,     3,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       1,     3,     2,     3,     2,     3,     2,     3,     2,     3,
       2,     3,     2,     3,     2,     3,     2,     3,     2,     3,
       2,     3,     2,     3,     2,     1,     1,     1,     1,     1,
       1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,    17,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    55,    57,    59,    56,    58,    60,
       0,     0,     0,     0,     5,     7,    30,    32,     0,    34,
       0,    36,     0,    38,     0,    40,     0,    44,     0,    46,
       0,    48,     0,    42,     0,    50,     0,    52,     0,    54,
       0,     3,    15,     0,     1,     4,     2,     0,     0,     0,
       6,    31,    18,    33,    19,    35,    20,    37,    21,    39,
      22,    43,    24,    45,    25,    47,    26,    41,    23,    49,
      27,    51,    28,    53,    29,    16,    11,    12,     8,    13,
       9,    14,    10
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    22,    23,    24,    25,    26
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -24
static const short yypact[] =
{
     134,   -24,   150,    56,    83,   161,   167,   176,   193,   208,
     202,   209,   210,   211,   -24,   -24,   -24,   -24,   -24,   -24,
     -23,     1,    20,     0,   109,   -24,   -24,   -24,    17,   -24,
       2,   -24,    31,   -24,    46,   -24,   163,   -24,   165,   -24,
     177,   -24,    73,   -24,   203,   -24,   151,   -24,   219,   -24,
     239,   -24,   -24,    -1,   -24,   -24,   -24,    30,    57,    84,
     -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,
     -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,
     -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,    29,   -24,
      41,   -24,   -24
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
     -24,   -24,   184,    44,   -24,   231
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      85,    55,    52,    63,     1,    51,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    61,    64,
      54,    62,    14,    15,    16,    17,    18,    19,    56,    86,
      21,    87,    65,     1,    66,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    67,    58,    59,
      68,    14,    15,    16,    17,    18,    19,    29,    89,    21,
       1,    59,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    75,     0,    76,    30,    14,    15,
      16,    17,    18,    19,    31,    91,    21,     1,     0,     2,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    88,    90,    92,    32,    14,    15,    16,    17,    18,
      19,     0,     1,    21,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,     0,    57,    58,    59,
      14,    15,    16,    17,    18,    19,     0,     1,    21,     2,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    27,    79,     0,    80,    14,    15,    16,    17,    18,
      19,    20,    33,    21,    69,     0,    71,    70,    35,    72,
       0,    14,    15,    16,    17,    18,    19,    37,    73,     0,
       0,    74,    14,    15,    16,    17,    18,    19,    14,    15,
      16,    17,    18,    19,    39,     0,     0,    14,    15,    16,
      17,    18,    19,    43,    77,    53,     0,    78,    60,    41,
      45,    47,    49,     0,    14,    15,    16,    17,    18,    19,
      81,     0,    82,    14,    15,    16,    17,    18,    19,    42,
      46,    48,    50,    28,     0,     0,    34,    36,    38,    40,
      83,    44,    84
};

static const yysigned_char yycheck[] =
{
       1,     1,     1,     1,     3,    28,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,     1,    17,
       0,     4,    21,    22,    23,    24,    25,    26,    28,    30,
      29,     1,     1,     3,     3,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,     1,    19,    20,
       4,    21,    22,    23,    24,    25,    26,     1,     1,    29,
       3,    20,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,     1,    -1,     3,    21,    21,    22,
      23,    24,    25,    26,     1,     1,    29,     3,    -1,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    57,    58,    59,    21,    21,    22,    23,    24,    25,
      26,    -1,     3,    29,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    -1,     3,    29,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,     1,     1,    -1,     3,    21,    22,    23,    24,    25,
      26,    27,     1,    29,     1,    -1,     1,     4,     1,     4,
      -1,    21,    22,    23,    24,    25,    26,     1,     1,    -1,
      -1,     4,    21,    22,    23,    24,    25,    26,    21,    22,
      23,    24,    25,    26,     1,    -1,    -1,    21,    22,    23,
      24,    25,    26,     1,     1,    21,    -1,     4,    24,     1,
       1,     1,     1,    -1,    21,    22,    23,    24,    25,    26,
       1,    -1,     3,    21,    22,    23,    24,    25,    26,    21,
      21,    21,    21,     2,    -1,    -1,     5,     6,     7,     8,
       1,    10,     3
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    21,    22,    23,    24,    25,    26,
      27,    29,    32,    33,    34,    35,    36,     1,    36,     1,
      21,     1,    21,     1,    36,     1,    36,     1,    36,     1,
      36,     1,    21,     1,    36,     1,    21,     1,    21,     1,
      21,    28,     1,    33,     0,     1,    28,    18,    19,    20,
      33,     1,     4,     1,    17,     1,     3,     1,     4,     1,
       4,     1,     4,     1,     4,     1,     3,     1,     4,     1,
       3,     1,     3,     1,     3,     1,    30,     1,    34,     1,
      34,     1,    34
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

    {
						ParsedSearchExpression(yyvsp[-1].pexpr);
						delete yyvsp[-1].pexpr;
						return 0;
					;}
    break;

  case 3:

    {
						CSearchExpr* pexpr = new CSearchExpr(new CSearchAttr(yyvsp[-1].pstr));
						ParsedSearchExpression(pexpr);
						delete pexpr;
						delete yyvsp[-1].pstr;
						return 0;
					;}
    break;

  case 4:

    {
						yyerror(GetResString(IDS_SEARCH_GENERALERROR));
						delete yyvsp[-1].pexpr;
						return 1;
					;}
    break;

  case 5:

    {
						yyval.pexpr = yyvsp[0].pexpr;
					;}
    break;

  case 6:

    {
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_AND);
						pexpr->Add(yyvsp[-1].pexpr);
						pexpr->Add(yyvsp[0].pexpr);
						yyval.pexpr = pexpr;
						delete yyvsp[-1].pexpr;
						delete yyvsp[0].pexpr;
					;}
    break;

  case 7:

    {
						yyval.pexpr = new CSearchExpr(yyvsp[0].pattr);
						delete yyvsp[0].pattr;
					;}
    break;

  case 8:

    {
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_AND);
						pexpr->Add(yyvsp[-2].pexpr);
						pexpr->Add(yyvsp[0].pexpr);
						yyval.pexpr = pexpr;
						delete yyvsp[-2].pexpr;
						delete yyvsp[0].pexpr;
					;}
    break;

  case 9:

    {
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_OR);
						pexpr->Add(yyvsp[-2].pexpr);
						pexpr->Add(yyvsp[0].pexpr);
						yyval.pexpr = pexpr;
						delete yyvsp[-2].pexpr;
						delete yyvsp[0].pexpr;
					;}
    break;

  case 10:

    {
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_NOT);
						pexpr->Add(yyvsp[-2].pexpr);
						pexpr->Add(yyvsp[0].pexpr);
						yyval.pexpr = pexpr;
						delete yyvsp[-2].pexpr;
						delete yyvsp[0].pexpr;
					;}
    break;

  case 11:

    {
						yyval.pexpr = yyvsp[-1].pexpr;
					;}
    break;

  case 12:

    {
						yyerror(GetResString(IDS_SEARCH_MISSINGANDRIGHT));
						delete yyvsp[-2].pexpr;
						return 1;
					;}
    break;

  case 13:

    {
						yyerror(GetResString(IDS_SEARCH_MISSINGORRIGHT));
						delete yyvsp[-2].pexpr;
						return 1;
					;}
    break;

  case 14:

    {
						yyerror(GetResString(IDS_SEARCH_MISSINGNOTRIGHT));
						delete yyvsp[-2].pexpr;
						return 1;
					;}
    break;

  case 15:

    {
						yyerror(GetResString(IDS_SEARCH_MISSINGEXPRPARANT));
						return 1;
					;}
    break;

  case 16:

    {
						yyerror(GetResString(IDS_SEARCH_MISSINGCLOSINGPARANT));
						delete yyvsp[-1].pexpr;
						return 1;
					;}
    break;

  case 17:

    {
						yyval.pattr = new CSearchAttr(yyvsp[0].pstr);
						delete yyvsp[0].pstr;
					;}
    break;

  case 18:

    {
						yyval.pattr = new CSearchAttr(FT_FILESIZE, yyvsp[-1].iopr, yyvsp[0].num);
					;}
    break;

  case 19:

    {
						yyval.pattr = new CSearchAttr(FT_FILETYPE, yyvsp[0].pstr);
						delete yyvsp[0].pstr;
					;}
    break;

  case 20:

    {
						yyval.pattr = new CSearchAttr(FT_FILEFORMAT, yyvsp[0].pstr);
						delete yyvsp[0].pstr;
					;}
    break;

  case 21:

    {
						yyval.pattr = new CSearchAttr(FT_SOURCES, yyvsp[-1].iopr, yyvsp[0].num);
					;}
    break;

  case 22:

    {
						yyval.pattr = new CSearchAttr(FT_COMPLETE_SOURCES, yyvsp[-1].iopr, yyvsp[0].num);
					;}
    break;

  case 23:

    {
						yyval.pattr = new CSearchAttr(FT_FILERATING, yyvsp[-1].iopr, yyvsp[0].num);
					;}
    break;

  case 24:

    {
						yyval.pattr = new CSearchAttr(FT_MEDIA_BITRATE, yyvsp[-1].iopr, yyvsp[0].num);
					;}
    break;

  case 25:

    {
						yyval.pattr = new CSearchAttr(FT_MEDIA_LENGTH, yyvsp[-1].iopr, yyvsp[0].num);
					;}
    break;

  case 26:

    {
						yyval.pattr = new CSearchAttr(FT_MEDIA_CODEC, yyvsp[0].pstr);
						delete yyvsp[0].pstr;
					;}
    break;

  case 27:

    {
						yyval.pattr = new CSearchAttr(FT_MEDIA_TITLE, yyvsp[0].pstr);
						delete yyvsp[0].pstr;
					;}
    break;

  case 28:

    {
						yyval.pattr = new CSearchAttr(FT_MEDIA_ALBUM, yyvsp[0].pstr);
						delete yyvsp[0].pstr;
					;}
    break;

  case 29:

    {
						yyval.pattr = new CSearchAttr(FT_MEDIA_ARTIST, yyvsp[0].pstr);
						delete yyvsp[0].pstr;
					;}
    break;

  case 30:

    { yyerrorf(GetResString(IDS_SEARCH_OPRERR), DbgGetSearchOperatorName(yyvsp[0].iopr)); return 1; ;}
    break;

  case 31:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@size")); return 1; ;}
    break;

  case 32:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@size")); return 1; ;}
    break;

  case 33:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@type")); return 1; ;}
    break;

  case 34:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@type")); return 1; ;}
    break;

  case 35:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@ext")); return 1; ;}
    break;

  case 36:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@ext")); return 1; ;}
    break;

  case 37:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@sources")); return 1; ;}
    break;

  case 38:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@sources")); return 1; ;}
    break;

  case 39:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@complete")); return 1; ;}
    break;

  case 40:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@complete")); return 1; ;}
    break;

  case 41:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@rating")); return 1; ;}
    break;

  case 42:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@rating")); return 1; ;}
    break;

  case 43:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@bitrate")); return 1; ;}
    break;

  case 44:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@bitrate")); return 1; ;}
    break;

  case 45:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@length")); return 1; ;}
    break;

  case 46:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@length")); return 1; ;}
    break;

  case 47:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@codec")); return 1; ;}
    break;

  case 48:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@codec")); return 1; ;}
    break;

  case 49:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@title")); return 1; ;}
    break;

  case 50:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@title")); return 1; ;}
    break;

  case 51:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@album")); return 1; ;}
    break;

  case 52:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@album")); return 1; ;}
    break;

  case 53:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@artist")); return 1; ;}
    break;

  case 54:

    { yyerrorf(GetResString(IDS_SEARCH_ATTRERR), _T("@artist")); return 1; ;}
    break;

  case 55:

    { yyval.iopr = ED2K_SEARCH_OP_EQUAL; ;}
    break;

  case 56:

    { yyval.iopr = ED2K_SEARCH_OP_GREATER; ;}
    break;

  case 57:

    { yyval.iopr = ED2K_SEARCH_OP_LESS; ;}
    break;

  case 58:

    { yyval.iopr = ED2K_SEARCH_OP_GREATER_EQUAL; ;}
    break;

  case 59:

    { yyval.iopr = ED2K_SEARCH_OP_LESS_EQUAL; ;}
    break;

  case 60:

    { yyval.iopr = ED2K_SEARCH_OP_NOTEQUAL; ;}
    break;


    }

/* Line 999 of yacc.c.  */


  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}





int yyerror(const char* errstr)
{
	// Errors created by yacc generated code
	//yyerror ("syntax error: cannot back up");
	//yyerror ("syntax error; also virtual memory exhausted");
	//yyerror ("syntax error");
	//yyerror ("parser stack overflow");

	if (strcmp(errstr, "syntax error") == 0) {
		// If there is already a error in the list, don't add the "syntax error" string.
		// This is needed to not 'overwrite' any errors which were placed by 'lex' there,
		// because we will read only the last error eventually.
		if (_astrParserErrors.GetCount() > 0)
			return EXIT_FAILURE;
	}
	else {
		if (_astrParserErrors.GetCount() > 0 && _astrParserErrors[_astrParserErrors.GetCount() - 1] != "syntax error")
			return EXIT_FAILURE;
	}

	USES_CONVERSION;
	_astrParserErrors.Add(A2CT(errstr));
	return EXIT_FAILURE;
}

int yyerror(LPCTSTR errstr)
{
	if (_tcscmp(errstr, _T("syntax error")) == 0) {
		// If there is already a error in the list, don't add the "syntax error" string.
		// This is needed to not 'overwrite' any errors which were placed by 'lex' there,
		// because we will read only the last error eventually.
		if (_astrParserErrors.GetCount() > 0)
			return EXIT_FAILURE;
	}
	else {
		if (_astrParserErrors.GetCount() > 0 && _astrParserErrors[_astrParserErrors.GetCount() - 1] != _T("syntax error"))
			return EXIT_FAILURE;
	}
	_astrParserErrors.Add(errstr);
	return EXIT_FAILURE;
}

int yyerrorf(LPCTSTR errstr, ...)
{
	// If there is already a error in the list, don't add the "syntax error" string.
	// This is needed to not 'overwrite' any errors which were placed by 'lex' there,
	// because we will read only the last error eventually.
	if (_tcscmp(errstr, _T("syntax error")) == 0) {
		if (_astrParserErrors.GetCount() > 0)
			return EXIT_FAILURE;
	}
	else {
		if (_astrParserErrors.GetCount() > 0 && _astrParserErrors[_astrParserErrors.GetCount() - 1] != _T("syntax error"))
			return EXIT_FAILURE;
	}

	va_list argp;
	va_start(argp, errstr);
	CString strError;
	strError.FormatV(errstr, argp);
	_astrParserErrors.Add(strError);
	va_end(argp);
	return EXIT_FAILURE;
}

