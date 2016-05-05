/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbRowCtrl.h
*@Description�� �ڴ����ݿ�ļ�¼������
*@Author:		jin.shaohua
*@Date��	    2013��1��29��
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_ROW_CONTRL_H__
#define __MINI_DATABASE_ROW_CONTRL_H__
#include "Helper/mdbStruct.h"
#include "Helper/SqlParserStruct.h"
#include "Control/mdbVarcharMgr.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{

    //�е�null��flagֵ
    class TMdbColumnNullFlag
    {
    public:
        TMdbColumnNullFlag();
        ~TMdbColumnNullFlag();
    public:
        int CalcNullFlag(int iColumnPos);//����collumnλ��
        int m_iColumnPos; //�ڼ���
        int m_iNullFlagOffset;//null��ʶ��λ��
        char m_cNullFlag;    //null��ʶ
    };
    //��¼����
    class TMdbRowCtrl
    {
    public:
        TMdbRowCtrl();
        ~TMdbRowCtrl();
        int Init(const char * sDsn,const char * sTableName);//��ʼ��
        int Init(const char* sDsn,TMdbTable* pTable);
        int SetColumnNull(TMdbColumn * const & pColumn,char* const & pDataAddr);//����null����
        int ClearColumnNULL(TMdbColumn * const & pColumn,char* const & pDataAddr);//����Null����
        bool IsColumnNull(TMdbColumn * const & pColumn,const char*  pDataAddr);//�Ƿ���null����
        int FillOneColumn(char* const & pDataAddr,TMdbColumn * const & pColumn,ST_MEM_VALUE * const & pstMemValue,int iFillType);//���ĳ��
        int GetOneColumnValue(char*  pDataAddr,TMdbColumn * const & pColumn,long long & llValue,
                                                                       char * & sValue,int iValueSize,int & iResultType);//��ȡĳ��ֵ
        int SetTimeStamp(char* const & pDataAddr, int iOffSet,long long iTimeStamp);              
        int GetTimeStamp(char* pDataAddr, int iOffSet,long long & iTimeStamp);
    private:
        int ClearColValueBlock();   //������ʱ��
        char * GetColValueBlockByPos(int iPos);//����column -pos ��ȡ��¼��ʱ��
    private:
        TMdbTable * m_pMdbTable;
        TMdbShmDSN * m_pShmDsn;
        //TMdbVarcharMgr m_tVarcharMgr;
		TMdbVarCharCtrl m_tVarcharCtrl;
        char *  m_pArrColValueBlock[MAX_COLUMN_COUNTS];//��ʱ��¼��
        TMdbColumnNullFlag * m_arrColNullFlag;//�е�null��ʶ
    };
//}
#endif

