/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbDAO.h		
*@Description�� �������miniDB�Ķ�̬DAO�Ŀ���
*@Author:		li.shugang
*@Date��	    2009��03��23��
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

        int iOper;              //����insert. update. delete
        char * sSQL; //��Ӧ��SQL
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

        //int iTableID;           //��ID
        std::string sTableName;
        int iLastOperType;     //��һ�ζԸñ�Ĳ���
        int iTabPos;
        TMdbNodeDAO* pNodeDAO[MAX_DAO_COUNTS][MAX_MYSQL_ARRAY_SIZE]; //DAO�б�
    };


    class TMdbDAO
    {
    public:
        TMdbDAO();
        ~TMdbDAO();

        /******************************************************************************
        * ��������	:  Init()
        * ��������	:  ��ʼ��������Oracle  
        * ����		:  pConfig, �����ļ�������Oracle�������Ϣ  
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int Init(TMdbConfig* pConfig,TMdbShmDSN *pShmDSN);

		int CheckMdbData(TMdbLCR& tLCR);

        /******************************************************************************
        * ��������	:  Execute()
        * ��������	:  ִ�в�������, Ѱ�ҵ�ƥ���DAO, ��Oracle�ύ����  
        * ����		:  pOneRecord, ���ݽ��  
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0���������ⷵ��-1, Oracle�Ͽ�����1
        * ����		:  li.shugang
        *******************************************************************************/
        int Execute(TMdbLCR& tLCR,bool commitFlag = false);

        /******************************************************************************
        * ��������	:  Commit()
        * ��������	:  ��Oracle�ύ����  
        * ����		:  �� 
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0���������ⷵ��-1, Oracle�Ͽ�����1
        * ����		:  li.shugang
        *******************************************************************************/
        int Commit(bool bCommitFlag = true);

        void Clear();
        int ClearDAOByTableId(const int iTableId);//����tableid����dao

    private:   
        //����DAO
        TMdbNodeDAO** CreateDAO(TMdbLCR& tLCR);

        //����DAO
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
        TMDBDBInterface* m_pDBLink;   //����
        //TMdbOneRecord *m_pOneRecord;
        TMDBDBQueryInterface *m_pQuery;       //�����ύ
        TMdbDatabase * m_pSelDBLink;
        TMdbQuery * m_pSelQuery;    //����query
        TMdbConfig *m_pMdbConfig;
        TMdbShmDSN *m_pShmDSN;
        TMdbData m_tData[MAX_COLUMN_COUNTS];
        TMdbTableDAO m_tTableDAO[MAX_TABLE_COUNTS]; //���еı��Ӧ��DAO, ʵ����д��Oracle�ı���10�����ң�����̫��
    };

//}

#endif //__MDB_DAO_H__

