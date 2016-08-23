/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbStartCheck.cpp
*@Description�� �ڴ����ݿ��Oracleˢ�¼����򣨴��ڴ�ͬ����oracle��
*@Author:		jiang.mingjun
*@Date��	    2009��07��23��
*@History:
******************************************************************************************/

#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbCommandlineParser.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbConfig.h"
#include "Helper/mdbFileList.h"

#ifdef WIN32
#pragma comment(lib,"Interface.lib")
#pragma comment(lib,"Helper.lib")
#pragma comment(lib,"Tools.lib")
#pragma comment(lib,"Control.lib")
#pragma comment(lib,"Monitor.lib")
#pragma comment(lib,"DataCheck.lib")
#pragma comment(lib,"OracleFlush.lib")
#pragma comment(lib,"Agent.lib")
#pragma comment(lib,"Replication.lib")
#endif

//using namespace QuickMDB;

void Help(int argc, char* argv[])
{
    printf("-------\n"
        " Usage\n"
        "   %s \n"
        "      -c <sDsn>  [-H -L] \n"
        " Example\n"
        "   %s \n"
        "      -c ocs  \n"
        " Note\n"
        "      -c data source .\n"
        "      -H|-h Print Help.\n"
        "-------\n" ,argv[0],argv[0]);
}

//���������в���
int ParseParam(int argc, char* argv[],char *pDSN,const int iDsnLen)
{
    //���������ʽ��ѡ��
    CommandLineParser clp(argc, argv);
    clp.set_check_condition("-H", 0);
    clp.set_check_condition("-h", 0);
    clp.set_check_condition("-c", 1);
    clp.check();

    const vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
    vector<CommandLineParser::OptArgsPair>::const_iterator it;
    for (it = pairs.begin(); it != pairs.end(); ++it)
    {
        const string& opt = (*it)._first;
        const vector<string>& args = (*it)._second;
        if(opt == "-h" || opt == "-H")
        {
            Help(argc,argv);
            return -1;
        }
        if(opt == "-c")
        {
            SAFESTRCPY(pDSN,iDsnLen,args[0].c_str());
            pDSN[strlen(pDSN)] = '\0';
            continue;
        }
    }
    if(strlen(pDSN) == 0)
    {
        Help(argc,argv);
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    //Process::SetProcessSignal();
    int iRet = 0;
    char sDsn[16];
    memset(sDsn,0,sizeof(sDsn));
    if(argc == 1)
    {
        Help(argc,argv);
        return iRet;
    }
    CHECK_RET(ParseParam(argc,argv,sDsn,sizeof(sDsn)),"Analytical parameter error.");
    //----------------------------------------------------------
    
#ifndef WIN32
    TADD_START(sDsn,"mdbStartCheck", 0,true,false);
#endif

    TMdbConfig *pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
    CHECK_OBJ(pConfig);
    
    //��ȡ·����Ϣ
    char sLogPath[MAX_PATH_NAME_LEN];
    memset(sLogPath, 0, sizeof(sLogPath));
    SAFESTRCPY(sLogPath,sizeof(sLogPath),pConfig->GetDSN()->sLogDir);
    if(sLogPath[strlen(sLogPath)-1] != '/')
    {
        sLogPath[strlen(sLogPath)] = '/';
    }
    sLogPath[strlen(sLogPath)] = '\0';

     //����ļ�Insert_Ora��Ora0��Ora1 ...��sizeΪ0��ɾ��������������Ϊ.OK�ļ� 
    char sFileOra[MAX_PATH_NAME_LEN];
    char sFileOraNew[MAX_PATH_NAME_LEN];
    char sTime[MAX_TIME_LEN];
    memset(sTime,0,MAX_TIME_LEN);
    MDB_UINT64 iFileSize = 0;
    int iOraRepCount = pConfig->GetDSN()->iOraRepCounts;
    for (int i = -1; i<iOraRepCount; i++)
    {
        if (-1 == i)
        {
            snprintf(sFileOra, MAX_PATH_NAME_LEN, "%s%s",sLogPath,"Insert_Ora");
        }
        else
        {
            snprintf(sFileOra, MAX_PATH_NAME_LEN, "%s%s%d", sLogPath, "Ora", i);
        }

        TADD_DETAIL("File name = %s", sFileOra);
        if(TMdbNtcFileOper::IsExist(sFileOra))
        {
            TMdbNtcFileOper::GetFileSize(sFileOra,iFileSize);
            TADD_DETAIL("Oracle rep file=[%s], file size=[%lld].", sFileOra, iFileSize);
            if (iFileSize>0)
            {
                memset(sFileOraNew,0,MAX_PATH_NAME_LEN);
                memset(sTime,0,MAX_TIME_LEN);
                TMdbDateTime::GetCurrentTimeStr(sTime);
                sprintf(sFileOraNew,"%s.%s",sFileOra,sTime);

                TMdbNtcFileOper::Rename(sFileOra, sFileOraNew);//������Ϊ.OK�ļ�
            }
            else//���ļ���ɾ��
            {
                TADD_DETAIL("Delete file = [%s], size = [0].", sFileOra);
                TMdbNtcFileOper::Remove(sFileOra);
            }
        }
    }
    

    int *ipid = new(std::nothrow) int[pConfig->GetDSN()->iOraRepCounts + 1];
	CHECK_OBJ(ipid);
	
	for (int i = 0; i < pConfig->GetDSN()->iOraRepCounts + 1; i++)
	{
		ipid[i] = 0;
	}
    if(pConfig->GetDSN()->bIsOraRep == true 
        && pConfig->GetIsStartOracleRep() == true)
    {
        //��������־ˢ�µ�Oracle�Ľ���
        int i=-1;
        char sNameTemp[MAX_NAME_LEN];
        for(i=-1; i<pConfig->GetDSN()->iOraRepCounts; ++i)
        {
            memset(sNameTemp, 0, sizeof(sNameTemp));
            sprintf(sNameTemp, "mdbDbRep %s %d %d", sDsn, pConfig->GetDSN()->iOraRepCounts, i);
            TADD_NORMAL("Start process [%s]", sNameTemp);
            system(sNameTemp);
        }

        //����Insert_Ora Ora        
        TADD_DETAIL("StartCheck : LocalPath=[%s].",sLogPath);
        TMdbFileList tFileList;
        tFileList.Init(sLogPath);
        while(1)
        {
            tFileList.GetFileList(1, 0, "Insert_Ora","");
            int iInsertCount = tFileList.GetFileCounts();
            tFileList.GetFileList(1, 0, "Ora",".OK");
            int iUpdateCount =  tFileList.GetFileCounts();
            if(iInsertCount == 0 && iUpdateCount == 0)
            {
                //kill���̺��˳�
                char sProcessName[MAX_NAME_LEN];
                memset(sProcessName,0,MAX_NAME_LEN);
                sprintf(sProcessName,"%s %s","mdbDbRep",sDsn);
                TMdbOS::GetPidByName(sProcessName,ipid);
                for(int j=0; j<pConfig->GetDSN()->iOraRepCounts + 1; j++ )
                {
                    if(0 != ipid[j])
                    {
                        TMdbOS::KillProc(ipid[j]);
                    }
                }
                break;
            }
            TMdbDateTime::Sleep(1);//����������ѭ��
        }
        TADD_NORMAL("StartCheck .");
    }
    SAFE_DELETE_ARRAY(ipid);
    return iRet;
}


