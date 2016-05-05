#ifndef _MDB_SQL_H_
#define _MDB_SQL_H_
#include <vector>
#include <string>
#include "Interface/mdbQuery.h"
#include "Control/mdbScript.h"
#include "Control/mdbDDLExecuteEngine.h"

//namespace QuickMDB{

#define ArraySize(X) (sizeof(X)/sizeof(X[0]))
#define BOOL_TO_Y(VALUE)  ((VALUE)?"Y":"")

#define SWITCH_MODE(_mode,_doDirect,_doClient)\
switch(_mode)\
{\
    case MODE_DIRECT:{_doDirect;}\
        break;\
    case MODE_CLIENT:{_doClient;}\
        break;\
    default:\
        printf("error");\
        break;\
}

#define CHECK_COLUMN_ISPK(pTable,iPos,bPriKey)\
for(int n=0; n<pTable->m_tPriKey.iColumnCounts; ++n)\
{\
    if(iPos == pTable->m_tPriKey.iColumnNo[n])\
    {\
        bPriKey = true;\
        break;\
    }\
}

#define CHECK_COLUMN_ISIDX(pTable,iPos,bIndex)\
for(int n=0; n<pTable->iIndexCounts; ++n)\
{\
    for(int temp=0; temp<MAX_INDEX_COLUMN_COUNTS; ++temp)\
    {\
        if(iPos == pTable->tIndex[n].iColumnNo[temp])\
        {\
            bIndex = true;\
            break;\
        }\
    }\
}

#define NUM_REVISE_MAX(_num,_max)  (_num) = ((_num) > (_max))?(_max):(_num);
#define NUM_REVISE_MIN(_num,_min)  (_num) = ((_num) < (_min))?(_min):(_num);


//����ID
enum E_RICH_SQL_PROPERTY
{
	PP_EXPLAIN 			= 1,//�Ƿ����SQL
	PP_AUTO_COMMIT 		= 2,//�Ƿ��Զ��ύ
	PP_SHOW_TIMESTAMP	= 3,//�Ƿ���ʾʱ���
};

//���Խṹ
struct ST_RICH_SQL_PROPERTY
{
	std::string sName; // name
	std::string sDoc;  //˵��
	int iId;
	int iValue;
};

//��������
enum E_CMD_TYPE
{
	CMD_EXEC_SQL = 1, // ִ��sql
	CMD_HELP     = 2, //help ˵��
	CMD_QUIT     = 3, //�˳�
	CMD_SET_PP   = 4, //����property
	CMD_NONE     = 5, //����cmd
	CMD_DESC     = 6, // ����ĳ����
	CMD_TABLES   = 7, //����sql
	CMD_COMMIT   = 8,//�����ύ
	CMD_ROLLBACK = 9,//�����ύ
	CMD_ENV      = 10,//��������
	CMD_EXEC_SQL_FILE = 11, // execute sql file
	CMD_EXEC_DDL =12,
	CMD_EXEC_START = 13,
	CMD_EXEC_DOWN = 14,
	CM_MDB_ERR = 15
};

//mdbSQLģʽ
enum E_MDBSQL_MODE
{
    MODE_DIRECT = 1,  //ֱ��ģʽ
    MODE_CLIENT = 2 //CS����
    
};
//�ؼ���
struct ST_KEYWORD
{
	std::string sName;	// name		 
	int 	   iCmdType;	
	std::string sDoc;	//˵��	 
    bool       bCmdConfirm;//����ǰ�Ƿ���Ҫȷ��
};

//sql file �����нṹ
struct ST_PARAM
{
	int 	    iSqlSeq;//sql ���
	int 	    iIndex;//����������
	int  	    iDataType;//�������������� DT_Int��DT_Char
	std::string sValue;	// ����ֵ ,string�洢���ݲ���������
	std::string sParamLine;	// ��������Ϣ
};

//sql file SQL�ṹ
struct ST_SQL
{
	int 	    iSqlSeq;//sql ���
	std::string sSQL;	// SQL ���
	std::string sLineInfo;	// SQL ����Ϣ
	std::vector<ST_PARAM> vParam;//��������
};

struct st_cmdopt
{
    char sUid[32];
    char sPwd[32];
    char sDsn[32];
	char sIP[32];
    char sPort[32];
};


//���ͻ���
class TMdbRichSQL
{
public:
	TMdbRichSQL();
	~TMdbRichSQL();
	int Init(const bool bIsOffLine = false);//��ʼ��
	int Login(const st_cmdopt tCmdOpt);//��½
	int Start(const bool bIsOffline=false);//��ʼ
private:
	ST_KEYWORD * FindCmd(const char * sCmd);//�������ʵ�cmd
	int ExecOnLineCommand(ST_KEYWORD * pCmd,char * sCmdStr);//ִ������
	int ExecuteSQL(ST_KEYWORD * pCmd,const char * sSQL);//ִ��SQL
	int ExecuteHelp(ST_KEYWORD * pCmd,const char * sCmdStr);//ִ��help	
	void GenSplitLine(char* pInOutStr,int iBuffLen,int iAddLen);//���ɷָ���
	int ShowSelectResult();//select���
	int ExecuteSetPP(ST_KEYWORD * pCmd,const char * sCmdStr);//ִ��set property 
	int Desc(ST_KEYWORD * pCmd,const char * sCmdStr);//����������
	int Tables(ST_KEYWORD * pCmd,const char * sCmdStr);//������б���Ϣ
	ST_RICH_SQL_PROPERTY * GetProperty(const char * sName,int iID);//��ȡproperty
	int Commit();//commit
	int RollBack();//rollback
	int Env();//��������
	bool IsDDLSql(const char *pszSQL); //�жϴ��ļ�������sql�ǲ���ddl���
	int ExecuteSQLFile(ST_KEYWORD * pCmd,const char * sCmdStr,const bool bIsOnLine=true);//ִ��SQL FILE
	bool IsComplete(ST_KEYWORD * pCmd,const char * sStr);
	int ExecuteClientSQL(ST_KEYWORD * pCmd,const char * sSQL);//ִ��CS SQL
	int ShowClientSelectResult();//CS select���
private:
	int PushSqlParam(const char * sMsg,std::vector<ST_SQL> *pvSQL,std::vector<ST_PARAM> *pvPARAM);//ST_SQL push_back  and ST_PARAM push_back 
	int AdjustVector(std::vector<ST_SQL> *pvSQL,std::vector<ST_PARAM> *pvPARAM);//ajust vector ST_SQL push_back

	int ExeStSql(std::vector<ST_SQL> *pvSQL);//TMdbQuery ִ��sql��������

	int ExecOffLineCommand(ST_KEYWORD * pCmd,char * sCmdStr);
	int ExecuteStart(ST_KEYWORD * pCmd,const char * sCmdStr);
	int ExecuteDown(ST_KEYWORD * pCmd,const char * sCmdStr);
	int OnLineExecuteDDLSQL(ST_KEYWORD * pCmd,const char * sSQL);
	int OffLineExecuteDDLSQL(const char * sSQL);
	void TrimLeftOrRightSpecialChar(char *pSQL);
	void strncpyEx(char * desc,char * source,int size,char endtag);
	int ShowErr(ST_KEYWORD * pCmd,const char * sCmdStr);
    int SaveOnLineDDL(const char* psDsn, const char* psSql);
    int SaveOffLineDDL(const char* psSql);
    bool ConfirmBeforeExecute(ST_KEYWORD * pCmd,const char *pSQL);//ִ��ɾ�������ش��DDL����ʱ����Ҫȷ�ϲ���
private:
	TMdbSqlParser * m_pMdbSqlParser;//new mdb sql parser
	TMdbDDLExecuteEngine * m_pDDLExecuteEngine;//SQLִ������
	TMdbDatabase m_tDB; //���ݿ�����
	TMdbQuery *  m_pQuery;//
	TMdbShmDSN * m_pShmDSN;//shmdsn;
	TMdbConfig *m_pConfig;
	TMdbClientDatabase m_tCSDB;//CSģʽ����
	TMdbClientQuery *   m_pCSQuery;//CSģʽ�Ĳ�ѯ
	int m_iMode;//ģʽ
	TMdbScript *m_pScript;
	TMdbErrorHelper m_tErrHelper;

	char sUid[32];
    char sPwd[32];
    char sDsn[32];

	char m_sExecStartTime[MAX_NAME_LEN];
	char m_sExecEndTime[MAX_NAME_LEN];
};

//}
#endif
