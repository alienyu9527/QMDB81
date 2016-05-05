/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbDAOBase.h		
*@Description�� �������miniDB�Ķ�̬DAO�Ŀ���
*@Author:		li.shugang
*@Date��	    2009��04��03��
*@History:
******************************************************************************************/
#ifndef __MDB_DAO_BASE_H__
#define __MDB_DAO_BASE_H__

#include "Dbflush/mdbFileParser.h"

//namespace QuickMDB{

    //һ�δ�������������
    #define MAX_DATA_COUNTS   512

    //���Ĳ�������
    #define MAX_PARAM_COUNTS  512


    //DAO��������ζ�����ţ�ֻ��Ҫ��¼���ͺ����������е�λ��
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


    //����ַ�����ʱ�������
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
        * ��������	:  SetOperType()
        * ��������	:  ���ò������ͣ�0-Insert; 1-Delete; 2-Update....
        * ����		:  pszSQL, SQL��� 
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void SetSQL(const char* pszSQL);

        /******************************************************************************
        * ��������	:  GetCounts()
        * ��������	:  ��ȡ��¼��
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��¼��
        * ����		:  li.shugang
        *******************************************************************************/
        int GetCounts();	

        /******************************************************************************
        * ��������	:  Execute()
        * ��������	:  ִ������������
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ���ݴ�����ʧ�ܷ���-1, Oracle���ӶϿ�����ʧ�ܷ���1
        * ����		:  li.shugang
        *******************************************************************************/
        int Execute(TMDBDBInterface* pDBLink);	

        /******************************************************************************
        * ��������	:  ClearArrayData()
        * ��������	:  ��ջ����¼����
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void ClearArrayData();

        /******************************************************************************
        * ��������	:  StartData()
        * ��������	:  ��ʼ�������
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void StartData();

        /******************************************************************************
        * ��������	:  AddData()
        * ��������	:  �������
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void AddData(TMdbData *pData);

        /******************************************************************************
        * ��������	:  EndData()
        * ��������	:  �����������
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void EndData();

        //����ύ��������,д���ļ���־�����Ժ���
        /******************************************************************************
        * ��������	:  WriteError()
        * ��������	:  ����ύ��������,д���ļ���־�����Ժ���
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void WriteError();      
        /******************************************************************************
        * ��������  :  ExecuteOne()
        * ��������  :  ������������
        * ����		:  pDBLink oracle����
        * ����ֵ    :  �ɹ�����0, ���ݴ�����ʧ�ܷ���-1, Oracle���ӶϿ�����ʧ�ܷ���1
        * ����		:  cao.peng
        *******************************************************************************/
        int ExecuteOne(TMDBDBInterface* pDBLink,TMdbLCR& tLCR);
    private:
        int  m_iCurCounts;  //���ݸ���
        char m_sSQL[MAX_SQL_LEN];     //SQL���

        int  m_iStringPos, m_iLongPos, m_TotalPos,m_iBlobPos;
        TMdbIntData * m_tIntData[MAX_PARAM_COUNTS];
        TMdbCharData * m_tCharData[MAX_PARAM_COUNTS];
        TMdbCharData * m_tBlobData[MAX_PARAM_COUNTS];
        TMdbDAOData * m_tDaoData[MAX_PARAM_COUNTS];
        TMDBDBQueryInterface *m_pQuery;
    };

//}


#endif //__MDB_DAO_BASE_H__

