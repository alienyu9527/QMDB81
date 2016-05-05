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
        //设置读取的目录
        int Init(const char* pszPath);
        
        //开始读取
        int GetFileList(MdbCheckFileType iFileType);
        
        //下一个文件名(带路径)
        int Next(char* pszFullFileName);    
        
        //清除记录，重新开始
        void Clear();
        
        int GetFileCounts()
        {
            return  m_iFileCounts;   
        }

    private:
        char m_sPath[MAX_PATH_NAME_LEN];   //路径名
        int  m_iFileCounts;                //本次读取文件数
        char *m_FileName[MAX_FILE_COUNTS]; //文件名   
        int  m_iCurPos;                    //这次文件位置
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
        static const int m_iMaxFileCount;//积压文件警戒值
    };
    
    class TTableRecordCount
    {
    public:
    	TTableRecordCount();
    	~TTableRecordCount();

    	int Init(const char *pszDSN);//初始化
    	int RecordCount(const char* pTableName);//获取oracle侧表记录总数
    	
    private:
    	bool IsNeedQueryFromOra(const char* pTableName);//判断是否需要从oracle侧统计表记录数
    	int Query();//从oracle侧查询记录数
    	int GenOraSSQL();//生成查询SQL
    	int ConnectOracle();//建立oracle链接

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

