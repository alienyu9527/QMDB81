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
	* 函数名称	:  GetErrMsg()
	* 函数描述	:  获取错误信息   
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  错误信息
	* 作者		:  li.shugang
	*******************************************************************************/
    virtual char * GetErrMsg() const;
private:
    string  m_strErrMsg;
};


#endif //__QUICK_MEMORY_DATABASE_EXCEPTION_H__
