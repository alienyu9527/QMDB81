#ifndef _MDB_ERROR_HELPER_H_
#define _MDB_ERROR_HELPER_H_
#include <string>
#include <map>

//namespace QuickMDB{

#define MAX_ERROR_DESC_LEN 64
struct TMdbError
{
	int m_iCode;          //错误码
	std::string m_sBrief; //错误描述
	std::string m_sCause; //产生原因
	std::string m_sAction;//解决方法
	void Clear()
	{
		m_iCode = -1;
		m_sBrief[0] = '\0';
		m_sCause[0] = '\0';
		m_sAction[0]='\0';
	}
};

//mdb 错误码帮助类
class TMdbErrorHelper
{
public:
	TMdbErrorHelper();
	~TMdbErrorHelper();
	void FillErrMsg(int iCode,const char* pszFormat, ...);//填充错误信息
	int GetErrCode(){return m_iErrCode;}
	const char * GetErrMsg(){return m_sErrMsg.c_str();}
	int InitErrorDescription();//初始化错误描述
	TMdbError * FindErrorByCode(int iCode);
	int Clear();
private:
	int m_iErrCode;
	std::string m_sErrMsg;
	//char m_sTemp[40960];
	char* m_psTemp;
	std::map<int ,TMdbError *> m_mapErrorDesc;
};

//}
#endif
