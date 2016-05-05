/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 18 "parser.y"

#include <assert.h>
#include "mdbSQLParser.h"
#include "mdbMalloc.h"

/*using namespace QuickMDB;*/

/*
** Disable all error recovery processing in the parser push-down
** automaton.
*/
#define YYNOERRORRECOVERY 1
# define testcase(X)
/*
** Make yytestcase() the same as testcase()
*/
#define yytestcase(X) testcase(X)

# define ALWAYS(X)      (X)
# define NEVER(X)       (X)
#line 969 "parser.y"

  /* This is a utility routine used to set the ExprSpan.zStart and
  ** ExprSpan.zEnd values of pOut so that the span covers the complete
  ** range of text beginning with pStart and going to the end of pEnd.
  */
  static void spanSet(ST_EXPR_SPAN *pOut, Token *pStart, Token *pEnd){
    pOut->zStart = pStart->z;
    pOut->zEnd = &pEnd->z[pEnd->n];
  }

  /* Construct a new Expr object from a single identifier.  Use the
  ** new Expr to populate pOut.  Set the span of pOut to be the identifier
  ** that created the expression.
  */
  static void spanExpr(TMdbSqlParser *pParse,ST_EXPR_SPAN *pOut, int op, Token *pValue){
    pOut->pExpr = pParse->m_tMdbExpr.BuildPExpr( op, 0, 0, pValue);
    pOut->zStart = pValue->z;
    pOut->zEnd = &pValue->z[pValue->n];
  }
#line 1045 "parser.y"

  /* This routine constructs a binary expression node out of two ExprSpan
  ** objects and uses the result to populate a new ExprSpan object.
  */
  static void spanBinaryExpr(TMdbSqlParser *pParse,
    ST_EXPR_SPAN *pOut,     /* Write the result here */
    int op,             /* The binary operation */
    ST_EXPR_SPAN *pLeft,    /* The left operand */
    ST_EXPR_SPAN *pRight    /* The right operand */
  ){
    pOut->pExpr = pParse->m_tMdbExpr.BuildPExpr( op, pLeft->pExpr, pRight->pExpr, 0);
    pOut->zStart = pLeft->zStart;
    pOut->zEnd = pRight->zEnd;
  }
#line 1090 "parser.y"

  /* Construct an expression node for a unary postfix operator
  */
  static void spanUnaryPostfix(TMdbSqlParser *pParse,
    ST_EXPR_SPAN *pOut,        /* Write the new expression node here */
    int op,                /* The operator */
    ST_EXPR_SPAN *pOperand,    /* The operand */
    Token *pPostOp         /* The operand token for setting the span */
  ){
    pOut->pExpr = pParse->m_tMdbExpr.BuildPExpr( op, pOperand->pExpr, 0, 0);
    pOut->zStart = pOperand->zStart;
    pOut->zEnd = &pPostOp->z[pPostOp->n];
  }                           
#line 1109 "parser.y"

  /* Construct an expression node for a unary prefix operator
  */
  static void spanUnaryPrefix(TMdbSqlParser *pParse,
    ST_EXPR_SPAN *pOut,        /* Write the new expression node here */
    int op,                /* The operator */
    ST_EXPR_SPAN *pOperand,    /* The operand */
    Token *pPreOp         /* The operand token for setting the span */
  ){
    pOut->pExpr = pParse->m_tMdbExpr.BuildPExpr( op, pOperand->pExpr, 0, 0);
    pOut->zStart = pPreOp->z;
    pOut->zEnd = pOperand->zEnd;
  }
#line 1125 "parser.y"

  /* A routine to convert a binary TK_IS or TK_ISNOT expression into a
  ** unary TK_ISNULL or TK_NOTNULL expression. */
  static void binaryToUnaryIfNull(TMdbSqlParser *pParse, ST_EXPR *pY, ST_EXPR *pA, int op){
    if(TK_NULL != pY->iOpcode)
    {//目前只支持is NULL,is not NULL
    	pParse->m_tError.FillErrMsg(ERR_SQL_INVALID,"expect NULL");
    }
    else
    {
	pA->iOpcode = op;
	QMDB_MALLOC->ReleaseExpr(pA->pRight);
	pA->pRight = NULL;
    }
  }
#line 108 "parser.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    mdbParserTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is mdbParserTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    mdbParserARG_SDECL     A static variable declaration for the %extra_argument
**    mdbParserARG_PDECL     A parameter declaration for the %extra_argument
**    mdbParserARG_STORE     Code to store %extra_argument into yypParser
**    mdbParserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 223
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 14
#define mdbParserTOKENTYPE Token
typedef union {
  int yyinit;
  mdbParserTOKENTYPE yy0;
  ST_SQL_STRUCT * yy89;
  ST_ID_LIST * yy101;
  ST_ID_LIST* yy157;
  ST_EXPR_SPAN yy175;
  struct LimitVal yy292;
  int yy304;
  ST_EXPR_LIST* yy331;
  ST_EXPR* yy352;
  ST_SET_LIST yy370;
  ST_EXPR * yy380;
  ST_EXPR_LIST * yy387;
  LikeOp yy393;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define mdbParserARG_SDECL TMdbSqlParser *pParse;
#define mdbParserARG_PDECL ,TMdbSqlParser *pParse
#define mdbParserARG_FETCH TMdbSqlParser *pParse = yypParser->pParse
#define mdbParserARG_STORE yypParser->pParse = pParse
#define YYNSTATE 577
#define YYNRULE 279
#define YYFALLBACK 1
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
#define YY_ACTTAB_COUNT (1190)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   568,   32,   32,   31,   31,   31,   30,  314,  311,  182,
 /*    10 */    36,   37,  244,   28,  542,  548,  540,  539,  541,  541,
 /*    20 */    34,   34,   35,   35,   35,   35,  468,   33,   33,   33,
 /*    30 */    33,   32,   32,   31,   31,   31,   30,  309,  305,  180,
 /*    40 */   568,   85,  546,  547,  544,  543,  356,   17,  571,  577,
 /*    50 */   325,   36,   37,  244,   28,  542,  548,  540,  539,  541,
 /*    60 */   541,   34,   34,   35,   35,   35,   35,  124,   33,   33,
 /*    70 */    33,   33,   32,   32,   31,   31,   31,   30,  545,  356,
 /*    80 */   189,  324,  156,  164,  534,  355,  160,   16,   36,   37,
 /*    90 */   244,   28,  542,  548,  540,  539,  541,  541,   34,   34,
 /*   100 */    35,   35,   35,   35,   96,   33,   33,   33,   33,   32,
 /*   110 */    32,   31,   31,   31,   30,  568,  164,  534,  355,  571,
 /*   120 */   471,  409,  470,  555,   36,   37,  244,   28,  542,  548,
 /*   130 */   540,  539,  541,  541,   34,   34,   35,   35,   35,   35,
 /*   140 */   572,   33,   33,   33,   33,   32,   32,   31,   31,   31,
 /*   150 */    30,   31,   31,   31,   30,  568,   29,   27,  361,  535,
 /*   160 */    36,   37,  244,   28,  542,  548,  540,  539,  541,  541,
 /*   170 */    34,   34,   35,   35,   35,   35,  353,   33,   33,   33,
 /*   180 */    33,   32,   32,   31,   31,   31,   30,  240,  155,  530,
 /*   190 */   425,   81,  350,  525,  154,  415,   36,   26,  244,   28,
 /*   200 */   542,  548,  540,  539,  541,  541,   34,   34,   35,   35,
 /*   210 */    35,   35,  192,   33,   33,   33,   33,   32,   32,   31,
 /*   220 */    31,   31,   30,   36,   37,  244,   28,  542,  548,  540,
 /*   230 */   539,  541,  541,   34,   34,   35,   35,   35,   35,  173,
 /*   240 */    33,   33,   33,   33,   32,   32,   31,   31,   31,   30,
 /*   250 */    37,  244,   28,  542,  548,  540,  539,  541,  541,   34,
 /*   260 */    34,   35,   35,   35,   35,  565,   33,   33,   33,   33,
 /*   270 */    32,   32,   31,   31,   31,   30,  244,   28,  542,  548,
 /*   280 */   540,  539,  541,  541,   34,   34,   35,   35,   35,   35,
 /*   290 */   269,   33,   33,   33,   33,   32,   32,   31,   31,   31,
 /*   300 */    30,  571,  259,  501,  857,  374,  235,  574,   10,   35,
 /*   310 */    35,   35,   35,  234,   33,   33,   33,   33,   32,   32,
 /*   320 */    31,   31,   31,   30,  262,  267,  571,  564,  318,  227,
 /*   330 */   504,   33,   33,   33,   33,   32,   32,   31,   31,   31,
 /*   340 */    30,  226,  558,  571,  571,  523,  528,  571,  571,  277,
 /*   350 */   273,  233,  557,  232,  520,   94,  200,  150,  154,  199,
 /*   360 */   225,  224,  171,  264,  170,  231,  198,  221,  179,  223,
 /*   370 */   158,    1,   83,  568,  157,    3,   48,  506,   47,  456,
 /*   380 */   209,  212,  216,  337,  230,  229,  197,  371,  571,  196,
 /*   390 */   273,  195,  291,  228,  194,  572,  372,  193,  220,  219,
 /*   400 */    41,  364,  146,  362,  367,  366,   40,  218,  566,  503,
 /*   410 */   137,  583,  365,  568,  572,  488,  129,   39,   57,  549,
 /*   420 */   559,  486,   19,  575,   24,  584,  243,  253,  114,  162,
 /*   430 */   422,  290,  402,  291,  572,  570,  250,  242,  340,  391,
 /*   440 */   286,  241,  274,   21,   22,  464,  322,  186,  572,  385,
 /*   450 */    23,  365,  363,  571,  570,  277,   38,  258,  549,  551,
 /*   460 */   315,  107,  554,   24,  571,   25,  127,  463,  465,  386,
 /*   470 */   162,  422,  290,  279,  570,  284,  249,  270,  371,  391,
 /*   480 */   110,  286,   21,   22,  357,  533,  383,  330,  217,   23,
 /*   490 */   365,  363,  552,  552,  108,   38,  155,  549,  551,  569,
 /*   500 */   350,  554,   24,  571,   86,  523,  556,  485,  128,  329,
 /*   510 */   467,  551,  551,  550,  202,  201,  284,  365,  571,  538,
 /*   520 */   127,   21,   22,  263,  549,  370,   15,  528,   23,   20,
 /*   530 */   363,  552,  552,  252,   38,  563,   94,  551,  402,  364,
 /*   540 */   554,  362,  214,  106,  521,  571,  191,  277,   21,   22,
 /*   550 */   551,  551,  550,  376,  568,   23,   66,  363,  352,  572,
 /*   560 */   364,   38,  362,  364,  551,  362,  568,  554,  457,   84,
 /*   570 */   364,  364,  362,  362,   29,   27,  361,   66,  387,  237,
 /*   580 */    57,  384,  537,  360,  536,  288,  287,   63,   66,  551,
 /*   590 */   551,  550,  339,  571,  568,  318,  261,  247,   11,  570,
 /*   600 */   297,  418,  245,  571,  360,  318,  568,  155,  571,  187,
 /*   610 */   528,  350,  571,  369,  165,  320,  551,  551,  550,   82,
 /*   620 */   571,  497,  526,  135,  351,  251,  500,  519,  499,  134,
 /*   630 */   402,  246,  349,  494,  136,  493,  353,  571,  364,  127,
 /*   640 */   362,  133,  532,  257,  556,  359,  453,  239,  571,  530,
 /*   650 */   325,  382,  236,  256,  377,   54,  453,  364,  568,  362,
 /*   660 */    93,  327,  364,  434,  362,  556,  358,  364,  556,  362,
 /*   670 */   364,  529,  362,  346,   55,  556,  556,  426,  364,   70,
 /*   680 */   362,  326,   92,  364,   68,  362,  364,   76,  362,  364,
 /*   690 */   364,  362,  362,  568,  364,   75,  362,  568,  568,  364,
 /*   700 */    77,  362,  364,   78,  362,  316,   95,  100,  364,  294,
 /*   710 */   362,   74,  451,   84,  568,  571,   73,  483,  364,   67,
 /*   720 */   362,  364,  364,  362,  362,   72,  313,  184,  364,  364,
 /*   730 */   362,  362,  364,  568,  362,   65,  348,  568,   69,   99,
 /*   740 */   296,  348,  260,  556,  293,   98,   97,  282,  151,   71,
 /*   750 */   571,  364,  277,  362,  568,  527,  364,  364,  362,  362,
 /*   760 */   522,  275,  556,  517,  364,  568,  362,  556,   64,  364,
 /*   770 */   512,  362,  556,   52,   62,  556,  429,  571,  143,  430,
 /*   780 */   364,   61,  362,  556,  222,  364,   60,  362,  556,  338,
 /*   790 */   571,  556,  127,   91,  556,  556,   90,   59,  364,  556,
 /*   800 */   362,  364,   58,  362,  556,  568,    8,  556,  364,  571,
 /*   810 */   319,  273,  271,  556,  207,   56,  121,  120,   53,  238,
 /*   820 */   571,  411,  524,  556,  331,    5,  556,  556,  406,   80,
 /*   830 */   399,   80,   51,  556,  556,  405,  335,  556,  397,   80,
 /*   840 */   462,  461,  431,  395,   79,  389,   79,  301,  298,   88,
 /*   850 */   190,  476,  571,  571,  518,  345,  556,  571,  571,  344,
 /*   860 */   343,  556,  556,  571,  571,  516,  515,  480,  571,  556,
 /*   870 */   553,  571,  126,  514,  556,  571,  479,  513,  571,  571,
 /*   880 */   511,  341,  381,  380,  571,  556,  510,  571,  477,  509,
 /*   890 */   556,  424,  412,  571,  571,  508,  507,  159,   14,  571,
 /*   900 */   394,  505,  125,  556,  562,  531,  556,   13,  571,    2,
 /*   910 */   248,    7,  571,  556,  502,  571,  571,  498,  496,    6,
 /*   920 */   571,  571,  495,  492,  571,  571,  491,  490,  571,  571,
 /*   930 */   489,  487,  571,  571,  484,  482,  571,  104,  328,  571,
 /*   940 */   571,  474,  466,  571,  571,  460,  459,  571,  571,  450,
 /*   950 */   449,  571,  571,  303,  438,  571,  571,  300,  436,  571,
 /*   960 */   571,  435,  163,  571,  571,  408,  388,  571,  571,  265,
 /*   970 */   375,  172,  255,  455,  454,  185,  183,  254,  312,  310,
 /*   980 */   308,  307,  306,  181,  441,  119,  118,  302,  117,  299,
 /*   990 */   116,  115,  169,  421,  433,  432,  168,  401,  428,  427,
 /*  1000 */   178,  278,  420,  109,  177,  417,  416,  414,  176,  105,
 /*  1010 */   413,  281,  167,  280,  410,  113,  404,  112,  403,  166,
 /*  1020 */   111,  393,  392,  175,  576,  379,  174,  101,  103,  102,
 /*  1030 */   378,  575,  573,    4,    9,  373,  347,  342,   89,  332,
 /*  1040 */   215,  336,  333,  205,  153,  439,  152,  213,  149,  148,
 /*  1050 */   147,  334,  145,  144,  481,  142,  141,  140,  139,   15,
 /*  1060 */    87,  138,   50,   49,  407,   18,  132,  321,  204,   12,
 /*  1070 */   458,  323,  131,  400,  130,   46,   45,   44,  398,  396,
 /*  1080 */   452,   43,   42,  572,   30,  390,  858,  423,  304,  858,
 /*  1090 */   437,  858,  858,  858,  858,  858,  858,  858,  858,  329,
 /*  1100 */   858,  858,  858,  858,  858,  211,  858,  858,  858,  858,
 /*  1110 */   858,  858,  475,  123,  122,  567,  208,  354,  858,  858,
 /*  1120 */   858,  447,  446,  445,  444,  442,  858,  858,  440,  289,
 /*  1130 */   285,  188,  283,  206,  268,  266,  858,  858,  368,  473,
 /*  1140 */   858,  858,  317,  203,  295,  210,  858,  292,  276,  272,
 /*  1150 */   560,  419,  161,  858,  478,  858,  858,  858,  561,  858,
 /*  1160 */   858,  858,  858,  858,  858,  472,  469,  858,  858,  858,
 /*  1170 */   858,  858,  858,  858,  858,  858,  858,  858,  858,  858,
 /*  1180 */   858,  858,  858,  858,  858,  448,  858,  858,  858,  443,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     5,   36,   37,   38,   39,   40,   41,   85,   86,   87,
 /*    10 */    15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
 /*    20 */    25,   26,   27,   28,   29,   30,   51,   32,   33,   34,
 /*    30 */    35,   36,   37,   38,   39,   40,   41,   85,   86,   87,
 /*    40 */    45,   66,   19,   20,   21,   22,   17,   11,  112,    0,
 /*    50 */   114,   15,   16,   17,   18,   19,   20,   21,   22,   23,
 /*    60 */    24,   25,   26,   27,   28,   29,   30,   72,   32,   33,
 /*    70 */    34,   35,   36,   37,   38,   39,   40,   41,   55,   17,
 /*    80 */   144,  145,   53,   54,   55,   56,   51,   51,   15,   16,
 /*    90 */    17,   18,   19,   20,   21,   22,   23,   24,   25,   26,
 /*   100 */    27,   28,   29,   30,   50,   32,   33,   34,   35,   36,
 /*   110 */    37,   38,   39,   40,   41,    5,   54,   55,   56,  112,
 /*   120 */     6,  114,    8,   50,   15,   16,   17,   18,   19,   20,
 /*   130 */    21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
 /*   140 */     5,   32,   33,   34,   35,   36,   37,   38,   39,   40,
 /*   150 */    41,   38,   39,   40,   41,   45,  219,  220,  221,   50,
 /*   160 */    15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
 /*   170 */    25,   26,   27,   28,   29,   30,  113,   32,   33,   34,
 /*   180 */    35,   36,   37,   38,   39,   40,   41,  124,   53,  126,
 /*   190 */   183,  184,   57,   50,   51,   50,   15,   16,   17,   18,
 /*   200 */    19,   20,   21,   22,   23,   24,   25,   26,   27,   28,
 /*   210 */    29,   30,   26,   32,   33,   34,   35,   36,   37,   38,
 /*   220 */    39,   40,   41,   15,   16,   17,   18,   19,   20,   21,
 /*   230 */    22,   23,   24,   25,   26,   27,   28,   29,   30,   49,
 /*   240 */    32,   33,   34,   35,   36,   37,   38,   39,   40,   41,
 /*   250 */    16,   17,   18,   19,   20,   21,   22,   23,   24,   25,
 /*   260 */    26,   27,   28,   29,   30,   50,   32,   33,   34,   35,
 /*   270 */    36,   37,   38,   39,   40,   41,   17,   18,   19,   20,
 /*   280 */    21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
 /*   290 */    65,   32,   33,   34,   35,   36,   37,   38,   39,   40,
 /*   300 */    41,  112,  148,  114,  108,  109,  110,  111,  154,   27,
 /*   310 */    28,   29,   30,  117,   32,   33,   34,   35,   36,   37,
 /*   320 */    38,   39,   40,   41,  135,  100,  112,   50,  114,   48,
 /*   330 */   134,   32,   33,   34,   35,   36,   37,   38,   39,   40,
 /*   340 */    41,   60,   52,  112,  112,  114,  114,  112,  112,  114,
 /*   350 */   114,  119,   52,   48,  122,  123,  160,   50,   51,  163,
 /*   360 */    79,   80,  166,  132,  168,   60,  170,   48,  172,   88,
 /*   370 */   174,   49,  176,    5,  178,   49,  180,   96,  182,  165,
 /*   380 */     1,    2,    3,    4,   79,   80,  190,  112,  112,  193,
 /*   390 */   114,  195,   17,   88,  198,    5,  121,  201,   79,   80,
 /*   400 */   204,  112,   97,  114,   36,   37,  210,   88,  133,   50,
 /*   410 */    51,   68,    5,   45,    5,   96,   97,   49,  129,   12,
 /*   420 */    52,   50,   51,   44,   17,   68,   47,  192,   53,   54,
 /*   430 */    55,   56,  197,   17,    5,   45,  200,   58,   59,  203,
 /*   440 */    65,   62,   63,   36,   37,   38,  157,  158,    5,  189,
 /*   450 */    43,    5,   45,  112,   45,  114,   49,   78,   12,   52,
 /*   460 */    81,   68,   55,   17,  112,   51,  114,   38,  113,  209,
 /*   470 */    54,   55,   56,   94,   45,  100,  200,   98,  112,  203,
 /*   480 */   101,   65,   36,   37,   38,   55,  113,   38,  136,   43,
 /*   490 */     5,   45,  102,  103,   51,   49,   53,   12,   52,  133,
 /*   500 */    57,   55,   17,  112,   51,  114,  217,   50,   51,   60,
 /*   510 */   155,  104,  105,  106,   79,   80,  100,    5,  112,   50,
 /*   520 */   114,   36,   37,  132,   12,  112,   73,  114,   43,   17,
 /*   530 */    45,  102,  103,  192,   49,  122,  123,   52,  197,  112,
 /*   540 */    55,  114,  136,  130,  131,  112,   49,  114,   36,   37,
 /*   550 */   104,  105,  106,  113,    5,   43,  129,   45,   49,    5,
 /*   560 */   112,   49,  114,  112,   52,  114,    5,   55,   50,   51,
 /*   570 */   112,  112,  114,  114,  219,  220,  221,  129,  205,  206,
 /*   580 */   129,  208,   50,  156,   50,   36,   37,  129,  129,  104,
 /*   590 */   105,  106,   57,  112,   45,  114,  138,   58,   49,   45,
 /*   600 */   112,   52,   63,  112,  156,  114,   45,   53,  112,  158,
 /*   610 */   114,   57,  112,   52,  114,  156,  104,  105,  106,  123,
 /*   620 */   112,   91,  114,   93,   52,  192,   91,   46,   93,   99,
 /*   630 */   197,   92,   10,   91,   99,   93,  113,  112,  112,  114,
 /*   640 */   114,   99,   50,  162,  217,  218,  165,  124,  112,  126,
 /*   650 */   114,  211,  212,  162,  214,  129,  165,  112,    5,  114,
 /*   660 */    49,  136,  112,  175,  114,  217,  218,  112,  217,  114,
 /*   670 */   112,   50,  114,   17,  129,  217,  217,  181,  112,  129,
 /*   680 */   114,  145,   49,  112,  129,  114,  112,  129,  114,  112,
 /*   690 */   112,  114,  114,    5,  112,  129,  114,    5,   45,  112,
 /*   700 */   129,  114,  112,  129,  114,   52,  129,  129,  112,  112,
 /*   710 */   114,  129,   50,   51,    5,  112,  129,  114,  112,  129,
 /*   720 */   114,  112,  112,  114,  114,  129,   86,   87,  112,  112,
 /*   730 */   114,  114,  112,   45,  114,  129,  112,   45,  129,  129,
 /*   740 */    52,  112,  139,  217,   52,  129,  129,  113,   61,  129,
 /*   750 */   112,  112,  114,  114,   45,  131,  112,  112,  114,  114,
 /*   760 */   131,   52,  217,   52,  112,    5,  114,  217,  129,  112,
 /*   770 */    46,  114,  217,  129,  129,  217,  179,  112,   61,  114,
 /*   780 */   112,  129,  114,  217,   48,  112,  129,  114,  217,   10,
 /*   790 */   112,  217,  114,   49,  217,  217,   64,  129,  112,  217,
 /*   800 */   114,  112,  129,  114,  217,   45,   49,  217,  112,  112,
 /*   810 */   114,  114,   52,  217,  136,  129,   83,   84,  129,  185,
 /*   820 */   112,  187,  114,  217,   38,  129,  217,  217,   50,   51,
 /*   830 */    50,   51,   67,  217,  217,  197,   65,  217,   50,   51,
 /*   840 */    70,   71,  177,   50,   51,   50,   51,   89,   90,   66,
 /*   850 */   152,  153,  112,  112,  114,  114,  217,  112,  112,  114,
 /*   860 */   114,  217,  217,  112,  112,  114,  114,   52,  112,  217,
 /*   870 */   114,  112,   49,  114,  217,  112,   39,  114,  112,  112,
 /*   880 */   114,  114,  215,  216,  112,  217,  114,  112,   50,  114,
 /*   890 */   217,  188,  189,  112,  112,  114,  114,   36,   26,  112,
 /*   900 */   203,  114,   68,  217,  127,  128,  217,   26,  112,    7,
 /*   910 */   114,   51,  112,  217,  114,  112,  112,  114,  114,    7,
 /*   920 */   112,  112,  114,  114,  112,  112,  114,  114,  112,  112,
 /*   930 */   114,  114,  112,  112,  114,  114,  112,   68,  114,  112,
 /*   940 */   112,  114,  114,  112,  112,  114,  114,  112,  112,  114,
 /*   950 */   114,  112,  112,  114,  114,  112,  112,  114,  114,  112,
 /*   960 */   112,  114,  114,  112,  112,  114,  114,  112,  112,  114,
 /*   970 */   114,   49,   82,   50,   50,   87,   87,   52,   52,   52,
 /*   980 */    52,   86,   52,   87,   52,    7,    7,   90,    7,   90,
 /*   990 */     7,    7,   49,   55,   50,   50,   49,   96,   50,   50,
 /*  1000 */    49,   95,   50,   82,   49,   52,   52,   50,   49,  125,
 /*  1010 */    50,   49,   49,   52,   50,   48,   50,   97,   50,   49,
 /*  1020 */   213,   50,   50,   49,  111,   50,   49,  186,  125,  207,
 /*  1030 */    50,   44,  118,   51,   49,  120,   13,   13,   49,   39,
 /*  1040 */   140,  137,   69,  149,  115,  171,  115,  142,  115,  115,
 /*  1050 */   115,  141,  116,  116,  143,  116,  116,  116,  116,   73,
 /*  1060 */   141,  115,   49,   49,  191,   77,  116,   74,  150,   75,
 /*  1070 */   161,   76,  116,  194,  116,   49,   49,   49,  196,  199,
 /*  1080 */   164,   49,   49,    5,   41,  202,  222,  189,   89,  222,
 /*  1090 */   173,  222,  222,  222,  222,  222,  222,  222,  222,   60,
 /*  1100 */   222,  222,  222,  222,  222,  140,  222,  222,  222,  222,
 /*  1110 */   222,  222,  143,  116,  116,  113,  140,  113,  222,  222,
 /*  1120 */   222,  113,  113,  113,  113,  113,  222,  222,  113,  113,
 /*  1130 */   113,  141,  113,  142,  113,  113,  222,  222,  113,  143,
 /*  1140 */   222,  222,  113,  151,  113,  142,  222,  113,  113,  113,
 /*  1150 */   113,  113,  147,  222,  153,  222,  222,  222,  128,  222,
 /*  1160 */   222,  222,  222,  222,  222,  159,  159,  222,  222,  222,
 /*  1170 */   222,  222,  222,  222,  222,  222,  222,  222,  222,  222,
 /*  1180 */   222,  222,  222,  222,  222,  167,  222,  222,  222,  169,
};
#define YY_SHIFT_USE_DFLT (-79)
#define YY_SHIFT_COUNT (374)
#define YY_SHIFT_MIN   (-78)
#define YY_SHIFT_MAX   (1078)
static const short yy_shift_ofst[] = {
 /*     0 */   379,  446,  485,  485,  554,   -5,  485,  485,  485,  409,
 /*    10 */   407,  485,  485,  485,  485,  485,  485,  485,  485,  485,
 /*    20 */   485,  485,  485,  485,  485,  485,  485,  485,  512,  485,
 /*    30 */   485,  485,  485,  485,  485,  485,  485,  485,  485,  485,
 /*    40 */   110,  110,  409,  409,  409,  409,  409,  409,  409,  409,
 /*    50 */   409,  409,   36,  145,  109,   73,  208,  208,  208,  208,
 /*    60 */   208,  208,  208,  208,  208,  208,  208,  181,  234,  259,
 /*    70 */   259,  282,  282,  282,  282,  282,  299,  -35,  113,  409,
 /*    80 */   409,  110,  110,  409,  409,  409,  409,  409,  409,  409,
 /*    90 */   409,  409,  409,  409,  110, 1043, 1078,  -79,  -79,  -79,
 /*   100 */   -79,  375,  416,   29,  429,   29,  443,  390,  135,  409,
 /*   110 */   409,  225,  409,  409,  409,  409,  409,  409,  409,  409,
 /*   120 */   409,  409,  409,  409,  409,  409,  409,  409,  409,  409,
 /*   130 */   409,  409,  409,  409,  409,  409,  409,  409,  409,  409,
 /*   140 */   409,  409,  409,  409,  409,  409,  409,  409,  409,  409,
 /*   150 */   409,  409,  409,  409,  409,  409,  409, 1078, 1078, 1039,
 /*   160 */  1078,  -79,  549,  416,  368,   62,  760,  709,  692,  688,
 /*   170 */   -48,  -78,  653,  561,  110,  110,  110,  110,  110,  758,
 /*   180 */   110,  110,  110,  110,  110,  110,  114,  114,  770,  453,
 /*   190 */   449,  110,  110, 1033, 1032, 1028, 1027, 1026,  999, 1014,
 /*   200 */  1013, 1024, 1024,  988,  995,  994,  993,  986,  973, 1000,
 /*   210 */   988,  973, 1000,  988,  986,  973, 1000,  989, 1024, 1024,
 /*   220 */  1024, 1023, 1024, 1024, 1024, 1024, 1024, 1024, 1023, 1023,
 /*   230 */  1023, 1023, 1023,  982,  985,  987,  -79,  -79,  -79,  -79,
 /*   240 */   -79,  319,  281,  305,   23,  535,  542,  530,  539,  795,
 /*   250 */   793,  788,  780,  778,  640,  733,  662,  518,  435,  -25,
 /*   260 */   457,  371,  359,  307,  143,  921,  980,  977,  975,  974,
 /*   270 */   920,  972,  971,  970,  901,  968,  966,  963,  967,  906,
 /*   280 */   964,  961,  962,  960,  959,  957,  955,  954,  953,  952,
 /*   290 */   951,  938,  949,  948,  947,  945,  944,  943,  984,  983,
 /*   300 */   899,  981,  979,  897,  978,  932,  896,  930,  895,  928,
 /*   310 */   889,  927,  888,  926,  925,  890,  924,  923,  922,  869,
 /*   320 */   414,  912,  860,  902,  881,  834,  872,  765,  838,  823,
 /*   330 */   837,  861,  786,  815,  783,  757,  771,  732,  744,  779,
 /*   340 */   736,  717,  724,  711,  633,  687,  581,  656,  190,  611,
 /*   350 */   622,  621,  572,  509,  592,  497,  430,  534,  532,  469,
 /*   360 */   414,  326,  393,  357,  343,  322,  300,  290,  277,  215,
 /*   370 */   190,  186,   35,   54,   49,
};
#define YY_REDUCE_USE_DFLT (-65)
#define YY_REDUCE_COUNT (240)
#define YY_REDUCE_MIN   (-64)
#define YY_REDUCE_MAX   (1038)
static const short yy_reduce_ofst[] = {
 /*     0 */   196,  448,  289,  427,  413,  355,  459,  451,  458,  232,
 /*    10 */   696,  689,  686,  673,  668,  657,  652,  645,  644,  639,
 /*    20 */   620,  617,  616,  610,  609,  606,  596,  590,  587,  582,
 /*    30 */   578,  577,  574,  571,  566,  558,  555,  550,  545,  526,
 /*    40 */   440,  373,  276,  236,  433,  341,  235,    7,  496,  491,
 /*    50 */   481,  -64,  -63,  -63,  -63,  -63,  -63,  -63,  -63,  -63,
 /*    60 */   -63,  -63,  -63,  -63,  -63,  -63,  -63,  -63,  -63,  -63,
 /*    70 */   -63,  -63,  -63,  -63,  -63,  -63,  -63,  -63,  -63,  697,
 /*    80 */   638,  634,  523,  665,  214,  678,  536,  525,  406,  603,
 /*    90 */   352,  189,  391,  231,   63,  -63,  275,  -63,  -63,  -63,
 /*   100 */   -63,  703,  260,  777,  756,  777,  629,  756,  624,  856,
 /*   110 */   855,  667,  852,  851,  848,  847,  844,  843,  840,  839,
 /*   120 */   836,  835,  832,  831,  828,  827,  824,  821,  820,  817,
 /*   130 */   816,  813,  812,  809,  808,  804,  803,  800,  796,  787,
 /*   140 */   782,  781,  775,  772,  767,  766,  763,  759,  752,  751,
 /*   150 */   746,  745,  741,  740,  708,  508,  500,  597,  488,  698,
 /*   160 */   366,  154, 1038,  898, 1037, 1030, 1036, 1035, 1034, 1031,
 /*   170 */  1020, 1018, 1029, 1025, 1022, 1021, 1019, 1017, 1016,  917,
 /*   180 */  1015, 1012, 1011, 1010, 1009, 1008, 1007, 1006, 1005, 1003,
 /*   190 */  1001, 1004, 1002,  883,  880,  882,  879,  873,  874,  916,
 /*   200 */   909,  998,  997,  996,  992,  918,  894,  991,  990,  976,
 /*   210 */   969,  919,  965,  911,  905,  910,  900,  904,  958,  956,
 /*   220 */   950,  946,  942,  941,  940,  939,  937,  936,  935,  934,
 /*   230 */   933,  931,  929,  915,  914,  913,  807,  822,  841,  903,
 /*   240 */   884,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   856,  843,  856,  843,  856,  668,  856,  856,  856,  856,
 /*    10 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*    20 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*    30 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*    40 */   803,  792,  856,  856,  856,  856,  856,  856,  856,  856,
 /*    50 */   856,  856,  684,  856,  856,  856,  674,  679,  645,  644,
 /*    60 */   670,  686,  685,  638,  637,  844,  845,  856,  826,  846,
 /*    70 */   825,  841,  852,  840,  837,  828,  827,  829,  830,  856,
 /*    80 */   856,  748,  596,  856,  856,  856,  856,  856,  856,  856,
 /*    90 */   856,  856,  856,  856,  596,  831,  856,  849,  848,  847,
 /*   100 */   832,  746,  791,  742,  856,  594,  613,  856,  856,  856,
 /*   110 */   856,  802,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   120 */   856,  856,  856,  856,  856,  856,  856,  664,  856,  856,
 /*   130 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   140 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   150 */   629,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   160 */   856,  660,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   170 */   856,  711,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   180 */   856,  856,  856,  856,  856,  856,  682,  682,  658,  669,
 /*   190 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   200 */   856,  588,  588,  683,  675,  673,  671,  669,  655,  650,
 /*   210 */   683,  655,  650,  683,  669,  655,  650,  856,  588,  588,
 /*   220 */   588,  585,  588,  588,  588,  588,  588,  588,  585,  585,
 /*   230 */   585,  585,  585,  612,  856,  856,  807,  796,  753,  601,
 /*   240 */   601,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   250 */   856,  856,  856,  856,  707,  856,  856,  856,  856,  856,
 /*   260 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   270 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   280 */   856,  856,  750,  856,  856,  856,  856,  856,  856,  856,
 /*   290 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   300 */   726,  856,  856,  722,  856,  856,  856,  856,  715,  856,
 /*   310 */   708,  856,  705,  856,  856,  856,  856,  856,  856,  856,
 /*   320 */   672,  856,  676,  856,  856,  646,  856,  856,  856,  856,
 /*   330 */   856,  856,  856,  856,  856,  856,  856,  856,  856,  856,
 /*   340 */   856,  856,  856,  628,  856,  856,  856,  856,  856,  856,
 /*   350 */   856,  856,  856,  598,  856,  856,  856,  856,  856,  856,
 /*   360 */   842,  856,  856,  820,  816,  581,  856,  856,  856,  856,
 /*   370 */   583,  856,  591,  856,  856,  812,  805,  804,  810,  809,
 /*   380 */   808,  806,  800,  794,  793,  797,  795,  789,  784,  783,
 /*   390 */   781,  786,  788,  787,  785,  780,  778,  773,  771,  770,
 /*   400 */   768,  766,  775,  777,  776,  774,  767,  765,  764,  747,
 /*   410 */   751,  749,  755,  799,  798,  763,  762,  761,  760,  759,
 /*   420 */   758,  757,  756,  754,  752,  744,  740,  739,  738,  736,
 /*   430 */   735,  733,  732,  731,  729,  727,  725,  723,  721,  719,
 /*   440 */   717,  716,  714,  712,  710,  709,  706,  704,  702,  701,
 /*   450 */   700,  694,  692,  696,  698,  697,  695,  691,  689,  688,
 /*   460 */   687,  657,  656,  663,  662,  667,  666,  661,  659,  678,
 /*   470 */   681,  680,  677,  648,  647,  643,  651,  653,  652,  649,
 /*   480 */   654,  642,  665,  641,  640,  639,  636,  782,  769,  724,
 /*   490 */   713,  693,  801,  745,  737,  811,  743,  734,  790,  741,
 /*   500 */   730,  635,  634,  633,  626,  625,  772,  728,  718,  699,
 /*   510 */   632,  624,  587,  779,  720,  703,  690,  627,  590,  586,
 /*   520 */   593,  616,  615,  631,  630,  618,  617,  614,  595,  599,
 /*   530 */   597,  603,  611,  610,  609,  608,  823,  822,  855,  853,
 /*   540 */   850,  838,  835,  854,  851,  839,  836,  834,  833,  824,
 /*   550 */   821,  819,  818,  817,  815,  814,  813,  607,  606,  605,
 /*   560 */   604,  602,  600,  592,  620,  619,  622,  623,  582,  621,
 /*   570 */   584,  583,  581,  589,  579,  580,  578,
};

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    0,  /*     SELECT => nothing */
    1,  /*     UPDATE => SELECT */
    1,  /*     DELETE => SELECT */
    1,  /*     INSERT => SELECT */
    1,  /*         ID => SELECT */
    1,  /*        ASC => SELECT */
    1,  /*         BY => SELECT */
    1,  /*       DESC => SELECT */
    1,  /*         NO => SELECT */
    1,  /*        KEY => SELECT */
    1,  /*     OFFSET => SELECT */
    1,  /*    SYSDATE => SELECT */
    1,  /*         IF => SELECT */
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  mdbParserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void mdbParserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "SELECT",        "UPDATE",        "DELETE",      
  "INSERT",        "ID",            "ASC",           "BY",          
  "DESC",          "NO",            "KEY",           "OFFSET",      
  "SYSDATE",       "IF",            "ANY",           "OR",          
  "AND",           "NOT",           "IS",            "MATCH",       
  "LIKE_KW",       "BETWEEN",       "IN",            "ISNULL",      
  "NOTNULL",       "NE",            "EQ",            "GT",          
  "LE",            "LT",            "GE",            "ESCAPE",      
  "BITAND",        "BITOR",         "LSHIFT",        "RSHIFT",      
  "PLUS",          "MINUS",         "STAR",          "SLASH",       
  "REM",           "CONCAT",        "COLLATE",       "BITNOT",      
  "SEMI",          "STRING",        "EXISTS",        "CREATE",      
  "TABLE",         "LP",            "RP",            "COMMA",       
  "INTEGER",       "CONSTRAINT",    "DEFAULT",       "NULL",        
  "REPTYPE",       "PRIMARY",       "DROP",          "TRUNCATE",    
  "INDEX",         "ON",            "ALTER",         "ADD",         
  "INTO",          "VALUES",        "FROM",          "SET",         
  "DOT",           "FIRST",         "DISTINCT",      "ALL",         
  "AS",            "WHERE",         "GROUP",         "HAVING",      
  "ORDER",         "LIMIT",         "USE",           "DATABASE",    
  "TABLESPACE",    "CONNECT",       "TO",            "ORACLE",      
  "MYSQL",         "PAGESIZE",      "ASKPAGE",       "STORAGE",     
  "USER",          "IDENTIFIED",    "ACCESSED",      "TABLESYS",    
  "MODIFY",        "COLUMN",        "LOAD",          "DATA",        
  "SEQUENCE",      "JOB",           "REMOVE",        "PARAMETER",   
  "PARAMTYPE",     "RENAME",        "NEXTVAL",       "CURRVAL",     
  "FLOAT",         "BLOB",          "VARIABLE",      "error",       
  "input",         "cmdlist",       "cmd",           "ecmd",        
  "id",            "ids",           "nm",            "ifnotexists", 
  "ifexists",      "create_table",  "create_table_args",  "columnlist",  
  "conslist_opt",  "propertylist",  "column",        "columnid",    
  "type",          "carglist",      "typetoken",     "carg",        
  "ccons",         "expr",          "conslist",      "tcons",       
  "idxlist",       "tableproperty",  "create_index",  "pkidxlist",   
  "tablename",     "inscollist_opt",  "itemlist",      "inscollist",  
  "hint",          "first",         "where_opt",     "limit_opt",   
  "setlist",       "setnm",         "select",        "distinct",    
  "selcollist",    "groupby_opt",   "having_opt",    "orderby_opt", 
  "hintcollist",   "hintcolumn",    "sclp",          "as",          
  "nexprlist",     "sortlist",      "sortitem",      "sortorder",   
  "create_database",  "create_database_args",  "syscolumnlist",  "alter_database",
  "alter_database_args",  "syscolumn",     "create_tablespace",  "page_opt",    
  "alter_tablespace",  "alter_page_opt",  "create_user",   "access_opt",  
  "alter_user",    "alter_access_opt",  "alter_table_addAttr",  "add_attr",    
  "alter_table_dropAttr",  "drop_attr",     "alter_table_modifAttr",  "modifyattr",  
  "alter_table_addcolum",  "table_column",  "alter_table_modifycolum",  "table_mcolumn",
  "modifycolumid",  "ctype",         "carglists",     "tokentype",   
  "cargs",         "cons",          "add_seq",       "add_seq_args",
  "seqcolumnlist",  "alter_seq",     "alter_seq_args",  "del_seq",     
  "del_seq_args",  "seqcolumn",     "add_job",       "add_job_args",
  "jobcolumnlist",  "alter_job",     "alter_job_args",  "jobcolumn",   
  "alter_flushsql_addparam",  "flushsql_param",  "data_type",     "parglists",   
  "atokentype",    "pargs",         "alter_flushsql_modifyparam",  "flushsql_mparam",
  "mdata_type",    "mparglists",    "mtokentype",    "mpargs",      
  "mcons",         "term",          "exprlist",      "likeop",      
  "between_op",    "in_op",       
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "input ::= cmdlist",
 /*   1 */ "cmdlist ::= cmd ecmd",
 /*   2 */ "cmdlist ::= ecmd",
 /*   3 */ "ecmd ::= SEMI",
 /*   4 */ "id ::= ID",
 /*   5 */ "ids ::= ID|STRING",
 /*   6 */ "nm ::= id",
 /*   7 */ "nm ::= STRING",
 /*   8 */ "ifnotexists ::=",
 /*   9 */ "ifnotexists ::= IF NOT EXISTS",
 /*  10 */ "ifexists ::= IF EXISTS",
 /*  11 */ "ifexists ::=",
 /*  12 */ "cmd ::= create_table create_table_args",
 /*  13 */ "create_table ::= CREATE TABLE ifnotexists nm",
 /*  14 */ "create_table_args ::= LP columnlist conslist_opt RP propertylist",
 /*  15 */ "columnlist ::= columnlist COMMA column",
 /*  16 */ "columnlist ::= column",
 /*  17 */ "column ::= columnid type carglist",
 /*  18 */ "columnid ::= nm",
 /*  19 */ "type ::=",
 /*  20 */ "type ::= typetoken",
 /*  21 */ "typetoken ::= ids",
 /*  22 */ "typetoken ::= ids LP INTEGER RP",
 /*  23 */ "carglist ::= carglist carg",
 /*  24 */ "carglist ::=",
 /*  25 */ "carg ::= CONSTRAINT nm ccons",
 /*  26 */ "carg ::= ccons",
 /*  27 */ "ccons ::= DEFAULT ids",
 /*  28 */ "ccons ::= DEFAULT INTEGER",
 /*  29 */ "ccons ::= DEFAULT PLUS INTEGER",
 /*  30 */ "ccons ::= DEFAULT MINUS INTEGER",
 /*  31 */ "ccons ::= DEFAULT LP expr RP",
 /*  32 */ "ccons ::= NULL",
 /*  33 */ "ccons ::= NOT NULL",
 /*  34 */ "ccons ::= REPTYPE LP ids RP",
 /*  35 */ "conslist_opt ::=",
 /*  36 */ "conslist_opt ::= COMMA conslist",
 /*  37 */ "conslist ::= conslist COMMA tcons",
 /*  38 */ "conslist ::= conslist tcons",
 /*  39 */ "conslist ::= tcons",
 /*  40 */ "tcons ::= CONSTRAINT nm",
 /*  41 */ "tcons ::= PRIMARY KEY LP idxlist RP",
 /*  42 */ "tcons ::= id LP INTEGER RP",
 /*  43 */ "tcons ::= id LP ids RP",
 /*  44 */ "propertylist ::= propertylist COMMA tableproperty",
 /*  45 */ "propertylist ::= tableproperty",
 /*  46 */ "tableproperty ::= id EQ ids",
 /*  47 */ "cmd ::= DROP TABLE ifexists nm",
 /*  48 */ "cmd ::= TRUNCATE TABLE ifexists nm",
 /*  49 */ "cmd ::= create_index",
 /*  50 */ "create_index ::= CREATE INDEX ifnotexists nm ON nm LP idxlist RP nm INTEGER",
 /*  51 */ "create_index ::= CREATE INDEX ifnotexists nm ON nm LP idxlist RP nm",
 /*  52 */ "create_index ::= CREATE INDEX ifnotexists nm ON nm LP idxlist RP",
 /*  53 */ "idxlist ::= idxlist COMMA nm",
 /*  54 */ "idxlist ::= nm",
 /*  55 */ "cmd ::= DROP INDEX ifexists nm ON nm",
 /*  56 */ "cmd ::= ALTER TABLE ifnotexists nm ADD PRIMARY KEY LP pkidxlist RP",
 /*  57 */ "pkidxlist ::= pkidxlist COMMA nm",
 /*  58 */ "pkidxlist ::= nm",
 /*  59 */ "cmd ::= INSERT INTO tablename inscollist_opt VALUES LP itemlist RP",
 /*  60 */ "itemlist ::= itemlist COMMA expr",
 /*  61 */ "itemlist ::= expr",
 /*  62 */ "inscollist_opt ::= LP inscollist RP",
 /*  63 */ "inscollist ::= inscollist COMMA nm",
 /*  64 */ "inscollist ::= nm",
 /*  65 */ "cmd ::= DELETE hint first FROM tablename where_opt limit_opt",
 /*  66 */ "cmd ::= UPDATE hint first tablename SET setlist where_opt limit_opt",
 /*  67 */ "setlist ::= setlist COMMA setnm EQ expr",
 /*  68 */ "setlist ::= setnm EQ expr",
 /*  69 */ "setnm ::= nm",
 /*  70 */ "setnm ::= nm DOT nm",
 /*  71 */ "cmd ::= SELECT hint first distinct selcollist FROM tablename where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  72 */ "hint ::= SLASH STAR PLUS hintcollist STAR SLASH",
 /*  73 */ "hint ::=",
 /*  74 */ "hintcollist ::= hintcolumn",
 /*  75 */ "hintcollist ::= hintcollist hintcolumn",
 /*  76 */ "hintcolumn ::= INDEX LP nm RP",
 /*  77 */ "first ::= FIRST INTEGER",
 /*  78 */ "first ::=",
 /*  79 */ "distinct ::= DISTINCT",
 /*  80 */ "distinct ::= ALL",
 /*  81 */ "distinct ::=",
 /*  82 */ "sclp ::= selcollist COMMA",
 /*  83 */ "sclp ::=",
 /*  84 */ "selcollist ::= sclp expr as",
 /*  85 */ "selcollist ::= sclp STAR",
 /*  86 */ "selcollist ::= sclp nm DOT STAR",
 /*  87 */ "tablename ::= nm",
 /*  88 */ "tablename ::= nm nm",
 /*  89 */ "as ::= AS nm",
 /*  90 */ "as ::= ids",
 /*  91 */ "as ::=",
 /*  92 */ "where_opt ::=",
 /*  93 */ "where_opt ::= WHERE expr",
 /*  94 */ "groupby_opt ::=",
 /*  95 */ "groupby_opt ::= GROUP BY nexprlist",
 /*  96 */ "having_opt ::=",
 /*  97 */ "having_opt ::= HAVING expr",
 /*  98 */ "orderby_opt ::=",
 /*  99 */ "orderby_opt ::= ORDER BY sortlist",
 /* 100 */ "sortlist ::= sortlist COMMA sortitem sortorder",
 /* 101 */ "sortlist ::= sortitem sortorder",
 /* 102 */ "sortitem ::= expr",
 /* 103 */ "sortorder ::= ASC",
 /* 104 */ "sortorder ::= DESC",
 /* 105 */ "sortorder ::=",
 /* 106 */ "limit_opt ::=",
 /* 107 */ "limit_opt ::= LIMIT expr",
 /* 108 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 109 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 110 */ "cmd ::= USE DATABASE ifexists nm",
 /* 111 */ "cmd ::= USE TABLESPACE ifexists nm",
 /* 112 */ "cmd ::= create_database create_database_args",
 /* 113 */ "create_database ::= CREATE DATABASE ifnotexists nm",
 /* 114 */ "create_database_args ::= LP syscolumnlist RP",
 /* 115 */ "cmd ::= alter_database alter_database_args",
 /* 116 */ "alter_database ::= ALTER DATABASE ifexists nm",
 /* 117 */ "alter_database_args ::= LP syscolumnlist RP",
 /* 118 */ "syscolumnlist ::= syscolumnlist COMMA syscolumn",
 /* 119 */ "syscolumnlist ::= syscolumn",
 /* 120 */ "syscolumn ::= nm LP ids RP",
 /* 121 */ "syscolumn ::= nm LP INTEGER RP",
 /* 122 */ "cmd ::= DROP DATABASE ifexists nm",
 /* 123 */ "cmd ::= CONNECT TO ORACLE nm",
 /* 124 */ "cmd ::= CONNECT TO MYSQL nm",
 /* 125 */ "cmd ::= create_tablespace page_opt",
 /* 126 */ "create_tablespace ::= CREATE TABLESPACE ifnotexists nm",
 /* 127 */ "page_opt ::= PAGESIZE INTEGER ASKPAGE INTEGER STORAGE ids",
 /* 128 */ "page_opt ::= PAGESIZE INTEGER ASKPAGE INTEGER",
 /* 129 */ "page_opt ::= PAGESIZE INTEGER STORAGE ids",
 /* 130 */ "page_opt ::= PAGESIZE INTEGER",
 /* 131 */ "page_opt ::= ASKPAGE INTEGER",
 /* 132 */ "page_opt ::= ASKPAGE INTEGER STORAGE ids",
 /* 133 */ "page_opt ::= STORAGE ids",
 /* 134 */ "page_opt ::=",
 /* 135 */ "cmd ::= alter_tablespace alter_page_opt",
 /* 136 */ "alter_tablespace ::= ALTER TABLESPACE ifexists nm",
 /* 137 */ "alter_page_opt ::= PAGESIZE INTEGER ASKPAGE INTEGER STORAGE ids",
 /* 138 */ "alter_page_opt ::= PAGESIZE INTEGER",
 /* 139 */ "alter_page_opt ::= ASKPAGE INTEGER",
 /* 140 */ "alter_page_opt ::= STORAGE ids",
 /* 141 */ "cmd ::= DROP TABLESPACE ifexists nm",
 /* 142 */ "cmd ::= create_user access_opt",
 /* 143 */ "create_user ::= CREATE USER ifnotexists nm",
 /* 144 */ "access_opt ::= IDENTIFIED BY nm ACCESSED BY nm",
 /* 145 */ "access_opt ::= IDENTIFIED BY nm",
 /* 146 */ "cmd ::= alter_user alter_access_opt",
 /* 147 */ "alter_user ::= ALTER USER ifexists nm",
 /* 148 */ "alter_access_opt ::= IDENTIFIED BY nm ACCESSED BY nm",
 /* 149 */ "alter_access_opt ::= IDENTIFIED BY nm",
 /* 150 */ "alter_access_opt ::= ACCESSED BY nm",
 /* 151 */ "cmd ::= DROP USER ifexists nm",
 /* 152 */ "cmd ::= alter_table_addAttr add_attr",
 /* 153 */ "alter_table_addAttr ::= ALTER TABLE ifnotexists nm ADD TABLESYS",
 /* 154 */ "add_attr ::= id LP INTEGER RP",
 /* 155 */ "add_attr ::= id LP ids RP",
 /* 156 */ "cmd ::= alter_table_dropAttr drop_attr",
 /* 157 */ "alter_table_dropAttr ::= ALTER TABLE ifnotexists nm DROP TABLESYS",
 /* 158 */ "drop_attr ::= nm",
 /* 159 */ "cmd ::= alter_table_modifAttr modifyattr",
 /* 160 */ "alter_table_modifAttr ::= ALTER TABLE ifnotexists nm MODIFY TABLESYS",
 /* 161 */ "modifyattr ::= id LP INTEGER RP",
 /* 162 */ "modifyattr ::= id LP ids RP",
 /* 163 */ "cmd ::= alter_table_addcolum table_column",
 /* 164 */ "alter_table_addcolum ::= ALTER TABLE ifnotexists nm ADD COLUMN",
 /* 165 */ "table_column ::= columnid type carglist",
 /* 166 */ "cmd ::= ALTER TABLE ifnotexists nm DROP COLUMN nm",
 /* 167 */ "cmd ::= alter_table_modifycolum table_mcolumn",
 /* 168 */ "alter_table_modifycolum ::= ALTER TABLE ifnotexists nm MODIFY COLUMN",
 /* 169 */ "table_mcolumn ::= modifycolumid ctype carglists",
 /* 170 */ "modifycolumid ::= nm",
 /* 171 */ "ctype ::=",
 /* 172 */ "ctype ::= tokentype",
 /* 173 */ "tokentype ::= ids",
 /* 174 */ "tokentype ::= ids LP INTEGER RP",
 /* 175 */ "carglists ::= carglists cargs",
 /* 176 */ "carglists ::=",
 /* 177 */ "cargs ::= CONSTRAINT nm cons",
 /* 178 */ "cargs ::= cons",
 /* 179 */ "cons ::= NULL",
 /* 180 */ "cons ::= NOT NULL",
 /* 181 */ "cons ::= REPTYPE LP ids RP",
 /* 182 */ "cons ::= DEFAULT ids",
 /* 183 */ "cons ::= DEFAULT INTEGER",
 /* 184 */ "cons ::= DEFAULT PLUS INTEGER",
 /* 185 */ "cons ::= DEFAULT MINUS INTEGER",
 /* 186 */ "cons ::= DEFAULT LP expr RP",
 /* 187 */ "cmd ::= LOAD DATA TABLE nm",
 /* 188 */ "cmd ::= add_seq add_seq_args",
 /* 189 */ "add_seq ::= ADD SEQUENCE",
 /* 190 */ "add_seq_args ::= LP seqcolumnlist RP",
 /* 191 */ "cmd ::= alter_seq alter_seq_args",
 /* 192 */ "alter_seq ::= ALTER SEQUENCE",
 /* 193 */ "alter_seq_args ::= LP seqcolumnlist RP",
 /* 194 */ "cmd ::= del_seq del_seq_args",
 /* 195 */ "del_seq ::= DROP SEQUENCE",
 /* 196 */ "del_seq_args ::= LP seqcolumnlist RP",
 /* 197 */ "seqcolumnlist ::= seqcolumnlist COMMA seqcolumn",
 /* 198 */ "seqcolumnlist ::= seqcolumn",
 /* 199 */ "seqcolumn ::= nm LP ids RP",
 /* 200 */ "seqcolumn ::= nm LP INTEGER RP",
 /* 201 */ "cmd ::= add_job add_job_args",
 /* 202 */ "add_job ::= CREATE JOB nm",
 /* 203 */ "add_job_args ::= LP jobcolumnlist RP",
 /* 204 */ "cmd ::= alter_job alter_job_args",
 /* 205 */ "alter_job ::= ALTER JOB nm",
 /* 206 */ "alter_job_args ::= LP jobcolumnlist RP",
 /* 207 */ "cmd ::= REMOVE JOB nm",
 /* 208 */ "jobcolumnlist ::= jobcolumnlist COMMA jobcolumn",
 /* 209 */ "jobcolumnlist ::= jobcolumn",
 /* 210 */ "jobcolumn ::= nm LP ids RP",
 /* 211 */ "jobcolumn ::= nm LP INTEGER RP",
 /* 212 */ "cmd ::= alter_flushsql_addparam flushsql_param",
 /* 213 */ "alter_flushsql_addparam ::= ALTER TABLE ifnotexists nm ADD PARAMETER nm",
 /* 214 */ "flushsql_param ::= data_type parglists",
 /* 215 */ "data_type ::=",
 /* 216 */ "data_type ::= atokentype",
 /* 217 */ "atokentype ::= ids",
 /* 218 */ "parglists ::= parglists pargs",
 /* 219 */ "parglists ::=",
 /* 220 */ "pargs ::= cons",
 /* 221 */ "cons ::= VALUES LP ids RP",
 /* 222 */ "cons ::= PARAMTYPE LP ids RP",
 /* 223 */ "cmd ::= alter_flushsql_modifyparam flushsql_mparam",
 /* 224 */ "alter_flushsql_modifyparam ::= ALTER TABLE ifnotexists nm MODIFY PARAMETER nm",
 /* 225 */ "flushsql_mparam ::= mdata_type mparglists",
 /* 226 */ "mdata_type ::=",
 /* 227 */ "mdata_type ::= mtokentype",
 /* 228 */ "mtokentype ::= ids",
 /* 229 */ "mparglists ::= mparglists mpargs",
 /* 230 */ "mparglists ::=",
 /* 231 */ "mpargs ::= mcons",
 /* 232 */ "mcons ::= VALUES LP ids RP",
 /* 233 */ "mcons ::= PARAMTYPE LP ids RP",
 /* 234 */ "cmd ::= ALTER TABLE ifnotexists nm DROP PARAMETER nm",
 /* 235 */ "cmd ::= RENAME nm TO nm",
 /* 236 */ "expr ::= term",
 /* 237 */ "expr ::= LP expr RP",
 /* 238 */ "term ::= NULL",
 /* 239 */ "expr ::= id",
 /* 240 */ "expr ::= nm DOT nm",
 /* 241 */ "expr ::= nm DOT NEXTVAL|CURRVAL",
 /* 242 */ "term ::= INTEGER|FLOAT|BLOB",
 /* 243 */ "term ::= STRING",
 /* 244 */ "expr ::= VARIABLE",
 /* 245 */ "expr ::= ID LP exprlist RP",
 /* 246 */ "expr ::= ID LP STAR RP",
 /* 247 */ "term ::= SYSDATE",
 /* 248 */ "expr ::= expr AND expr",
 /* 249 */ "expr ::= expr OR expr",
 /* 250 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 251 */ "expr ::= expr EQ|NE expr",
 /* 252 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 253 */ "expr ::= expr PLUS|MINUS expr",
 /* 254 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 255 */ "expr ::= expr CONCAT expr",
 /* 256 */ "likeop ::= LIKE_KW",
 /* 257 */ "likeop ::= NOT LIKE_KW",
 /* 258 */ "likeop ::= MATCH",
 /* 259 */ "likeop ::= NOT MATCH",
 /* 260 */ "expr ::= expr likeop expr",
 /* 261 */ "expr ::= expr ISNULL|NOTNULL",
 /* 262 */ "expr ::= expr NOT NULL",
 /* 263 */ "expr ::= expr IS expr",
 /* 264 */ "expr ::= expr IS NOT expr",
 /* 265 */ "exprlist ::= nexprlist",
 /* 266 */ "exprlist ::=",
 /* 267 */ "nexprlist ::= nexprlist COMMA expr",
 /* 268 */ "nexprlist ::= expr",
 /* 269 */ "expr ::= NOT expr",
 /* 270 */ "expr ::= BITNOT expr",
 /* 271 */ "expr ::= MINUS expr",
 /* 272 */ "expr ::= PLUS expr",
 /* 273 */ "between_op ::= BETWEEN",
 /* 274 */ "between_op ::= NOT BETWEEN",
 /* 275 */ "expr ::= expr between_op expr AND expr",
 /* 276 */ "in_op ::= IN",
 /* 277 */ "in_op ::= NOT IN",
 /* 278 */ "expr ::= expr in_op LP exprlist RP",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to mdbParser and mdbParserFree.
*/
void *mdbParserAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  mdbParserARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    case 129: /* expr */
    case 217: /* term */
{
#line 964 "parser.y"

#line 1165 "parser.c"
}
      break;
    case 132: /* idxlist */
    case 135: /* pkidxlist */
    case 139: /* inscollist */
{
#line 262 "parser.y"
QMDB_MALLOC->ReleaseIdList((yypminor->yy157));
#line 1174 "parser.c"
}
      break;
    case 137: /* inscollist_opt */
{
#line 309 "parser.y"
QMDB_MALLOC->ReleaseIdList((yypminor->yy101));
#line 1181 "parser.c"
}
      break;
    case 138: /* itemlist */
    case 149: /* groupby_opt */
    case 154: /* sclp */
    case 156: /* nexprlist */
    case 218: /* exprlist */
{
#line 301 "parser.y"
QMDB_MALLOC->ReleaseExprList((yypminor->yy331));
#line 1192 "parser.c"
}
      break;
    case 140: /* hint */
    case 148: /* selcollist */
    case 151: /* orderby_opt */
    case 152: /* hintcollist */
    case 157: /* sortlist */
{
#line 375 "parser.y"
QMDB_MALLOC->ReleaseExprList((yypminor->yy387));
#line 1203 "parser.c"
}
      break;
    case 142: /* where_opt */
{
#line 461 "parser.y"
QMDB_MALLOC->ReleaseExpr((yypminor->yy380));
#line 1210 "parser.c"
}
      break;
    case 144: /* setlist */
{
#line 339 "parser.y"
QMDB_MALLOC->ReleaseIdList((yypminor->yy370).pIdList);QMDB_MALLOC->ReleaseExprList((yypminor->yy370).pExprList);
#line 1217 "parser.c"
}
      break;
    case 146: /* select */
{
#line 367 "parser.y"

#line 1224 "parser.c"
}
      break;
    case 150: /* having_opt */
    case 158: /* sortitem */
{
#line 474 "parser.y"
QMDB_MALLOC->ReleaseExpr((yypminor->yy352));
#line 1232 "parser.c"
}
      break;
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(pParser, yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from mdbParserAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void mdbParserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int mdbParserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_COUNT
   || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          yy_lookahead[j]==YYWILDCARD
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   mdbParserARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
#line 11 "parser.y"

	pParse->m_tError.FillErrMsg(ERR_OS_NO_MEMROY,"overflow");
#line 1417 "parser.c"
   mdbParserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 108, 1 },
  { 109, 2 },
  { 109, 1 },
  { 111, 1 },
  { 112, 1 },
  { 113, 1 },
  { 114, 1 },
  { 114, 1 },
  { 115, 0 },
  { 115, 3 },
  { 116, 2 },
  { 116, 0 },
  { 110, 2 },
  { 117, 4 },
  { 118, 5 },
  { 119, 3 },
  { 119, 1 },
  { 122, 3 },
  { 123, 1 },
  { 124, 0 },
  { 124, 1 },
  { 126, 1 },
  { 126, 4 },
  { 125, 2 },
  { 125, 0 },
  { 127, 3 },
  { 127, 1 },
  { 128, 2 },
  { 128, 2 },
  { 128, 3 },
  { 128, 3 },
  { 128, 4 },
  { 128, 1 },
  { 128, 2 },
  { 128, 4 },
  { 120, 0 },
  { 120, 2 },
  { 130, 3 },
  { 130, 2 },
  { 130, 1 },
  { 131, 2 },
  { 131, 5 },
  { 131, 4 },
  { 131, 4 },
  { 121, 3 },
  { 121, 1 },
  { 133, 3 },
  { 110, 4 },
  { 110, 4 },
  { 110, 1 },
  { 134, 11 },
  { 134, 10 },
  { 134, 9 },
  { 132, 3 },
  { 132, 1 },
  { 110, 6 },
  { 110, 10 },
  { 135, 3 },
  { 135, 1 },
  { 110, 8 },
  { 138, 3 },
  { 138, 1 },
  { 137, 3 },
  { 139, 3 },
  { 139, 1 },
  { 110, 7 },
  { 110, 8 },
  { 144, 5 },
  { 144, 3 },
  { 145, 1 },
  { 145, 3 },
  { 110, 12 },
  { 140, 6 },
  { 140, 0 },
  { 152, 1 },
  { 152, 2 },
  { 153, 4 },
  { 141, 2 },
  { 141, 0 },
  { 147, 1 },
  { 147, 1 },
  { 147, 0 },
  { 154, 2 },
  { 154, 0 },
  { 148, 3 },
  { 148, 2 },
  { 148, 4 },
  { 136, 1 },
  { 136, 2 },
  { 155, 2 },
  { 155, 1 },
  { 155, 0 },
  { 142, 0 },
  { 142, 2 },
  { 149, 0 },
  { 149, 3 },
  { 150, 0 },
  { 150, 2 },
  { 151, 0 },
  { 151, 3 },
  { 157, 4 },
  { 157, 2 },
  { 158, 1 },
  { 159, 1 },
  { 159, 1 },
  { 159, 0 },
  { 143, 0 },
  { 143, 2 },
  { 143, 4 },
  { 143, 4 },
  { 110, 4 },
  { 110, 4 },
  { 110, 2 },
  { 160, 4 },
  { 161, 3 },
  { 110, 2 },
  { 163, 4 },
  { 164, 3 },
  { 162, 3 },
  { 162, 1 },
  { 165, 4 },
  { 165, 4 },
  { 110, 4 },
  { 110, 4 },
  { 110, 4 },
  { 110, 2 },
  { 166, 4 },
  { 167, 6 },
  { 167, 4 },
  { 167, 4 },
  { 167, 2 },
  { 167, 2 },
  { 167, 4 },
  { 167, 2 },
  { 167, 0 },
  { 110, 2 },
  { 168, 4 },
  { 169, 6 },
  { 169, 2 },
  { 169, 2 },
  { 169, 2 },
  { 110, 4 },
  { 110, 2 },
  { 170, 4 },
  { 171, 6 },
  { 171, 3 },
  { 110, 2 },
  { 172, 4 },
  { 173, 6 },
  { 173, 3 },
  { 173, 3 },
  { 110, 4 },
  { 110, 2 },
  { 174, 6 },
  { 175, 4 },
  { 175, 4 },
  { 110, 2 },
  { 176, 6 },
  { 177, 1 },
  { 110, 2 },
  { 178, 6 },
  { 179, 4 },
  { 179, 4 },
  { 110, 2 },
  { 180, 6 },
  { 181, 3 },
  { 110, 7 },
  { 110, 2 },
  { 182, 6 },
  { 183, 3 },
  { 184, 1 },
  { 185, 0 },
  { 185, 1 },
  { 187, 1 },
  { 187, 4 },
  { 186, 2 },
  { 186, 0 },
  { 188, 3 },
  { 188, 1 },
  { 189, 1 },
  { 189, 2 },
  { 189, 4 },
  { 189, 2 },
  { 189, 2 },
  { 189, 3 },
  { 189, 3 },
  { 189, 4 },
  { 110, 4 },
  { 110, 2 },
  { 190, 2 },
  { 191, 3 },
  { 110, 2 },
  { 193, 2 },
  { 194, 3 },
  { 110, 2 },
  { 195, 2 },
  { 196, 3 },
  { 192, 3 },
  { 192, 1 },
  { 197, 4 },
  { 197, 4 },
  { 110, 2 },
  { 198, 3 },
  { 199, 3 },
  { 110, 2 },
  { 201, 3 },
  { 202, 3 },
  { 110, 3 },
  { 200, 3 },
  { 200, 1 },
  { 203, 4 },
  { 203, 4 },
  { 110, 2 },
  { 204, 7 },
  { 205, 2 },
  { 206, 0 },
  { 206, 1 },
  { 208, 1 },
  { 207, 2 },
  { 207, 0 },
  { 209, 1 },
  { 189, 4 },
  { 189, 4 },
  { 110, 2 },
  { 210, 7 },
  { 211, 2 },
  { 212, 0 },
  { 212, 1 },
  { 214, 1 },
  { 213, 2 },
  { 213, 0 },
  { 215, 1 },
  { 216, 4 },
  { 216, 4 },
  { 110, 7 },
  { 110, 4 },
  { 129, 1 },
  { 129, 3 },
  { 217, 1 },
  { 129, 1 },
  { 129, 3 },
  { 129, 3 },
  { 217, 1 },
  { 217, 1 },
  { 129, 1 },
  { 129, 4 },
  { 129, 4 },
  { 217, 1 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 219, 1 },
  { 219, 2 },
  { 219, 1 },
  { 219, 2 },
  { 129, 3 },
  { 129, 2 },
  { 129, 3 },
  { 129, 3 },
  { 129, 4 },
  { 218, 1 },
  { 218, 0 },
  { 156, 3 },
  { 156, 1 },
  { 129, 2 },
  { 129, 2 },
  { 129, 2 },
  { 129, 2 },
  { 220, 1 },
  { 220, 2 },
  { 129, 5 },
  { 221, 1 },
  { 221, 2 },
  { 129, 5 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  mdbParserARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/
  yygotominor = yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 4: /* id ::= ID */
      case 5: /* ids ::= ID|STRING */ yytestcase(yyruleno==5);
      case 6: /* nm ::= id */ yytestcase(yyruleno==6);
      case 7: /* nm ::= STRING */ yytestcase(yyruleno==7);
      case 89: /* as ::= AS nm */ yytestcase(yyruleno==89);
      case 90: /* as ::= ids */ yytestcase(yyruleno==90);
#line 87 "parser.y"
{yygotominor.yy0 = yymsp[0].minor.yy0;}
#line 1815 "parser.c"
        break;
      case 8: /* ifnotexists ::= */
      case 11: /* ifexists ::= */ yytestcase(yyruleno==11);
      case 80: /* distinct ::= ALL */ yytestcase(yyruleno==80);
      case 81: /* distinct ::= */ yytestcase(yyruleno==81);
      case 273: /* between_op ::= BETWEEN */ yytestcase(yyruleno==273);
      case 276: /* in_op ::= IN */ yytestcase(yyruleno==276);
#line 103 "parser.y"
{yygotominor.yy304 = 0;}
#line 1825 "parser.c"
        break;
      case 9: /* ifnotexists ::= IF NOT EXISTS */
      case 10: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==10);
      case 79: /* distinct ::= DISTINCT */ yytestcase(yyruleno==79);
      case 274: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==274);
      case 277: /* in_op ::= NOT IN */ yytestcase(yyruleno==277);
#line 104 "parser.y"
{yygotominor.yy304 = 1;}
#line 1834 "parser.c"
        break;
      case 13: /* create_table ::= CREATE TABLE ifnotexists nm */
#line 114 "parser.y"
{
	pParse->BuildCreateTable(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);
}
#line 1841 "parser.c"
        break;
      case 14: /* create_table_args ::= LP columnlist conslist_opt RP propertylist */
      case 114: /* create_database_args ::= LP syscolumnlist RP */ yytestcase(yyruleno==114);
      case 117: /* alter_database_args ::= LP syscolumnlist RP */ yytestcase(yyruleno==117);
#line 117 "parser.y"
{
	
}
#line 1850 "parser.c"
        break;
      case 17: /* column ::= columnid type carglist */
      case 33: /* ccons ::= NOT NULL */ yytestcase(yyruleno==33);
      case 190: /* add_seq_args ::= LP seqcolumnlist RP */ yytestcase(yyruleno==190);
      case 193: /* alter_seq_args ::= LP seqcolumnlist RP */ yytestcase(yyruleno==193);
      case 196: /* del_seq_args ::= LP seqcolumnlist RP */ yytestcase(yyruleno==196);
      case 203: /* add_job_args ::= LP jobcolumnlist RP */ yytestcase(yyruleno==203);
      case 206: /* alter_job_args ::= LP jobcolumnlist RP */ yytestcase(yyruleno==206);
#line 129 "parser.y"
{
}
#line 1862 "parser.c"
        break;
      case 18: /* columnid ::= nm */
#line 131 "parser.y"
{
	pParse->AddColumn(&yymsp[0].minor.yy0);
}
#line 1869 "parser.c"
        break;
      case 21: /* typetoken ::= ids */
#line 142 "parser.y"
{	pParse->AddColumnAttribute("data-type",&yymsp[0].minor.yy0);}
#line 1874 "parser.c"
        break;
      case 22: /* typetoken ::= ids LP INTEGER RP */
#line 143 "parser.y"
{
	pParse->AddColumnAttribute("data-type",&yymsp[-3].minor.yy0);
	pParse->AddColumnAttribute("data-len",&yymsp[-1].minor.yy0);
}
#line 1882 "parser.c"
        break;
      case 27: /* ccons ::= DEFAULT ids */
#line 153 "parser.y"
{  pParse->AddColumnAttribute("default-value",&yymsp[0].minor.yy0);}
#line 1887 "parser.c"
        break;
      case 28: /* ccons ::= DEFAULT INTEGER */
#line 154 "parser.y"
{ pParse->AddColumnAttribute("default-value",&yymsp[0].minor.yy0);}
#line 1892 "parser.c"
        break;
      case 29: /* ccons ::= DEFAULT PLUS INTEGER */
#line 157 "parser.y"
{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER,0,0,&yymsp[0].minor.yy0);
	ST_EXPR * temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_UPLUS, temp, 0, 0);
	struct _Token str;
	ST_EXPR *  pstExpr= temp2;
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] ={0};
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue);
	str.z = value;
	str.n =strlen(value);
	pParse->AddColumnAttribute("default-value",&str);

}
#line 1909 "parser.c"
        break;
      case 30: /* ccons ::= DEFAULT MINUS INTEGER */
#line 171 "parser.y"
{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER,0,0,&yymsp[0].minor.yy0);
	ST_EXPR * temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_UMINUS, temp, 0, 0);
	struct _Token str; 
	ST_EXPR *  pstExpr= temp2; 
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] = {0}; 
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue); 
	str.z = value; 
	str.n =strlen(value); 
	pParse->AddColumnAttribute("default-value",&str);

}
#line 1926 "parser.c"
        break;
      case 31: /* ccons ::= DEFAULT LP expr RP */
#line 186 "parser.y"
{
	struct _Token str;
	ST_EXPR *  pstExpr= yymsp[-1].minor.yy175.pExpr;
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] = {0};
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue);
	str.z = value;
	str.n =strlen(value);
	pParse->AddColumnAttribute("default-value",&str);

}
#line 1941 "parser.c"
        break;
      case 32: /* ccons ::= NULL */
#line 197 "parser.y"
{ pParse->AddColumnNULLAttribute();}
#line 1946 "parser.c"
        break;
      case 34: /* ccons ::= REPTYPE LP ids RP */
#line 200 "parser.y"
{ pParse->AddColumnAttribute("rep-type",&yymsp[-1].minor.yy0);}
#line 1951 "parser.c"
        break;
      case 35: /* conslist_opt ::= */
#line 205 "parser.y"
{yygotominor.yy0.n = 0; yygotominor.yy0.z = 0;}
#line 1956 "parser.c"
        break;
      case 36: /* conslist_opt ::= COMMA conslist */
#line 206 "parser.y"
{yygotominor.yy0 = yymsp[-1].minor.yy0;}
#line 1961 "parser.c"
        break;
      case 41: /* tcons ::= PRIMARY KEY LP idxlist RP */
#line 211 "parser.y"
{
	pParse->AddTablePrimayKey(yymsp[-1].minor.yy157);
}
#line 1968 "parser.c"
        break;
      case 42: /* tcons ::= id LP INTEGER RP */
      case 43: /* tcons ::= id LP ids RP */ yytestcase(yyruleno==43);
      case 154: /* add_attr ::= id LP INTEGER RP */ yytestcase(yyruleno==154);
      case 155: /* add_attr ::= id LP ids RP */ yytestcase(yyruleno==155);
#line 216 "parser.y"
{
	pParse->AddTableAttribute(&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0);
}
#line 1978 "parser.c"
        break;
      case 46: /* tableproperty ::= id EQ ids */
#line 228 "parser.y"
{

    pParse->AddTableAttribute(&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0);
}
#line 1986 "parser.c"
        break;
      case 47: /* cmd ::= DROP TABLE ifexists nm */
#line 235 "parser.y"
{
    pParse->BuildDropTable(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);
}
#line 1993 "parser.c"
        break;
      case 48: /* cmd ::= TRUNCATE TABLE ifexists nm */
#line 241 "parser.y"
{
    pParse->BuildTruncateTable(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);
}
#line 2000 "parser.c"
        break;
      case 50: /* create_index ::= CREATE INDEX ifnotexists nm ON nm LP idxlist RP nm INTEGER */
#line 249 "parser.y"
{
	pParse->BuildCreateIndex(yymsp[-8].minor.yy304,&yymsp[-7].minor.yy0,&yymsp[-5].minor.yy0,yymsp[-3].minor.yy157,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2007 "parser.c"
        break;
      case 51: /* create_index ::= CREATE INDEX ifnotexists nm ON nm LP idxlist RP nm */
#line 253 "parser.y"
{
	pParse->BuildCreateIndex(yymsp[-7].minor.yy304,&yymsp[-6].minor.yy0,&yymsp[-4].minor.yy0,yymsp[-2].minor.yy157,&yymsp[0].minor.yy0);
}
#line 2014 "parser.c"
        break;
      case 52: /* create_index ::= CREATE INDEX ifnotexists nm ON nm LP idxlist RP */
#line 257 "parser.y"
{
	pParse->BuildCreateIndex(yymsp[-6].minor.yy304,&yymsp[-5].minor.yy0,&yymsp[-3].minor.yy0,yymsp[-1].minor.yy157,NULL);
}
#line 2021 "parser.c"
        break;
      case 53: /* idxlist ::= idxlist COMMA nm */
      case 57: /* pkidxlist ::= pkidxlist COMMA nm */ yytestcase(yyruleno==57);
#line 264 "parser.y"
{
    {yygotominor.yy157 = pParse->IdListAppend(yymsp[-2].minor.yy157,&yymsp[0].minor.yy0);}
}
#line 2029 "parser.c"
        break;
      case 54: /* idxlist ::= nm */
      case 58: /* pkidxlist ::= nm */ yytestcase(yyruleno==58);
#line 267 "parser.y"
{
   {yygotominor.yy157 = pParse->IdListAppend(0,&yymsp[0].minor.yy0);}
}
#line 2037 "parser.c"
        break;
      case 55: /* cmd ::= DROP INDEX ifexists nm ON nm */
#line 274 "parser.y"
{ pParse->BuildDropIndex(yymsp[-3].minor.yy304,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0);}
#line 2042 "parser.c"
        break;
      case 56: /* cmd ::= ALTER TABLE ifnotexists nm ADD PRIMARY KEY LP pkidxlist RP */
#line 278 "parser.y"
{
	pParse->BuildCreatePrimKey(yymsp[-7].minor.yy304,&yymsp[-6].minor.yy0,yymsp[-1].minor.yy157);
}
#line 2049 "parser.c"
        break;
      case 59: /* cmd ::= INSERT INTO tablename inscollist_opt VALUES LP itemlist RP */
#line 297 "parser.y"
{pParse->BuildInsert(yymsp[-4].minor.yy101,yymsp[-1].minor.yy331);}
#line 2054 "parser.c"
        break;
      case 60: /* itemlist ::= itemlist COMMA expr */
      case 267: /* nexprlist ::= nexprlist COMMA expr */ yytestcase(yyruleno==267);
#line 304 "parser.y"
{yygotominor.yy331 = pParse->m_tMdbExpr.ExprListAppend(yymsp[-2].minor.yy331,yymsp[0].minor.yy175.pExpr);}
#line 2060 "parser.c"
        break;
      case 61: /* itemlist ::= expr */
      case 268: /* nexprlist ::= expr */ yytestcase(yyruleno==268);
#line 306 "parser.y"
{yygotominor.yy331 = pParse->m_tMdbExpr.ExprListAppend(0,yymsp[0].minor.yy175.pExpr);}
#line 2066 "parser.c"
        break;
      case 62: /* inscollist_opt ::= LP inscollist RP */
#line 314 "parser.y"
{yygotominor.yy101 = yymsp[-1].minor.yy157;}
#line 2071 "parser.c"
        break;
      case 63: /* inscollist ::= inscollist COMMA nm */
#line 316 "parser.y"
{yygotominor.yy157 = pParse->IdListAppend(yymsp[-2].minor.yy157,&yymsp[0].minor.yy0);}
#line 2076 "parser.c"
        break;
      case 64: /* inscollist ::= nm */
#line 318 "parser.y"
{yygotominor.yy157 = pParse->IdListAppend(0,&yymsp[0].minor.yy0);}
#line 2081 "parser.c"
        break;
      case 65: /* cmd ::= DELETE hint first FROM tablename where_opt limit_opt */
#line 324 "parser.y"
{
  pParse->BuildDelete(yymsp[-5].minor.yy387,yymsp[-4].minor.yy380,yymsp[-1].minor.yy380,yymsp[0].minor.yy292.pLimit,yymsp[0].minor.yy292.pOffset);
}
#line 2088 "parser.c"
        break;
      case 66: /* cmd ::= UPDATE hint first tablename SET setlist where_opt limit_opt */
#line 333 "parser.y"
{
  pParse->BuildUpdate(yymsp[-6].minor.yy387,yymsp[-5].minor.yy380,yymsp[-2].minor.yy370,yymsp[-1].minor.yy380,yymsp[0].minor.yy292.pLimit,yymsp[0].minor.yy292.pOffset);
}
#line 2095 "parser.c"
        break;
      case 67: /* setlist ::= setlist COMMA setnm EQ expr */
#line 341 "parser.y"
{
  yygotominor.yy370.pExprList = pParse->m_tMdbExpr.ExprListAppend( yymsp[-4].minor.yy370.pExprList, yymsp[0].minor.yy175.pExpr);
  pParse->m_tMdbExpr.ExprListSetName(yygotominor.yy370.pExprList, &yymsp[-2].minor.yy0, 1);
  yygotominor.yy370.pIdList = pParse->IdListAppend(yymsp[-4].minor.yy370.pIdList ,&yymsp[-2].minor.yy0);
}
#line 2104 "parser.c"
        break;
      case 68: /* setlist ::= setnm EQ expr */
#line 346 "parser.y"
{
  yygotominor.yy370.pExprList  = pParse->m_tMdbExpr.ExprListAppend( 0, yymsp[0].minor.yy175.pExpr);
  pParse->m_tMdbExpr.ExprListSetName( yygotominor.yy370.pExprList, &yymsp[-2].minor.yy0, 1);
  yygotominor.yy370.pIdList = pParse->IdListAppend(0,&yymsp[-2].minor.yy0);
}
#line 2113 "parser.c"
        break;
      case 69: /* setnm ::= nm */
#line 352 "parser.y"
{
	yygotominor.yy0=yymsp[0].minor.yy0;
}
#line 2120 "parser.c"
        break;
      case 70: /* setnm ::= nm DOT nm */
#line 355 "parser.y"
{
	if(false == pParse->IsTableAlias(&yymsp[-2].minor.yy0))
	{
		pParse->m_tError.FillErrMsg(ERR_SQL_INVALID,"table alias is error[%s]",yymsp[-2].minor.yy0.z);
	}
	yygotominor.yy0=yymsp[0].minor.yy0;
}
#line 2131 "parser.c"
        break;
      case 71: /* cmd ::= SELECT hint first distinct selcollist FROM tablename where_opt groupby_opt having_opt orderby_opt limit_opt */
#line 370 "parser.y"
{
  pParse->BuildSelect(yymsp[-10].minor.yy387,yymsp[-9].minor.yy380,yymsp[-8].minor.yy304,yymsp[-7].minor.yy387,yymsp[-4].minor.yy380,yymsp[-3].minor.yy331,yymsp[-2].minor.yy352,yymsp[-1].minor.yy387,yymsp[0].minor.yy292.pLimit,yymsp[0].minor.yy292.pOffset);//进入此规则后调用的处理逻辑
}
#line 2138 "parser.c"
        break;
      case 72: /* hint ::= SLASH STAR PLUS hintcollist STAR SLASH */
#line 380 "parser.y"
{yygotominor.yy387=yymsp[-2].minor.yy387;}
#line 2143 "parser.c"
        break;
      case 73: /* hint ::= */
#line 381 "parser.y"
{yygotominor.yy387=0;}
#line 2148 "parser.c"
        break;
      case 74: /* hintcollist ::= hintcolumn */
#line 385 "parser.y"
{yygotominor.yy387 = pParse->m_tMdbExpr.ExprListAppend(NULL, yymsp[0].minor.yy380);}
#line 2153 "parser.c"
        break;
      case 75: /* hintcollist ::= hintcollist hintcolumn */
#line 387 "parser.y"
{yygotominor.yy387 = pParse->m_tMdbExpr.ExprListAppend(yymsp[-1].minor.yy387, yymsp[0].minor.yy380);}
#line 2158 "parser.c"
        break;
      case 76: /* hintcolumn ::= INDEX LP nm RP */
#line 389 "parser.y"
{
	ST_EXPR *pRight = pParse->m_tMdbExpr.BuildPExpr( TK_INDEX, 0, 0, 0);
    ST_EXPR *pLeft = pParse->m_tMdbExpr.BuildPExpr(TK_STRING, 0, 0, &yymsp[-1].minor.yy0);
    yygotominor.yy380 = pParse->m_tMdbExpr.BuildPExpr( TK_HINT, pLeft, pRight, 0);
}
#line 2167 "parser.c"
        break;
      case 77: /* first ::= FIRST INTEGER */
#line 398 "parser.y"
{
	yygotominor.yy380 = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER, 0, 0, &yymsp[0].minor.yy0);
}
#line 2174 "parser.c"
        break;
      case 78: /* first ::= */
#line 401 "parser.y"
{yygotominor.yy380=0;}
#line 2179 "parser.c"
        break;
      case 82: /* sclp ::= selcollist COMMA */
#line 419 "parser.y"
{yygotominor.yy331 = yymsp[-1].minor.yy387;}
#line 2184 "parser.c"
        break;
      case 83: /* sclp ::= */
      case 94: /* groupby_opt ::= */ yytestcase(yyruleno==94);
      case 266: /* exprlist ::= */ yytestcase(yyruleno==266);
#line 420 "parser.y"
{yygotominor.yy331 = 0;}
#line 2191 "parser.c"
        break;
      case 84: /* selcollist ::= sclp expr as */
#line 422 "parser.y"
{
   yygotominor.yy387 = pParse->m_tMdbExpr.ExprListAppend( yymsp[-2].minor.yy331, yymsp[-1].minor.yy175.pExpr);
   if( yymsp[0].minor.yy0.n>0 ) pParse->m_tMdbExpr.ExprListSetName( yygotominor.yy387, &yymsp[0].minor.yy0, 1);
   pParse->m_tMdbExpr.ExprListSetSpan(yygotominor.yy387,&yymsp[-1].minor.yy175);
}
#line 2200 "parser.c"
        break;
      case 85: /* selcollist ::= sclp STAR */
#line 428 "parser.y"
{
  ST_EXPR *p = pParse->m_tMdbExpr.BuildExpr( TK_ALL);
  yygotominor.yy387 = pParse->m_tMdbExpr.ExprListAppend( yymsp[-1].minor.yy331, p);
}
#line 2208 "parser.c"
        break;
      case 86: /* selcollist ::= sclp nm DOT STAR */
#line 433 "parser.y"
{
  ST_EXPR *pRight = pParse->m_tMdbExpr.BuildPExpr( TK_ALL, 0, 0, &yymsp[0].minor.yy0);
  ST_EXPR *pLeft = pParse->m_tMdbExpr.BuildPExpr(TK_ID_TABLENAME, 0, 0, &yymsp[-2].minor.yy0);
  ST_EXPR *pDot = pParse->m_tMdbExpr.BuildPExpr( TK_DOT, pLeft, pRight, 0);
  yygotominor.yy387 = pParse->m_tMdbExpr.ExprListAppend(yymsp[-3].minor.yy331, pDot);
}
#line 2218 "parser.c"
        break;
      case 87: /* tablename ::= nm */
#line 442 "parser.y"
{
	pParse->FillTableName(&yymsp[0].minor.yy0,NULL); 
}
#line 2225 "parser.c"
        break;
      case 88: /* tablename ::= nm nm */
#line 445 "parser.y"
{
	pParse->FillTableName(&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0); 
}
#line 2232 "parser.c"
        break;
      case 91: /* as ::= */
#line 456 "parser.y"
{yygotominor.yy0.n = 0;}
#line 2237 "parser.c"
        break;
      case 92: /* where_opt ::= */
#line 463 "parser.y"
{yygotominor.yy380 = 0;}
#line 2242 "parser.c"
        break;
      case 93: /* where_opt ::= WHERE expr */
#line 464 "parser.y"
{yygotominor.yy380 = yymsp[0].minor.yy175.pExpr;}
#line 2247 "parser.c"
        break;
      case 95: /* groupby_opt ::= GROUP BY nexprlist */
      case 265: /* exprlist ::= nexprlist */ yytestcase(yyruleno==265);
#line 471 "parser.y"
{yygotominor.yy331 = yymsp[0].minor.yy331;}
#line 2253 "parser.c"
        break;
      case 96: /* having_opt ::= */
#line 475 "parser.y"
{yygotominor.yy352 = 0;}
#line 2258 "parser.c"
        break;
      case 97: /* having_opt ::= HAVING expr */
      case 102: /* sortitem ::= expr */ yytestcase(yyruleno==102);
#line 476 "parser.y"
{yygotominor.yy352 = yymsp[0].minor.yy175.pExpr;}
#line 2264 "parser.c"
        break;
      case 98: /* orderby_opt ::= */
#line 487 "parser.y"
{yygotominor.yy387 = 0;}
#line 2269 "parser.c"
        break;
      case 99: /* orderby_opt ::= ORDER BY sortlist */
#line 488 "parser.y"
{yygotominor.yy387 = yymsp[0].minor.yy387;}
#line 2274 "parser.c"
        break;
      case 100: /* sortlist ::= sortlist COMMA sortitem sortorder */
#line 489 "parser.y"
{
  yygotominor.yy387 = pParse->m_tMdbExpr.ExprListAppend(yymsp[-3].minor.yy387,yymsp[-1].minor.yy352);
  if(yygotominor.yy387) yygotominor.yy387->pExprItems[yygotominor.yy387->iItemNum-1].iSortOrder = yymsp[0].minor.yy304;
}
#line 2282 "parser.c"
        break;
      case 101: /* sortlist ::= sortitem sortorder */
#line 493 "parser.y"
{
  yygotominor.yy387 = pParse->m_tMdbExpr.ExprListAppend(0,yymsp[-1].minor.yy352);
  if( yygotominor.yy387 && (yygotominor.yy387->pExprItems) ) yygotominor.yy387->pExprItems[0].iSortOrder = yymsp[0].minor.yy304;
}
#line 2290 "parser.c"
        break;
      case 103: /* sortorder ::= ASC */
      case 105: /* sortorder ::= */ yytestcase(yyruleno==105);
#line 501 "parser.y"
{yygotominor.yy304 = MDB_SO_ASC;}
#line 2296 "parser.c"
        break;
      case 104: /* sortorder ::= DESC */
#line 502 "parser.y"
{yygotominor.yy304 = MDB_SO_DESC;}
#line 2301 "parser.c"
        break;
      case 106: /* limit_opt ::= */
#line 520 "parser.y"
{yygotominor.yy292.pLimit = 0; yygotominor.yy292.pOffset = 0;}
#line 2306 "parser.c"
        break;
      case 107: /* limit_opt ::= LIMIT expr */
#line 521 "parser.y"
{yygotominor.yy292.pLimit = yymsp[0].minor.yy175.pExpr; yygotominor.yy292.pOffset = 0;}
#line 2311 "parser.c"
        break;
      case 108: /* limit_opt ::= LIMIT expr OFFSET expr */
#line 523 "parser.y"
{yygotominor.yy292.pLimit = yymsp[-2].minor.yy175.pExpr; yygotominor.yy292.pOffset = yymsp[0].minor.yy175.pExpr;}
#line 2316 "parser.c"
        break;
      case 109: /* limit_opt ::= LIMIT expr COMMA expr */
#line 525 "parser.y"
{yygotominor.yy292.pOffset = yymsp[-2].minor.yy175.pExpr; yygotominor.yy292.pLimit = yymsp[0].minor.yy175.pExpr;}
#line 2321 "parser.c"
        break;
      case 110: /* cmd ::= USE DATABASE ifexists nm */
#line 529 "parser.y"
{pParse->BuildUseDsn(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);}
#line 2326 "parser.c"
        break;
      case 111: /* cmd ::= USE TABLESPACE ifexists nm */
#line 534 "parser.y"
{pParse->BuildUseTablespace(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);}
#line 2331 "parser.c"
        break;
      case 113: /* create_database ::= CREATE DATABASE ifnotexists nm */
#line 540 "parser.y"
{
    pParse->BuildCreateDsn(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);
}
#line 2338 "parser.c"
        break;
      case 116: /* alter_database ::= ALTER DATABASE ifexists nm */
#line 551 "parser.y"
{
    pParse->BuildAlterDsn(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);
}
#line 2345 "parser.c"
        break;
      case 120: /* syscolumn ::= nm LP ids RP */
      case 121: /* syscolumn ::= nm LP INTEGER RP */ yytestcase(yyruleno==121);
#line 562 "parser.y"
{
    pParse->AddDsnAttribute(&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0);
}
#line 2353 "parser.c"
        break;
      case 122: /* cmd ::= DROP DATABASE ifexists nm */
#line 572 "parser.y"
{ pParse->BuildDropDsn(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);}
#line 2358 "parser.c"
        break;
      case 123: /* cmd ::= CONNECT TO ORACLE nm */
#line 576 "parser.y"
{ pParse->BuildDataSourceForOracle(&yymsp[0].minor.yy0);}
#line 2363 "parser.c"
        break;
      case 124: /* cmd ::= CONNECT TO MYSQL nm */
#line 577 "parser.y"
{ pParse->BuildDataSourceForMySQL(&yymsp[0].minor.yy0);}
#line 2368 "parser.c"
        break;
      case 126: /* create_tablespace ::= CREATE TABLESPACE ifnotexists nm */
#line 582 "parser.y"
{
    pParse->BuildCreateTableSpace(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);
}
#line 2375 "parser.c"
        break;
      case 127: /* page_opt ::= PAGESIZE INTEGER ASKPAGE INTEGER STORAGE ids */
#line 586 "parser.y"
{
    pParse->AddTablespaceAttribute(&yymsp[-4].minor.yy0,&yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);
}
#line 2382 "parser.c"
        break;
      case 128: /* page_opt ::= PAGESIZE INTEGER ASKPAGE INTEGER */
#line 590 "parser.y"
{
    pParse->AddTablespaceAttribute(&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2389 "parser.c"
        break;
      case 129: /* page_opt ::= PAGESIZE INTEGER STORAGE ids */
#line 594 "parser.y"
{
    pParse->AddTablespaceAttribute(&yymsp[-2].minor.yy0,NULL,&yymsp[0].minor.yy0);
}
#line 2396 "parser.c"
        break;
      case 130: /* page_opt ::= PAGESIZE INTEGER */
#line 598 "parser.y"
{
    pParse->AddTablespaceAttribute(&yymsp[0].minor.yy0);
}
#line 2403 "parser.c"
        break;
      case 131: /* page_opt ::= ASKPAGE INTEGER */
#line 602 "parser.y"
{
    pParse->AddTablespaceAttribute(NULL,&yymsp[0].minor.yy0);
}
#line 2410 "parser.c"
        break;
      case 132: /* page_opt ::= ASKPAGE INTEGER STORAGE ids */
#line 606 "parser.y"
{
    pParse->AddTablespaceAttribute(NULL,&yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);
}
#line 2417 "parser.c"
        break;
      case 133: /* page_opt ::= STORAGE ids */
#line 610 "parser.y"
{
    pParse->AddTablespaceAttribute(NULL,NULL, &yymsp[0].minor.yy0);
}
#line 2424 "parser.c"
        break;
      case 134: /* page_opt ::= */
#line 614 "parser.y"
{
    pParse->AddTablespaceAttribute();
}
#line 2431 "parser.c"
        break;
      case 136: /* alter_tablespace ::= ALTER TABLESPACE ifexists nm */
#line 621 "parser.y"
{
    pParse->BuildAlterTableSpace(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);
}
#line 2438 "parser.c"
        break;
      case 137: /* alter_page_opt ::= PAGESIZE INTEGER ASKPAGE INTEGER STORAGE ids */
#line 625 "parser.y"
{
    pParse->ModifyTablespaceAttribute(&yymsp[-4].minor.yy0,&yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);
}
#line 2445 "parser.c"
        break;
      case 138: /* alter_page_opt ::= PAGESIZE INTEGER */
#line 629 "parser.y"
{
    pParse->ModifyTablespaceAttribute(&yymsp[0].minor.yy0);
}
#line 2452 "parser.c"
        break;
      case 139: /* alter_page_opt ::= ASKPAGE INTEGER */
#line 633 "parser.y"
{
    pParse->ModifyTablespaceAttribute(NULL,&yymsp[0].minor.yy0);
}
#line 2459 "parser.c"
        break;
      case 140: /* alter_page_opt ::= STORAGE ids */
#line 637 "parser.y"
{
    pParse->ModifyTablespaceAttribute(NULL,NULL,&yymsp[0].minor.yy0);
}
#line 2466 "parser.c"
        break;
      case 141: /* cmd ::= DROP TABLESPACE ifexists nm */
#line 643 "parser.y"
{
    pParse->BuildDropTableSpace(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0);
}
#line 2473 "parser.c"
        break;
      case 143: /* create_user ::= CREATE USER ifnotexists nm */
#line 650 "parser.y"
{
    pParse->BuildUser(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0,TK_CREATE);
}
#line 2480 "parser.c"
        break;
      case 144: /* access_opt ::= IDENTIFIED BY nm ACCESSED BY nm */
#line 654 "parser.y"
{
    pParse->AddUserAttribute(&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2487 "parser.c"
        break;
      case 145: /* access_opt ::= IDENTIFIED BY nm */
#line 658 "parser.y"
{
    pParse->AddUserAttribute(&yymsp[0].minor.yy0);
}
#line 2494 "parser.c"
        break;
      case 147: /* alter_user ::= ALTER USER ifexists nm */
#line 665 "parser.y"
{
    pParse->BuildUser(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0,TK_ALTER);
}
#line 2501 "parser.c"
        break;
      case 148: /* alter_access_opt ::= IDENTIFIED BY nm ACCESSED BY nm */
#line 669 "parser.y"
{
    pParse->ModifyUserAttribute(&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2508 "parser.c"
        break;
      case 149: /* alter_access_opt ::= IDENTIFIED BY nm */
#line 673 "parser.y"
{
    pParse->ModifyUserAttribute(&yymsp[0].minor.yy0);
}
#line 2515 "parser.c"
        break;
      case 150: /* alter_access_opt ::= ACCESSED BY nm */
#line 677 "parser.y"
{
    pParse->ModifyUserAttribute(NULL,&yymsp[0].minor.yy0);
}
#line 2522 "parser.c"
        break;
      case 151: /* cmd ::= DROP USER ifexists nm */
#line 683 "parser.y"
{ pParse->BuildUser(yymsp[-1].minor.yy304,&yymsp[0].minor.yy0,TK_DROP);}
#line 2527 "parser.c"
        break;
      case 153: /* alter_table_addAttr ::= ALTER TABLE ifnotexists nm ADD TABLESYS */
#line 689 "parser.y"
{
    pParse->BuildModifyTableAttribute(yymsp[-3].minor.yy304,&yymsp[-2].minor.yy0,"add");
}
#line 2534 "parser.c"
        break;
      case 157: /* alter_table_dropAttr ::= ALTER TABLE ifnotexists nm DROP TABLESYS */
#line 706 "parser.y"
{
	pParse->BuildModifyTableAttribute(yymsp[-3].minor.yy304,&yymsp[-2].minor.yy0,"drop");
}
#line 2541 "parser.c"
        break;
      case 158: /* drop_attr ::= nm */
#line 711 "parser.y"
{
	pParse->DropTableAttribute(&yymsp[0].minor.yy0);
}
#line 2548 "parser.c"
        break;
      case 160: /* alter_table_modifAttr ::= ALTER TABLE ifnotexists nm MODIFY TABLESYS */
#line 718 "parser.y"
{
	pParse->BuildModifyTableAttribute(yymsp[-3].minor.yy304,&yymsp[-2].minor.yy0,"modify");
}
#line 2555 "parser.c"
        break;
      case 161: /* modifyattr ::= id LP INTEGER RP */
      case 162: /* modifyattr ::= id LP ids RP */ yytestcase(yyruleno==162);
#line 723 "parser.y"
{
	pParse->ModifyTableAttribute(&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0);
}
#line 2563 "parser.c"
        break;
      case 164: /* alter_table_addcolum ::= ALTER TABLE ifnotexists nm ADD COLUMN */
#line 736 "parser.y"
{
	pParse->BuildAddTableColumn(yymsp[-3].minor.yy304,&yymsp[-2].minor.yy0);
}
#line 2570 "parser.c"
        break;
      case 166: /* cmd ::= ALTER TABLE ifnotexists nm DROP COLUMN nm */
#line 744 "parser.y"
{
	pParse->BuildDropTableColumn(yymsp[-4].minor.yy304,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2577 "parser.c"
        break;
      case 168: /* alter_table_modifycolum ::= ALTER TABLE ifnotexists nm MODIFY COLUMN */
#line 751 "parser.y"
{
	pParse->BuildModifyTableColumn(yymsp[-3].minor.yy304,&yymsp[-2].minor.yy0);
}
#line 2584 "parser.c"
        break;
      case 170: /* modifycolumid ::= nm */
#line 756 "parser.y"
{
    pParse->ModifyColumn(&yymsp[0].minor.yy0);
}
#line 2591 "parser.c"
        break;
      case 173: /* tokentype ::= ids */
#line 763 "parser.y"
{	pParse->ModifyColumnAttribute("data-type",&yymsp[0].minor.yy0);}
#line 2596 "parser.c"
        break;
      case 174: /* tokentype ::= ids LP INTEGER RP */
#line 764 "parser.y"
{
	pParse->ModifyColumnAttribute("data-type",&yymsp[-3].minor.yy0);
	pParse->ModifyColumnAttribute("data-len",&yymsp[-1].minor.yy0);
}
#line 2604 "parser.c"
        break;
      case 179: /* cons ::= NULL */
#line 775 "parser.y"
{ pParse->ModifyColumnNULLAttribute(1);}
#line 2609 "parser.c"
        break;
      case 180: /* cons ::= NOT NULL */
#line 776 "parser.y"
{pParse->ModifyColumnNULLAttribute(0);}
#line 2614 "parser.c"
        break;
      case 181: /* cons ::= REPTYPE LP ids RP */
#line 777 "parser.y"
{ pParse->ModifyColumnAttribute("rep-type",&yymsp[-1].minor.yy0);}
#line 2619 "parser.c"
        break;
      case 182: /* cons ::= DEFAULT ids */
#line 778 "parser.y"
{  pParse->ModifyColumnAttribute("default-value",&yymsp[0].minor.yy0);}
#line 2624 "parser.c"
        break;
      case 183: /* cons ::= DEFAULT INTEGER */
#line 779 "parser.y"
{ pParse->ModifyColumnAttribute("default-value",&yymsp[0].minor.yy0);}
#line 2629 "parser.c"
        break;
      case 184: /* cons ::= DEFAULT PLUS INTEGER */
#line 782 "parser.y"
{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER,0,0,&yymsp[0].minor.yy0);
	ST_EXPR * temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_UPLUS, temp, 0, 0);
	struct _Token str;
	ST_EXPR *  pstExpr= temp2;
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] ={0};
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue);
	str.z = value;
	str.n =strlen(value);
	pParse->ModifyColumnAttribute("default-value",&str);

}
#line 2646 "parser.c"
        break;
      case 185: /* cons ::= DEFAULT MINUS INTEGER */
#line 796 "parser.y"
{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER,0,0,&yymsp[0].minor.yy0);
	ST_EXPR * temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_UMINUS, temp, 0, 0);
	struct _Token str; 
	ST_EXPR *  pstExpr= temp2; 
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] = {0}; 
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue); 
	str.z = value; 
	str.n =strlen(value); 
	pParse->ModifyColumnAttribute("default-value",&str);

}
#line 2663 "parser.c"
        break;
      case 186: /* cons ::= DEFAULT LP expr RP */
#line 811 "parser.y"
{
	struct _Token str;
	ST_EXPR *  pstExpr= yymsp[-1].minor.yy175.pExpr;
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] = {0};
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue);
	str.z = value;
	str.n =strlen(value);
	pParse->ModifyColumnAttribute("default-value",&str);

}
#line 2678 "parser.c"
        break;
      case 187: /* cmd ::= LOAD DATA TABLE nm */
#line 825 "parser.y"
{
	pParse->BuildLoadData(&yymsp[0].minor.yy0);
}
#line 2685 "parser.c"
        break;
      case 189: /* add_seq ::= ADD SEQUENCE */
#line 832 "parser.y"
{
    pParse->BuildAddSequence();
}
#line 2692 "parser.c"
        break;
      case 192: /* alter_seq ::= ALTER SEQUENCE */
#line 843 "parser.y"
{
    pParse->BuildAlterSequence();
}
#line 2699 "parser.c"
        break;
      case 195: /* del_seq ::= DROP SEQUENCE */
#line 853 "parser.y"
{
    pParse->BuildDelSequence();
}
#line 2706 "parser.c"
        break;
      case 199: /* seqcolumn ::= nm LP ids RP */
      case 200: /* seqcolumn ::= nm LP INTEGER RP */ yytestcase(yyruleno==200);
#line 864 "parser.y"
{
    pParse->AddSequenceValue(&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0);
}
#line 2714 "parser.c"
        break;
      case 202: /* add_job ::= CREATE JOB nm */
#line 875 "parser.y"
{
    pParse->BuildCreateJob(&yymsp[0].minor.yy0);
}
#line 2721 "parser.c"
        break;
      case 205: /* alter_job ::= ALTER JOB nm */
#line 885 "parser.y"
{
    pParse->BuildAlterJob(&yymsp[0].minor.yy0);
}
#line 2728 "parser.c"
        break;
      case 207: /* cmd ::= REMOVE JOB nm */
#line 894 "parser.y"
{
    pParse->BuildRemoveJob(&yymsp[0].minor.yy0);
}
#line 2735 "parser.c"
        break;
      case 210: /* jobcolumn ::= nm LP ids RP */
      case 211: /* jobcolumn ::= nm LP INTEGER RP */ yytestcase(yyruleno==211);
#line 901 "parser.y"
{
    pParse->AddJobValue(&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0);
}
#line 2743 "parser.c"
        break;
      case 213: /* alter_flushsql_addparam ::= ALTER TABLE ifnotexists nm ADD PARAMETER nm */
#line 912 "parser.y"
{
	pParse->BuildAddFlushSQLParam(yymsp[-4].minor.yy304,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2750 "parser.c"
        break;
      case 217: /* atokentype ::= ids */
#line 920 "parser.y"
{	pParse->AddFlushSQLorLoadSQLParamAttr("data-type",&yymsp[0].minor.yy0);}
#line 2755 "parser.c"
        break;
      case 221: /* cons ::= VALUES LP ids RP */
#line 926 "parser.y"
{ pParse->AddFlushSQLorLoadSQLParamAttr("value",&yymsp[-1].minor.yy0);}
#line 2760 "parser.c"
        break;
      case 222: /* cons ::= PARAMTYPE LP ids RP */
#line 927 "parser.y"
{ pParse->AddFlushSQLorLoadSQLParamAttr("parameter-type",&yymsp[-1].minor.yy0);}
#line 2765 "parser.c"
        break;
      case 224: /* alter_flushsql_modifyparam ::= ALTER TABLE ifnotexists nm MODIFY PARAMETER nm */
#line 932 "parser.y"
{
	pParse->BuildModifyFlushSQLParam(yymsp[-4].minor.yy304,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0,"modify");
}
#line 2772 "parser.c"
        break;
      case 228: /* mtokentype ::= ids */
#line 940 "parser.y"
{	pParse->ModifyFlushSQLorLoadSQLParamAttr("data-type",&yymsp[0].minor.yy0);}
#line 2777 "parser.c"
        break;
      case 232: /* mcons ::= VALUES LP ids RP */
#line 946 "parser.y"
{ pParse->ModifyFlushSQLorLoadSQLParamAttr("value",&yymsp[-1].minor.yy0);}
#line 2782 "parser.c"
        break;
      case 233: /* mcons ::= PARAMTYPE LP ids RP */
#line 947 "parser.y"
{ pParse->ModifyFlushSQLorLoadSQLParamAttr("parameter-type",&yymsp[-1].minor.yy0);}
#line 2787 "parser.c"
        break;
      case 234: /* cmd ::= ALTER TABLE ifnotexists nm DROP PARAMETER nm */
#line 951 "parser.y"
{
	pParse->BuildModifyFlushSQLParam(yymsp[-4].minor.yy304,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0,"drop");
}
#line 2794 "parser.c"
        break;
      case 235: /* cmd ::= RENAME nm TO nm */
#line 957 "parser.y"
{
    pParse->BuildRenameTable(&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2801 "parser.c"
        break;
      case 236: /* expr ::= term */
#line 990 "parser.y"
{yygotominor.yy175=yymsp[0].minor.yy175;}
#line 2806 "parser.c"
        break;
      case 237: /* expr ::= LP expr RP */
#line 991 "parser.y"
{yygotominor.yy175.pExpr=yymsp[-1].minor.yy175.pExpr;spanSet(&yygotominor.yy175,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0);}
#line 2811 "parser.c"
        break;
      case 238: /* term ::= NULL */
#line 992 "parser.y"
{spanExpr(pParse,&yygotominor.yy175,yymsp[0].major,&yymsp[0].minor.yy0);}
#line 2816 "parser.c"
        break;
      case 239: /* expr ::= id */
#line 993 "parser.y"
{spanExpr(pParse,&yygotominor.yy175,TK_ID,&yymsp[0].minor.yy0);}
#line 2821 "parser.c"
        break;
      case 240: /* expr ::= nm DOT nm */
#line 995 "parser.y"
{
  ST_EXPR *temp1 = pParse->m_tMdbExpr.BuildPExpr(TK_ID_TABLENAME, 0, 0, &yymsp[-2].minor.yy0);
  ST_EXPR *temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_ID, 0, 0, &yymsp[0].minor.yy0);
  yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildPExpr(TK_DOT, temp1, temp2, 0);
  spanSet(&yygotominor.yy175,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2831 "parser.c"
        break;
      case 241: /* expr ::= nm DOT NEXTVAL|CURRVAL */
#line 1003 "parser.y"
{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_STRING,0,0,&yymsp[-2].minor.yy0);
	ST_EXPR_LIST * plist = pParse->m_tMdbExpr.ExprListAppend(0,temp);
	yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildExprFunction(plist,&yymsp[0].minor.yy0);
	spanSet(&yygotominor.yy175,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2841 "parser.c"
        break;
      case 242: /* term ::= INTEGER|FLOAT|BLOB */
      case 243: /* term ::= STRING */ yytestcase(yyruleno==243);
#line 1010 "parser.y"
{spanExpr(pParse,&yygotominor.yy175, yymsp[0].major, &yymsp[0].minor.yy0);}
#line 2847 "parser.c"
        break;
      case 244: /* expr ::= VARIABLE */
#line 1013 "parser.y"
{
  spanExpr(pParse,&yygotominor.yy175, TK_VARIABLE, &yymsp[0].minor.yy0);
  spanSet(&yygotominor.yy175, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
}
#line 2855 "parser.c"
        break;
      case 245: /* expr ::= ID LP exprlist RP */
#line 1019 "parser.y"
{
  yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildExprFunction(yymsp[-1].minor.yy331, &yymsp[-3].minor.yy0);
  spanSet(&yygotominor.yy175,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
  
//  if( D && yygotominor.yy175.pExpr ){
//    yygotominor.yy175.pExpr->flags |= EP_Distinct;
//  }
  
}
#line 2868 "parser.c"
        break;
      case 246: /* expr ::= ID LP STAR RP */
#line 1029 "parser.y"
{
  yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildExprFunction( 0, &yymsp[-3].minor.yy0);
  spanSet(&yygotominor.yy175,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2876 "parser.c"
        break;
      case 247: /* term ::= SYSDATE */
#line 1035 "parser.y"
{
 //sysdate values are  treated as functions that return constants 
  yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildExprFunction(0,&yymsp[0].minor.yy0);
  if( yygotominor.yy175.pExpr ){
    yygotominor.yy175.pExpr->iOpcode = TK_CONST_FUNC;  
  }
  spanSet(&yygotominor.yy175, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
}
#line 2888 "parser.c"
        break;
      case 248: /* expr ::= expr AND expr */
      case 249: /* expr ::= expr OR expr */ yytestcase(yyruleno==249);
      case 250: /* expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==250);
      case 251: /* expr ::= expr EQ|NE expr */ yytestcase(yyruleno==251);
      case 252: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==252);
      case 253: /* expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==253);
      case 254: /* expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==254);
      case 255: /* expr ::= expr CONCAT expr */ yytestcase(yyruleno==255);
#line 1062 "parser.y"
{spanBinaryExpr(pParse,&yygotominor.yy175,yymsp[-1].major,&yymsp[-2].minor.yy175,&yymsp[0].minor.yy175);}
#line 2900 "parser.c"
        break;
      case 256: /* likeop ::= LIKE_KW */
      case 258: /* likeop ::= MATCH */ yytestcase(yyruleno==258);
#line 1073 "parser.y"
{yygotominor.yy393.eOperator = yymsp[0].minor.yy0; yygotominor.yy393.iNot = 0;}
#line 2906 "parser.c"
        break;
      case 257: /* likeop ::= NOT LIKE_KW */
      case 259: /* likeop ::= NOT MATCH */ yytestcase(yyruleno==259);
#line 1074 "parser.y"
{yygotominor.yy393.eOperator = yymsp[0].minor.yy0; yygotominor.yy393.iNot = 1;}
#line 2912 "parser.c"
        break;
      case 260: /* expr ::= expr likeop expr */
#line 1077 "parser.y"
{
  ST_EXPR_LIST  *pList;
  pList = pParse->m_tMdbExpr.ExprListAppend(0, yymsp[0].minor.yy175.pExpr);
  pList = pParse->m_tMdbExpr.ExprListAppend(pList, yymsp[-2].minor.yy175.pExpr);
  yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildExprFunction( pList, &yymsp[-1].minor.yy393.eOperator);
  if( yymsp[-1].minor.yy393.iNot ) yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildPExpr(TK_NOT, yygotominor.yy175.pExpr, 0, 0);
  yygotominor.yy175.zStart = yymsp[-2].minor.yy175.zStart;
  yygotominor.yy175.zEnd = yymsp[0].minor.yy175.zEnd;
  if( yygotominor.yy175.pExpr ) yygotominor.yy175.pExpr->flags |= EP_InfixFunc;
}
#line 2926 "parser.c"
        break;
      case 261: /* expr ::= expr ISNULL|NOTNULL */
#line 1106 "parser.y"
{spanUnaryPostfix(pParse,&yygotominor.yy175,yymsp[0].major,&yymsp[-1].minor.yy175,&yymsp[0].minor.yy0);}
#line 2931 "parser.c"
        break;
      case 262: /* expr ::= expr NOT NULL */
#line 1107 "parser.y"
{spanUnaryPostfix(pParse,&yygotominor.yy175,TK_NOTNULL,&yymsp[-2].minor.yy175,&yymsp[0].minor.yy0);}
#line 2936 "parser.c"
        break;
      case 263: /* expr ::= expr IS expr */
#line 1148 "parser.y"
{
  spanBinaryExpr(pParse,&yygotominor.yy175,TK_IS,&yymsp[-2].minor.yy175,&yymsp[0].minor.yy175);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy175.pExpr, yygotominor.yy175.pExpr, TK_ISNULL);
}
#line 2944 "parser.c"
        break;
      case 264: /* expr ::= expr IS NOT expr */
#line 1152 "parser.y"
{
  spanBinaryExpr(pParse,&yygotominor.yy175,TK_ISNOT,&yymsp[-3].minor.yy175,&yymsp[0].minor.yy175);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy175.pExpr, yygotominor.yy175.pExpr, TK_NOTNULL);
}
#line 2952 "parser.c"
        break;
      case 269: /* expr ::= NOT expr */
      case 270: /* expr ::= BITNOT expr */ yytestcase(yyruleno==270);
#line 1174 "parser.y"
{spanUnaryPrefix(pParse,&yygotominor.yy175,yymsp[-1].major,&yymsp[0].minor.yy175,&yymsp[-1].minor.yy0);}
#line 2958 "parser.c"
        break;
      case 271: /* expr ::= MINUS expr */
#line 1176 "parser.y"
{spanUnaryPrefix(pParse,&yygotominor.yy175,TK_UMINUS,&yymsp[0].minor.yy175,&yymsp[-1].minor.yy0);}
#line 2963 "parser.c"
        break;
      case 272: /* expr ::= PLUS expr */
#line 1177 "parser.y"
{spanUnaryPrefix(pParse,&yygotominor.yy175,TK_UPLUS,&yymsp[0].minor.yy175,&yymsp[-1].minor.yy0);}
#line 2968 "parser.c"
        break;
      case 275: /* expr ::= expr between_op expr AND expr */
#line 1185 "parser.y"
{
  ST_EXPR_LIST *pList = pParse->m_tMdbExpr.ExprListAppend(0, yymsp[-4].minor.yy175.pExpr);
  pList = pParse->m_tMdbExpr.ExprListAppend(pList, yymsp[-2].minor.yy175.pExpr);
  pList = pParse->m_tMdbExpr.ExprListAppend(pList, yymsp[0].minor.yy175.pExpr);
  yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildPExpr( TK_BETWEEN, 0, 0, 0);
  yygotominor.yy175.pExpr->pFunc = QMDB_MALLOC->AllocExprFunc();
  if( yygotominor.yy175.pExpr ){
     yygotominor.yy175.pExpr->pFunc->pFuncArgs = pList;
  }else{
	QMDB_MALLOC->ReleaseExprList(pList);
  } 
  if( yymsp[-3].minor.yy304 ) yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildPExpr(TK_NOT, yygotominor.yy175.pExpr, 0, 0);
  yygotominor.yy175.zStart = yymsp[-4].minor.yy175.zStart;
  yygotominor.yy175.zEnd = yymsp[0].minor.yy175.zEnd;
}
#line 2987 "parser.c"
        break;
      case 278: /* expr ::= expr in_op LP exprlist RP */
#line 1211 "parser.y"
{
    if( yymsp[-1].minor.yy331==0 ){
      /* Expressions of the form
      **
      **      expr1 IN ()
      **      expr1 NOT IN ()
      **
      ** simplify to constants 0 (false) and 1 (true), respectively,
      ** regardless of the value of expr1.
      */
      yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildPExpr( TK_INTEGER, 0, 0, &mdbIntTokens[yymsp[-3].minor.yy304]);
    }else{
      yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildPExpr( TK_IN, yymsp[-4].minor.yy175.pExpr, 0, &mdbFuncInToken);
        yygotominor.yy175.pExpr->pFunc = QMDB_MALLOC->AllocExprFunc();
      if( yygotominor.yy175.pExpr ){
        yygotominor.yy175.pExpr->pFunc->pFuncArgs = yymsp[-1].minor.yy331;
      }else{
      }
      if( yymsp[-3].minor.yy304 ) yygotominor.yy175.pExpr = pParse->m_tMdbExpr.BuildPExpr(TK_NOT, yygotominor.yy175.pExpr, 0, 0);
    }
    yygotominor.yy175.zStart = yymsp[-4].minor.yy175.zStart;
    yygotominor.yy175.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3014 "parser.c"
        break;
      default:
      /* (0) input ::= cmdlist */ yytestcase(yyruleno==0);
      /* (1) cmdlist ::= cmd ecmd */ yytestcase(yyruleno==1);
      /* (2) cmdlist ::= ecmd */ yytestcase(yyruleno==2);
      /* (3) ecmd ::= SEMI */ yytestcase(yyruleno==3);
      /* (12) cmd ::= create_table create_table_args */ yytestcase(yyruleno==12);
      /* (15) columnlist ::= columnlist COMMA column */ yytestcase(yyruleno==15);
      /* (16) columnlist ::= column */ yytestcase(yyruleno==16);
      /* (19) type ::= */ yytestcase(yyruleno==19);
      /* (20) type ::= typetoken */ yytestcase(yyruleno==20);
      /* (23) carglist ::= carglist carg */ yytestcase(yyruleno==23);
      /* (24) carglist ::= */ yytestcase(yyruleno==24);
      /* (25) carg ::= CONSTRAINT nm ccons */ yytestcase(yyruleno==25);
      /* (26) carg ::= ccons */ yytestcase(yyruleno==26);
      /* (37) conslist ::= conslist COMMA tcons */ yytestcase(yyruleno==37);
      /* (38) conslist ::= conslist tcons */ yytestcase(yyruleno==38);
      /* (39) conslist ::= tcons */ yytestcase(yyruleno==39);
      /* (40) tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==40);
      /* (44) propertylist ::= propertylist COMMA tableproperty */ yytestcase(yyruleno==44);
      /* (45) propertylist ::= tableproperty */ yytestcase(yyruleno==45);
      /* (49) cmd ::= create_index */ yytestcase(yyruleno==49);
      /* (112) cmd ::= create_database create_database_args */ yytestcase(yyruleno==112);
      /* (115) cmd ::= alter_database alter_database_args */ yytestcase(yyruleno==115);
      /* (118) syscolumnlist ::= syscolumnlist COMMA syscolumn */ yytestcase(yyruleno==118);
      /* (119) syscolumnlist ::= syscolumn */ yytestcase(yyruleno==119);
      /* (125) cmd ::= create_tablespace page_opt */ yytestcase(yyruleno==125);
      /* (135) cmd ::= alter_tablespace alter_page_opt */ yytestcase(yyruleno==135);
      /* (142) cmd ::= create_user access_opt */ yytestcase(yyruleno==142);
      /* (146) cmd ::= alter_user alter_access_opt */ yytestcase(yyruleno==146);
      /* (152) cmd ::= alter_table_addAttr add_attr */ yytestcase(yyruleno==152);
      /* (156) cmd ::= alter_table_dropAttr drop_attr */ yytestcase(yyruleno==156);
      /* (159) cmd ::= alter_table_modifAttr modifyattr */ yytestcase(yyruleno==159);
      /* (163) cmd ::= alter_table_addcolum table_column */ yytestcase(yyruleno==163);
      /* (165) table_column ::= columnid type carglist */ yytestcase(yyruleno==165);
      /* (167) cmd ::= alter_table_modifycolum table_mcolumn */ yytestcase(yyruleno==167);
      /* (169) table_mcolumn ::= modifycolumid ctype carglists */ yytestcase(yyruleno==169);
      /* (171) ctype ::= */ yytestcase(yyruleno==171);
      /* (172) ctype ::= tokentype */ yytestcase(yyruleno==172);
      /* (175) carglists ::= carglists cargs */ yytestcase(yyruleno==175);
      /* (176) carglists ::= */ yytestcase(yyruleno==176);
      /* (177) cargs ::= CONSTRAINT nm cons */ yytestcase(yyruleno==177);
      /* (178) cargs ::= cons */ yytestcase(yyruleno==178);
      /* (188) cmd ::= add_seq add_seq_args */ yytestcase(yyruleno==188);
      /* (191) cmd ::= alter_seq alter_seq_args */ yytestcase(yyruleno==191);
      /* (194) cmd ::= del_seq del_seq_args */ yytestcase(yyruleno==194);
      /* (197) seqcolumnlist ::= seqcolumnlist COMMA seqcolumn */ yytestcase(yyruleno==197);
      /* (198) seqcolumnlist ::= seqcolumn */ yytestcase(yyruleno==198);
      /* (201) cmd ::= add_job add_job_args */ yytestcase(yyruleno==201);
      /* (204) cmd ::= alter_job alter_job_args */ yytestcase(yyruleno==204);
      /* (208) jobcolumnlist ::= jobcolumnlist COMMA jobcolumn */ yytestcase(yyruleno==208);
      /* (209) jobcolumnlist ::= jobcolumn */ yytestcase(yyruleno==209);
      /* (212) cmd ::= alter_flushsql_addparam flushsql_param */ yytestcase(yyruleno==212);
      /* (214) flushsql_param ::= data_type parglists */ yytestcase(yyruleno==214);
      /* (215) data_type ::= */ yytestcase(yyruleno==215);
      /* (216) data_type ::= atokentype */ yytestcase(yyruleno==216);
      /* (218) parglists ::= parglists pargs */ yytestcase(yyruleno==218);
      /* (219) parglists ::= */ yytestcase(yyruleno==219);
      /* (220) pargs ::= cons */ yytestcase(yyruleno==220);
      /* (223) cmd ::= alter_flushsql_modifyparam flushsql_mparam */ yytestcase(yyruleno==223);
      /* (225) flushsql_mparam ::= mdata_type mparglists */ yytestcase(yyruleno==225);
      /* (226) mdata_type ::= */ yytestcase(yyruleno==226);
      /* (227) mdata_type ::= mtokentype */ yytestcase(yyruleno==227);
      /* (229) mparglists ::= mparglists mpargs */ yytestcase(yyruleno==229);
      /* (230) mparglists ::= */ yytestcase(yyruleno==230);
      /* (231) mpargs ::= mcons */ yytestcase(yyruleno==231);
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  mdbParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  mdbParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  mdbParserARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 8 "parser.y"

	pParse->m_tError.FillErrMsg(ERR_SQL_INVALID,"syntax_error");
#line 3143 "parser.c"
  mdbParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  mdbParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  mdbParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "mdbParserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void mdbParser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  mdbParserTOKENTYPE yyminor       /* The value for the token */
  mdbParserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  mdbParserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor,yyminorunion);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
