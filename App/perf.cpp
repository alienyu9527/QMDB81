#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "Interface/mdbQuery.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "Helper/mdbDateTime.h"
#include "Helper/mdbStruct.h"


using namespace std;

//try-catch ��
#define _TRY_CATCH_BEGIN_ try{

#define _TRY_CATCH_END_ }\
catch(TMdbException& e)\
{\
    printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());\
    iRet = -1;\
}\
catch(...)\
{\
    printf("UnKown error!\n");\
    iRet = -1;\
}

//��½�û������
struct INFO
{
	char UID[32];
	char PWD[32];
	char DSN[32];
};

INFO g_info[] = {
{"uid","pwd","dsn"},  //Mhash
{"uid","pwd","test"}	//hash


};

int CYCLE_NUM = 40000;
int FLAG = 0;
int T_INDEX = 0;
//�ж�_retֵ,_ret�����Ǻ������ء�����Ϊ0�򱨴����ش�����
#define CHECK_RET(_ret,...) if((iRet = _ret)!= 0){ printf(__VA_ARGS__);return iRet;}
//��ȫɾ��ָ��
#define SAFE_DELETE(_obj) if(NULL != _obj){delete _obj;_obj=NULL;}
//��ȫɾ��ָ������
#define SAFE_DELETE_ARRAY(_obj) if(NULL != _obj){delete[] _obj;_obj=NULL;}
//�ж�ָ���Ƿ�Ϊ�գ���Ϊ�գ��������ش�����
#define CHECK_OBJ(_obj) if(NULL == _obj){printf( #_obj" is null"); return ERROR_APP_PARAM_IS_NULL;}
//�ж�_retֵ������Ϊ0���򱨴�break
#define CHECK_RET_BREAK(_ret,...) if((iRet = _ret)!=0){printf(__VA_ARGS__);break;}
//��ȫcloseָ��
#define SAFE_CLOSE(_fp) if(NULL != _fp){fclose(_fp);_fp=NULL;}
//��ȫfreeָ��
#define SAFE_FREE(obj) if (NULL != obj){free(obj);obj = NULL;}

#define ArraySize(X) (int)(sizeof(X)/sizeof(X[0]))

/******************************************************************************
* ��������	:  ConnectMDB
* ��������	:  ֱ���������ݿ�
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int ConnectMDB(TMdbDatabase & tDB,int index)
{
    int iRet = 0;
    _TRY_CATCH_BEGIN_
    tDB.SetLogin(g_info[index].UID, g_info[index].PWD, g_info[index].DSN);//�����û��������룬dsn
    if(tDB.Connect() == false)
    {
        printf("ERROR : Can't connect to [%s/%s@%s].\n",
			g_info[index].UID, g_info[index].PWD, g_info[index].DSN);
        return -1;
    }
    else
    {
        printf("Success Direct connect to [%s/%s@%s].\n",
			g_info[index].UID, g_info[index].PWD, g_info[index].DSN);
    }
    _TRY_CATCH_END_
    return iRet;
}


void* TestInsert(void* p)
{
    int iRet = 0;
	int iaffact = 0;
	int index = *(int*)p;
    TMdbDatabase mdb;
    TMdbQuery* pQuery = NULL;

	
    printf("\nTestInsert\n");
    try
    {
       	printf("begin to connect mdb\n");
       	ConnectMDB(mdb,0);

		printf("begin to CreateDBQuery\n");
        pQuery = mdb.CreateDBQuery();
        pQuery->Close();

		printf("begin to SetSQL\n");
		pQuery->SetSQL("insert into ylx(SESSION_ID,routing_id,SERV_BEGIN_TIME,NAME) values(:SESSION_ID,:routing_id,:SERV_BEGIN_TIME,:NAME);");

		char m_sExecStartTime[256];
		char m_sExecEndTime[256];
		float fDiffTime = 0.0;

		struct timeval tTV;
    	gettimeofday(&tTV, NULL);
    	TMdbDateTime::GetCurrentTimeStr(m_sExecStartTime);
    	sprintf(&m_sExecStartTime[14],"%03d",tTV.tv_usec/1000);
        for(int i=0;i<CYCLE_NUM;i++)
    	{
    		char sSession_ID[128] = {0};
			sprintf(sSession_ID,"%d",i);

			
	 		pQuery->SetParameter(0,sSession_ID);
			pQuery->SetParameter(1, i + index* CYCLE_NUM);
	        pQuery->SetParameter(2,"20160222125959");
	        pQuery->SetParameter(3,"ylx");

			
	 		pQuery->Execute();

			pQuery->Commit();
			
			iaffact += pQuery->RowsAffected();
    	}    

		    gettimeofday(&tTV, NULL);
		    TMdbDateTime::GetCurrentTimeStr(m_sExecEndTime);
		    sprintf(&m_sExecEndTime[14],"%03d",tTV.tv_usec/1000);
		    fDiffTime = TMdbDateTime::GetDiffMSecond(m_sExecEndTime,m_sExecStartTime);
		    printf("Done in %0.3f seconds.\n",fDiffTime/1000);
		
    }
    catch(...)
    {
        printf("exception\n");
        iRet = -1;
    }
    SAFE_DELETE(pQuery);
}

void* TestUpdate(void* p)
{
    int iRet = 0;
	int iaffact = 0;
	int index = *(int*)p;
	srand(getpid());
	int irand = rand()%8;
	
    TMdbDatabase mdb;
    TMdbQuery* pQuery = NULL;
    printf("\nTestInsert\n");
    try
    {
       	printf("begin to connect mdb\n");
       	ConnectMDB(mdb,0);

		printf("begin to CreateDBQuery\n");
        pQuery = mdb.CreateDBQuery();
        pQuery->Close();

		printf("begin to SetSQL\n");
		pQuery->SetSQL("Update ylx set SESSION_ID=:New where routing_id=:Old;");

		char m_sExecStartTime[256];
		char m_sExecEndTime[256];
		float fDiffTime = 0.0;

		struct timeval tTV;
    	gettimeofday(&tTV, NULL);
    	TMdbDateTime::GetCurrentTimeStr(m_sExecStartTime);
    	sprintf(&m_sExecStartTime[14],"%03d",tTV.tv_usec/1000);
        for(int i=0;i<CYCLE_NUM;i++)
    	{

	 		pQuery->SetParameter(0,"new");
			pQuery->SetParameter(1,CYCLE_NUM*index+i);
	        
	 		pQuery->Execute();

			pQuery->Commit();
			
			iaffact += pQuery->RowsAffected();
    	}    

		    gettimeofday(&tTV, NULL);
		    TMdbDateTime::GetCurrentTimeStr(m_sExecEndTime);
		    sprintf(&m_sExecEndTime[14],"%03d",tTV.tv_usec/1000);
		    fDiffTime = TMdbDateTime::GetDiffMSecond(m_sExecEndTime,m_sExecStartTime);
		    printf("Done in %0.3f seconds.Update iAffect:%d\n",fDiffTime/1000,iaffact);
		
    }
    catch(...)
    {
        printf("exception\n");
        iRet = -1;
    }
    SAFE_DELETE(pQuery);
}



void* TestSelect(void* p)
{
    int iRet = 0;
	int iaffact = 0;
	int index = *(int*)p;
    TMdbDatabase mdb;
    TMdbQuery* pQuery = NULL;
    printf("\nTestInsert\n");
    try
    {
       	printf("begin to connect mdb\n");
       	ConnectMDB(mdb,1);

		printf("begin to CreateDBQuery\n");
        pQuery = mdb.CreateDBQuery();
        pQuery->Close();

		printf("begin to SetSQL\n");
		pQuery->SetSQL("select * from ylx where routing_id = :routing_id");

		char m_sExecStartTime[256];
		char m_sExecEndTime[256];
		float fDiffTime = 0.0;

		struct timeval tTV;
    	gettimeofday(&tTV, NULL);
    	TMdbDateTime::GetCurrentTimeStr(m_sExecStartTime);
    	sprintf(&m_sExecStartTime[14],"%03d",tTV.tv_usec/1000);
        for(int i=0;i<CYCLE_NUM;i++)
    	{
			pQuery->SetParameter(0,i);
			
	        pQuery->Open();

			while(pQuery->Next()){iaffact++;}
			
    	}    

		    gettimeofday(&tTV, NULL);
		    TMdbDateTime::GetCurrentTimeStr(m_sExecEndTime);
		    sprintf(&m_sExecEndTime[14],"%03d",tTV.tv_usec/1000);
		    fDiffTime = TMdbDateTime::GetDiffMSecond(m_sExecEndTime,m_sExecStartTime);
		    printf("Done in %0.3f seconds.\n",fDiffTime/1000);
		
    }
    catch(...)
    {
        printf("exception\n");
        iRet = -1;
    }
    SAFE_DELETE(pQuery);
}



void* TestDelete(void* p)
{
    int iRet = 0;
	int iaffact = 0;
	int index = *(int*)p;
    TMdbDatabase mdb;
    TMdbQuery* pQuery = NULL;
    
    printf("\nTestInsert\n");
    try
    {
       	printf("begin to connect mdb\n");
       	ConnectMDB(mdb,0);

		printf("begin to CreateDBQuery\n");
        pQuery = mdb.CreateDBQuery();
        pQuery->Close();

		printf("begin to SetSQL\n");
		pQuery->SetSQL("Delete from  ylx  where routing_id=:Old");

		char m_sExecStartTime[256];
		char m_sExecEndTime[256];
		float fDiffTime = 0.0;

		struct timeval tTV;
    	gettimeofday(&tTV, NULL);
    	TMdbDateTime::GetCurrentTimeStr(m_sExecStartTime);
    	sprintf(&m_sExecStartTime[14],"%03d",tTV.tv_usec/1000);
		//char blob[8] = {'y','l','x',0,0,'l','a',0};
        for(int i=0;i<CYCLE_NUM;i++)
    	{
	 		pQuery->SetParameter(0,CYCLE_NUM*index+i);
	 		pQuery->Execute();
			pQuery->Commit();
			iaffact += pQuery->RowsAffected();
    	}    

		    gettimeofday(&tTV, NULL);
		    TMdbDateTime::GetCurrentTimeStr(m_sExecEndTime);
		    sprintf(&m_sExecEndTime[14],"%03d",tTV.tv_usec/1000);
		    fDiffTime = TMdbDateTime::GetDiffMSecond(m_sExecEndTime,m_sExecStartTime);
		    printf("Done in %0.3f seconds.Delete iAffect:%d\n",fDiffTime/1000,iaffact);
		
    }
    catch(...)
    {
        printf("exception\n");
        iRet = -1;
    }
    SAFE_DELETE(pQuery);
}



int main(int argc, char** argv)
{		
		if(argc >1) CYCLE_NUM=atoi(argv[1]);
		if(argc >2) FLAG=atoi(argv[2]);
		if(argc >3) T_INDEX = atoi(argv[3]);
		int iRet = 0;
		int thid = T_INDEX;


		switch(FLAG)
		{
			case 0:
				TestSelect((void*)&thid);	
				break;
			case 1:
				TestInsert((void*)&thid);	
				break;
			case 2:
				TestUpdate((void*)&thid);	
				break;	
			case 3:
				TestDelete((void*)&thid);	
				break;

		}
		
		

		
		return 0;
}
