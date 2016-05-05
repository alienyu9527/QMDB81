/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbDataExport.h     
*@Description�� �ڴ������ݵ����ķ��ʽӿ�
*@Author:       li.shugang
*@Date��        2009��10��30��
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_DATA_EXPORT_H__
#define __QUICK_MEMORY_DATABASE_DATA_EXPORT_H__

#include <string>
#include <iostream>

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Interface/mdbQuery.h"
#include "Dbflush/mdbDAOBase.h"


//namespace QuickMDB{

    class TMdbShmDSN;
    class TMdbTableCtrl;



    class TMdbDataExport
    {
    public:
        TMdbDataExport();
        ~TMdbDataExport();
        
    public:
        /******************************************************************************
        * ��������  :  Login()
        * ��������  :  ���ݵ�¼��Ϣ����¼�ڴ�/Oracle���ݿ�  
        * ����      :  pszInf, �ڴ����ݿ��DSN����
        * ���      :  ��
        * ����ֵ    :  �ɹ�����0�����򷵻�-1
        * ����      :  li.shugang
        *******************************************************************************/
        int Login(const char* pszInf);
        
        /******************************************************************************
        * ��������  :  Export()
        * ��������  :  ��������  
        * ����      :  pszTableName, ��Ҫ�����ı�   
        * ����      :  pszObjTable,  ������Ŀ���   
        * ����      :  pszWhere,     ��ѯ���� 
        * ���      :  ��
        * ����ֵ    :  �ɹ����ص����ļ�¼�������򷵻�-1
        * ����      :  li.shugang
        *******************************************************************************/
        int Export(const char* pszTableName, const char* pszObjTable,const char* pszWhere="");
		int ForceExport(const char* pszTableName, const char* pszObjTable,const char* pszWhere="");
        
    private:
    	//��ȡmdb�û���Ϣ
        int GetMDBUser(const char* pszDSN, char* pszUID, char* pszPWD);
    	//��ȡoracle�û���Ϣ
        int GetOraUser(char* pszDSN, char* pszUID, char* pszPWD);
    	//����ͬ�����ԡ�ƴд��ѯSQL
        int CheckMDBTable(const char* pszTableName);
    	//ƴдOracle insert SQL
        int CheckOraTable(const char* pszTableName);
		int ForceCheckOraTable(const char* pszTableName);
        
    private:
        TMdbDatabase m_tDB;     
        TMDBDBInterface* m_pDBLink;   //����
        TMdbConfig *m_pConfig;  
        char m_sMDBSQL[MAX_SQL_LEN]; //��Ӧ��SQL
        char m_sORaSQL[MAX_SQL_LEN]; //��Ӧ��SQL    
        char *m_pCountSQL;
        TMdbDAOBase* m_pDAO;
        TMdbTable* m_pTable;

    	string sWhere;
    };

//}


#endif //__QUICK_MEMORY_DATABASE_DATA_EXPORT_H__




