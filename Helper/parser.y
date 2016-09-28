%token_prefix TK_

%token_type{Token}
%default_type{Token}

%extra_argument{TMdbSqlParser *pParse}

%syntax_error {
	pParse->m_tError.FillErrMsg(ERR_SQL_INVALID,"syntax_error");
}
%stack_overflow {
	pParse->m_tError.FillErrMsg(ERR_OS_NO_MEMROY,"overflow");
}


%name mdbParser

%include{
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
}





// The following directive causes tokens ABORT, AFTER, ASC, etc. to
// fallback to ID if they will not parse as their original value.
// This obviates the need for the "id" nonterminal.
//
%fallback SELECT UPDATE DELETE INSERT ID
 ASC    BY   DESC NO KEY  OFFSET SYSDATE IF 
  .
%wildcard ANY.

// Define operator precedence early so that this is the first occurance
// of the operator tokens in the grammer.  Keeping the operators together
// causes them to be assigned integer values that are close together,
// which keeps parser tables smaller.
//
// The token values assigned to these symbols is determined by the order
// in which lemon first sees them.  It must be the case that ISNULL/NOTNULL,
// NE/EQ, GT/LE, and GE/LT are separated by only a single value.  See
// the sqlite3ExprIfFalse() routine for additional information on this
// constraint.
//
%left OR.
%left AND.
%right NOT.
%left IS MATCH LIKE_KW BETWEEN IN ISNULL NOTNULL NE EQ.
%left GT LE LT GE.
%right ESCAPE.
%left BITAND BITOR LSHIFT RSHIFT.
%left PLUS MINUS.
%left STAR SLASH REM.
%left CONCAT.
%left COLLATE.
%right BITNOT.

input ::= cmdlist. //输入由一堆命令组成
cmdlist ::= cmd ecmd.//命令集是由命令+终结命令组成
cmdlist ::= ecmd.//命令集也有可能只有终结命令
ecmd ::= SEMI.  //终结命令是;

// An IDENTIFIER can be a generic identifier, or one of several
// keywords.  Any non-standard keyword can also be an identifier.
//标识符可以标识一些关键字或者非关键字

%type id {Token}
id(A) ::= ID(X).         {A = X;}

// And "ids" is an identifer-or-string.
//ids可能是ID或者字串
%type ids {Token}
ids(A) ::= ID|STRING(X).   {A = X;}

// The name of a column or table can be any of the following:
//一个列或表的名字
%type nm {Token}
nm(A) ::= id(X).         {A = X;}
nm(A) ::= STRING(X).     {A = X;}



%type ifnotexists {int}
ifnotexists(A) ::= .              {A = 0;}
ifnotexists(A) ::= IF NOT EXISTS. {A = 1;}

%type ifexists {int}
ifexists(A) ::= IF EXISTS.   {A = 1;}
ifexists(A) ::= .            {A = 0;}


///////////////////// The CREATE TABLE statement ////////////////////////////
//
cmd ::= create_table create_table_args.
create_table ::= CREATE TABLE  ifnotexists(E) nm(Y).{
	pParse->BuildCreateTable(E,&Y);
}
create_table_args ::= LP columnlist conslist_opt RP propertylist.{
	
}

columnlist ::= columnlist COMMA column.
columnlist ::= column.

// A "column" is a complete description of a single column in a
// CREATE TABLE statement.  This includes the column name, its
// datatype, and other keywords such as PRIMARY KEY, UNIQUE, REFERENCES,
// NOT NULL and so forth.
//
column::= columnid type carglist. {
}
columnid::= nm(X). {
	pParse->AddColumn(&X);
}

// A typetoken is really one or more tokens that form a type name such
// as can be found after the column name in a CREATE TABLE statement.
// Multiple tokens are concatenated to form the value of the typetoken.
//
%type typetoken {Token}
type ::= .
type ::= typetoken.                   {}
typetoken ::= ids(X).   {	pParse->AddColumnAttribute("data-type",&X);}
typetoken::= ids(X) LP INTEGER(Z) RP. {
	pParse->AddColumnAttribute("data-type",&X);
	pParse->AddColumnAttribute("data-len",&Z);
}

carglist ::= carglist carg.
carglist ::= .

carg ::= CONSTRAINT nm ccons.
carg ::= ccons.
ccons ::= DEFAULT ids(X).       {  pParse->AddColumnAttribute("default-value",&X);}
ccons ::= DEFAULT INTEGER(X).{ pParse->AddColumnAttribute("default-value",&X);}

ccons ::= DEFAULT PLUS  INTEGER(X).
{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER,0,0,&X);
	ST_EXPR * temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_UPLUS, temp, 0, 0);
	struct _Token str;
	ST_EXPR *  pstExpr= temp2;
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] ={0};
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue);
	str.z = value;
	str.n =(unsigned int)(strlen(value));
	pParse->AddColumnAttribute("default-value",&str);

}
ccons ::= DEFAULT MINUS INTEGER(X).
{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER,0,0,&X);
	ST_EXPR * temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_UMINUS, temp, 0, 0);
	struct _Token str; 
	ST_EXPR *  pstExpr= temp2; 
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] = {0}; 
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue); 
	str.z = value; 
	str.n =(unsigned int)(strlen(value)); 
	pParse->AddColumnAttribute("default-value",&str);

}

ccons ::= DEFAULT LP expr(X) RP.
{
	struct _Token str;
	ST_EXPR *  pstExpr= X.pExpr;
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] = {0};
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue);
	str.z = value;
	str.n =(unsigned int)(strlen(value));
	pParse->AddColumnAttribute("default-value",&str);

}
ccons ::= NULL.                 { pParse->AddColumnNULLAttribute();}
ccons ::= NOT NULL.{
}
ccons ::= REPTYPE LP ids(X) RP.  { pParse->AddColumnAttribute("rep-type",&X);}
  
// For the time being, the only constraint we care about is the primary
// key and UNIQUE.  Both create indices.
//
conslist_opt(A) ::= .                   {A.n = 0; A.z = 0;}
conslist_opt(A) ::= COMMA(X) conslist.  {A = X;}
conslist ::= conslist COMMA tcons.
conslist ::= conslist tcons.
conslist ::= tcons.
tcons ::= CONSTRAINT nm.
tcons ::= PRIMARY KEY LP idxlist(X) RP. {
	pParse->AddTablePrimayKey(X);
}

tcons ::= id(X) LP INTEGER(Y) RP.
{
	pParse->AddTableAttribute(&X,&Y);
}

tcons ::= id(X) LP ids(Y) RP.
{
	pParse->AddTableAttribute(&X,&Y);
}

propertylist ::= propertylist COMMA tableproperty.
propertylist ::= tableproperty.

tableproperty ::= id(X) EQ ids(Y). {

    pParse->AddTableAttribute(&X,&Y);
}

////////////////////////// The DROP TABLE /////////////////////////////////////
//
cmd ::= DROP TABLE ifexists(E) nm(X). {
    pParse->BuildDropTable(E,&X);
}

////////////////////////// The TRUNCATE TABLE /////////////////////////////////////
//
cmd ::= TRUNCATE TABLE ifexists(E) nm(X). {
    pParse->BuildTruncateTable(E,&X);
}

///////////////////////////// The CREATE INDEX command ///////////////////////
//
cmd ::= create_index .

create_index ::= CREATE INDEX ifnotexists(NE) nm(X) ON nm(Y) LP idxlist(Z) RP nm(E) INTEGER(F) . {
	pParse->BuildCreateIndex(NE,&X,&Y,Z,&E,&F);
}

create_index ::= CREATE INDEX ifnotexists(NE) nm(X) ON nm(Y) LP idxlist(Z) RP nm(E) . {
	pParse->BuildCreateIndex(NE,&X,&Y,Z,&E);
}

create_index ::= CREATE INDEX ifnotexists(NE) nm(X) ON nm(Y) LP idxlist(Z) RP . {
	pParse->BuildCreateIndex(NE,&X,&Y,Z,NULL);
}

%type idxlist {ST_ID_LIST*}
%destructor idxlist {QMDB_MALLOC->ReleaseIdList($$);}

idxlist(A) ::= idxlist(X) COMMA nm(Y).  {
    {A = pParse->IdListAppend(X,&Y);}
}
idxlist(A) ::= nm(Y). {
   {A = pParse->IdListAppend(0,&Y);}
}


///////////////////////////// The DROP INDEX command /////////////////////////
//
cmd ::= DROP INDEX ifexists(E) nm(X) ON nm(Y).   { pParse->BuildDropIndex(E,&X,&Y);}

///////////////////////////// The CREATE PRIMARY command ///////////////////////
//
cmd ::= ALTER TABLE ifnotexists(NE) nm(X) ADD PRIMARY KEY LP pkidxlist(Z) RP. {
	pParse->BuildCreatePrimKey(NE,&X,Z);
}

%type pkidxlist {ST_ID_LIST*}
%destructor pkidxlist {QMDB_MALLOC->ReleaseIdList($$);}

pkidxlist(A) ::= pkidxlist(X) COMMA nm(Y).  {
    {A = pParse->IdListAppend(X,&Y);}
}

pkidxlist(A) ::= nm(Y). {
   {A = pParse->IdListAppend(0,&Y);}
}


////////////////////////// The INSERT command /////////////////////////////////
//
cmd ::= INSERT INTO tablename inscollist_opt(F) VALUES LP itemlist(Y) RP.
            {pParse->BuildInsert(F,Y);}


%type itemlist {ST_EXPR_LIST*}
%destructor itemlist {QMDB_MALLOC->ReleaseExprList($$);}

itemlist(A) ::= itemlist(X) COMMA expr(Y).
    {A = pParse->m_tMdbExpr.ExprListAppend(X,Y.pExpr);}
itemlist(A) ::= expr(X).
    {A = pParse->m_tMdbExpr.ExprListAppend(0,X.pExpr);}

%type inscollist_opt {ST_ID_LIST *}
%destructor inscollist_opt {QMDB_MALLOC->ReleaseIdList($$);}
%type inscollist {ST_ID_LIST*}
%destructor inscollist {QMDB_MALLOC->ReleaseIdList($$);}

//inscollist_opt(A) ::= .                       {A = 0;}
inscollist_opt(A) ::= LP inscollist(X) RP.    {A = X;}
inscollist(A) ::= inscollist(X) COMMA nm(Y).
    {A = pParse->IdListAppend(X,&Y);}
inscollist(A) ::= nm(Y).
    {A = pParse->IdListAppend(0,&Y);}



/////////////////////////// The DELETE statement /////////////////////////////
//
cmd ::= DELETE hint(H) first(F) FROM tablename where_opt(W)  limit_opt(L). {
  pParse->BuildDelete(H,F,W,L.pLimit,L.pOffset);
}



////////////////////////// The UPDATE command ////////////////////////////////
//

cmd ::= UPDATE  hint(H) first(F) tablename SET setlist(Y) where_opt(W)  limit_opt(L).  {
  pParse->BuildUpdate(H,F,Y,W,L.pLimit,L.pOffset);
}


%type setlist {ST_SET_LIST}
%destructor setlist {QMDB_MALLOC->ReleaseIdList($$.pIdList);QMDB_MALLOC->ReleaseExprList($$.pExprList);}

setlist(A) ::= setlist(Z) COMMA setnm(X) EQ expr(Y). {
  A.pExprList = pParse->m_tMdbExpr.ExprListAppend( Z.pExprList, Y.pExpr);
  pParse->m_tMdbExpr.ExprListSetName(A.pExprList, &X, 1);
  A.pIdList = pParse->IdListAppend(Z.pIdList ,&X);
}
setlist(A) ::= setnm(X) EQ expr(Y). {
  A.pExprList  = pParse->m_tMdbExpr.ExprListAppend( 0, Y.pExpr);
  pParse->m_tMdbExpr.ExprListSetName( A.pExprList, &X, 1);
  A.pIdList = pParse->IdListAppend(0,&X);
}

setnm(A) ::= nm(X).{
	A=X;
}
setnm(A) ::= nm(X) DOT nm(Y).{
	if(false == pParse->IsTableAlias(&X))
	{
		pParse->m_tError.FillErrMsg(ERR_SQL_INVALID,"table alias is error[%s]",X.z);
	}
	A=Y;
}

//////////////////////// The SELECT statement /////////////////////////////////
//select语句

%type select {ST_SQL_STRUCT *}
%destructor select {}
//一个select语句由SELECT关键字，distinct段，selcolist 段以及from段组成
cmd ::= SELECT hint(H) first(F) distinct(D)  selcollist(W) FROM tablename where_opt(Y) 
						groupby_opt(P) having_opt(Q) orderby_opt(Z) limit_opt(L). {
  pParse->BuildSelect(H,F,D,W,Y,P,Q,Z,L.pLimit,L.pOffset);//进入此规则后调用的处理逻辑
}

%type hint {ST_EXPR_LIST *}
%destructor hint {QMDB_MALLOC->ReleaseExprList($$);}

%type hintcollist {ST_EXPR_LIST *}
%destructor hintcollist {QMDB_MALLOC->ReleaseExprList($$);}

hint(A) ::=  SLASH STAR PLUS hintcollist(X) STAR SLASH.  {A=X;}
hint(A) ::= .{A=0;}

%type hintcolumn {ST_EXPR *}

hintcollist(A) ::= hintcolumn(Y).   {A = pParse->m_tMdbExpr.ExprListAppend(NULL, Y);}

hintcollist(A) ::= hintcollist(X) hintcolumn(Y).   {A = pParse->m_tMdbExpr.ExprListAppend(X, Y);}

hintcolumn(A) ::= INDEX LP nm(Y) RP.   {
	ST_EXPR *pRight = pParse->m_tMdbExpr.BuildPExpr( TK_INDEX, 0, 0, 0);
    ST_EXPR *pLeft = pParse->m_tMdbExpr.BuildPExpr(TK_STRING, 0, 0, &Y);
    A = pParse->m_tMdbExpr.BuildPExpr( TK_HINT, pLeft, pRight, 0);
}



%type first{ST_EXPR *}
first(A) ::=FIRST INTEGER(X).{
	A = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER, 0, 0, &X);
}
first(A) ::= .{A=0;}

%type distinct {int}
distinct(A) ::= DISTINCT.   {A = 1;}
distinct(A) ::= ALL.        {A = 0;}
distinct(A) ::= .           {A = 0;}

// selcollist is a list of expressions that are to become the return
// values of the SELECT statement.  The "*" in statements like
// "SELECT * FROM ..." is encoded as a special expression with an
// opcode of TK_ALL.
//
%type selcollist {ST_EXPR_LIST *}
%destructor selcollist {QMDB_MALLOC->ReleaseExprList($$);}

%type sclp {ST_EXPR_LIST*}
%destructor sclp {QMDB_MALLOC->ReleaseExprList($$);}

sclp(A) ::= selcollist(X) COMMA.             {A = X;}
sclp(A) ::= .                                {A = 0;}

selcollist(A) ::= sclp(P) expr(X) as(Y).     {
   A = pParse->m_tMdbExpr.ExprListAppend( P, X.pExpr);
   if( Y.n>0 ) pParse->m_tMdbExpr.ExprListSetName( A, &Y, 1);
   pParse->m_tMdbExpr.ExprListSetSpan(A,&X);
}

selcollist(A) ::= sclp(P) STAR. {
  ST_EXPR *p = pParse->m_tMdbExpr.BuildExpr( TK_ALL);
  A = pParse->m_tMdbExpr.ExprListAppend( P, p);
}

selcollist(A) ::= sclp(P) nm(X) DOT STAR(Y). {
  ST_EXPR *pRight = pParse->m_tMdbExpr.BuildPExpr( TK_ALL, 0, 0, &Y);
  ST_EXPR *pLeft = pParse->m_tMdbExpr.BuildPExpr(TK_ID_TABLENAME, 0, 0, &X);
  ST_EXPR *pDot = pParse->m_tMdbExpr.BuildPExpr( TK_DOT, pLeft, pRight, 0);
  A = pParse->m_tMdbExpr.ExprListAppend(P, pDot);
}


//tablename
tablename ::= nm(X).{
	pParse->FillTableName(&X,NULL); 
}
tablename ::= nm(X) nm(Y).{
	pParse->FillTableName(&X,&Y); 
}


// An option "AS <id>" phrase that can follow one of the expressions that
// define the result set, or one of the tables in the FROM clause.
//
%type as {Token}
as(X) ::= AS nm(Y).    {X = Y;}
as(X) ::= ids(Y).      {X = Y;}
as(X) ::= .            {X.n = 0;}



%type where_opt {ST_EXPR *}
%destructor where_opt {QMDB_MALLOC->ReleaseExpr($$);}

where_opt(A) ::= .                    {A = 0;}
where_opt(A) ::= WHERE expr(X).       {A = X.pExpr;}



%type groupby_opt {ST_EXPR_LIST*}
%destructor groupby_opt {QMDB_MALLOC->ReleaseExprList($$);}
groupby_opt(A) ::= .                      {A = 0;}
groupby_opt(A) ::= GROUP BY nexprlist(X). {A = X;}

%type having_opt {ST_EXPR*}
%destructor having_opt {QMDB_MALLOC->ReleaseExpr($$);}
having_opt(A) ::= .                {A = 0;}
having_opt(A) ::= HAVING expr(X).  {A = X.pExpr;}


//order by statement
%type orderby_opt {ST_EXPR_LIST *}
%destructor orderby_opt {QMDB_MALLOC->ReleaseExprList($$);}
%type sortlist {ST_EXPR_LIST *}
%destructor sortlist {QMDB_MALLOC->ReleaseExprList($$);}
%type sortitem {ST_EXPR*}
%destructor sortitem {QMDB_MALLOC->ReleaseExpr($$);}

orderby_opt(A) ::= .                          {A = 0;}
orderby_opt(A) ::= ORDER BY sortlist(X).      {A = X;}
sortlist(A) ::= sortlist(X) COMMA sortitem(Y) sortorder(Z). {
  A = pParse->m_tMdbExpr.ExprListAppend(X,Y);
  if(A) A->pExprItems[A->iItemNum-1].iSortOrder = Z;
}
sortlist(A) ::= sortitem(Y) sortorder(Z). {
  A = pParse->m_tMdbExpr.ExprListAppend(0,Y);
  if( A && (A->pExprItems) ) A->pExprItems[0].iSortOrder = Z;
}
sortitem(A) ::= expr(X).   {A = X.pExpr;}

%type sortorder {int}

sortorder(A) ::= ASC.           {A = MDB_SO_ASC;}
sortorder(A) ::= DESC.          {A = MDB_SO_DESC;}
sortorder(A) ::= .              {A = MDB_SO_ASC;}



%type limit_opt {struct LimitVal}

// The destructor for limit_opt will never fire in the current grammar.
// The limit_opt non-terminal only occurs at the end of a single production
// rule for SELECT statements.  As soon as the rule that create the 
// limit_opt non-terminal reduces, the SELECT statement rule will also
// reduce.  So there is never a limit_opt non-terminal on the stack 
// except as a transient.  So there is never anything to destroy.
//
//%destructor limit_opt {
//  sqlite3ExprDelete(pParse->db, $$.pLimit);
//  sqlite3ExprDelete(pParse->db, $$.pOffset);
//}
limit_opt(A) ::= .                    {A.pLimit = 0; A.pOffset = 0;}
limit_opt(A) ::= LIMIT expr(X).       {A.pLimit = X.pExpr; A.pOffset = 0;}
limit_opt(A) ::= LIMIT expr(X) OFFSET expr(Y). 
                                      {A.pLimit = X.pExpr; A.pOffset = Y.pExpr;}
limit_opt(A) ::= LIMIT expr(X) COMMA expr(Y). 
                                      {A.pOffset = X.pExpr; A.pLimit = Y.pExpr;}

///////////////////////////// The USE DSN command /////////////////////////
//
cmd ::= USE DATABASE ifexists(E) nm(X).   {pParse->BuildUseDsn(E,&X);}


///////////////////////////// The USE TABLESPACE command /////////////////////////
//
cmd ::= USE TABLESPACE ifexists(E) nm(X).   {pParse->BuildUseTablespace(E,&X);}


///////////////////////////// The CREATE DSN command /////////////////////////
//
cmd ::= create_database create_database_args.
create_database ::= CREATE DATABASE  ifnotexists(E) nm(Y).{
    pParse->BuildCreateDsn(E,&Y);
}

create_database_args ::= LP syscolumnlist RP.{
	
}

///////////////////////////// The ALTER DSN command /////////////////////////
//
cmd ::= alter_database alter_database_args.
alter_database ::= ALTER DATABASE  ifexists(E) nm(Y).{
    pParse->BuildAlterDsn(E,&Y);
}

alter_database_args ::= LP syscolumnlist RP.{
	
}

syscolumnlist ::= syscolumnlist COMMA syscolumn.
syscolumnlist ::= syscolumn.
    
syscolumn::= nm(X) LP ids(Y) RP. {
    pParse->AddDsnAttribute(&X,&Y);
}

syscolumn::= nm(X) LP INTEGER(Z) RP. {
    pParse->AddDsnAttribute(&X,&Z);
}

///////////////////////////// The DROP DSN command /////////////////////////
//
cmd ::= DROP DATABASE ifexists(E) nm(X).   { pParse->BuildDropDsn(E,&X);}

///////////////////////////// The CREATE SOURCE DATA command /////////////////////////
//
cmd ::= CONNECT TO ORACLE nm(X).   { pParse->BuildDataSourceForOracle(&X);}
cmd ::= CONNECT TO MYSQL nm(X).   { pParse->BuildDataSourceForMySQL(&X);}

///////////////////////////// The CREATE TABLESPACE command /////////////////////////
//
cmd ::= create_tablespace page_opt.
create_tablespace ::= CREATE TABLESPACE ifnotexists(E) nm(X).{
    pParse->BuildCreateTableSpace(E,&X);
}

page_opt ::= PAGESIZE INTEGER(Y) ASKPAGE INTEGER(Z) STORAGE ids(E) .{
    pParse->AddTablespaceAttribute(&Y,&Z, &E);
}

page_opt ::= PAGESIZE INTEGER(Y) ASKPAGE INTEGER(Z) .{
    pParse->AddTablespaceAttribute(&Y,&Z);
}

page_opt ::= PAGESIZE INTEGER(Y) STORAGE ids(E) .{
    pParse->AddTablespaceAttribute(&Y,NULL,&E);
}

page_opt ::= PAGESIZE INTEGER(Y).{
    pParse->AddTablespaceAttribute(&Y);
}

page_opt ::= ASKPAGE INTEGER(Z).{
    pParse->AddTablespaceAttribute(NULL,&Z);
}

page_opt ::= ASKPAGE INTEGER(Z) STORAGE ids(E).{
    pParse->AddTablespaceAttribute(NULL,&Z, &E);
}

page_opt ::= STORAGE ids(E).{
    pParse->AddTablespaceAttribute(NULL,NULL, &E);
}

page_opt ::= .{
    pParse->AddTablespaceAttribute();
}

///////////////////////////// The ALTER  TABLESPACE command /////////////////////////
//
cmd ::= alter_tablespace alter_page_opt.
alter_tablespace ::= ALTER TABLESPACE ifexists(E) nm(X). {
    pParse->BuildAlterTableSpace(E,&X);
}

alter_page_opt ::= PAGESIZE INTEGER(Y) ASKPAGE INTEGER(Z) STORAGE ids(E) .{
    pParse->ModifyTablespaceAttribute(&Y,&Z, &E);
}

alter_page_opt ::= PAGESIZE INTEGER(Y).{
    pParse->ModifyTablespaceAttribute(&Y);
}

alter_page_opt ::= ASKPAGE INTEGER(Z).{
    pParse->ModifyTablespaceAttribute(NULL,&Z);
}

alter_page_opt ::= STORAGE ids(E).{
    pParse->ModifyTablespaceAttribute(NULL,NULL,&E);
}

///////////////////////////// The DROP  TABLESPACE command /////////////////////////
//
cmd ::= DROP TABLESPACE ifexists(E) nm(X).   {
    pParse->BuildDropTableSpace(E,&X);
}

///////////////////////////// The CREATE USER command /////////////////////////
//
cmd ::= create_user access_opt.  
create_user ::= CREATE USER ifnotexists(E) nm(X).{
    pParse->BuildUser(E,&X,TK_CREATE);
}

access_opt ::= IDENTIFIED BY nm(Z) ACCESSED BY nm(L).{
    pParse->AddUserAttribute(&Z,&L);
}

access_opt ::= IDENTIFIED BY nm(Z).{
    pParse->AddUserAttribute(&Z);
}

///////////////////////////// The ALTER  USER command /////////////////////////
//
cmd ::= alter_user alter_access_opt.
alter_user ::= ALTER USER ifexists(E) nm(X).{
    pParse->BuildUser(E,&X,TK_ALTER);
}

alter_access_opt ::= IDENTIFIED BY nm(Z) ACCESSED BY nm(L).{
    pParse->ModifyUserAttribute(&Z,&L);
}

alter_access_opt ::= IDENTIFIED BY nm(Z).{
    pParse->ModifyUserAttribute(&Z);
}

alter_access_opt ::= ACCESSED BY nm(Z).{
    pParse->ModifyUserAttribute(NULL,&Z);
}

///////////////////////////// The DROP  USER command /////////////////////////
//
cmd ::= DROP USER ifexists(E) nm(X).   { pParse->BuildUser(E,&X,TK_DROP);}


///////////////////////////// The ADD TABLE ATTR command /////////////////////////
//
cmd ::= alter_table_addAttr add_attr.
alter_table_addAttr ::= ALTER TABLE ifnotexists(X) nm(Y) ADD  TABLESYS. {
    pParse->BuildModifyTableAttribute(X,&Y,"add");
}

add_attr ::= id(X) LP INTEGER(Y) RP.
{
	pParse->AddTableAttribute(&X,&Y);
}

add_attr ::= id(X) LP ids(Y) RP.
{
	pParse->AddTableAttribute(&X,&Y);
}

///////////////////////////// The DROP TABLE ATTR command /////////////////////////
//
cmd ::= alter_table_dropAttr drop_attr.
alter_table_dropAttr ::= ALTER TABLE ifnotexists(X) nm(Y) DROP TABLESYS.{
	pParse->BuildModifyTableAttribute(X,&Y,"drop");
}

drop_attr ::= nm(X).
{
	pParse->DropTableAttribute(&X);
}

///////////////////////////// The MODIFY TABLE ATTR command /////////////////////////
//
cmd ::= alter_table_modifAttr modifyattr.
alter_table_modifAttr ::= ALTER TABLE ifnotexists(E) nm(Y) MODIFY TABLESYS.{
	pParse->BuildModifyTableAttribute(E,&Y,"modify");
}

modifyattr ::= id(X) LP INTEGER(Y) RP.
{
	pParse->ModifyTableAttribute(&X,&Y);
}

modifyattr ::= id(X) LP ids(Y) RP.
{
	pParse->ModifyTableAttribute(&X,&Y);
}


///////////////////////////// The ADD TABLE COLUMN command /////////////////////////
//
cmd ::= alter_table_addcolum table_column.
alter_table_addcolum ::= ALTER TABLE  ifnotexists(E) nm(Y) ADD  COLUMN.{
	pParse->BuildAddTableColumn(E,&Y);
}

table_column ::= columnid type carglist.     

///////////////////////////// The DROP TABLE COLUMN command /////////////////////////
//
cmd ::= ALTER TABLE  ifnotexists(E) nm(Y) DROP COLUMN nm(Z).{
	pParse->BuildDropTableColumn(E,&Y,&Z);
}

///////////////////////////// The MODIFY TABLE COLUMN command /////////////////////////
//
cmd ::= alter_table_modifycolum table_mcolumn.
alter_table_modifycolum ::= ALTER TABLE  ifnotexists(E) nm(Y) MODIFY COLUMN.{
	pParse->BuildModifyTableColumn(E,&Y);
}

table_mcolumn ::= modifycolumid ctype carglists. 
modifycolumid ::= nm(X).  {
    pParse->ModifyColumn(&X);
}

%type tokentype {Token}
ctype ::= .
ctype ::= tokentype.                   {}
tokentype ::= ids(X).   {	pParse->ModifyColumnAttribute("data-type",&X);}
tokentype::= ids(X) LP INTEGER(Z) RP. {
	pParse->ModifyColumnAttribute("data-type",&X);
	pParse->ModifyColumnAttribute("data-len",&Z);
}

carglists ::= carglists cargs.
carglists ::= .

cargs ::= CONSTRAINT nm cons.
cargs ::= cons.

cons ::= NULL.                 { pParse->ModifyColumnNULLAttribute(1);}
cons ::= NOT NULL.             {pParse->ModifyColumnNULLAttribute(0);}
cons ::= REPTYPE LP ids(X) RP.  { pParse->ModifyColumnAttribute("rep-type",&X);}
cons ::= DEFAULT ids(X).  {  pParse->ModifyColumnAttribute("default-value",&X);}
cons ::= DEFAULT INTEGER(X).{ pParse->ModifyColumnAttribute("default-value",&X);}

cons ::= DEFAULT PLUS  INTEGER(X).
{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER,0,0,&X);
	ST_EXPR * temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_UPLUS, temp, 0, 0);
	struct _Token str;
	ST_EXPR *  pstExpr= temp2;
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] ={0};
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue);
	str.z = value;
	str.n =(unsigned int)(strlen(value));
	pParse->ModifyColumnAttribute("default-value",&str);

}
cons ::= DEFAULT MINUS INTEGER(X).
{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_INTEGER,0,0,&X);
	ST_EXPR * temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_UMINUS, temp, 0, 0);
	struct _Token str; 
	ST_EXPR *  pstExpr= temp2; 
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] = {0}; 
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue); 
	str.z = value; 
	str.n = (unsigned int)(strlen(value)); 
	pParse->ModifyColumnAttribute("default-value",&str);

}

cons ::= DEFAULT LP expr(X) RP.
{
	struct _Token str;
	ST_EXPR *  pstExpr= X.pExpr;
	pParse->m_tMdbExpr.CalcExpr(pstExpr);
	char value[256] = {0};
	snprintf(value,sizeof(value),"%lld",pstExpr->pExprValue->lValue);
	str.z = value;
	str.n =(unsigned int)(strlen(value));
	pParse->ModifyColumnAttribute("default-value",&str);

}

///////////////////////////// The LOAD DATA command /////////////////////////
//
cmd ::= LOAD DATA TABLE nm(Y).{
	pParse->BuildLoadData(&Y);
}

///////////////////////////// The ADD SEQUENCE command /////////////////////////
//
cmd ::= add_seq add_seq_args.
add_seq ::= ADD SEQUENCE.{
    pParse->BuildAddSequence();
}

add_seq_args ::= LP seqcolumnlist RP.{
}


///////////////////////////// The ALTER SEQUENCE command /////////////////////////
//
cmd ::= alter_seq alter_seq_args.
alter_seq ::= ALTER SEQUENCE.{
    pParse->BuildAlterSequence();
}

alter_seq_args ::= LP seqcolumnlist RP.{
}

///////////////////////////// The DROP SEQUENCE command /////////////////////////
//
cmd ::= del_seq del_seq_args.
del_seq ::= DROP SEQUENCE.{
    pParse->BuildDelSequence();
}

del_seq_args ::= LP seqcolumnlist RP.{
}


seqcolumnlist ::= seqcolumnlist COMMA seqcolumn.
seqcolumnlist ::= seqcolumn.
    
seqcolumn::= nm(X) LP ids(Y) RP. {
    pParse->AddSequenceValue(&X,&Y);
}

seqcolumn::= nm(X) LP INTEGER(Z) RP. {
    pParse->AddSequenceValue(&X,&Z);
}

///////////////////////////// The ADD JOB command /////////////////////////
//
cmd ::= add_job add_job_args.
add_job ::= CREATE JOB nm(Y).{
    pParse->BuildCreateJob(&Y);
}

add_job_args ::= LP jobcolumnlist RP.{
}

///////////////////////////// The ALTER JOB command /////////////////////////
//
cmd ::= alter_job alter_job_args.
alter_job ::= ALTER JOB nm(Y).{
    pParse->BuildAlterJob(&Y);
}

alter_job_args ::= LP jobcolumnlist RP.{
}

///////////////////////////// The REMOVE JOB command /////////////////////////
//
cmd ::= REMOVE JOB nm(Y).{
    pParse->BuildRemoveJob(&Y);
}

jobcolumnlist ::= jobcolumnlist COMMA jobcolumn.
jobcolumnlist ::= jobcolumn.
    
jobcolumn::= nm(X) LP ids(Y) RP. {
    pParse->AddJobValue(&X,&Y);
}

jobcolumn::= nm(X) LP INTEGER(Z) RP. {
    pParse->AddJobValue(&X,&Z);
}

///////////////////////////// The ADD FLUSH-SQL or LOAD-SQL PARAMETER command /////////////////////////
//
cmd ::= alter_flushsql_addparam flushsql_param.
alter_flushsql_addparam ::= ALTER TABLE ifnotexists(E) nm(X) ADD  PARAMETER nm(Y).{
	pParse->BuildAddFlushSQLParam(E,&X,&Y);
}
flushsql_param ::= data_type parglists. 

%type atokentype {Token}
data_type ::= .
data_type ::= atokentype.                   {}
atokentype ::= ids(X).   {	pParse->AddFlushSQLorLoadSQLParamAttr("data-type",&X);}

parglists ::= parglists pargs.
parglists ::= .

pargs ::= cons.
cons ::= VALUES LP ids(X) RP.   { pParse->AddFlushSQLorLoadSQLParamAttr("value",&X);}
cons ::= PARAMTYPE LP ids(X) RP.{ pParse->AddFlushSQLorLoadSQLParamAttr("parameter-type",&X);}

///////////////////////////// The MODIFY TABLE COLUMN command /////////////////////////
//
cmd ::= alter_flushsql_modifyparam flushsql_mparam.
alter_flushsql_modifyparam ::= ALTER TABLE ifnotexists(E) nm(X) MODIFY PARAMETER nm(Y).{
	pParse->BuildModifyFlushSQLParam(E,&X,&Y,"modify");
}
flushsql_mparam ::= mdata_type mparglists. 

%type mtokentype {Token}
mdata_type ::= .
mdata_type ::= mtokentype.                   {}
mtokentype ::= ids(X).   {	pParse->ModifyFlushSQLorLoadSQLParamAttr("data-type",&X);}

mparglists ::= mparglists mpargs.
mparglists ::= .

mpargs ::= mcons.
mcons ::= VALUES LP ids(X) RP.   { pParse->ModifyFlushSQLorLoadSQLParamAttr("value",&X);}
mcons ::= PARAMTYPE LP ids(X) RP.  { pParse->ModifyFlushSQLorLoadSQLParamAttr("parameter-type",&X);}

///////////////////////////// The DROP FLUSH-SQL or LOAD-SQL PARAMETER command /////////////////////////
//
cmd ::= ALTER TABLE  ifnotexists(E) nm(Y) DROP PARAMETER nm(Z).{
	pParse->BuildModifyFlushSQLParam(E,&Y,&Z,"drop");
}

////////////////////////// The RENAME TABLE /////////////////////////////////////
//
cmd ::= RENAME nm(X) TO nm(Y). {
    pParse->BuildRenameTable(&X,&Y);
}

/////////////////////////// Expression Processing /////////////////////////////
//
%type expr {ST_EXPR_SPAN}
%destructor expr {}

%type term {ST_EXPR_SPAN}
%destructor term {}

%include {
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
}

expr(A) ::=term(X). {A=X;}
expr(A) ::=LP(B) expr(X) RP(E).{A.pExpr=X.pExpr;spanSet(&A,&B,&E);}
term(A) ::= NULL(X). {spanExpr(pParse,&A,@X,&X);}
expr(A) ::= id(X). {spanExpr(pParse,&A,TK_ID,&X);}

expr(A) ::= nm(X) DOT nm(Y). {
  ST_EXPR *temp1 = pParse->m_tMdbExpr.BuildPExpr(TK_ID_TABLENAME, 0, 0, &X);
  ST_EXPR *temp2 = pParse->m_tMdbExpr.BuildPExpr(TK_ID, 0, 0, &Y);
  A.pExpr = pParse->m_tMdbExpr.BuildPExpr(TK_DOT, temp1, temp2, 0);
  spanSet(&A,&X,&Y);
}

//sequence nextval,currval
expr(A) ::= nm(X) DOT NEXTVAL|CURRVAL(Y).{
	ST_EXPR * temp = pParse->m_tMdbExpr.BuildPExpr(TK_STRING,0,0,&X);
	ST_EXPR_LIST * plist = pParse->m_tMdbExpr.ExprListAppend(0,temp);
	A.pExpr = pParse->m_tMdbExpr.BuildExprFunction(plist,&Y);
	spanSet(&A,&X,&Y);
}

term(A) ::= INTEGER|FLOAT|BLOB(X).  {spanExpr(pParse,&A, @X, &X);}
term(A) ::= STRING(X).              {spanExpr(pParse,&A, @X, &X);}

expr(A) ::= VARIABLE(X).     {
  spanExpr(pParse,&A, TK_VARIABLE, &X);
  spanSet(&A, &X, &X);
}


expr(A) ::= ID(X) LP  exprlist(Y) RP(E). {
  A.pExpr = pParse->m_tMdbExpr.BuildExprFunction(Y, &X);
  spanSet(&A,&X,&E);
  
//  if( D && A.pExpr ){
//    A.pExpr->flags |= EP_Distinct;
//  }
  
}

expr(A) ::= ID(X) LP STAR RP(E). {
  A.pExpr = pParse->m_tMdbExpr.BuildExprFunction( 0, &X);
  spanSet(&A,&X,&E);
}


term(A) ::= SYSDATE(OP). {
 //sysdate values are  treated as functions that return constants 
  A.pExpr = pParse->m_tMdbExpr.BuildExprFunction(0,&OP);
  if( A.pExpr ){
    A.pExpr->iOpcode = TK_CONST_FUNC;  
  }
  spanSet(&A, &OP, &OP);
}


%include {
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
}


expr(A) ::= expr(X) AND(OP) expr(Y).    {spanBinaryExpr(pParse,&A,@OP,&X,&Y);}
expr(A) ::= expr(X) OR(OP) expr(Y).     {spanBinaryExpr(pParse,&A,@OP,&X,&Y);}
expr(A) ::= expr(X) LT|GT|GE|LE(OP) expr(Y).{spanBinaryExpr(pParse,&A,@OP,&X,&Y);}
expr(A) ::= expr(X) EQ|NE(OP) expr(Y).  {spanBinaryExpr(pParse,&A,@OP,&X,&Y);}
expr(A) ::= expr(X) BITAND|BITOR|LSHIFT|RSHIFT(OP) expr(Y). {spanBinaryExpr(pParse,&A,@OP,&X,&Y);}
expr(A) ::= expr(X) PLUS|MINUS(OP) expr(Y).{spanBinaryExpr(pParse,&A,@OP,&X,&Y);}
expr(A) ::= expr(X) STAR|SLASH|REM(OP) expr(Y). {spanBinaryExpr(pParse,&A,@OP,&X,&Y);}
expr(A) ::= expr(X) CONCAT(OP) expr(Y). {spanBinaryExpr(pParse,&A,@OP,&X,&Y);}


%type likeop {LikeOp}
likeop(A) ::= LIKE_KW(X).     {A.eOperator = X; A.iNot = 0;}
likeop(A) ::= NOT LIKE_KW(X). {A.eOperator = X; A.iNot = 1;}
likeop(A) ::= MATCH(X).       {A.eOperator = X; A.iNot = 0;}
likeop(A) ::= NOT MATCH(X).   {A.eOperator = X; A.iNot = 1;}
expr(A) ::= expr(X) likeop(OP) expr(Y).  [LIKE_KW]  {
  ST_EXPR_LIST  *pList;
  pList = pParse->m_tMdbExpr.ExprListAppend(0, Y.pExpr);
  pList = pParse->m_tMdbExpr.ExprListAppend(pList, X.pExpr);
  A.pExpr = pParse->m_tMdbExpr.BuildExprFunction( pList, &OP.eOperator);
  if( OP.iNot ) A.pExpr = pParse->m_tMdbExpr.BuildPExpr(TK_NOT, A.pExpr, 0, 0);
  A.zStart = X.zStart;
  A.zEnd = Y.zEnd;
  if( A.pExpr ) A.pExpr->flags |= EP_InfixFunc;
}



%include {
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
}


expr(A) ::= expr(X) ISNULL|NOTNULL(E).   {spanUnaryPostfix(pParse,&A,@E,&X,&E);}
expr(A) ::= expr(X) NOT NULL(E). {spanUnaryPostfix(pParse,&A,TK_NOTNULL,&X,&E);}

%include {
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
}


%include {
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
}

//    expr1 IS expr2
//    expr1 IS NOT expr2
//
// If expr2 is NULL then code as TK_ISNULL or TK_NOTNULL.  If expr2
// is any other expression, code as TK_IS or TK_ISNOT.
// 
expr(A) ::= expr(X) IS expr(Y).     {
  spanBinaryExpr(pParse,&A,TK_IS,&X,&Y);
  binaryToUnaryIfNull(pParse, Y.pExpr, A.pExpr, TK_ISNULL);
}
expr(A) ::= expr(X) IS NOT expr(Y). {
  spanBinaryExpr(pParse,&A,TK_ISNOT,&X,&Y);
  binaryToUnaryIfNull(pParse, Y.pExpr, A.pExpr, TK_NOTNULL);
}


%type exprlist {ST_EXPR_LIST*}
%destructor exprlist {QMDB_MALLOC->ReleaseExprList($$);}
%type nexprlist {ST_EXPR_LIST*}
%destructor nexprlist {QMDB_MALLOC->ReleaseExprList($$);}

exprlist(A) ::= nexprlist(X).                {A = X;}
exprlist(A) ::= .                            {A = 0;}
nexprlist(A) ::= nexprlist(X) COMMA expr(Y).
    {A = pParse->m_tMdbExpr.ExprListAppend(X,Y.pExpr);}
nexprlist(A) ::= expr(Y).
    {A = pParse->m_tMdbExpr.ExprListAppend(0,Y.pExpr);}

    



expr(A) ::= NOT(B) expr(X).    {spanUnaryPrefix(pParse,&A,@B,&X,&B);}
expr(A) ::= BITNOT(B) expr(X). {spanUnaryPrefix(pParse,&A,@B,&X,&B);}
expr(A) ::= MINUS(B) expr(X). [BITNOT] {spanUnaryPrefix(pParse,&A,TK_UMINUS,&X,&B);}
expr(A) ::= PLUS(B) expr(X). [BITNOT] {spanUnaryPrefix(pParse,&A,TK_UPLUS,&X,&B);}




%type between_op {int}
between_op(A) ::= BETWEEN.     {A = 0;}
between_op(A) ::= NOT BETWEEN. {A = 1;}
expr(A) ::= expr(W) between_op(N) expr(X) AND expr(Y). [BETWEEN] {
  ST_EXPR_LIST *pList = pParse->m_tMdbExpr.ExprListAppend(0, W.pExpr);
  pList = pParse->m_tMdbExpr.ExprListAppend(pList, X.pExpr);
  pList = pParse->m_tMdbExpr.ExprListAppend(pList, Y.pExpr);
  A.pExpr = pParse->m_tMdbExpr.BuildPExpr( TK_BETWEEN, 0, 0, 0);
  A.pExpr->pFunc = QMDB_MALLOC->AllocExprFunc();
  if( A.pExpr ){
     A.pExpr->pFunc->pFuncArgs = pList;
  }else{
	QMDB_MALLOC->ReleaseExprList(pList);
  } 
  if( N ) A.pExpr = pParse->m_tMdbExpr.BuildPExpr(TK_NOT, A.pExpr, 0, 0);
  A.zStart = W.zStart;
  A.zEnd = Y.zEnd;
}



//in statement




  %type in_op {int}
  in_op(A) ::= IN.      {A = 0;}
  in_op(A) ::= NOT IN.  {A = 1;}
  expr(A) ::= expr(X) in_op(N) LP exprlist(Y) RP(E). [IN] {
    if( Y==0 ){
      /* Expressions of the form
      **
      **      expr1 IN ()
      **      expr1 NOT IN ()
      **
      ** simplify to constants 0 (false) and 1 (true), respectively,
      ** regardless of the value of expr1.
      */
      A.pExpr = pParse->m_tMdbExpr.BuildPExpr( TK_INTEGER, 0, 0, &mdbIntTokens[N]);
    }else{
      A.pExpr = pParse->m_tMdbExpr.BuildPExpr( TK_IN, X.pExpr, 0, &mdbFuncInToken);
        A.pExpr->pFunc = QMDB_MALLOC->AllocExprFunc();
      if( A.pExpr ){
        A.pExpr->pFunc->pFuncArgs = Y;
      }else{
      }
      if( N ) A.pExpr = pParse->m_tMdbExpr.BuildPExpr(TK_NOT, A.pExpr, 0, 0);
    }
    A.zStart = X.zStart;
    A.zEnd = &E.z[E.n];
  }


    
