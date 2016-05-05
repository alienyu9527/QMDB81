#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "Helper/TBaseException.h"
#ifdef _WIN32
#pragma warning(disable: 4996)
#endif



TBaseException::TBaseException()
{
    m_strErrMsg="Unknown Exception";
}

TBaseException::~TBaseException()
{
    m_strErrMsg="";
}


TBaseException::TBaseException(const char * fmt, ...)
{
    va_list ap;
    va_start (ap,fmt);

    try{
        char sLine[10240];
		memset(sLine,0,sizeof(sLine));
        vsprintf(sLine,fmt,ap);
        m_strErrMsg = sLine;
        vprintf(fmt,ap);
        printf("\n\n");
    }
    catch(...)
    {
        printf("Write Buffer Overflow. [%s]\n",fmt);
    }
    va_end (ap);
}

TBaseException::TBaseException(const string strErrMsg)
{
    printf("%s\n\n",strErrMsg.c_str());
    m_strErrMsg=strErrMsg;
}

/******************************************************************************
* ��������	:  GetErrMsg()
* ��������	:  ��ȡ������Ϣ   
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ������Ϣ
* ����		:  li.shugang
*******************************************************************************/
char* TBaseException::GetErrMsg() const
{
    return ((char*)m_strErrMsg.c_str());
}
