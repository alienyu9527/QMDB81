/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbDBInterface.cpp		
*@Description�� �ṩDB���ʽӿڻ���
*@Author:		li.shugang
*@Date��	    2009��08��13��
*@History:
******************************************************************************************/
#include "Helper/mdbDBInterface.h"
#if 0
/*

TMdbArrayString::TMdbArrayString()
{
    Clear();
}


TMdbArrayString::~TMdbArrayString()
{
    if( NULL != sParamValue )
    {//�ͷ�ԭ�����ڴ�
    	delete []sParamValue;
        sParamValue = NULL;
    }

}

     
void TMdbArrayString::Clear()
{
    iInterval  = -1;
    iArraySize = -1;
    
    memset(sParamName, 0, sizeof(sParamName));
    sParamValue = NULL;
}
void TMdbArrayString::Resize()
{
    if( NULL != sParamValue )
    {//�ͷ�ԭ�����ڴ�
    	delete []sParamValue;
        sParamValue = NULL;
    }
	if (iArraySize <= 0)
	{
	    TADD_ERROR("[%s : %d] : TMdbArrayString::Resize() : error iArraySize[%d].", __FILE__, __LINE__,iArraySize);
		return ;
	}
	sParamValue = new char*[iArraySize];
	memset(sParamValue,0,sizeof(char*)*iArraySize);
	
}


TMdbArrayLong::TMdbArrayLong()
{
    Clear();
}


TMdbArrayLong::~TMdbArrayLong()
{  
	if( NULL != iParamValue )
	{
		delete []iParamValue;
        iParamValue = NULL;
	}
}

void TMdbArrayLong::Resize()
{
	if( NULL != iParamValue )
	{
		delete []iParamValue;
        iParamValue = NULL;
	}
    if (iArraySize <= 0)
	{
	    TADD_ERROR("[%s : %d] : TMdbArrayLong::Resize() : error iArraySize[%d].", __FILE__, __LINE__,iArraySize);
		return ;
	}
	iParamValue = new long long[iArraySize];
	memset(iParamValue,0,sizeof(long long)*iArraySize);
} 
     
void TMdbArrayLong::Clear()
{
    iInterval  = -1;
    iArraySize = -1;
    memset(sParamName, 0, sizeof(sParamName));
    iParamValue = NULL;
}
*/
#endif

