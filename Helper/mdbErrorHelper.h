#ifndef _MDB_ERROR_HELPER_H_
#define _MDB_ERROR_HELPER_H_
#include <string>
#include <map>

//namespace QuickMDB{

#define MAX_ERROR_DESC_LEN 64
struct TMdbError
{
	int m_iCode;          //������
	std::string m_sBrief; //��������
	std::string m_sCause; //����ԭ��
	std::string m_sAction;//�������
	void Clear()
	{
		m_iCode = -1;
		m_sBrief[0] = '\0';
		m_sCause[0] = '\0';
		m_sAction[0]='\0';
	}
};

//mdb �����������
class TMdbErrorHelper
{
public:
	TMdbErrorHelper();
	~TMdbErrorHelper();
	void FillErrMsg(int iCode,const char* pszFormat, ...);//��������Ϣ
	int GetErrCode(){return m_iErrCode;}
	const char * GetErrMsg(){return m_sErrMsg.c_str();}
	int InitErrorDescription();//��ʼ����������
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
