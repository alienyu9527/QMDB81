/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbFlushSequence .cpp
*@Description： 刷出sequence到oracle
*@Author:	jin.shaohua
*@Date：	    2013.10
*@History:
******************************************************************************************/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "Helper/TThreadLog.h"
#include "Replication/mdbQueueLog.h"
//#include "Helper/mdbProcess.h"

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


int FlushSequence(TMdbShmDSN * pShmDSN, FILE * & pSeqFile)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pShmDSN);
    if(pSeqFile == NULL)
    {
        SAFE_CLOSE(pSeqFile);
        char sFileName[MAX_NAME_LEN] = {0};
        sprintf(sFileName,"%s%s",pShmDSN->GetInfo()->sStorageDir,"Sequence.mdb");
        pSeqFile = fopen (sFileName,"wb+");
        CHECK_OBJ(pSeqFile);
    }

    //判断是否需要
    bool isHave=false;
    TShmList<TMemSeq>::iterator itor = pShmDSN->m_MemSeqList.begin();
    for(;itor != pShmDSN->m_MemSeqList.end();++itor)
    {
        isHave = true;
    }

    if(isHave == false){return iRet;}
    
    CHECK_RET(fseek(pSeqFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
    itor = pShmDSN->m_MemSeqList.begin();
    for(;itor != pShmDSN->m_MemSeqList.end();++itor)
    {
        TMemSeq *pSeq = &(*itor);
        if(fwrite(pSeq, sizeof(TMemSeq), 1, pSeqFile)!= 1 )
        {
            CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
    }

    fflush(pSeqFile);

    TADD_FUNC(" Finish.");
    return iRet;
}



int main(int argc, char* argv[])
{

    //int iRet = 0;

    if(argc != 2|| strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        printf("-------\n"
               " Usage:\n"
               "   %s <sDsn> \n"
               "   %s [ -H | -h ] \n"
               " Example:\n"
               "   %s Check \n"
               " Note:\n"
               "     <sDsn>: data source.\n"
               "     -H|-h Print Help.\n"
               "-------\n" ,argv[0],argv[0],argv[0]);
        return 0;
    }
    //Process::SetProcessSignal();
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); /* run in backgroud */
#endif
    char * sDsn = argv[1];
    char sName[64] = {0};
    memset(sName, 0, sizeof(sName));
    sprintf(sName, "%s %s", argv[0], sDsn);
    TADD_START(sDsn,sName, 0, false,true);
    int iRet = 0;
    TMdbShmDSN * pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
    CHECK_OBJ(pShmDSN);
    TMdbProcCtrl tProcCtrl;
    tProcCtrl.Init(sDsn);
    FILE * pSeqFile = NULL;
    while(1)
    {
        if(tProcCtrl.IsCurProcStop())
        {
            TADD_NORMAL("Stop....");
            break;
        }
        if(0 != FlushSequence(pShmDSN,pSeqFile))
        {//有问题
            SAFE_CLOSE(pSeqFile);
        }
        tProcCtrl.UpdateProcHeart(1);//心跳
    }
    SAFE_CLOSE(pSeqFile);
    return iRet;
}


