/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbCacheLog.h		
*@Description�� ����ʽ�����־�ļ�����־��Ϣ��д�뻺�棬Ȼ��ʱ�������
*@Author:		jin.shaohua
*@Date��	    2012.11
*@History:
******************************************************************************************/
#ifndef _T_CACHE_LOG_H_
#define _T_CACHE_LOG_H_
#include <time.h>
#include "Helper/TThreadLog.h"
#include "Helper/mdbStruct.h"


//namespace QuickMDB{

#define MAX_CACHE_LOG_SIZE 1024*1024*10   //��󻺴�2M
//ˢ����������
enum E_FLUSH_CYCLE_TYPE
{
	FLUSH_CYCLE_TIME     = 1,//����ʱ��,��λ:��
	FLUSH_CYCLE_SIZE      = 2,//��С����λ:KB
	FLUSH_CYCLE_COUNT  = 3 //����
};

//����ʽ��־���
class TCacheLog
{
public:
	TCacheLog();
	~TCacheLog();
	int SetLogFile(const char * sFullPathName,int iLogFileSize);
	int SetFlushCycle(int iCycleType,int iCycleValue);//��������ļ�����
	int Log(const char * sInfo);
	int FlushImmediately();//����ˢ��
private:
	int AllocCache(int iSize);//���뻺��
	int CacheToFile();//����������ļ�
	bool bNeedToFlush();//�Ƿ���Ҫˢ��
	int CheckAndBackup(const char * sFileName , FILE*& fp,int iMaxSizeM);
private:
	int m_iCycleType;//ˢ����������
	int m_iCycleValue;//ˢ������ֵ
	char * m_pCache;//ָ�򻺴�ռ�
	char    m_sLogFile[MAX_PATH_NAME_LEN];//��־�ļ�
	int      m_iCacheSize;//�����С
	int      m_iCurCachePos;//��ǰ����λ��
	int      m_iLogFileSizeM;//��־�ļ���С
private:
	int 	   m_iLogCount;//��־����
	time_t m_tNextLogTime;//�´���־�����ʱ��
};

//}
#endif 

