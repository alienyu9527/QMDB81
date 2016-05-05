/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbDBInterface.cpp		
*@Description： 提供DB访问接口基类
*@Author:		li.shugang
*@Date：	    2009年08月13日
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
    {//释放原来的内存
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
    {//释放原来的内存
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

