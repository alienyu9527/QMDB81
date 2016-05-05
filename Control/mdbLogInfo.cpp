/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbInfo.h		
*@Description�� �����ӡ�ڴ����ݿ�ĸ�����Ϣ
*@Author:		li.shugang
*@Date��	    2009��03��10��
*@History:
******************************************************************************************/
#include "Control/mdbLogInfo.h"
#include "Interface/mdbQuery.h"

//namespace QuickMDB{
    TMdbLogInfo::TMdbLogInfo()
    {
        m_pShmDSN = NULL;
        m_pDsn    = NULL;      
    }


    TMdbLogInfo::~TMdbLogInfo()
    {
    }


    /******************************************************************************
    * ��������	:  Connect()
    * ��������	:  ����ĳ��DSN�����ǲ��ڹ�����ע���κ���Ϣ    
    * ����		:  pszDSN, ��������������DSN 
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbLogInfo::Connect(const char* pszDSN)
    {
        if(pszDSN == NULL)
        {
            return -1;    
        }
        m_pShmDSN= TMdbShmMgr::GetShmDSN(pszDSN);
        
        m_pDsn = m_pShmDSN->GetInfo();
        
        if(m_pDsn == NULL)
            return -1;
        else
            return 0;
    }

    /******************************************************************************
    * ��������	:  SetMonitor()
    * ��������	:  ���ü�ؽ��̵���־����    
    * ����		:  iLogLevel, ��־����
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbLogInfo::SetMonitor(int iLogLevel)
    {
        if(m_pDsn == NULL)
            return -1;
        
        m_pDsn->iLogLevel = iLogLevel;
        return 0;
    }



    /******************************************************************************
    * ��������	:  SetLocalLink()
    * ��������	:  ���ñ���������־����    
    * ����		:  iPid������ID
    * ����		:  iTid���߳�ID
    * ����		:  iLogLevel, ��־����
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbLogInfo::SetLocalLink(int iPid, int iTid, int iLogLevel)
    {
#if 0
        if(m_pDsn == NULL)
            return -1;
        
        
        TMdbLocalLink *pLocalLink = (TMdbLocalLink *)m_pShmDSN->GetLocalLinkAddr();
        
        for(int i=0; i<MAX_LINK_COUNTS; ++i)
        {
            IS_LOG(3)
                pLocalLink->Print();
                
            if(pLocalLink->iPID < 0)
            {
                ++pLocalLink;
                continue;    
            }
            
            if(pLocalLink->iPID == iPid && pLocalLink->iTID == iTid)
            {
                pLocalLink->iLogLevel = iLogLevel;
                
                IS_LOG(3)
                    pLocalLink->Print();
                
                return 0;
            }
            else if(pLocalLink->iPID == iPid && iTid == -1)
            {
                pLocalLink->iLogLevel = iLogLevel;
                
                IS_LOG(3)
                    pLocalLink->Print();
            }        	

            ++pLocalLink;
        }
        #endif
        return -1;
    }

    int TMdbLogInfo::SetLocalLink(int iPid, int iLogLevel)
    {
        #if 0
        if(m_pDsn == NULL)
            return -1;
       
        
        TMdbLocalLink *pLocalLink = (TMdbLocalLink *)m_pShmDSN->GetLocalLinkAddr();
        
        for(int i=0; i<MAX_LINK_COUNTS; ++i)
        {
            IS_LOG(3)
                pLocalLink->Print();
                
            if(pLocalLink->iPID < 0)
            {
                ++pLocalLink;
                continue;    
            }
            
            if(pLocalLink->iPID == iPid)
            {
                pLocalLink->iLogLevel = iLogLevel;
               // printf("-------------------------------2\n");
                
                IS_LOG(3)
                    pLocalLink->Print();
                
                return 0;
            }

            ++pLocalLink;
        }
        #endif
        return -1;
    }

    int TMdbLogInfo::SetLocalLink(TMdbDSN  *pDsn, int iPid, int iTid, int iLogLevel)
    {
        #if 0
        TMdbLocalLink *pLocalLink = (TMdbLocalLink *)m_pShmDSN->GetLocalLinkAddr();
        
        for(int i=0; i<MAX_LINK_COUNTS; ++i)
        {
            if(pLocalLink->iPID < 0)
            {
                ++pLocalLink;
                continue;    
            }
            
            if(pLocalLink->iPID == iPid && pLocalLink->iTID == iTid)
            {
                pLocalLink->iLogLevel = iLogLevel;
                
                return 0;
            }

            ++pLocalLink;
        }
        #endif
        return -1;
    }

    int TMdbLogInfo::SetLocalLink(TMdbDSN  *pDsn, int iPid, int iLogLevel)
    {

        #if 0
        TMdbLocalLink *pLocalLink = (TMdbLocalLink *)m_pShmDSN->GetLocalLinkAddr();
        
        for(int i=0; i<MAX_LINK_COUNTS; ++i)
        {
            if(pLocalLink->iPID < 0)
            {
                ++pLocalLink;
                continue;    
            }
            
            if(pLocalLink->iPID == iPid )
            {
                pLocalLink->iLogLevel = iLogLevel;
                
                return 0;
            }    	

            ++pLocalLink;
        }
        #endif
        return -1;
    }


    /******************************************************************************
    * ��������	:  SetRemoteLink()
    * ��������	:  ����Զ��������־����    
    * ����		:  pszIP, Զ��IP
    * ����		:  iPort, Զ�̶˿�
    * ����		:  iLogLevel, ��־����
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbLogInfo::SetRemoteLink(const char* pszIP, int iLogLevel)    
    {
#if 0
        if(m_pDsn == NULL)
            return -1;
        
        TMdbRemoteLink *pRemoteLink = (TMdbRemoteLink *)m_pShmDSN->GetRemoteLinkAddr();
        
        for(int i=0; i<MAX_LINK_COUNTS; ++i)
        {            
            if(pRemoteLink->sIP[0] == 0)
            {
                ++pRemoteLink;
                continue;    
            }
            
            pRemoteLink->Print();
            
            if(strcmp(pRemoteLink->sIP, pszIP) == 0)
            {
                pRemoteLink->iLogLevel = iLogLevel;
                
                return 0;    
            } 

            ++pRemoteLink;
        }
        #endif
        return -1;
    }


    int TMdbLogInfo::SetRemoteLink(TMdbDSN  *pDsn,int pid,int tid, int iLogLevel)
    {   
#if 0
        TMdbRemoteLink *pRemoteLink = (TMdbRemoteLink *)m_pShmDSN->GetRemoteLinkAddr();
        
        for(int i=0; i<MAX_LINK_COUNTS; ++i)
        {
            
            if(pRemoteLink->iPID == pid && pRemoteLink->iTID == tid)
            {
                pRemoteLink->iLogLevel = iLogLevel;
                
                return 0;
            }

            ++pRemoteLink;
        }
        #endif
        return -1;
    	//return 0;
    }	

    int TMdbLogInfo::SetRemoteLink(TMdbDSN  *pDsn,int pid, int iLogLevel)
    {
#if 0
        TMdbRemoteLink *pRemoteLink = (TMdbRemoteLink *)m_pShmDSN->GetRemoteLinkAddr();
        
        for(int i=0; i<MAX_LINK_COUNTS; ++i)
        {
            
            if(pRemoteLink->iPID == pid )
            {
                pRemoteLink->iLogLevel = iLogLevel;
                
                return 0;
            }

            ++pRemoteLink;
        }
        #endif
        return -1;
    }	


    /******************************************************************************
    * ��������	:  SetProc()
    * ��������	:  ���ý�����־��Ϣ    
    * ����		:  iPid, ����ID
    * ����		:  iLogLevel, ��־����
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbLogInfo::SetProc(int iPid, int iLogLevel)
    {
        if(m_pDsn == NULL)
            return -1;
        TMdbProc *pProc = m_pShmDSN->GetProcByPid(iPid);
        if(NULL == pProc){return -1;}
        pProc->iLogLevel = iLogLevel;  
        pProc->Print();
        return 0;
    }

    int TMdbLogInfo::SetProc(TMdbDSN  *pDsn, int ipid, int iLogLevel)
    {
        #if 0
    	char *pAddr = (char*)pDsn + pDsn->iProcAddr;
        TMdbProc *pProc = (TMdbProc *)pAddr;
        
        for(int i=0; i<MAX_PROCESS_COUNTS; ++i)
        {
            //IS_LOG(3)
             //   pProc->Print();
                
            if(pProc->iPid == ipid)
            {
                pProc->iLogLevel = iLogLevel;  
                  
                //IS_LOG(3)
                 //   pProc->Print();
                
                return 0;
            }    
            
            ++pProc;
        }
        #endif
        return -1;
        
    }

    int TMdbLogInfo::SetProc(TMdbDSN *pDsn, char *processName, int iLogLevel)
    {
        #if 0
    	char *pAddr = (char*)pDsn + pDsn->iProcAddr;
        TMdbProc *pProc = (TMdbProc *)pAddr;
        
        for(int i=0; i<MAX_PROCESS_COUNTS; ++i)
        {
            //IS_LOG(3)
             //   pProc->Print();
                
            if(TMdbStrFunc::StrNoCaseCmp(pProc->sName,processName) == 0)
            {
                pProc->iLogLevel = iLogLevel;  
                  
                //IS_LOG(3)
                 //   pProc->Print();
                
                return 0;
            }    
            
            ++pProc;
        }
        #endif
        return -1;
    }

    int TMdbLogInfo::SetProc(const char *pProcessName, int iLogLevel)
    {
         if(m_pDsn == NULL)
             return -1;
         TMdbProc *pProc = m_pShmDSN->GetProcByName(pProcessName);
         if(NULL == pProc){return -1;}
         pProc->iLogLevel = iLogLevel;  
         pProc->Print();
         return 0;
    }


//}
