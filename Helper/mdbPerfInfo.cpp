#include <iostream>
#include <string>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#ifdef WIN32
#include <process.h>
#endif
#include <sstream>
#include "Helper/mdbPerfInfo.h"
#include "Helper/mdbOS.h"
#include <unistd.h>

//namespace QuickMDB{

    char  g_MdbprogName[64];

    TMdbPerfStatMgr g_MdbPerfStatMgr;

    TMdbPerfStatMgr::TMdbPerfStatMgr()
    {
        memset(m_PSContainer,0x00,sizeof(m_PSContainer));
        
        m_iCount=0;
        memset(g_MdbprogName,0,sizeof(g_MdbprogName));
        

        if(getenv("QuickMDB_HOME"))
        {
#ifdef WIN32
            //TMdbOS::GetProcessNameByPID(_getpid(),g_MdbprogName);
            sprintf(m_PerfFileName,"%s\\log\\MdbPerfStat_%s_%d",getenv("QuickMDB_HOME"),g_MdbprogName,_getpid());
#else
            //TMdbOS::GetProcessNameByPID(getpid(),g_MdbprogName);
            sprintf(m_PerfFileName,"%s/log/MdbPerfStat_%s_%d",getenv("QuickMDB_HOME"),g_MdbprogName,getpid());
    		//printf("[%s:%d] *******************filename=%s\n",__FILE__,__LINE__,m_PerfFileName);
#endif
        }
        else
        {
#ifdef WIN32
            sprintf(m_PerfFileName,"MdbPerfStat_%s_%d",g_MdbprogName,_getpid());
#else
            sprintf(m_PerfFileName,"MdbPerfStat_%s_%d",g_MdbprogName,getpid());
#endif   
        }
        
    }

    /******************************************************************************
    * ��������	:  ShowStatic()
    * ��������	:  ��ȡ��������ͳ�Ƶ���Ϣ	 
    * ����		:  ��  
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbPerfStatMgr::ShowStatic()
    {
        for (int i=0;i<m_iCount;i++)
            m_PSContainer[i]->Stat();
    }

    /******************************************************************************
    * ��������	:  AddPS()
    * ��������	:  ��������ͳ�Ƶ�	 
    * ����		:  ����ͳ�ƶ���
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbPerfStatMgr::AddPS(TMdbPerfStat * thePS)
    {
        if(m_iCount<MaxPerfStatePointNbr)//���֧��MaxPerfStatePointNbr������ͳ�Ƶ�
            m_PSContainer[m_iCount++]=thePS;

    }
    TMdbPerfStat::TMdbPerfStat()
    {
        ClearStat();
        m_perCount = -1;
        m_fOsValue = 1000000.0;
        theMicSec=0;
        memset(m_functionName,0,sizeof(m_functionName));
    }

    TMdbPerfStat::TMdbPerfStat(const char *sFunName,int perCount):m_perCount(perCount)
    {
        ClearStat();

        strncpy(m_functionName,sFunName,sizeof(m_functionName)-1);
        g_MdbPerfStatMgr.AddPS(this);

        m_fOsValue = 1000000.0;
        theMicSec=0;
    }

    /******************************************************************************
    * ��������	:  SetLastLog()
    * ��������	:  �Ƿ�������¼	 
    * ����		:  ��  
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbPerfStat::SetLastLog()
    {
    	m_bLastLog = true;
    }

    TMdbPerfStat::~TMdbPerfStat()
    {
        Stat();
    }

    /******************************************************************************
    * ��������	:  Stat()
    * ��������	:  ����ͳ��
    * ����		:  ��  
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbPerfStat::Stat()
    {
        if(m_microseconds>0)
        {
            time_t tCurrent;
            struct tm *tm_Cur = NULL;        
            FILE *fp = NULL;

            if((fp=fopen(g_MdbPerfStatMgr.m_PerfFileName,"a+")) != NULL)
            {
                char sCurtime[30];
    			memset(sCurtime,0,sizeof(sCurtime));
                time(&tCurrent); //ȡ�õ�ǰʱ���time_tֵ
                time(&m_tLogTime);//��־��¼ʱ���time_tֵ
                tm_Cur = localtime(&tCurrent); //ȡ�õ�ǰʱ���tmֵ
                /*sprintf(sCurtime,"%04d%02d%02d %02d:%02d:%02d",tm_Cur->tm_year+1900,tm_Cur->tm_mon+1,tm_Cur->tm_mday,tm_Cur->tm_hour,tm_Cur->tm_min,tm_Cur->tm_sec);*/
                sprintf(sCurtime,"%02d:%02d:%02d",tm_Cur->tm_hour,tm_Cur->tm_min,tm_Cur->tm_sec);
                fprintf (fp," LogTime:[%s] Func:[%-30s] \n   Count:[%6d]  Total:[%8.5f](s)  Rate:[%8.0f]/s 0.01s[%4d] 0.1s[%4d] 1s[%3d] >1s[%3d]\n", 
                                sCurtime,m_functionName,m_counter,m_Totalmicseconds/m_fOsValue,
    				(float)m_counter*m_fOsValue/m_Totalmicseconds,
    				m_PerTimeMap[0],m_PerTimeMap[1],m_PerTimeMap[2],m_PerTimeMap[3]); 
                for ( int ix = 0; ix < (int)m_vecSqls.size(); ++ix )
                {
                    fprintf(fp," %s",m_vecSqls[ix].c_str());
                }
                
                fflush(fp);
                fclose(fp);
            }
        }
        ClearStat();

    }

    /******************************************************************************
    * ��������	:  ClearStat()
    * ��������	:  ���ָ��ͳ����	 
    * ����		:  ��  
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbPerfStat::ClearStat()
    {
        m_Totalmicseconds = 0;
        m_microseconds = 0;

        m_TimeMap[0]=0;
        m_TimeMap[1]=0;
        m_TimeMap[2]=0;
        m_TimeMap[3]=0;

        m_PerTimeMap[0]=0;
        m_PerTimeMap[1]=0;
        m_PerTimeMap[2]=0;
        m_PerTimeMap[3]=0;

        m_counter = 0;

        time(&m_tLogTime);

        m_vecSqls.clear();
    	m_bLastLog = false;

    }


    /******************************************************************************
    * ��������	:  begin()
    * ��������	:  ����ָ�꿪ʼͳ��	 
    * ����		:  ��  
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbPerfStat::begin() 
    {    
        theMicSec=0;
#ifdef WIN32
        m_beginPoint=clock();
#else
        gettimeofday(&m_beginPoint, NULL);        
#endif
    }

    /******************************************************************************
    * ��������	:  end()
    * ��������	:  ����ͳ��
    * ����		:  ��  
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbPerfStat::end()
    {
        static int theLevel=0;
#ifdef WIN32
        theMicSec =  (clock() - m_beginPoint);
#else
        gettimeofday(&m_endPoint, NULL);                
        theMicSec= (m_endPoint.tv_sec - m_beginPoint.tv_sec) * 1000000 + (m_endPoint.tv_usec - m_beginPoint.tv_usec);    
#endif
        m_microseconds +=theMicSec;
        theLevel=theMicSec / 100000;
        m_Totalmicseconds += theMicSec;
        
        if(theLevel<=1)//0.01s
        {
            m_TimeMap[0]++;
            m_PerTimeMap[0]++;
        }
        else if(theLevel<=10)//0.1s
        {
            m_TimeMap[1]++;
            m_PerTimeMap[1]++;
        }
        else if(theLevel<=100)//1//1s
        {
            m_TimeMap[2]++;
            m_PerTimeMap[2]++;
        }
        else //>1s
        {
            m_TimeMap[3]++;
            m_PerTimeMap[3]++;
        }

        ++m_counter; 
        if(!m_bLastLog && m_counter%1000 == 0 && m_counter>0 )//ִ��1000�� ���м��һ��
        {
            time_t tCurrent;
            time(&tCurrent);//����ʱ���time_tֵ
            if(m_counter >= 100000 || (tCurrent - m_tLogTime) >= 30)   //ִ�д�������10w�� ��or�����ϴ�ͳ��ʱ��30��
            {
                Stat();
            }
        }
    }

    /******************************************************************************
    * ��������	:  SetSqlInfo()
    * ��������	:  ����ͳ��sql	 
    * ����		:  ͳ��sql  
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbPerfStat::SetSqlInfo(const char *sMySql)
    {
        char *p_SqlInfo = new(std::nothrow) char[strlen(sMySql)+ 33];
        memset(p_SqlInfo,0,strlen(sMySql)+ 33);
        sprintf(p_SqlInfo,"%d : %s\n",m_counter + 1,sMySql);
        m_vecSqls.push_back(p_SqlInfo);
        delete []p_SqlInfo;
    }

    /******************************************************************************
    * ��������	:  GetSqlInfo()
    * ��������	:  ��ȡsql	 
    * ����		:  ��  
    * ���		:  ��
    * ����ֵ	:  sql
    * ����		:  li.shugang
    *******************************************************************************/
    vector<string> TMdbPerfStat::GetSqlInfo()
    {
        return m_vecSqls;
    }

//}
