////////////////////////////////////////////////
// Name: TMutex.h
// Author: Li.ShuGang
// Date: 2008/10/28
// Description: ����
////////////////////////////////////////////////

#ifndef __MEMORY_DATABASE_MUTEX_H_
#define __MEMORY_DATABASE_MUTEX_H_

#include <stdio.h>
#include <string.h>
#include <string>
#ifdef WIN32

#pragma warning(disable:4312)           //unsigned int ����int ת����HANDLE
#pragma warning(disable:4311)           //void*ת����long
#pragma warning(disable:4996)

#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
//#include "Helper/TThreadLog.h"



#endif


//namespace QuickMDB{

#define STR_FORMAT_SEM_ID               "SEM%09d"           //WIN32���������ɱ�ʶ����
#define SEM_ID_MOD                      10000000000         //�������ɱ�ʶȡģ
#define MAX_SEM_ID_SIZE                 64                  //SEM_ID�ַ�����ʽ�ĳ���
#define MAX_SEM_ERROR_MSG_SIZE          1024                //�ź����������Ϣ����󳤶�
#define SEM_BASE                        200                 //�ź���������Ϣ��ʼ����

#define ERROR_SEM_PARAMETER             SEM_BASE+1          //��������
#define ERROR_SEM_CREATE                SEM_BASE+2          //�ź�������ʧ��
#define ERROR_SEM_OPEN                  SEM_BASE+3          //�ź�����ȡ�źű�ʶ����ʧ��
#define ERROR_SEM_SET_VALUE             SEM_BASE+4          //�ź��������ź�ֵʧ��
#define ERROR_SEM_P_OPERATION           SEM_BASE+5          //�ź���P����ʧ��
#define ERROR_SEM_V_OPERATION           SEM_BASE+6          //�ź���V����ʧ��
#define ERROR_SEM_DESTROY               SEM_BASE+7          //�ź���ɾ��ʧ��
#define ERROR_SEM_EXIST                 SEM_BASE+8          //�ź����Ѿ�����
#define ERROR_SEM_FTOK                  SEM_BASE+9          //FTOKʧ��
#define ERROR_SEM_GETVAL                SEM_BASE+10         //�ź�����ȡֵʧ��

using namespace std;

//�������������ģ����ǵ��Ʒ�ҵ����ص㣬�ݲ������������͵���
class TMutex //: public TBaseMutex
{
public:
	TMutex(bool bFlag = false);  //����ڹ����ʱ��ͳ�ʼ��bFlag=true
	~TMutex();	

public:
	/******************************************************************************
	* ��������	:  Create()
	* ��������	:  ������ 
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
    int Create();
	/******************************************************************************
	* ��������	:  Destroy()
	* ��������	:  ������ 
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
    int Destroy();
	/******************************************************************************
	* ��������	:  Lock()
	* ��������	:  ����
	* ����		:  bFlag ���Ƿ��Ѿ�������tCurtime ������ʱ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
    int Lock(bool bFlag,  struct timeval *tCurtime = NULL);
    /******************************************************************************
	* ��������	:  UnLock()
	* ��������	:  ����
	* ����		:  bFlag ���Ƿ��Ѿ�����
	* ���		:  ��
	* ����ֵ	:  0 �ɹ���-1 ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
    int UnLock(bool bFlag,  const char* pszTime=NULL);
    /******************************************************************************
	* ��������	:  TryLock()
	* ��������	:  ����������ģʽ
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  �ɹ��򷵻�0, �����򷵻ش�����
	* ����		:  li.shugang
	*******************************************************************************/
    int TryLock();
    
    int GetLockPID(){return m_iLockPID;}
	bool IsLock(){return bIsLock;}
	bool IsCreate(){return bIsCreate;}
	
    struct timeval m_tCurTime;
	
private:
	pthread_mutex_t     mutex;
	pthread_mutexattr_t mattr;
	bool bIsCreate;
	bool bIsLock;
    int    m_iLockPID;//��ס�Ľ���ID
};


//����Ҫ����ʱ���������������ٿռ�����
class TMiniMutex
{
public:
	TMiniMutex(bool bFlag = false);  //����ڹ����ʱ��ͳ�ʼ��bFlag=true
	~TMiniMutex();	

public:
    int Create();
    int Destroy();
    int Lock(bool bFlag);
    int UnLock(bool bFlag);
    int TryLock();
	bool IsLock(){return bIsLock;}
	bool IsCreate(){return bIsCreate;}

private:
	pthread_mutex_t     mutex;
	pthread_mutexattr_t mattr;   
	bool bIsCreate;
	bool bIsLock;
};



//�������������ģ����ǵ��Ʒ�ҵ����ص㣬�ݲ������������͵���
class TOMutex //: public TBaseMutex
{
public:
	TOMutex(bool bFlag = false);  //����ڹ����ʱ��ͳ�ʼ��bFlag=true
	~TOMutex();	

public:
	/******************************************************************************
	* ��������	:  Create()
	* ��������	:  ������ 
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
    int Create();
	/******************************************************************************
	* ��������	:  Destroy()
	* ��������	:  ������ 
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
    int Destroy();        
	/******************************************************************************
	* ��������	:  Lock()
	* ��������	:  ����
	* ����		:  bFlag ���Ƿ��Ѿ�������tCurtime ������ʱ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
    int Lock(bool bFlag, int iPID, int iTID); 
    /******************************************************************************
	* ��������	:  UnLock()
	* ��������	:  ����
	* ����		:  bFlag ���Ƿ��Ѿ�����
	* ���		:  ��
	* ����ֵ	:  0 �ɹ���-1 ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
    int UnLock(bool bFlag, int iPID, int iTID);
    /******************************************************************************
	* ��������	:  GetErrMsg()
	* ��������	:  ��ȡ������Ϣ   
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ������Ϣ
	* ����		:  li.shugang
	*******************************************************************************/
    const char* GetErrMsg(int iErrno);
    /******************************************************************************
	* ��������	:  GetErrMsg()
	* ��������	:  ��ȡ������Ϣ   
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ������Ϣ
	* ����		:  li.shugang
	*******************************************************************************/
	static int GetErrorCode();
	/******************************************************************************
	* ��������	:  GetErrMsgByCode()
	* ��������	:  ͨ���������ȡ������Ϣ   
	* ����		:  iErrCode ������
	* ���		:  ��
	* ����ֵ	:  ������Ϣ
	* ����		:  li.shugang
	*******************************************************************************/
    static const char* GetErrMsgByCode(int iErrCode,int iDetailCode=0);
       	
private:
#ifdef WIN32
	static int iErrCode;                                //ϵͳ�������
	HANDLE mutex;
#else
	pthread_mutex_t     mutex;
	pthread_mutexattr_t mattr;
	pthread_cond_t	    cond;

#endif
	bool bIsCreate;
	bool bIsLock;
	int  m_iPID;
	int  m_iTID;
	int  m_iCounts;
};

//}

#endif //__MEMORY_DATABASE_MUTEX_H_

