/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbReadOraLog.h
*@Description： 读取oracle同步文件，并写入oracle
*@Author:       li.shugang
*@Date：        2009年03月30日
*@History:
******************************************************************************************/
#include "Dbflush/mdbReadDbLog.h"
#include "Dbflush/mdbFileParser.h"
#include "Dbflush/mdbDAO.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"
#include "Control/mdbProcCtrl.h"

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#include <dos.h>
//#include <dir.h>
#else
#include "dirent.h"
#endif


//namespace QuickMDB{

    mdbReadOraLog::mdbReadOraLog()
    {
        m_pShmDSN = NULL;
        m_pConfig = NULL;
        m_bIsAttach = true;
        memset(m_sDsn,0,MAX_NAME_LEN);
        memset(m_sLogPath,0,MAX_PATH_NAME_LEN);
        memset(m_sErrorLogPath,0,MAX_PATH_NAME_LEN);
        memset(m_sCurFileName,0,MAX_PATH_NAME_LEN);
        m_fpErr = NULL;
    }

    mdbReadOraLog::~mdbReadOraLog()
    {
        SAFE_DELETE(m_pShmDSN);
        SAFE_CLOSE(m_fpErr);
    }

    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  初始化
    * 输入		:  pszDSN - dsn 名
    * 输入		:  pszName - 进程名
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    int mdbReadOraLog::Init(const char* pszDSN, const char* pszName)
    {
        TADD_FUNC("mdbReadOraLog::Init(%s) : Start.", pszDSN);
        SAFESTRCPY(m_sDsn,sizeof(m_sDsn),pszDSN);
        int iRet = 0;
        m_sDsn[strlen(m_sDsn)] = '\0';
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        CHECK_OBJ(m_pConfig);
        //同步日志目录以及错误日志目录
        memset(m_sLogPath, 0, sizeof(m_sLogPath));
        memset(m_sErrorLogPath,0,sizeof(m_sLogPath));
        SAFESTRCPY(m_sLogPath,sizeof(m_sLogPath),m_pConfig->GetDSN()->sLogDir);
        SAFESTRCPY(m_sErrorLogPath,sizeof(m_sErrorLogPath),m_pConfig->GetDSN()->errorLog);
        if(m_sLogPath[strlen(m_sLogPath)-1] != '/')
        {
            m_sLogPath[strlen(m_sLogPath)] = '/';
        }
        if(m_sErrorLogPath[strlen(m_sErrorLogPath)-1] != '/')
        {
            m_sErrorLogPath[strlen(m_sErrorLogPath)] = '/';
        }
        if(false == TMdbNtcDirOper::IsExist(m_sLogPath))
        {
            if(false == TMdbNtcDirOper::MakeFullDir(m_sLogPath))
            {
                CHECK_RET(ERR_OS_CREATE_DIR,"Mkdir(%s) failed.", m_sLogPath);
            }
        }
        if(false == TMdbNtcDirOper::IsExist(m_sErrorLogPath))
        {
            if(false == TMdbNtcDirOper::MakeFullDir(m_sErrorLogPath))
            {
                CHECK_RET(ERR_OS_CREATE_DIR,"Mkdir(%s) failed.", m_sErrorLogPath);
            }
        }
        //TADD_NORMAL("Oracle-Log-Dir      = [%s].", m_sLogPath);
        //找到管理区地址,连接上共享内存
        m_pShmDSN = new(std::nothrow) TMdbShmDSN();
		if(m_pShmDSN ==  NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new m_pShmDSN");
			return ERR_OS_NO_MEMROY;
		}
        m_pShmDSN->TryAttach();
        iRet = m_pShmDSN->Attach(pszDSN, *m_pConfig);
        if(iRet < 0)
       {
            TADD_NORMAL("This Process Begin Start Type With Not Attach.");
            m_bIsAttach = false;
            CHECK_RET(m_mdbDao.Init(m_pConfig, NULL),"Parameter Error, Init Dao Failed.");
        }
        else
        {
            m_bIsAttach = true;
            m_tProcCtrl.Init(pszDSN);
            CHECK_RET(m_tDDChangeNotify.Attach(pszDSN),"m_tDDChangeNotify. Attach failed");//

            CHECK_RET(m_mdbDao.Init(m_pConfig, m_pShmDSN),"Parameter Error, Init Dao failed.");
        }

		m_tFileList.Init(m_sLogPath);
        m_tExecFileList.Init(m_sLogPath);
        m_tRenameFileList.Init(m_sLogPath);\
         SAFE_CLOSE(m_fpErr);
        TADD_FUNC("Init(%s) : Finish.", pszDSN);
        return 0;
    }

    int mdbReadOraLog::InitDao()
    {
        int iRet = 0;
        while(true)
        {
            if (m_bIsAttach)
            {
                iRet = m_mdbDao.Init(m_pConfig, m_pShmDSN);
            }
            else
            {
                iRet = m_mdbDao.Init(m_pConfig, NULL);
            }
            
            if(iRet == ERR_APP_CONNCET_ORACLE_FAILED)
            {
                TADD_ERROR(ERR_APP_CONNCET_ORACLE_FAILED,"Can't Connect to Oracle,Init Dao Faild.");
                TMdbDateTime::Sleep(1);
            }
            else
            {
                break;
            }

           if(CheckAndUpdateProcStat() == false)
           {
                iRet = -1;
                break;
           }
        }
        return iRet;
    }


    int mdbReadOraLog::Start(int iCounts, int iPos)
    {
        if(m_bIsAttach == false)
        {
            TADD_NORMAL("OraRep(iCounts=%d, iPos=%d) : Start .", iCounts, iPos);
        }
        else
        {
            TADD_NORMAL("Start(iCounts=%d, iPos=%d) : Start .", iCounts, iPos);
        }
        int iRet = 0;
        while(true)
        {
            if(CheckAndUpdateProcStat() == false){return 0;}
            if(iPos == -1)
            {
                //获取insert,delete同步文件
                m_tRenameFileList.GetFileList(iCounts, iPos, "Ora", "");
                m_tExecFileList.GetFileList(iCounts, iPos, "Insert_Ora", "");
                TADD_DETAIL("File num = %d", m_tExecFileList.GetFileCounts());
            }
            else
            {
                //获取update同步文件
                m_tRenameFileList.GetFileList(iCounts, iPos, "Ora", "");
                m_tExecFileList.GetFileList(iCounts, iPos, "Ora", ".OK");
            }
            CheckMdbDDChange();
            if(IsFileExist(iPos) == false)
            {
                if(m_bIsAttach == false && m_tFileList.GetFileCounts() == 0)
                {
                    //如果是单独起进程处理，则同步完退出进程
                    TADD_FUNC("OraRep(iCounts=%d, iPos=%d) : Finish .", iCounts, iPos);
                    return 0;
                }
                continue;
            }
            
            while(m_tExecFileList.Next(m_sCurFileName) == 0)
            {
                TADD_NORMAL("Rep [%s] Start.", m_sCurFileName);
                if(m_bIsAttach == false)
                {
                    TADD_WARNING("Rep [%s] Start.", m_sCurFileName);
                }
                else
                {
                    TADD_NORMAL("Rep [%s] Start.", m_sCurFileName);
                }
                if(CheckAndUpdateProcStat() == false){return 0;}
                if(RepOneFile(m_sCurFileName,false) != 0)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Rep [%s] Failed.", m_sCurFileName);
                    return 0;
                }

                if(iPos == -1)
                {
                    RenameUpdate(&m_sCurFileName[strlen(m_sCurFileName)-14]);
                }
            }
            TMdbDateTime::MSleep(100);
        }
        TADD_NORMAL("Start(iCounts=%d, iPos=%d) : Finish.", iCounts, iPos);
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  RenameUpdate
    * 函数描述	:  更新update操作的数据日志文件
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    void mdbReadOraLog::RenameUpdate(const char* pszTime)
    {
        TADD_FUNC("(pszTime=%s) : Start.m_sLogPath=[%s].", pszTime,m_sLogPath);
        DIR *dp = opendir(m_sLogPath);
        if(dp == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"opendir(%s) failed.",m_sLogPath);
            return ;
        }
        //开始读取
        struct dirent *dirp;
        while((dirp=readdir(dp)) != NULL)
        {
            TADD_DETAIL("TMdbFileList::GetFileList() : d_name=[%s].", dirp->d_name);
            int iLen = strlen(dirp->d_name);
            if(iLen > 15 && strncmp(dirp->d_name, "Ora", 3) == 0 && dirp->d_name[iLen-1] != 'K')
            {
                TADD_DETAIL("TMdbFileList::GetFileList() : pszTime=[%s], d_name_time=[%s] ,d_name=[%s].", pszTime, &dirp->d_name[iLen-17],dirp->d_name);
                if( strcmp(pszTime, &dirp->d_name[iLen-14]) > 0)
                {
                    char sFileName[MAX_PATH_NAME_LEN];
                    memset(sFileName, 0, MAX_PATH_NAME_LEN);

                    char sNewFileName[MAX_PATH_NAME_LEN];
                    memset(sNewFileName, 0, MAX_PATH_NAME_LEN);

                    //超过最长长度,以当前时间YYYYMMDDHHM24SS表示
                    sprintf(sFileName, "%s%s", m_sLogPath, dirp->d_name);
                    sprintf(sNewFileName, "%s.OK", sFileName);
                    TADD_NORMAL("TMdbOraLog::CheckAndBackupUpdate() : sFileName=[%s] ==> sFileNameOld=[%s].", sFileName, sNewFileName);
                    //将日志文件重命名
                    TMdbNtcFileOper::Rename(sFileName, sNewFileName);
                }
            }
        }
        closedir(dp);
        TADD_FUNC("mdbReadOraLog::RenameUpdate(pszTime=%s) : Finish.", pszTime);
    }

    void mdbReadOraLog::RenameUpdateFile(const char* sFullFileName)
    {
        int iLen = strlen(sFullFileName);
        if(sFullFileName[iLen-1] != 'K')
        {
            char sNewFileName[MAX_PATH_NAME_LEN];
            memset(sNewFileName, 0, MAX_PATH_NAME_LEN);
            sprintf(sNewFileName, "%s.OK", sFullFileName);
            TMdbNtcFileOper::Rename(sFullFileName, sNewFileName);
        }
        return;
    }
    bool mdbReadOraLog::CheckAndUpdateProcStat()
    {
        if(m_bIsAttach == false)
        {
            return true;
        }
        
        if(m_tProcCtrl.IsCurProcStop())
        {    
            TADD_NORMAL("Get stop state:Stop...");
            return false;
        }

        m_tProcCtrl.UpdateProcHeart(0);

        return true;
    }

    bool mdbReadOraLog::IsFileExist(int iPos)
    {
        if(m_tExecFileList.GetFileCounts() != 0)
		{
			return true;
		}
        
        if(m_tRenameFileList.GetFileCounts() != 0)
        {
        	if(iPos == -1)
    		{
	            while(m_tRenameFileList.Next(m_sCurFileName) == 0)
	            {
	                if(TMdbNtcFileOper::IsExist(m_sCurFileName))
	                {
	                    RenameUpdateFile(m_sCurFileName);
	                }
	            }
		        return false;
    		}
			else
			{
				return true;
			}
        }
        TMdbDateTime::MSleep(100);
        return false;
    }

	/******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  初始化，用来工具解析同步日志的sql
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  miao.jianxin
    *******************************************************************************/

	int mdbReadOraLog::Init(const char* pszDSN)
	{
		TADD_FUNC("mdbReadOraLog::Init(%s) : Start.", pszDSN);
        SAFESTRCPY(m_sDsn,sizeof(m_sDsn),pszDSN);
        int iRet = 0;
        m_sDsn[strlen(m_sDsn)] = '\0';
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        CHECK_OBJ(m_pConfig);
        
       
       
        //找到管理区地址,连接上共享内存
        m_pShmDSN = new(std::nothrow) TMdbShmDSN();
		if(m_pShmDSN ==  NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY, "can't create new m_pShmDSN");
			return ERR_OS_NO_MEMROY;
		}
        m_pShmDSN->TryAttach();
        iRet = m_pShmDSN->Attach(pszDSN, *m_pConfig);
        if(iRet < 0)
        {
            TADD_NORMAL("This Process Begin Start Type With Not Attach.");
            m_bIsAttach = false;
           
        }
        else
        {
            m_bIsAttach = true;
         
          
        }

	
        TADD_FUNC("Init(%s) : Finish.", pszDSN);
        return 0;
	}

	
	/******************************************************************************
    * 函数名称	:  ParseLogFile
    * 函数描述	:  解析同步日志文件，分析出sql以及参数值
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  miao.jianxin
    *******************************************************************************/
	
	int mdbReadOraLog::ParseLogFile(char* sFileName)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		char* pLCR = NULL;
        int iLen = 0;
		CHECK_RET(m_tLogParser.ParseFile(sFileName),"ParseFile[%s] Failed",sFileName);
		while(true)
        {
            
            iLen = 0;
            if(m_tLogParser.NextRecord(pLCR, iLen) == 0)
            {
                if(pLCR == NULL){break;} //文件处理完了
            }
            else
            {
                TADD_ERROR(-1,"Get Next Record Faild");
                continue;
            }      
            if(m_tLogParser.Analyse(pLCR, m_tLCR) != 0){TADD_ERROR(-1,"Analyse Record Faild");continue;}
           
        	TMdbTable* pTable = NULL;
        	if (m_pShmDSN != NULL)
        	{
           		pTable = m_pShmDSN->GetTableByName(m_tLCR.m_sTableName.c_str());
        	}
       		else
        	{
            	pTable = m_pConfig->GetTableByName(m_tLCR.m_sTableName.c_str());
        	}
        
        	CHECK_OBJ(pTable);
        	m_tLCR.GetSQL(pTable);
			
			printf("---the sql is %s---\n",m_tLCR.m_sSQL);
			printf("---the sql paramet value--- is:\n");
			for(long unsigned int i=0; i<m_tLCR.m_vColms.size(); i++)
			{

				if(m_tLCR.m_vColms[i].m_bNull == true)
					printf("%s:%s\n",m_tLCR.m_vColms[i].m_sColmName.c_str(),"NULL");
				else
					printf("%s:%s\n",m_tLCR.m_vColms[i].m_sColmName.c_str(),m_tLCR.m_vColms[i].m_sColmValue.c_str());
			}
          
           
        }
		return iRet;
		
	}


	
    int mdbReadOraLog::RepOneFile(char* sFileName,bool iOneCommitFlag)
    {
        TADD_FUNC("Start.");
        
        int iRet = 0;
        int iRecordCount = 0;
        iRecordCount = 0;
        CHECK_RET(m_tLogParser.ParseFile(sFileName),"ParseFile[%s] Failed",sFileName);
        SAFE_CLOSE(m_fpErr);
        m_tRepFileStat.Clear();
        iRet = Excute(iRecordCount,iOneCommitFlag);
        if(iRet ==0 && iRecordCount > 0 && iOneCommitFlag == false){iRet = Commit();}

        //如果成功执行某个文件，则删除文件
        if(iRet == 0)
        {
            if(m_bIsAttach == false)
            {
                TADD_WARNING("Rep SuccessFul FileName = [%s][%d].", sFileName,iRecordCount);
                m_tRepFileStat.PrintStatInfo();
            }
            else
            {
                TADD_NORMAL("Rep SuccessFul FileName = [%s][%d].", sFileName,iRecordCount);
                m_tRepFileStat.PrintStatInfo();
            }
            TADD_NORMAL("Remove file[%s]", sFileName);
            TMdbNtcFileOper::Remove(sFileName);
            return iRet;
        }

        //如果数据库连不上，则重新练数据库
        if(iRet > 0)
        {
            if(m_bIsAttach == false){return iRet;}
            iRet = InitDao();
            if(iRet != 0){return iRet;}
            return RepOneFile(sFileName,false);
        }
        else
        {
            return RepOneFile(sFileName,true);
        }

        return iRet;
        TADD_FUNC("End.");
    }

    int mdbReadOraLog::Excute(int &iRecordCount,bool iOneCommitFlag)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //TMdbOneRecord* pOneRecord = NULL;
        char* pLCR = NULL;
        int iLen = 0;
        time_t tStart; //统计每1千条化的时间
        time_t tEnd;
        time(&tStart);
        while(true)
        {
            if(m_bIsAttach == true && iRecordCount%1000 == 0)
            {
                time(&tEnd);
                if(tEnd - tStart > m_pConfig->GetProAttr()->iHeartBeatWarning)
                {
                    TADD_WARNING("RepToOracle Very Slowly,Please Check Config");
                }
                tStart = tEnd;
                m_tProcCtrl.UpdateProcHeart(0);
            }
            iLen = 0;
            if(m_tLogParser.NextRecord(pLCR, iLen) == 0)
            {
                if(pLCR == NULL){break;} //文件处理完了
            }
            else
            {
                TADD_ERROR(-1,"Get Next Record Faild");
                continue;
            }      
            if(m_tLogParser.Analyse(pLCR, m_tLCR) != 0){TADD_ERROR(-1,"Analyse Record Faild");continue;}
            //pOneRecord = m_fileParser.Next();
            //if(pOneRecord == NULL){break;}
            iRecordCount++;
            m_tRepFileStat.Stat(m_tLCR);
            iRet = m_mdbDao.Execute(m_tLCR);
            if(iOneCommitFlag == true)
            {
                if(iRet == 0){ m_mdbDao.Commit();}
                if(iRet > 0){break;}
                if(iRet < 0)
                {
                     RecordError(m_sLogPath,m_sErrorLogPath,pLCR,iLen);
                     iRet = 0;
                    continue;
                }
                else
                {
                    continue;
                }
            }
            if (iRet != 0){break;}
        }
        TADD_FUNC("End.");
        return iRet;
    }

    int mdbReadOraLog::Commit()
    {
        TADD_FUNC("Start.");
        TADD_FUNC("End.");
        return m_mdbDao.Commit();
    }
    /******************************************************************************
    * 函数名称	:  CheckMdbDDChange
    * 函数描述	:  检测是否有数据定义变更
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int mdbReadOraLog::CheckMdbDDChange()
    {
        int iRet = 0;
        /*
        if(m_bIsAttach)
        {
            std::vector<int > vDeleteTable;
            m_tDDChangeNotify.GetDeleteTable( vDeleteTable);//获取已删除的表id
            std::vector<int>::iterator itor  = vDeleteTable.begin();
            for(;itor != vDeleteTable.end();++itor)
            {
                TADD_NORMAL("Try Clear Dao tableid[%d]",*itor);
                m_mdbDao.ClearDAOByTableId(*itor);//清理
                m_fileParser.ClearSQLCacheByTableId(*itor);
            }
        }
        */
        return iRet;
    }

    bool mdbReadOraLog::RecordError(const char* pszLogPath,const char* pszErrorLogPath,char* psErrRecd, int iRecdLen)
    {
        TADD_FUNC("Start.");
        
        char sErrorFile[512] = {0};
        char sFileName[256] = {0};
        
        char *pFileName = m_sCurFileName+strlen(pszLogPath);
        strcpy(sFileName,pFileName);
        sprintf(sErrorFile, "%s%s_ERROR", pszErrorLogPath,sFileName);

        if(m_fpErr == NULL)
        {
            m_fpErr = fopen(sErrorFile, "a+");
            if(m_fpErr == NULL)
            {
                TADD_ERROR(-1,"[%s : %d] :  fopen(%s) failed.", __FILE__, __LINE__, sErrorFile);
                return -1;   
            }
        }
        
        int iLenW = fwrite(psErrRecd, iRecdLen, 1, m_fpErr);	
        if (iLenW < 1)
        {
            TADD_ERROR(-1,"[%s : %d] : fwrite() failed(iLenW=%d, iLen=%d), errno=%d, errmsg=[%s].", 
                __FILE__, __LINE__, iLenW, iRecdLen, errno, strerror(errno));	
            return -1;
        }

        fflush(m_fpErr);
        TADD_FUNC("Finished.");
        return true;
    }
//}

