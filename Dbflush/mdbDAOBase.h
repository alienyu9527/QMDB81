/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbDAOBase.h		
*@Description： 负责管理miniDB的动态DAO的控制
*@Author:		li.shugang
*@Date：	    2009年04月03日
*@History:
******************************************************************************************/
#ifndef __MDB_DAO_BASE_H__
#define __MDB_DAO_BASE_H__

#include "Dbflush/mdbFileParser.h"

//namespace QuickMDB{

    //一次处理的最大数据量
    #define MAX_DATA_COUNTS   512

    //最大的参数数量
    #define MAX_PARAM_COUNTS  512


    //DAO的数据如何独立存放，只需要记录类型和所在类型中的位置
    class TMdbDAOData
    {
    public:
        char m_sName[MAX_NAME_LEN];
        int iType;    //1-long; 2-char*;
        int iPos;         
    };

    class TMdbIntData
    {
    public:
        TMdbIntData();
        ~TMdbIntData();
        long long* piValue;
        char sName[64];
        short isNull[MAX_DATA_COUNTS];
    };

    class TMdbCharData
    {
    public:
        TMdbCharData();
        ~TMdbCharData();
        char* psValue;
        char* ppsValue[MAX_DATA_COUNTS];
        char sName[64];
        int iLength;
        short isNull[MAX_DATA_COUNTS];
    };


    //存放字符串、时间等类型
    class TMdbDAOString
    {
    public:
        void Clear()
        {
            for(int i=0; i<MAX_DATA_COUNTS; ++i)
            {
                memset(&sValue[i], 0, MAX_BLOB_LEN);
            }
        }

        char sValue[MAX_DATA_COUNTS][MAX_BLOB_LEN];         
    };



    class TMdbDAOBase
    {
    public:
        TMdbDAOBase();
        ~TMdbDAOBase();

        /******************************************************************************
        * 函数名称	:  SetOperType()
        * 函数描述	:  设置操作类型：0-Insert; 1-Delete; 2-Update....
        * 输入		:  pszSQL, SQL语句 
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void SetSQL(const char* pszSQL);

        /******************************************************************************
        * 函数名称	:  GetCounts()
        * 函数描述	:  获取记录数
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  记录数
        * 作者		:  li.shugang
        *******************************************************************************/
        int GetCounts();	

        /******************************************************************************
        * 函数名称	:  Execute()
        * 函数描述	:  执行批处理命令
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0, 数据错误导致失败返回-1, Oracle链接断开导致失败返回1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Execute(TMDBDBInterface* pDBLink);	

        /******************************************************************************
        * 函数名称	:  ClearArrayData()
        * 函数描述	:  清空缓存记录链表
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void ClearArrayData();

        /******************************************************************************
        * 函数名称	:  StartData()
        * 函数描述	:  开始添加数据
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void StartData();

        /******************************************************************************
        * 函数名称	:  AddData()
        * 函数描述	:  添加数据
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void AddData(TMdbData *pData);

        /******************************************************************************
        * 函数名称	:  EndData()
        * 函数描述	:  结束添加数据
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void EndData();

        //如果提交发现问题,写入文件日志，供以后处理
        /******************************************************************************
        * 函数名称	:  WriteError()
        * 函数描述	:  如果提交发现问题,写入文件日志，供以后处理
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void WriteError();      
        /******************************************************************************
        * 函数名称  :  ExecuteOne()
        * 函数描述  :  单条处理命令
        * 输入		:  pDBLink oracle链接
        * 返回值    :  成功返回0, 数据错误导致失败返回-1, Oracle链接断开导致失败返回1
        * 作者		:  cao.peng
        *******************************************************************************/
        int ExecuteOne(TMDBDBInterface* pDBLink,TMdbLCR& tLCR);
    private:
        int  m_iCurCounts;  //数据个数
        char m_sSQL[MAX_SQL_LEN];     //SQL语句

        int  m_iStringPos, m_iLongPos, m_TotalPos,m_iBlobPos;
        TMdbIntData * m_tIntData[MAX_PARAM_COUNTS];
        TMdbCharData * m_tCharData[MAX_PARAM_COUNTS];
        TMdbCharData * m_tBlobData[MAX_PARAM_COUNTS];
        TMdbDAOData * m_tDaoData[MAX_PARAM_COUNTS];
        TMDBDBQueryInterface *m_pQuery;
    };

//}


#endif //__MDB_DAO_BASE_H__

