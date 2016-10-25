/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbPageCtrl.cpp		
*@Description�� �������ĳ��ҳ�Ŀռ���Ϣ����ȡһ������ҳ������ҳ
*@Author:		li.shugang
*@Date��	    2008��11��30��
*@History:
******************************************************************************************/
#include "Control/mdbPageCtrl.h"
#include "string.h"


//namespace QuickMDB{

    TMdbPageCtrl::TMdbPageCtrl()
    {
		m_pDSN = NULL;
    }


    TMdbPageCtrl::~TMdbPageCtrl()
    {
        
    }
    /******************************************************************************
    * ��������	:  SetDSN()
    * ��������	:  ɾ��ҳ��һ������    
    * ����		:  iPos, ���ݵĽڵ�λ��  
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����û���ҵ������򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbPageCtrl::SetDSN(const char* pszDSN, E_MUTEX_TYPE eMutexType)
    {
        return m_tMutex.Init(pszDSN, m_pDSN, eMutexType);
    }
    /******************************************************************************
    * ��������	:  Attach()
    * ��������	:  ����һ��ҳ    
    * ����		:  pAddr, ҳ���׵�ַ 
    * ����		:  bReadLock, �Ƿ��ж���
    * ����		:  bWriteLock, �Ƿ���д�� 
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbPageCtrl::Attach(char* pAddr, bool bReadLock, bool bWriteLock)
    {
        TADD_FUNC("Start.");
        //�������Ƿ�Ϸ�
        CHECK_OBJ(pAddr);
        m_bReadLock = bReadLock;
        m_bWriteLock = bWriteLock;
        //��ʼ��ҳ��Ϣ
        m_pAddr = pAddr;
        m_pPage = (TMdbPage*)pAddr;
        return 0;
    }
    //�Ӷ���
    int TMdbPageCtrl::RLock()
    {
        if(m_bReadLock)
        {
			return m_tMutex.Lock(m_pPage->m_iPageID);
        }
        return 0;
    }
    //��д��
    int TMdbPageCtrl::WLock()
    {
        if(m_bWriteLock)
        {
			return m_tMutex.Lock(m_pPage->m_iPageID);
        }
        return 0;
    }
    //�����
    void TMdbPageCtrl::UnRLock()
    {
        if(m_bReadLock)
        {
            m_tMutex.UnLock(m_pPage->m_iPageID);
        }
    }
    //��д��
    void TMdbPageCtrl::UnWLock()
    {
        if(m_bWriteLock)
        {
            m_tMutex.UnLock(m_pPage->m_iPageID);
        }
    }
    /******************************************************************************
    * ��������	:  InsertData()
    * ��������	:  ��ҳ�в���һ������    
    * ����		:  pData, ���ݵ��׵�ַ  
    * ����		:  iLen, ���ݵĴ�С 
    * ���		:  rowID, ���ص�ROWID,   pMemData - �ڴ��ַ
    * ����ֵ	:  �ɹ�����0�����û�пռ��򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbPageCtrl::InsertData(unsigned char* pData, int iLen, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn)
    {
        TADD_FUNC("Start.");
        //�������Ƿ�Ϸ�
        if(pData==NULL || iLen<0 ||iLen>1024*1024*10)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. pData=%p, iSize=%d.",pData, iLen);
            return ERR_APP_INVALID_PARAM;    
        }
        if(WLock() < 0)//����
        	return ERR_SQL_EXECUTE_TIMEOUT;
        int iRet = 0;
        do
        {   //����һ���¿ռ�
            int iDataOffset = 0;
            iRet = m_pPage->GetFreeRecord(iDataOffset,iLen);
            if(iRet == ERR_PAGE_NO_MEMORY)
            {//pageû�пռ��ˣ����豨��
                break;
            }
            else if(0 != iRet)
            {//��������
                CHECK_RET_BREAK(iRet,"GetFreeRecord failed,RowId[%d|%d],size=[%d],page=[%s]",
                        rowID.GetPageID(),rowID.GetDataOffset(),iLen,m_pPage->ToString().c_str());
            }
            rowID.SetDataOffset(m_pPage->DataOffsetToRecordPos(iDataOffset));//��������λ��
            rowID.SetPageID(m_pPage->m_iPageID);
            //rowID.iPageID = m_pPage->m_iPageID;
            //pMemData = m_pAddr + rowID.iDataOffset;
            pMemData = m_pAddr + iDataOffset;
            memcpy(pMemData, pData, iLen);
            //�޸�ͷ��Ϣ:��¼��������ƫ������ʣ��ռ��С��ʱ���
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);
            ++(m_pPage->m_iRecordCounts); 
            if(UpdateLsn){UpdatePageLSN();}
            TADD_DETAIL("page_id=[%d],offsetPos=[%d],rowid=[%d].",m_pPage->m_iPageID,iDataOffset,rowID.m_iRowID);
            
        }while(0);
        UnWLock();//����
        TADD_FUNC("Finish.");
        return iRet;
    }

	/******************************************************************************
	* ��������	:  InsertData()
	* ��������	:  ��ҳ�в���һ������	 
	* ����		:  pData, ���ݵ��׵�ַ	
	* ����		:  iLen, ���������ݵĴ�С 
	* ����		:  iSize, ҳ��¼�Ĵ�С 
	* ���		:  rowID, ���ص�ROWID,	 pMemData - �ڴ��ַ
	* ����ֵ	:  �ɹ�����0�����û�пռ��򷵻�-1
	* ����		:  li.shugang
	*******************************************************************************/
	int TMdbPageCtrl::InsertData(unsigned char* pData,int iLen, int iSize, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn)
	{
		TADD_FUNC("Start.");
		//�������Ƿ�Ϸ�
		if(pData==NULL || iSize<0 ||iSize>1024*1024*10)
		{
			TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. pData=%p, iSize=%d.",pData, iSize);
			return ERR_APP_INVALID_PARAM;	 
		}
		if(WLock() < 0)//����
			return ERR_SQL_EXECUTE_TIMEOUT;
		int iRet = 0;
		do
		{	//����һ���¿ռ�
			int iDataOffset = 0;
			iRet = m_pPage->GetFreeRecord(iDataOffset,iSize);
			if(iRet == ERR_PAGE_NO_MEMORY)
			{//pageû�пռ��ˣ����豨��
				break;
			}
			else if(0 != iRet)
			{//��������
				CHECK_RET_BREAK(iRet,"GetFreeRecord failed,RowId[%d|%d],size=[%d],page=[%s]",
						rowID.GetPageID(),rowID.GetDataOffset(),iSize,m_pPage->ToString().c_str());
			}
			rowID.SetDataOffset(m_pPage->DataOffsetToRecordPos(iDataOffset));//��������λ��
			rowID.SetPageID(m_pPage->m_iPageID);
			//rowID.iPageID = m_pPage->m_iPageID;
			//pMemData = m_pAddr + rowID.iDataOffset;
			pMemData = m_pAddr + iDataOffset;
			memcpy(pMemData, pData, iLen);
			//�޸�ͷ��Ϣ:��¼��������ƫ������ʣ��ռ��С��ʱ���
			SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);
			++(m_pPage->m_iRecordCounts); 
			if(UpdateLsn){UpdatePageLSN();}
			TADD_DETAIL("page_id=[%d],offsetPos=[%d],rowid=[%d].",m_pPage->m_iPageID,iDataOffset,rowID.m_iRowID);
			
		}while(0);
		UnWLock();//����
		TADD_FUNC("Finish.");
		return iRet;
	}


    int TMdbPageCtrl::InsertData_NoMutex(unsigned char* pData, int iLen, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn)
    {
        TADD_FUNC("Start.");
        //�������Ƿ�Ϸ�
        if(pData==NULL || iLen<0 ||iLen>1024*1024*10)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. pData=%p, iSize=%d.",pData, iLen);
            return ERR_APP_INVALID_PARAM;    
        }
        int iRet = 0;
        do
        {   //����һ���¿ռ�
            int iDataOffset = 0;
            iRet = m_pPage->GetFreeRecord(iDataOffset,iLen);
            if(iRet == ERR_PAGE_NO_MEMORY)
            {//pageû�пռ��ˣ����豨��
                break;
            }
            else if(0 != iRet)
            {//��������
                CHECK_RET_BREAK(iRet,"GetFreeRecord failed,RowId[%d|%d],size=[%d],page=[%s]",
                        rowID.GetPageID(),rowID.GetDataOffset(),iLen,m_pPage->ToString().c_str());
            }
            rowID.SetDataOffset(m_pPage->DataOffsetToRecordPos(iDataOffset));//��������λ��
            rowID.SetPageID(m_pPage->m_iPageID);
            //rowID.iPageID = m_pPage->m_iPageID;
            //pMemData = m_pAddr + rowID.iDataOffset;
            pMemData = m_pAddr + iDataOffset;
            memcpy(pMemData, pData, iLen);
            //�޸�ͷ��Ϣ:��¼��������ƫ������ʣ��ռ��С��ʱ���
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);
            ++(m_pPage->m_iRecordCounts); 
            if(UpdateLsn){UpdatePageLSN();}
            TADD_DETAIL("page_id=[%d],offsetPos=[%d],rowid=[%d].",m_pPage->m_iPageID,iDataOffset,rowID.m_iRowID);
            
        }while(0);
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbPageCtrl::InsertData_NoMutex(unsigned char* pData, int iLen, int iSize, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn)
    {
        TADD_FUNC("Start.");
        //�������Ƿ�Ϸ�
        if(pData==NULL || iLen<0 ||iLen>1024*1024*10)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. pData=%p, iSize=%d.",pData, iLen);
            return ERR_APP_INVALID_PARAM;    
        }
        int iRet = 0;
        do
        {   //����һ���¿ռ�
            int iDataOffset = 0;
            iRet = m_pPage->GetFreeRecord(iDataOffset,iSize);
            if(iRet == ERR_PAGE_NO_MEMORY)
            {//pageû�пռ��ˣ����豨��
                break;
            }
            else if(0 != iRet)
            {//��������
                CHECK_RET_BREAK(iRet,"GetFreeRecord failed,RowId[%d|%d],size=[%d],page=[%s]",
                        rowID.GetPageID(),rowID.GetDataOffset(),iLen,m_pPage->ToString().c_str());
            }
            rowID.SetDataOffset(m_pPage->DataOffsetToRecordPos(iDataOffset));//��������λ��
            rowID.SetPageID(m_pPage->m_iPageID);
            pMemData = m_pAddr + iDataOffset;
            memcpy(pMemData, pData, iLen);
            //�޸�ͷ��Ϣ:��¼��������ƫ������ʣ��ռ��С��ʱ���
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);
            ++(m_pPage->m_iRecordCounts); 
            if(UpdateLsn){UpdatePageLSN();}
            TADD_DETAIL("page_id=[%d],offsetPos=[%d],rowid=[%d].",m_pPage->m_iPageID,iDataOffset,rowID.m_iRowID);
            
        }while(0);
        TADD_FUNC("Finish.");
        return iRet;
    }
	
    /******************************************************************************
    * ��������	:  DeleteData()
    * ��������	:  ɾ��ҳ��һ������    
    * ����		:  iPos, ���ݵĽڵ�λ��  
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����û���ҵ������򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbPageCtrl::DeleteData(int iDataOffset,bool UpdateLsn)
    {
        TADD_FUNC("(iPos=%d) : Start.", iDataOffset);
        int iRet = 0;
        if(WLock() < 0)//����
        	return ERR_SQL_EXECUTE_TIMEOUT;
        do
        {
            CHECK_RET_BREAK(m_pPage->PushBack(m_pPage->RecordPosToDataOffset(iDataOffset)),"PushBack(%d) page=[%s] failed.",iDataOffset,m_pPage->ToString().c_str());//�黹ƫ��
            //��ӡ���нڵ�ķֲ����
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);//ʱ���
            --m_pPage->m_iRecordCounts;
            if(UpdateLsn){UpdatePageLSN();}
        }while(0);
        UnWLock();
        TADD_FUNC("Finish.");
        return iRet;
    }
    int TMdbPageCtrl::DeleteData_NoMutex(int iDataOffset,bool UpdateLsn)
    {
        TADD_FUNC("(iPos=%d) : Start.", iDataOffset);
        int iRet = 0;
        do
        {
            CHECK_RET_BREAK(m_pPage->PushBack(m_pPage->RecordPosToDataOffset(iDataOffset)),"PushBack(%d) page=[%s] failed.",iDataOffset,m_pPage->ToString().c_str());//�黹ƫ��
            //��ӡ���нڵ�ķֲ����
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);//ʱ���
            --m_pPage->m_iRecordCounts;
            if(UpdateLsn){UpdatePageLSN();}
        }while(0);
        TADD_FUNC("Finish.");
        return iRet;
    }
//}

