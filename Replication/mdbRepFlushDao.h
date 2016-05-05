/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbFlushDao.h		
*@Description: ��ͬ������ˢ�µ�MDB��
*@Author:		jiang.lili
*@Date��	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_FLUSH_DAO_H__
#define __ZTE_MINI_DATABASE_REP_FLUSH_DAO_H__

#include "Control/mdbRepCommon.h"
#include "Interface/mdbQuery.h"
#include "Helper/mdbRepRecd.h"
//#include "Interface/mdbRollback.h"
//namespace QuickMDB
//{
    #define MAX_FLUSN_DAO_COUNTS 30
    
    /**
    * @brief �ڵ�
    * 
    */
    class TFlushDAONode
    {
    public:
        TFlushDAONode();
        ~TFlushDAONode();
        void Clear();
        TMdbQuery* CreateQuery(const char * sSQL,int iNodePos,TMdbDatabase* ptDB);
    public:
        char m_sSQL[MAX_SQL_LEN];
        TMdbQuery * m_pQuery;//ִ��sql

    };
    
    /**
    * @brief ÿ�ű��sql��queryָ����Ϣ
    * 
    */
    class TRepFlushDao:public TMdbNtcBaseObject
    {
    public:
        TRepFlushDao();
        ~TRepFlushDao();
        int Init(std::string strTableName, TMdbDatabase* pDataBase)throw (TMdbException);//��ʼ��
        int GetQuery(TMdbQuery * &pQuery, TMdbRepRecd & tRecd, int * iDropIndex)throw (TMdbException);//����query
        TMdbQuery * GetSelectQuery(TMdbRepRecd & tRecd)throw (TMdbException);
        TMdbQuery * GetLoadQuery(const char* sRoutinglist);
        TMdbQuery* GetRecdQuery()throw (TMdbException);
        
    private:
        int SetSQL(TFlushDAONode *pNode, int iSqlType)throw (TMdbException);
        int GetSQL(TMdbRepRecd& tRecd, int iSqlType, int * iDropIndex = NULL);
        int InitTabProperty()throw (TMdbException);
        int GetColumnDataType(const char* psDataType);
        TMdbColumn* GetColumnInfo(const char* psName);
        void GetWhereSql(TMdbRepRecd& tRecd, char* psWhere);
    public:
        char m_sSQL[MAX_SQL_LEN];
        TMdbDatabase *m_pDataBase;
        TFlushDAONode * m_arrDaoNode[MAX_FLUSN_DAO_COUNTS];
        TMdbQuery *  m_pRecdQry;
        std::string m_strTableName;//����
        std::vector<std::string> m_arPKs;//��������
        std::vector<std::string> m_arCols;//�������м���
        std::vector<TMdbColumn> m_vColms;
    };
    /**
    * @brief ���ݼ��أ�ȡ���ݺͲ������ݿ���
    * 
    */
    class TRepLoadDao
    {
    public:
        TRepLoadDao();
        ~TRepLoadDao();
    public:
        int Init(TMdbDatabase *pDataBase,TMdbConfig* pMdbCfg);
        TMdbQuery* GetQuery(std::string strTableName, bool bSelect, const char* sRoutinglist = NULL)throw (TMdbException);
    private: 
        int SetSelectSQL(TFlushDAONode *pNode, const char* sRoutinglist=NULL)throw (TMdbException);
        int SetInsertSQL(TFlushDAONode *pNode)throw (TMdbException);
    public:
        TFlushDAONode *m_pNode;
        TMdbDatabase *m_pDataBase;
        std::string m_strTableName;//����
        std::vector<std::string> m_arCols;//�������м���
        
        TMdbConfig* m_pMdbCfg;
    };

    /**
    * @brief ˢ�¿���
    * 
    */
    class TRepFlushDAOCtrl
    {
    public:
        TRepFlushDAOCtrl();
        ~TRepFlushDAOCtrl();
        int Init(const char * sDsn);
        int Execute(const char * sMsg, bool bCheckTime);//ִ��
    private:
        int SetQuery(int * iDropIndex)throw (TMdbException);//����query
        int SetParam(int * iDropIndex)throw (TMdbException);//���ò���
        int SetPkParam() throw (TMdbException);
        int QueryRecd();
        bool CheckTimeStamp(long long iMdbTime, long long iRecdTIme);
        TRepColm* GetPkColm(const char* psColumName);
    private:
        TMdbNtcStrMap m_tMapFlushDao;
        TMdbDatabase* m_ptDatabase;
        TMdbRep12Decode* m_pRcd12Parser;
        TMdbRepRecdDecode * m_pRcdParser;//���ݽ�����
        TMdbRepRecd m_tCurRedoRecd;//��ǰ����ļ�¼
        TMdbQuery  * m_pCurQuery;//ִ�е�query;
        TRepFlushDao *m_pCurFlushDao;//��ǰ��DAO
        long long m_iTimeStmp;
        bool m_bExist;
        TMdbConfig* m_pConfig;
    };

//}

#endif
