/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbRowCtrl.h
*@Description： 内存数据库的记录级管理
*@Author:		jin.shaohua
*@Date：	    2013年1月29日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_ROW_CONTRL_H__
#define __MINI_DATABASE_ROW_CONTRL_H__
#include "Helper/mdbStruct.h"
#include "Helper/SqlParserStruct.h"
#include "Control/mdbVarcharMgr.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{

    //列的null的flag值
    class TMdbColumnNullFlag
    {
    public:
        TMdbColumnNullFlag();
        ~TMdbColumnNullFlag();
    public:
        int CalcNullFlag(int iColumnPos);//设置collumn位置
        int m_iColumnPos; //第几列
        int m_iNullFlagOffset;//null标识的位置
        char m_cNullFlag;    //null标识
    };
    //记录管理
    class TMdbRowCtrl
    {
    public:
        TMdbRowCtrl();
        ~TMdbRowCtrl();
        int Init(const char * sDsn,const char * sTableName);//初始化
        int Init(const char* sDsn,TMdbTable* pTable);
        int SetColumnNull(TMdbColumn * const & pColumn,char* const & pDataAddr);//设置null数据
        int ClearColumnNULL(TMdbColumn * const & pColumn,char* const & pDataAddr);//清理Null数据
        bool IsColumnNull(TMdbColumn * const & pColumn,const char*  pDataAddr);//是否是null数据
        int FillOneColumn(char* const & pDataAddr,TMdbColumn * const & pColumn,ST_MEM_VALUE * const & pstMemValue,int iFillType);//填充某列
        int GetOneColumnValue(char*  pDataAddr,TMdbColumn * const & pColumn,long long & llValue,
                                                                       char * & sValue,int iValueSize,int & iResultType);//获取某列值
        int SetTimeStamp(char* const & pDataAddr, int iOffSet,long long iTimeStamp);              
        int GetTimeStamp(char* pDataAddr, int iOffSet,long long & iTimeStamp);
    private:
        int ClearColValueBlock();   //清理临时区
        char * GetColValueBlockByPos(int iPos);//根据column -pos 获取记录临时区
    private:
        TMdbTable * m_pMdbTable;
        TMdbShmDSN * m_pShmDsn;
        //TMdbVarcharMgr m_tVarcharMgr;
		TMdbVarCharCtrl m_tVarcharCtrl;
        char *  m_pArrColValueBlock[MAX_COLUMN_COUNTS];//临时记录区
        TMdbColumnNullFlag * m_arrColNullFlag;//列的null标识
    };
//}
#endif

