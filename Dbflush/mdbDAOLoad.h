/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbDAOLoad.h		
*@Description�� ����ѱ��е�������������
*@Author:		li.shugang
*@Date��	    2008��12��16��
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_DAO_LOAD_H__
#define __QUICK_MEMORY_DATABASE_DAO_LOAD_H__

#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Interface/mdbQuery.h"

//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{


#define _TRY_BEGIN_ try{
#define _TRY_END_ }\
catch(TMDBDBExcpInterface &oe)\
{\
    TADD_ERROR(ERROR_UNKNOWN," TDBException:%s\n", oe.GetErrMsg());\
    TADD_ERROR(ERROR_UNKNOWN,"QuerySQL:\n%s\n", oe.GetErrSql());\
    return ERROR_UNKNOWN;\
}\
catch(TMdbException& e)\
{\
    TADD_ERROR(ERROR_UNKNOWN,"TMDBException:%s\n", e.GetErrMsg());\
    TADD_ERROR(ERROR_UNKNOWN," QuerySQL:\n%s\n", e.GetErrSql());\
    return ERROR_UNKNOWN;\
}

#define _TRY_END_NO_RETURN_ }\
catch(TMDBDBExcpInterface &oe)\
{\
    TADD_ERROR(ERROR_UNKNOWN," TDBException:%s\n", oe.GetErrMsg());\
    TADD_ERROR(ERROR_UNKNOWN,"QuerySQL:\n%s\n", oe.GetErrSql());\
}\
catch(TMdbException& e)\
{\
    TADD_ERROR(ERROR_UNKNOWN,"TMDBException:%s\n", e.GetErrMsg());\
    TADD_ERROR(ERROR_UNKNOWN," QuerySQL:\n%s\n", e.GetErrSql());\
}\
catch(...)\
{\
	TADD_ERROR(ERROR_UNKNOWN,"ERROR\n");\
}


class TMdbDAOLoad
{
public:
    /******************************************************************************
    * ��������	:  TMdbDAOLoad()
    * ��������	:  ��ȡOracle������
    * ����		:  pConfig�����ò��� 
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    TMdbDAOLoad(TMdbConfig *pConfig);

    ~TMdbDAOLoad();

    /******************************************************************************
    * ��������	:  Connect()
    * ��������	:  ����Oracle���ݿ� 
    * ����		:  �� 
    * ���		:  ��
    * ����ֵ	:  �ɹ�����true�����򷵻�false
    * ����		:  li.shugang
    *******************************************************************************/
    bool Connect();

    /******************************************************************************
    * ��������	:  DisConnect()
    * ��������	:  �Ͽ�Oracle���� 
    * ����		:  �� 
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void DisConnect();

    /******************************************************************************
    * ��������	:  Init()
    * ��������	:  ��ʼ��ĳ����ƴд��SQL���    
    * ����		:  pTable, ������ı� 
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int Init(TMdbShmDSN* pShmDSN, TMdbTable* pTable, const char* sRoutingList = NULL,const char* sFilerSql = NULL);

    /******************************************************************************
    * ��������	:  Load()
    * ��������	:  ����ĳ���� 
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int Load(TMdbDatabase* pMdb);
    /******************************************************************************
    * ��������	:  LoadSequence()
    * ��������	:  ��������
    * ����		:  pMdb,pSeq,pDsn
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int LoadSequence(TMdbDatabase* pMdb,TMdbDSN* pDsn);

    /******************************************************************************
    * ��������	:  ReLoad()
    * ��������	:  ��������ĳ����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  zhang.lin
    *******************************************************************************/
    int ReLoad(TMdbDatabase* pMdb);

    int SetSequenceName(char* sSeqenceName,char* sDsnName);
	
	

private:
    int InitLoadSql(const char* sRoutingList = NULL,const char* sFilterSql = NULL);
    int InitInsertSql();
    /******************************************************************************
    * ��������	:  InitQueryInQMDB()
    * ��������	:  ƴдУ���Ƿ��Ѿ���QMDB�д��ڵ�SQL
    * ����		:
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  cao.peng
    *******************************************************************************/
    int InitQueryInQMDB();

    int InitCheckFieldSql();
    //������Ƿ���oracle���д���
    int CheckFieldExist();
    int LoadAsString(TMdbDatabase* pMdb);
    int LoadAsNormal(TMdbDatabase* pMdb);

    int ReLoadAsNormal(TMdbDatabase* pMdb);
    int ReLoadAsString(TMdbDatabase* pMdb);

    bool CheckRecord(TMdbQuery * pQMDBQuery,TMDBDBQueryInterface* pOraQuery);
    bool CheckAsStringRecord(TMdbQuery * pQMDBQuery,TMDBDBQueryInterface* pOraQuery);
    void SetLoadSQLParameter();
    bool IsLoadColumn(TMdbColumn &tColumn);
    int GetLoadAsStringSQL(char sSQL[],const int iSize,const char* sFilterSql = NULL);
    int GetLoadAsNormalSQL(char sSQL[],const int iSize,const char* sFilterSql = NULL);
    bool isKey(int iColumnNo);//�ж�ĳ���Ƿ�Ϊ������
    int UpdateMDBStringData(const TMdbTable* pTable,TMdbNtcSplit &tSplit,int &iCount);//��������
    int UpdateMDBNormalData(const TMdbTable* pTable,int &iCount);//��������
    int InitQueryForMDB(TMdbDatabase* pMdb);//��ʼ������QUERY
    int InitUpdateSQLInMDB();//��ʼ������SQL

    /******************************************************************************
	* ��������	:  AddRouteIDForLoadSQL()
	* ��������	:  Ϊ��ˢ��SQL���·��
	* ����		:  pTable ��
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  cao.peng
	*******************************************************************************/
	void AddRouteIDForLoadSQL(const char* sRoutingList);

    /******************************************************************************
    * ��������	:  HasWhereCond()
    * ��������	:  �ж�һ�ű��Load SQL������Ƿ����where����
    * ����		:  sTableName ����
    * ����		:  sLoadSQL sql���
    * ���		:  ��
    * ����ֵ	:  ���ڷ���true,���򷵻�false
    * ����		:  jiang.lili
    *******************************************************************************/
    bool HasWhereCond(const char* sTableName, const char * sLoadSQL);
private:
    int m_iSeqCacheSize; //����Cache��С
    char m_sSQL[MAX_SQL_LEN];  //����SQL(select)����load-sql�������ü���SQL��
    char m_sMSQL[MAX_SQL_LEN];  //�洢ƴд������SQL,insert sql

    char m_sCkFieldSQL[MAX_SQL_LEN];

    char m_sQMSQL[MAX_SQL_LEN]; //�洢��ѯSQL��У���Ƿ��Ѿ����� select %s from %s where %s=:%s and %s=:%s
    char m_sUpdateSQL[MAX_SQL_LEN];//���ڸ���QMDB�������SQL
    char m_sDSN[MAX_NAME_LEN]; //Oracle DSN
    char m_sUID[MAX_NAME_LEN]; //Oracle UID
    char m_sPWD[MAX_NAME_LEN]; //Oracle PWD

    TMDBDBInterface* m_pDBLink;   //����
    TMDBDBQueryInterface*    m_pOraQuery;    //����
    TMdbTable* m_pTable;
    TMdbShmDSN* m_pShmDSN;
    TMdbQuery *m_pInsertQuery;
    TMdbQuery *m_pCheckQuery;
    TMdbQuery *m_pUpdateQuery;
    std::map<string,bool> m_mapFieldExist;
};


//}

#endif //__QUICK_MEMORY_DATABASE_DAO_LOAD_H__

