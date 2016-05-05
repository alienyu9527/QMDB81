////////////////////////////////////////////////
// Name: TMutex.cpp
// Author: Li.ShuGang
// Date: 2008/10/28
// Description: ����
////////////////////////////////////////////////

#include "Helper/TMutex.h"
#include "Helper/TThreadLog.h"
////////////////////////////////////////////////
// �������� TMutex
////////////////////////////////////////////////

//namespace QuickMDB{

#ifdef WIN32
int TMutex::iErrCode=0;
#endif
TMutex::TMutex(bool bFlag)
{
    bIsCreate = false;
    bIsLock   = false;
    m_iLockPID = -1;
    if(bFlag)
    {
        int iRet = Create();
        if(iRet == 0)
            bIsCreate = true;
    }
    m_tCurTime.tv_sec = 0;
    m_tCurTime.tv_usec = 0;
}


TMutex::~TMutex()
{
    Destroy();
}


/******************************************************************************
* ��������	:  Create()
* ��������	:  ������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TMutex::Create()
{
    bIsCreate = false;
    bIsLock   = false;
    m_tCurTime.tv_sec  = 0;
    m_tCurTime.tv_usec = 0;

#ifdef WIN32
    TMutex::iErrCode=0;
    int iRet = 0;
    mutex = CreateMutex(NULL,0,NULL);
    if(mutex == NULL)
    {
        iErrCode=GetLastError();
        printf("CreateMutex error: %d\n", GetLastError());
        return -1;
    }

#else
    //���û���������
    int iRet = pthread_mutexattr_init(&mattr);
    if(iRet != 0)
    {
        return iRet;
    }

    //Ĭ�ϣ����̼�ʹ��
    iRet = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    if(iRet != 0)
    {
        return iRet;
    }

#ifdef OS_HP
    iRet =  pthread_mutexattr_setspin_np(&mattr, PTHREAD_MUTEX_SPINONLY_NP);
    if(iRet != 0)
    {
        return iRet;
    }
#endif

    //���û�����
    iRet = pthread_mutex_init(&mutex, &mattr);
    if(iRet != 0)
    {
        return iRet;
    }

#endif


    bIsCreate = true;

    bIsLock = false;

    return iRet;
}


/******************************************************************************
* ��������	:  Destroy()
* ��������	:  ������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TMutex::Destroy()
{
#ifdef WIN32
    int iRet=0;
    TMutex::iErrCode=0;
    if(bIsCreate == false)
        return 0;
    if(mutex == NULL)
    {
        return ERROR_SEM_PARAMETER;
    }
    if(CloseHandle(mutex)==FALSE)
    {
        iRet=ERROR_SEM_DESTROY;
    }
    iErrCode=GetLastError();

#else
    if(bIsCreate == false)
        return 0;

    //�ͷŻ���������
    int iRet = pthread_mutexattr_destroy(&mattr);
    if(iRet != 0)
    {
        return iRet;
    }

    //�ͷŻ�����
    iRet = pthread_mutex_destroy(&mutex);
    if(iRet != 0)
    {
        return iRet;
    }
#endif
    bIsCreate = false;
    m_iLockPID = -1;
    return iRet;
}


/******************************************************************************
* ��������	:  Lock()
* ��������	:  ����
* ����		:  bFlag ���Ƿ��Ѿ�������tCurtime ������ʱ��
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TMutex::Lock(bool bFlag, struct timeval *tCurtime)
{
    if(bFlag)
    {
#ifdef WIN32
        DWORD dwWaitResult;
        TMutex::iErrCode=0;

        if(mutex == NULL)
        {
            return ERROR_SEM_PARAMETER;
        }

        dwWaitResult=WaitForSingleObject(mutex,INFINITE);//�ȴ��ź���������
        if(dwWaitResult != WAIT_OBJECT_0)
        {
            return -1;
        }
        m_iLockPID = TMDB_CUR_PID;
        bIsLock = true;
        return 0;
#else
        int iCount = 0;
        while(iCount < 5)
        {
            int iRet = pthread_mutex_lock(&mutex);
            if(iRet != 0)
            {
                iCount++;
            }
            else
            {
                break;
            }
        }

        if(iCount >=5)
        {
            printf("Lock Failed");
            return -1;
        }
        if(tCurtime != NULL)
        {
            if(tCurtime->tv_sec != 0 || tCurtime->tv_usec != 0)
            {
                m_tCurTime.tv_sec = tCurtime->tv_sec;
                m_tCurTime.tv_usec = tCurtime->tv_usec;
            }
        }
        m_iLockPID = TMDB_CUR_PID;
        bIsLock = true;
        return 0;
#endif

    }
    else
    {
        return 0;
    }
}

/******************************************************************************
* ��������	:  TryLock()
* ��������	:  ����������ģʽ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �ɹ��򷵻�0, �����򷵻ش�����
* ����		:  li.shugang
*******************************************************************************/
int TMutex::TryLock()
{
#ifdef WIN32
    return 0;
#else
    return pthread_mutex_trylock(&mutex); /* acquire the mutex */
#endif
}

/******************************************************************************
* ��������	:  UnLock()
* ��������	:  ����
* ����		:  bFlag ���Ƿ��Ѿ�����
* ���		:  ��
* ����ֵ	:  0 �ɹ���-1 ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TMutex::UnLock(bool bFlag, const char* pszTime)
{
    if(bFlag)
    {
#ifdef WIN32
        ReleaseMutex(mutex);
        iErrCode=GetLastError();
        m_iLockPID = -1;
        return 0;
#else
        //m_sTime[0] = '\0';
        int iCount = 0;
        while(iCount < 5)
        {
            int iRet = pthread_mutex_unlock(&mutex);
            if(iRet != 0)
            {
                iCount++;
            }
            else
            {
                break;
            }
        }
        if(iCount >=5)
        {
            printf("UnLock Failed");
            return -1;
        }
        m_tCurTime.tv_sec = 0;
        m_tCurTime.tv_usec = 0;
        m_iLockPID = -1;
        bIsLock = false;
        return 0;
#endif
    }
    else
    {
        return 0;
    }
}


/******************************************************************************
* ��������	:  GetErrMsg()
* ��������	:  ��ȡ������Ϣ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ������Ϣ
* ����		:  li.shugang
*******************************************************************************/
const char* TMutex::GetErrMsg(int iErrno)
{
    return strerror(iErrno);
}

/******************************************************************************
* ��������	:  GetErrorCode()
* ��������	:  ��ȡ�����룬����win32ϵͳʹ��
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ������
* ����		:  li.shugang
*******************************************************************************/
#ifdef WIN32
int TMutex::GetErrorCode()
{
    return TMutex::iErrCode;
}
#endif

/******************************************************************************
* ��������	:  GetErrMsgByCode()
* ��������	:  ͨ���������ȡ������Ϣ
* ����		:  iErrCode ������
* ���		:  ��
* ����ֵ	:  ������Ϣ
* ����		:  li.shugang
*******************************************************************************/
const char* TMutex::GetErrMsgByCode(int iErrCode,int iDetailCode)
{
    static string  strErrMsg="";

    strErrMsg="TSem::ErrMsg=";

    switch(iErrCode)
    {
    case ERROR_SEM_PARAMETER:
    {
        strErrMsg=strErrMsg+"Sem Input parameter error";
        break;
    }
    case ERROR_SEM_CREATE:
    {
        strErrMsg=strErrMsg+"Sem Create action error";
        break;
    }
    case ERROR_SEM_OPEN:
    {
        strErrMsg=strErrMsg+"Sem Open action error";
        break;
    }
    case ERROR_SEM_SET_VALUE:
    {
        strErrMsg=strErrMsg+"Sem Set value action error";
        break;
    }
    case ERROR_SEM_DESTROY:
    {
        strErrMsg=strErrMsg+"Sem Destroy action error";
        break;
    }
    case ERROR_SEM_P_OPERATION:
    {
        strErrMsg=strErrMsg+"Sem P action error";
        break;
    }
    case ERROR_SEM_V_OPERATION:
    {
        strErrMsg=strErrMsg+"Sem V action error";
        break;
    }
    case ERROR_SEM_EXIST:
    {
        strErrMsg=strErrMsg+"Sem has existed";
        break;
    }
    case ERROR_SEM_FTOK:
    {
        strErrMsg=strErrMsg+"Unix ftok() error";
        break;
    }
    case ERROR_SEM_GETVAL:
    {
        strErrMsg=strErrMsg+"Unix semctl(GETVAL) error";
        break;
    }
    default:
    {
        strErrMsg="";
        break;
    }
    }//end of swtich(iErrCode)



    return strErrMsg.c_str();

}



////////////////////////////////////////////////
// �������� TOMutex
////////////////////////////////////////////////
#ifdef WIN32
int TOMutex::iErrCode=0;
#endif
TOMutex::TOMutex(bool bFlag)
{
    bIsCreate = false;
    bIsLock   = false;

    if(bFlag)
    {
        int iRet = Create();
        if(iRet == 0)
            bIsCreate = true;
    }
}


TOMutex::~TOMutex()
{
    Destroy();
}


/******************************************************************************
* ��������	:  Create()
* ��������	:  ������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TOMutex::Create()
{
    bIsCreate = false;
    bIsLock   = false;

    m_iPID = -1;
    m_iTID = -1;
    m_iCounts = 0;

#ifdef WIN32
    TOMutex::iErrCode=0;
    int iRet = 0;
    mutex = CreateMutex(NULL,0,NULL);
    if(mutex == NULL)
    {
        iErrCode=GetLastError();
        printf("CreateMutex error: %d\n", GetLastError());
        return -1;
    }

#else
    //���û���������
    int iRet = pthread_mutexattr_init(&mattr);
    if(iRet != 0)
    {
        return iRet;
    }

    //Ĭ�ϣ����̼�ʹ��
    iRet = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    if(iRet != 0)
    {
        return iRet;
    }

#ifdef OS_HP
    iRet =  pthread_mutexattr_setspin_np(&mattr, PTHREAD_MUTEX_SPINONLY_NP);
    if(iRet != 0)
    {
        return iRet;
    }
#endif

    //���û�����
    iRet = pthread_mutex_init(&mutex, &mattr);
    if(iRet != 0)
    {
        return iRet;
    }

    pthread_cond_init(&cond, NULL);

#endif

    bIsCreate = true;
    bIsLock = false;
    return iRet;
}


/******************************************************************************
* ��������	:  Destroy()
* ��������	:  ������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TOMutex::Destroy()
{
#ifdef WIN32
    int iRet=0;
    TOMutex::iErrCode=0;
    if(bIsCreate == false)
        return 0;
    if(mutex == NULL)
    {
        return ERROR_SEM_PARAMETER;
    }
    if(CloseHandle(mutex)==FALSE)
    {
        iRet=ERROR_SEM_DESTROY;
    }
    iErrCode=GetLastError();

#else
    if(bIsCreate == false)
        return 0;

    //�ͷŻ���������
    int iRet = pthread_mutexattr_destroy(&mattr);
    if(iRet != 0)
    {
        return iRet;
    }

    //�ͷŻ�����
    iRet = pthread_mutex_destroy(&mutex);
    if(iRet != 0)
    {
        return iRet;
    }

    pthread_cond_destroy(&cond);

#endif
    return iRet;
}


/******************************************************************************
* ��������	:  Lock()
* ��������	:  ����
* ����		:  bFlag ���Ƿ��Ѿ�������tCurtime ������ʱ��
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TOMutex::Lock(bool bFlag, int iPID, int iTID)
{
    if(bFlag)
    {
#ifdef WIN32
        DWORD dwWaitResult;
        TMutex::iErrCode=0;

        if(mutex == NULL)
        {
            return ERROR_SEM_PARAMETER;
        }

        dwWaitResult=WaitForSingleObject(mutex,INFINITE);//�ȴ��ź���������
        if(dwWaitResult != WAIT_OBJECT_0)
        {
            return -1;
        }
        return 0;
#else
        return pthread_mutex_lock(&mutex); /* acquire the mutex */
        /*
        int iRet = 0;
        struct timespec ts;
        pthread_mutex_lock(&mutex);

        while(bIsLock == true)
        {
        	if(m_iPID == iPID && m_iTID == iTID)
        	{
        		++m_iCounts;
        		break;
        	}
        	else
        	{
        		struct timeval tv;

        		gettimeofday(&tv, NULL);
        		ts.tv_sec = tv.tv_sec + 1;
        		ts.tv_nsec = tv.tv_usec * 1000;

        		iRet = pthread_cond_timedwait(&cond, &mutex, &ts);
        		if(iRet == ETIMEDOUT)
        		{
        			if(bIsLock == true)	//����Ѿ���ʱ���Է���û���ͷţ��򷵻ش���
        			{
        				pthread_mutex_unlock(&mutex);
        				return -1;
        			}
        			else  //����Ѿ���ʱ���Է�Ҳ�Ѿ��ͷţ��򷵻سɹ�
        			{
        				bIsLock = true;
        				m_iPID = iPID;
        				m_iTID = iTID;
        				m_iCounts = 1;
        				pthread_mutex_unlock(&mutex);
        				return 0;
        			}
        		}
        	}
        }

        bIsLock = true;
        if(m_iPID == -1)
        {
        	m_iPID = iPID;
        	m_iTID = iTID;
        	m_iCounts = 1;
        }

        pthread_mutex_unlock(&mutex);
        return 0;*/
#endif

    }
    else
    {
        return 0;
    }
}


/******************************************************************************
* ��������	:  UnLock()
* ��������	:  ����
* ����		:  bFlag ���Ƿ��Ѿ�����
* ���		:  ��
* ����ֵ	:  0 �ɹ���-1 ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TOMutex::UnLock(bool bFlag, int iPID, int iTID)
{
    if(bFlag)
    {
#ifdef WIN32
        ReleaseMutex(mutex);
        iErrCode=GetLastError();
        return 0;
#else
        //return pthread_mutex_unlock(&mutex); /* release the mutex */

        pthread_mutex_lock(&mutex);

        --m_iCounts;
        if(m_iCounts <= 0)
        {
            bIsLock = false;
            m_iPID = -1;
            m_iTID = -1;
        }

        pthread_mutex_unlock(&mutex);

        if(bIsLock == false)
        {
            pthread_cond_signal(&cond);
        }

        return 0;
#endif
    }
    else
    {
        return 0;
    }
}


/******************************************************************************
* ��������	:  GetErrMsg()
* ��������	:  ��ȡ������Ϣ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ������Ϣ
* ����		:  li.shugang
*******************************************************************************/
const char* TOMutex::GetErrMsg(int iErrno)
{
    return strerror(iErrno);
}

/******************************************************************************
* ��������	:  GetErrMsg()
* ��������	:  ��ȡ������Ϣ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ������Ϣ
* ����		:  li.shugang
*******************************************************************************/
#ifdef WIN32
int TOMutex::GetErrorCode()
{
    return TOMutex::iErrCode;
}
#endif

/******************************************************************************
* ��������	:  GetErrMsgByCode()
* ��������	:  ͨ���������ȡ������Ϣ
* ����		:  iErrCode ������
* ���		:  ��
* ����ֵ	:  ������Ϣ
* ����		:  li.shugang
*******************************************************************************/
const char* TOMutex::GetErrMsgByCode(int iErrCode,int iDetailCode)
{
    static string  strErrMsg="";

    strErrMsg="TSem::ErrMsg=";

    switch(iErrCode)
    {
    case ERROR_SEM_PARAMETER:
    {
        strErrMsg=strErrMsg+"Sem Input parameter error";
        break;
    }
    case ERROR_SEM_CREATE:
    {
        strErrMsg=strErrMsg+"Sem Create action error";
        break;
    }
    case ERROR_SEM_OPEN:
    {
        strErrMsg=strErrMsg+"Sem Open action error";
        break;
    }
    case ERROR_SEM_SET_VALUE:
    {
        strErrMsg=strErrMsg+"Sem Set value action error";
        break;
    }
    case ERROR_SEM_DESTROY:
    {
        strErrMsg=strErrMsg+"Sem Destroy action error";
        break;
    }
    case ERROR_SEM_P_OPERATION:
    {
        strErrMsg=strErrMsg+"Sem P action error";
        break;
    }
    case ERROR_SEM_V_OPERATION:
    {
        strErrMsg=strErrMsg+"Sem V action error";
        break;
    }
    case ERROR_SEM_EXIST:
    {
        strErrMsg=strErrMsg+"Sem has existed";
        break;
    }
    case ERROR_SEM_FTOK:
    {
        strErrMsg=strErrMsg+"Unix ftok() error";
        break;
    }
    case ERROR_SEM_GETVAL:
    {
        strErrMsg=strErrMsg+"Unix semctl(GETVAL) error";
        break;
    }
    default:
    {
        strErrMsg="";
        break;
    }
    }//end of swtich(iErrCode)

    return strErrMsg.c_str();
}

//}
