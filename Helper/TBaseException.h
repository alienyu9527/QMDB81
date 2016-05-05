#ifndef __QUICK_MEMORY_DATABASE_EXCEPTION_H__
#define __QUICK_MEMORY_DATABASE_EXCEPTION_H__

#include <string>
using namespace std;

class TBaseException
{
public:
    TBaseException();
    virtual ~TBaseException();
    TBaseException(const char * fmt, ...);
    TBaseException(const string strErrMsg);
	/******************************************************************************
	* ��������	:  GetErrMsg()
	* ��������	:  ��ȡ������Ϣ   
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ������Ϣ
	* ����		:  li.shugang
	*******************************************************************************/
    virtual char * GetErrMsg() const;
private:
    string  m_strErrMsg;
};


#endif //__QUICK_MEMORY_DATABASE_EXCEPTION_H__
