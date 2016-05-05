#include "Interface/mdbQuery.h"
#include "stdio.h"
struct ST_PERF_TEST_ZIP_L_TIME
{
    long long int_1;
    long long int_2;
    char        str_1[34];
    char        str_2[34];
    char        str_3[34];
    char        str_4[34];
    long long int_3;
    long long int_4;
    long long        date_1;
    long long        date_2;
    long long        date_3;
    long long        date_4;
    void show()
    {
        printf("int_1[%lld],str_1[%s],str_2[%s],date_1[%lld]\n",int_1,str_1,str_2,date_1);
    }
};


int main()
{
	char sUser[256]="dc";
	char sPwd[256]="dc";
	char sDsn[256]="dc";
	
	ST_PERF_TEST_ZIP_L_TIME stPerfTest;
	
	int iTimeSize = sizeof(long long);
	int arrColumn[12] = {0};
	arrColumn[0] = 0;
	arrColumn[1] = arrColumn[0]+ sizeof(long long);
	arrColumn[2] = arrColumn[1]+ sizeof(long long);
	arrColumn[3] = arrColumn[2]+ 34;
	arrColumn[4] = arrColumn[3]+ 34;
	arrColumn[5] = arrColumn[4]+ 34;
	arrColumn[6] = arrColumn[5]+ 34;
	arrColumn[7] = arrColumn[6]+ sizeof(long long);
	arrColumn[8] = arrColumn[7]+ sizeof(long long);
	arrColumn[9] = arrColumn[8]+ iTimeSize;
	arrColumn[10] = arrColumn[9]+ iTimeSize;
	arrColumn[11] = arrColumn[10]+ iTimeSize;
	
	
	
	
	try
	{
		TMdbDatabase tDB;
		if(false == tDB.Connect(sUser,  sPwd, sDsn))
		{
			printf("db not connected!\n");
			return 0;
		}
		TMdbNosqlQuery* pQuery = tDB.CreateNosqlQuery();
		if(NULL == pQuery)
		{
			printf("create nosql query failed..\n");
			return 0;
		}
		
		pQuery->Close();
		pQuery->SetTable("LZIP");
		pQuery->SetKey("int_1",2);
		pQuery->SetKey("str_1",'2');
		pQuery->Find();
		int iCnt = 0;
		while(pQuery->Next())
		{
			pQuery->GetValue(&stPerfTest,arrColumn);
			stPerfTest.show();
			iCnt++;
		}
		
		pQuery->Close();
		
		if(0 == iCnt)
		{
			printf("no record found\n");
		}
		
	}
	catch(TMdbException& e)
	{
		printf("errmsg[%s]\n", e.GetErrMsg());
	}

	return 0;
}