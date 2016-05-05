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

//try-catch 宏
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

//登陆用户密码等
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
int THREAD_NUM = 2;
int DSN_INDEX = 0;
//判断_ret值,_ret可能是函数返回。若不为0则报错并返回错误码
#define CHECK_RET(_ret,...) if((iRet = _ret)!= 0){ printf(__VA_ARGS__);return iRet;}
//安全删除指针
#define SAFE_DELETE(_obj) if(NULL != _obj){delete _obj;_obj=NULL;}
//安全删除指针数组
#define SAFE_DELETE_ARRAY(_obj) if(NULL != _obj){delete[] _obj;_obj=NULL;}
//判断指针是否为空，若为空，报错并返回错误码
#define CHECK_OBJ(_obj) if(NULL == _obj){printf( #_obj" is null"); return ERROR_APP_PARAM_IS_NULL;}
//判断_ret值，若不为0，则报错并break
#define CHECK_RET_BREAK(_ret,...) if((iRet = _ret)!=0){printf(__VA_ARGS__);break;}
//安全close指针
#define SAFE_CLOSE(_fp) if(NULL != _fp){fclose(_fp);_fp=NULL;}
//安全free指针
#define SAFE_FREE(obj) if (NULL != obj){free(obj);obj = NULL;}

#define ArraySize(X) (int)(sizeof(X)/sizeof(X[0]))

/******************************************************************************
* 函数名称	:  ConnectMDB
* 函数描述	:  直连链接数据库
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int ConnectMDB(TMdbDatabase & tDB,int index)
{
    int iRet = 0;
    _TRY_CATCH_BEGIN_
    tDB.SetLogin(g_info[index].UID, g_info[index].PWD, g_info[index].DSN);//设置用户名，密码，dsn
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
       	ConnectMDB(mdb,index);

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
			pQuery->SetParameter(1,i);
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
       	ConnectMDB(mdb,index);

		printf("begin to CreateDBQuery\n");
        pQuery = mdb.CreateDBQuery();
        pQuery->Close();

		printf("begin to SetSQL\n");
		pQuery->SetSQL("select * from ylx where 1=1");

		char m_sExecStartTime[256];
		char m_sExecEndTime[256];
		float fDiffTime = 0.0;
		
		char recordBuff[8192] = {0};
		int colOffset[4];
		colOffset[0] = 0;
		colOffset[1]  = colOffset[0]+256;
		colOffset[2] = colOffset[1]+8;
		colOffset[3]=colOffset[2]+15;

		TMdbColumnAddr *pMdbColumnAddr = new TMdbColumnAddr();

		struct timeval tTV;
    	gettimeofday(&tTV, NULL);

		//@111
    	TMdbDateTime::GetCurrentTimeStr(m_sExecStartTime);
    	sprintf(&m_sExecStartTime[14],"%03d",tTV.tv_usec/1000);	
	    pQuery->Open();
		while(pQuery->Next())
		{				
			pQuery->GetValue((void*)recordBuff, colOffset);				
		}
			 
	    gettimeofday(&tTV, NULL);
	    TMdbDateTime::GetCurrentTimeStr(m_sExecEndTime);
	    sprintf(&m_sExecEndTime[14],"%03d",tTV.tv_usec/1000);
	    fDiffTime = TMdbDateTime::GetDiffMSecond(m_sExecEndTime,m_sExecStartTime);
	    printf("GetVal Old Done in %0.3f seconds.\n",fDiffTime/1000);
		//end


		//@222
		pQuery->Close();
		pQuery->SetSQL("select * from ylx where 1=1");
		TMdbDateTime::GetCurrentTimeStr(m_sExecStartTime);
    	sprintf(&m_sExecStartTime[14],"%03d",tTV.tv_usec/1000);	
	    pQuery->Open();
		while(pQuery->Next())
		{				
			pQuery->GetValue(pMdbColumnAddr);				
		}
			 
	    gettimeofday(&tTV, NULL);
	    TMdbDateTime::GetCurrentTimeStr(m_sExecEndTime);
	    sprintf(&m_sExecEndTime[14],"%03d",tTV.tv_usec/1000);
	    fDiffTime = TMdbDateTime::GetDiffMSecond(m_sExecEndTime,m_sExecStartTime);
	    printf("GetVal New Done in %0.3f seconds.\n",fDiffTime/1000);
		
		
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
		if(argc >2) THREAD_NUM=atoi(argv[2]);
		if(argc >3) DSN_INDEX = atoi(argv[3]);
		int iRet = 0;
		int index = DSN_INDEX;

		if(THREAD_NUM == 0)
		{
			TestSelect((void*)&index);	
			return 0;
		}
		
		pthread_t* pidArrr = new pthread_t[THREAD_NUM];


		for(int i = 0  ; i<1 ; i++)
		{
			pidArrr[i]=0;
			CHECK_RET(pthread_create(&pidArrr[i], NULL , TestSelect, (void*)&index),"Can't pthread_create1()");
		}
		
		
		
		for(int i = 0  ; i<1 ; i++)
		{
			pthread_join(pidArrr[i],NULL);		
		}

		SAFE_DELETE_ARRAY(pidArrr);
		
		return 0;
}
