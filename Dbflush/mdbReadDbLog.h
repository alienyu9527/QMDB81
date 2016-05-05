/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbReadOraLog.h		
*@Description�� �������miniDB�Ķ�̬DAO�Ŀ���
*@Author:		li.shugang
*@Date��	    2009��03��30��
*@History:
******************************************************************************************/
#ifndef __MDB_READ_ORACLE_LOG_H__
#define __MDB_READ_ORACLE_LOG_H__

#include "Helper/mdbFileList.h"
#include "Control/mdbMgrShm.h"
#include "Control/mdbProcCtrl.h"
#include "Dbflush/mdbDAO.h"
#include "Control/mdbDDChangeNotify.h"
#include "Control/mdbStorageEngine.h"

//namespace QuickMDB{


    class mdbReadOraLog
    {
    public:
        mdbReadOraLog();
        ~mdbReadOraLog();

        int Init(const char* pszDSN, const char* pszName);
        int InitDao();
        int Start(int iCounts, int iPos);
		int Init(const char* pszDSN);
		int ParseLogFile(char* sFileName);

    private:
        void RenameUpdate(const char* pszTime);
        void RenameUpdateFile(const char* sFullFileName);
        bool CheckAndUpdateProcStat();
        bool IsFileExist(int iPos);//�Ƿ���ͬ���ļ�
        int RepOneFile(char* sFileName,bool iOneCommitFlag);
        int Excute(int &iRecordCount,bool iOneCommitFlag);
        int Commit();
        int CheckMdbDDChange();//����Ƿ������ݶ�����
        bool RecordError(const char* pszLogPath,const char* pszErrorLogPath,char* psErrRecd, int iRecdLen);

    private:
        TMdbFileList m_tFileList;
        TMdbProcCtrl m_tProcCtrl;
        TMdbShmDSN *m_pShmDSN;
        TMdbConfig *m_pConfig;
        char m_sLogPath[MAX_PATH_NAME_LEN];
        char m_sErrorLogPath[MAX_PATH_NAME_LEN];
        char m_sCurFileName[MAX_PATH_NAME_LEN];
        bool m_bIsAttach;
        char m_sDsn[MAX_NAME_LEN];
        TMdbDAO m_mdbDao;
        TMdbFileList m_tExecFileList;
        TMdbFileList m_tRenameFileList;
        //TMdbFileParser m_fileParser;
        TMdbDDChangeNotify m_tDDChangeNotify;
        TMdbOraRepFileStat m_tRepFileStat;
		TMdbRedoLogParser m_tLogParser;
		TMdbLCR m_tLCR;

        FILE* m_fpErr;
    	
    };

//}


#endif //__MDB_READ_ORACLE_LOG_H__

