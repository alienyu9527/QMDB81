#ifndef __QUICK_MEMORY_DATABASE_MDB_CHECK_H__
#define __QUICK_MEMORY_DATABASE_MDB_CHECK_H__
#include <string>
#include <iostream>

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbFileList.h"
#include "Interface/mdbQuery.h"

//namespace QuickMDB{


    enum MdbCheckFileType
    {
        CheckAllFile, CheckRepFile, CheckOraFile
    };
    class TMdbFileListEx
    {
    public:
        TMdbFileListEx();
        ~TMdbFileListEx();
        
    public:
        //���ö�ȡ��Ŀ¼
        int Init(const char* pszPath);
        
        //��ʼ��ȡ
        int GetFileList(MdbCheckFileType iFileType);
        
        //��һ���ļ���(��·��)
        int Next(char* pszFullFileName);    
        
        //�����¼�����¿�ʼ
        void Clear();
        
        int GetFileCounts()
        {
            return  m_iFileCounts;   
        }

    private:
        char m_sPath[MAX_PATH_NAME_LEN];   //·����
        int  m_iFileCounts;                //���ζ�ȡ�ļ���
        char *m_FileName[MAX_FILE_COUNTS]; //�ļ���   
        int  m_iCurPos;                    //����ļ�λ��
    };


    class TMDBCheckState
    {
    public:
    	TMDBCheckState();
    	~TMDBCheckState();
    	
    public:
    	int Init(char *pszDSN);
    	
    	void CheckAllProcess();

    	void CheckOraRep();

    	void CheckPeerRep();

    	void CheckLog();

        int CheckIndex();
    	
    private:
    	TMdbFileListEx m_tFileList;
    	TMdbConfig* m_pConfig;
    	TMdbShmDSN* m_pShmDSN;
    	FILE* m_fp; 
        static const int m_iMaxFileCount;//��ѹ�ļ�����ֵ
    };
    
    class TTableRecordCount
    {
    public:
    	TTableRecordCount();
    	~TTableRecordCount();

    	int Init(const char *pszDSN);//��ʼ��
    	int RecordCount(const char* pTableName);//��ȡoracle����¼����
    	
    private:
    	bool IsNeedQueryFromOra(const char* pTableName);//�ж��Ƿ���Ҫ��oracle��ͳ�Ʊ��¼��
    	int Query();//��oracle���ѯ��¼��
    	int GenOraSSQL();//���ɲ�ѯSQL
    	int ConnectOracle();//����oracle����

    private:
    	char m_sSSQL[MAX_SQL_LEN];
    	TMdbConfig* m_pConfig;
    	TMdbShmDSN* m_pShmDSN;
    	TMDBDBInterface *m_pDBLink;
    	TMDBDBQueryInterface *m_pQueryOra;
    	TMdbTable * m_pTable;
    };

//}    

#endif

