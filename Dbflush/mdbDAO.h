/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbDAO.h		
*@Description： 负责管理miniDB的动态DAO的控制
*@Author:		li.shugang
*@Date：	    2009年03月23日
*@History:
******************************************************************************************/
#ifndef __MDB_DAO_H__
#define __MDB_DAO_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Helper/TDBFactory.h"
#include "Dbflush/mdbDAOBase.h"

//namespace QuickMDB{

    #define MAX_DAO_COUNTS 60
	#define MAX_MYSQL_ARRAY_SIZE 10


    class TMdbNodeDAO
    {
    public:
        TMdbNodeDAO();
        ~TMdbNodeDAO();

        int iOper;              //操作insert. update. delete
        char * sSQL; //对应的SQL
		int iCount;
		int dataColCount;
		TMdbData * m_tData[MAX_COLUMN_COUNTS];
        TMdbDAOBase* pDAO;
    };


    class TMdbTableDAO
    {
    public:
        TMdbTableDAO();
        ~TMdbTableDAO();
        void Clear();

        //int iTableID;           //表ID
        std::string sTableName;
        int iLastOperType;     //上一次对该表的操作
        int iTabPos;
        TMdbNodeDAO* pNodeDAO[MAX_DAO_COUNTS][MAX_MYSQL_ARRAY_SIZE]; //DAO列表
    };


    class TMdbDAO
    {
    public:
        TMdbDAO();
        ~TMdbDAO();

        /******************************************************************************
        * 函数名称	:  Init()
        * 函数描述	:  初始化：连接Oracle  
        * 输入		:  pConfig, 配置文件，带有Oracle的相关信息  
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Init(TMdbConfig* pConfig,TMdbShmDSN *pShmDSN);

		int CheckMdbData(TMdbLCR& tLCR);

        /******************************************************************************
        * 函数名称	:  Execute()
        * 函数描述	:  执行操作数据, 寻找到匹配的DAO, 向Oracle提交数据  
        * 输入		:  pOneRecord, 数据结果  
        * 输出		:  无
        * 返回值	:  成功返回0，数据问题返回-1, Oracle断开返回1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Execute(TMdbLCR& tLCR,bool commitFlag = false);

        /******************************************************************************
        * 函数名称	:  Commit()
        * 函数描述	:  向Oracle提交数据  
        * 输入		:  无 
        * 输出		:  无
        * 返回值	:  成功返回0，数据问题返回-1, Oracle断开返回1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Commit(bool bCommitFlag = true);

        void Clear();
        int ClearDAOByTableId(const int iTableId);//根据tableid清理dao

    private:   
        //生成DAO
        TMdbNodeDAO** CreateDAO(TMdbLCR& tLCR);

        //查找DAO
        TMdbNodeDAO** FindDAO(TMdbLCR& tLCR);
		int PushData(TMdbLCR& tLcr);
		int PushData(TMdbLCR& tLcr, TMdbNodeDAO* pNodeDao, int iCount);
		int GetSelectSQL(TMdbLCR & tLcr);
		int GetSQL(TMdbLCR& tLcr);
		int GetSQL(TMdbLCR & tLcr, int reptNum, char* sSQL);
    private:
        char m_sDSN[MAX_NAME_LEN]; //DSN
        char m_sUID[MAX_NAME_LEN]; //UID
        char m_sPWD[MAX_NAME_LEN]; //PWD
        TMDBDBInterface* m_pDBLink;   //链接
        //TMdbOneRecord *m_pOneRecord;
        TMDBDBQueryInterface *m_pQuery;       //单次提交
        TMdbDatabase * m_pSelDBLink;
        TMdbQuery * m_pSelQuery;    //反查query
        TMdbConfig *m_pMdbConfig;
        TMdbShmDSN *m_pShmDSN;
        TMdbData m_tData[MAX_COLUMN_COUNTS];
        TMdbTableDAO m_tTableDAO[MAX_TABLE_COUNTS]; //所有的表对应的DAO, 实际上写入Oracle的表大概10个左右，不会太多
    };

//}

#endif //__MDB_DAO_H__

