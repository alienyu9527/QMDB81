/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbOS.cpp
*@Description�� �ڴ����ݿ��ϵͳ�ײ����
*@Author:		li.shugang
*@Date��	    2009��07��13��
*@History:
******************************************************************************************/
#include "Helper/mdbOS.h"
#include "Helper/mdbDateTime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/statvfs.h>

#include "Helper/mdbStruct.h"

#ifndef WIN32
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#pragma comment (lib, "Ws2_32.lib")

#include <process.h>
#include <windows.h>
#include <tlhelp32.h>
#include <winsock.h>

#pragma warning(disable:4018)
#endif

#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#else
#include<stdlib.h>
#endif // #ifdef WIN32

//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

#ifdef _WIN32
static const char *  inet_ntop4(const char *src, char *dst, size_t size)
{
    static const char fmt[] = "%u.%u.%u.%u";
    char tmp[sizeof("255.255.255.255")];
    int l;
    l = _snprintf(tmp, size, fmt, src[0], src[1], src[2], src[3]);

    if (l <= 0 || l >= size)
    {
        return (NULL);
    }
    strncpy(dst, tmp, size);
    return (dst);
}


const char *  inet_ntop(int af, const char *src, char *dst, size_t size)
{

    switch (af)
    {
    case AF_INET:
        return (inet_ntop4(src, dst, size));
    default:
        return (NULL);

    }

    /** NOTREACHED */

}
#endif



/******************************************************************************
* ��������	:  GetPID()
* ��������	:  ��ȡ����ID
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �ɹ����ؽ���ID��ʧ���򷵻�-1
* ����		:  li.shugang
*******************************************************************************/
int TMdbOS::GetPID()
{
#ifdef WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}


/******************************************************************************
* ��������	:  GetTID()
* ��������	:  ��ȡ�߳�ID
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �ɹ������߳�ID��ʧ���򷵻�-1
* ����		:  li.shugang
*******************************************************************************/
unsigned long TMdbOS::GetTID()
{
#ifdef WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

/******************************************************************************
* ��������	:  IsProcExistByPopen()
* ��������	:  �жϽ����Ƿ����
* ����		:  ����id
* ���		:  ��
* ����ֵ	:  true/false
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbOS::IsProcExistByKill(int iPID)
{
    if(iPID <= 0)
        return false;
#ifdef WIN32
    return IsProcExistByPopen(iPID);
#else
    return kill(iPID, 0)==0? true : false;
#endif
}


/******************************************************************************
* ��������	:  IsProcExistByPopen()
* ��������	:  �жϽ����Ƿ����
* ����		:  ����id
* ���		:  ��
* ����ֵ	:  true/false
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbOS::IsProcExistByPopen(int iPID)
{
    if(iPID <= 0)
        return false;
#ifdef WIN32
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if( hProcessSnap == INVALID_HANDLE_VALUE )
    {
        return false;
    }
    // Set the size of the structure before using it.
    pe32.dwSize = sizeof( PROCESSENTRY32 );
    if( !Process32First( hProcessSnap, &pe32 ) )
    {
        CloseHandle( hProcessSnap );     // Must clean up the snapshot object!
        return false;
    }
    do
    {
        if(pe32.th32ProcessID == iPID)
        {
            CloseHandle( hProcessSnap );
            return true;
        }
    }
    while( Process32Next( hProcessSnap, &pe32 ) );
    CloseHandle( hProcessSnap );
    return false;
#else

#if defined(OS_AIX) || defined(OS_LINUX) || defined(OS_SUN)
    char sProcFile[MAX_FILE_NAME] = {0};
    memset(sProcFile,0,sizeof(sProcFile));
    snprintf(sProcFile,sizeof(sProcFile),"/proc/%d/",iPID);
    return TMdbNtcDirOper::IsExist(sProcFile);
#else
    char Cmd[128] = {0};
    memset(Cmd,0,sizeof(Cmd));
    snprintf(Cmd,sizeof(Cmd),"%s\"%d\"%s  ","ps -ef|grep -v grep|grep ",iPID,"|awk '{print $2}'");
    TADD_FLOW("cmd=[%s]",Cmd);
    bool bRet = false;
    FILE * p = popen(Cmd,"r");
    if(NULL == p)
    {
        TADD_ERROR(ERROR_UNKNOWN,"errorno=%d, error-msg=%s\n.", errno, strerror(errno));
        return false;
    }
    char pidBuf[16] = {0};
    while(1)
    {
        memset(pidBuf,0,sizeof(pidBuf));
        fgets(pidBuf,sizeof(pidBuf),p);
        if(strlen(pidBuf)==0)
        {
            break;
        }
        if(iPID == atoi(pidBuf))
        {
            bRet = true;
            break;
        }
    }
    pclose(p);    
    return bRet;
#endif
#endif
}

/******************************************************************************
* ��������  :  IsProcExist
* ��������  :  �жϽ����Ƿ����
* ����      :  ����ȫ��
* ���      :
* ����ֵ    :  �ɹ�����true ,ʧ�ܷ���false
* ����      :  jin.shaohua
*******************************************************************************/
bool TMdbOS::IsProcExist(const char * sFullProcName)
{
    char sFullName[MAX_NAME_LEN] = {0};
    SAFESTRCPY(sFullName,sizeof(sFullName),sFullProcName);
    TMdbNtcStrFunc::Trim(sFullName);
    if(0 == sFullName[0])
    {
        return false;
    }
    //��ȡ������
    TMdbNtcSplit tSplit;
    tSplit.SplitString(sFullName,' ');
    char sProcName[MAX_NAME_LEN] = {0};
    SAFESTRCPY(sProcName,sizeof(sProcName),tSplit[0]);
    //��� fullname֮��Ŀո�
    char sFullNameNoSpace[MAX_NAME_LEN] = {0};
    TMdbNtcStrFunc::Replace(sFullName," ","",sFullNameNoSpace);
    int arrPid[MAX_PROCESS_COUNTS] = {0};
    TMdbOS::GetPidByName(sProcName,arrPid);
    int i = 0;
    for(i = 0; i < MAX_PROCESS_COUNTS; ++i)
    {
        if(arrPid[i] != 0)
        {
            char sTemp[MAX_NAME_LEN] = {0};
            TMdbOS::GetProcessNameByPID(arrPid[i], sTemp,sizeof(sTemp));

            if(TMdbNtcStrFunc::StrNoCaseCmp(sTemp,sFullNameNoSpace) == 0)
            {
                return true;
            }
        }
    }
    return false;
}


int TMdbOS::MySystem(const char *cmd,int &ipid)
{
#ifdef WIN32
    return 0;
#else
    pid_t pid;
    int status;

    if (NULL == cmd)
    {
        return(1);
    }
    if((pid = fork()) < 0)
    {
        status = -1;
    }
    else if(pid == 0)
    {
        execl("/bin/sh", "sh", "-C", cmd,(char *)0);
        _exit(127);
    }
    else
    {
        while(waitpid(pid, &status, 0) < 0)
        {
            if(errno == EINTR)
            {
                status = -1;
                break;
            }
        }
    }
    ipid = (int)pid;
    return status;

#endif
}

//��ȡ��ǰ�û��µĽ����������ڵ�pid
int  TMdbOS::GetPidByName(const char *processName,int pid[])
{
#ifndef WIN32
    char buf[10];
    char Cmd[100];
    int i = 0;
    FILE *p;
    memset(Cmd,0,sizeof(Cmd));
    snprintf(Cmd,sizeof(Cmd),"%s\"%s\"%s","ps -fu $LOGNAME |grep -v grep|grep ",processName,"|awk '{print $2}'");
    Cmd[strlen(Cmd)] = '\0';
    p = popen(Cmd,"r");
    if(p == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"errorno=%d, error-msg=%s\n.", errno, strerror(errno));
        return 0;
    }
    while(1)
    {
        memset(buf,0,sizeof(buf));
        fgets(buf,sizeof(buf),p);
        if(strlen(buf)==0)
        {
            break;
        }
        pid[i] = atoi(buf);
        i++;
    }
    pclose(p);
    return 0;
#else
    return 0;
#endif
}

int TMdbOS::GetProcessNameByPID(int pid, char *processName,const int iLen)
{
    //ps -ef|grep -v grep|grep 655422|awk '{printf "%s %s %s %s %s %s\n" ,$8,$9,$10,$11,$12,$13}'
    char buf[100];
    char Cmd[200];
    memset(buf,0,sizeof(buf));
    FILE* ptr = NULL;
    memset(Cmd,0,sizeof(Cmd));
	snprintf(Cmd,sizeof(Cmd),"UNIX95=1;export UNIX95;ps -e -o pid,args | grep -v awk|  grep -v grep | awk '{if ($1 == %d) print $2$3$4$5$6$7$8}'",pid);
    Cmd[strlen(Cmd)] = '\0';
    ptr = popen(Cmd, "r");
    if (ptr)
    {
        while (fgets(buf, sizeof(buf), ptr) != NULL)
        {
            strncpy(processName,buf,iLen);
            TMdbNtcStrFunc::Trim(processName);
            if((int)strlen(buf) >= iLen)
            {
                processName[iLen-1] = '\0';
            }
            else
            {
                processName[strlen(processName)] = '\0';
            }
            break;
        }
		TMdbNtcStrFunc::Trim(processName,'\n');
		pclose(ptr);
    }
    
    return 0;
}

int TMdbOS::GetProcFullNameByPID(int pid, char *processName,const int iLen)
{
#ifndef WIN32
    char buf[MAX_PATH_NAME_LEN];
    char Cmd[MAX_PATH_NAME_LEN];
    int iListCount = 0;
    memset(buf,0,sizeof(buf));
    FILE* ptr = NULL;
    memset(Cmd,0,sizeof(Cmd));
    snprintf(Cmd,sizeof(Cmd),"UNIX95=1;export UNIX95;ps -o pid,args -u ${LOGNAME}|awk '{if ($1 == \"%d\") print}'",pid);
    ptr = popen(Cmd, "r");
    if (ptr)
    {
        unsigned int iList;
        TMdbNtcSplit mdbSplitBuf;
        TMdbNtcSplit mdbSplitCmd;
        while (fgets(buf, sizeof(buf), ptr) != NULL)
        {
            iListCount = 0;
            TMdbNtcStrFunc::Trim(buf,'\n');
            TMdbNtcStrFunc::Trim(buf,' ');
            mdbSplitBuf.SplitString(buf, ' ');
            memset(Cmd,0,sizeof(Cmd));
            if(mdbSplitBuf.GetFieldCount() < 2) break;//����2λ
            strncpy(Cmd,mdbSplitBuf[1],MAX_PATH_NAME_LEN);
            mdbSplitCmd.SplitString(Cmd, '/');
            iListCount = mdbSplitCmd.GetFieldCount();
            strncpy(processName,mdbSplitCmd[iListCount-1],iLen);
            for(iList = 2; iList< mdbSplitBuf.GetFieldCount(); iList++)
            {
                snprintf(processName+strlen(processName),iLen-strlen(processName)," %s",mdbSplitBuf[iList]);
            }

            if((int)strlen(buf) >= iLen)
            {
                processName[iLen-1] = '\0';
            }
            else
            {
                processName[strlen(processName)] = '\0';
            }
            break;
        }
        pclose(ptr);
    }
    
    return 0;
#else
    return 0;
#endif
}

bool TMdbOS::KillProc(int pid)
{
    //ֱ�ӷ�����ֹ��Ϣ
#ifdef WIN32
    // ��Ŀ����̣�ȡ�ý��̾��
    HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)pid);
    if(hProcess != NULL)
    {
        // ��ֹ����
        ::TerminateProcess(hProcess, 0);
    }
    CloseHandle(hProcess);
    TMdbDateTime::Sleep(1);
#else
    int iRet = 0;
    kill(pid, SIGTERM);

    //���ߵȴ����̽���
    TMdbDateTime::Sleep(1);

    //���û��ɱ����һֱɱ
    int iKillCounts = 0;
    while(1)
    {
        if(iKillCounts > 2)
        {
            iRet = kill(pid, SIGKILL);
        }
        else
        {
            iRet = kill(pid, SIGTERM);

        }
        if(iRet !=0 )
        {
            if(errno == ESRCH)
                break;
        }

        if(IsProcExistByKill(pid) == false)
        {
            break;
        }

        char sTemp[128];
        memset(sTemp, 0, sizeof(sTemp));
        snprintf(sTemp,sizeof(sTemp),"kill -9 %d", pid);
        system(sTemp);

        ++iKillCounts;
        if(iKillCounts >= 5)
        {
            printf("[%s : %d] : Can't kill process: pid=[%d] too many times.\n",
                   __FILE__, __LINE__, pid);
            return false;
        }
    }
#endif

    return true;
}

/******************************************************************************
* ��������	:  GetIP()
* ��������	:  ��ȡ����IP
* ����		:
* ���		:  sIP, IP��ַ
* ����ֵ	:  �ɹ������߳�0��ʧ���򷵻�-1
* ����		:  li.shugang
*******************************************************************************/
int TMdbOS::GetIP(char sIP[][32])
{
    int i = 0;
    /*
     char szName[255] = {0};

     //��ȡ����������
     if(gethostname(szName, sizeof(szName)) == -1)
     {
         return -1;
     }

     struct hostent *hp = gethostbyname(szName);
     if(NULL == hp)
     {
         return -1;
     }

     strcpy(sIP, inet_ntoa(*(struct in_addr *)*(hp->h_addr_list)));
    */
#ifdef OS_LINUX

    //register int fd, intrface, retn = 0;
    register int fd, intrface = 0;
    struct ifreq buf[16];
   // struct arpreq arp;
    struct ifconf ifc;
    if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof buf;
        ifc.ifc_buf = (caddr_t) buf;
        if (!ioctl (fd, SIOCGIFCONF, (char *) &ifc))
        {
            //��ȡ�ӿ���Ϣ
            intrface = ifc.ifc_len / sizeof (struct ifreq);
            //printf("interface num is intrface=%d\n\n\n",intrface);
            //���ݽ����Ϣѭ����ȡ�豸IP��MAC��ַ
            while (intrface-- > 0)
            {
                //��ȡ�豸����
                //printf ("net device %s\n", buf[intrface].ifr_name);
                /*
                //�ж���������
                if (!(ioctl (fd, SIOCGIFFLAGS, (char *) &buf[intrface])))
                {
                	if (buf[intrface].ifr_flags & IFF_PROMISC)
                	{
                		puts ("the interface is PROMISC");
                		retn++;
                	}
                }
                else
                {
                 	char str[256];
                	sprintf (str, "cpm: ioctl device %s", buf[intrface].ifr_name);
                	perror (str);
                }
                //�ж�����״̬
                if (buf[intrface].ifr_flags & IFF_UP)
                {
                		puts("the interface status is UP");
                }
                else
                {
                	puts("the interface status is DOWN");
                }
                */
                //��ȡ��ǰ������IP��ַ
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))
                {
                    ///puts ("IP address is:");
                    //puts(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
                    strcpy(sIP[i],inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
                    i++;
                    //puts("");
                    //puts (buf[intrface].ifr_addr.sa_data);
                }
                else
                {
                    char str[256];
                    memset(str,0,sizeof(str));
                    snprintf (str, sizeof(str), "cpm: ioctl device %s", buf[intrface].ifr_name);
                    perror (str);
                }


            } //while
        }
        else
        {
            perror ("cpm: ioctl");
        }
    }
    else
    {
        perror ("cpm: socket");
    }

    close (fd);
#else

    char **pptr;
    struct hostent *hptr;
    char hostname[32];
    char str[32];
    memset(hostname,0,sizeof(hostname));
    memset(str,0,sizeof(str));

    if(gethostname(hostname,sizeof(hostname)) )
    {
        printf("gethostname calling error\n");
        return 1;
    }

    if( (hptr = gethostbyname(hostname) ) == NULL )
    {
        printf("gethostbyname error for host:%s\n", hostname);
        return 1;
    }

    //printf("official hostname:%s\n",hptr->h_name);

    //for(pptr = hptr->h_aliases; *pptr != NULL; pptr++)
    //	printf(" alias:%s\n",*pptr);

    /* ���ݵ�ַ���ͣ�����ַ����� */
    switch(hptr->h_addrtype)
    {
    case AF_INET:
    case AF_INET6:
        pptr=hptr->h_addr_list;
        /* ���ղŵõ������е�ַ������������е�����inet_ntop()���� */
        for(; *pptr!=NULL; pptr++)
        {
            //printf(" address:%s\n", inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));

            strcpy(sIP[i],inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
            i++;
        }
        break;
    default:
        printf("unknown address type\n");
        break;
    }
#endif
    return 0;
}

/******************************************************************************
* ��������	:  GetDiskSpace()
* ��������	:  ��ȡ���̿ռ��С(��֧��win32ϵͳ)
* ����		:  ��ŵ������̿ռ��С�ļ�·��
* ���		:  ��
* ����ֵ	:  ���̿ռ��С
* ����		:  jiang.mingjun
*******************************************************************************/
unsigned long long   TMdbOS::GetDiskSpace(char* sPath)
{
    struct statvfs sfs;
    statvfs(sPath, &sfs);
    unsigned long long  ullTotalSapceKB =  sfs.f_blocks*(sfs.f_bsize/1024);
    unsigned long long  ullFreeSpaceKB  =  sfs.f_bavail*(sfs.f_bsize/1024);
    unsigned long long  ullUsedSpaceKB =  (sfs.f_blocks - sfs.f_bfree)*(sfs.f_bsize/1024);

    char sFileName[MAX_PATH_NAME_LEN];
    memset(sFileName,0,sizeof(sFileName));
    snprintf(sFileName, sizeof(sFileName), "%s/diskSpace", sPath);
    FILE* m_fp = NULL;
    m_fp = fopen(sFileName, "w+");
    if(m_fp == NULL)
    {
        printf("Open File Error,FileName=[%s]\n",sFileName);
        return 0;
    }
    int percent =  ullUsedSpaceKB * 100/(ullUsedSpaceKB+ullFreeSpaceKB) + 1;
    fprintf(m_fp,"%s      [total]%llu    [used]%llu  [free]%llu   [used]%d%% ",sPath,
            ullTotalSapceKB, ullUsedSpaceKB, ullFreeSpaceKB, percent);

    SAFE_CLOSE(m_fp);
    return ullFreeSpaceKB;
}

/******************************************************************************
* ��������	:  IsLocalIP
* ��������	:  �ж��Ƿ��Ǳ���IP
* ����		:
* ����		:
* ���		:
* ����ֵ	:  TRUE /FALSE
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbOS::IsLocalIP(const char * sIP)
{
    if(NULL == sIP && 0 == sIP[0])
    {
        return false;
    }
    char sLocalIPArr[16][32] = {{0}};
    if(0 != GetIP(sLocalIPArr))
    {
        return false;
    }
    if(0 == sLocalIPArr[0][0])
    {
        return true;   //û���ҵ�
    }
    int i = 0;
    for(i = 0; i < 16; ++i)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(sIP,sLocalIPArr[i]) == 0)
        {
            //find
            return true;
        }
    }
    return false;

}

//}
