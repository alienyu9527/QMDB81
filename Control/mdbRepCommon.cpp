/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepCommon.h		
*@Description: �����Ƭ�������õĽṹ��ͱ���
*@Author:		jiang.lili
*@Date��	    2014/05/4
*@History:
******************************************************************************************/
#include "Control/mdbRepCommon.h"
//namespace QuickMDB
//{
    TMdbShmRepMgr::TMdbShmRepMgr(const char* sDsn): m_pShmRep(NULL), m_pShm(NULL)
    {
        m_strName.Assign(sDsn);
        m_strName.ToUpper();
        m_strName.Append("_");
        m_strName.Append(MDB_SHM_ROUTING_REP_NAME);
    }

    TMdbShmRepMgr::~TMdbShmRepMgr()
    {
        SAFE_DELETE(m_pShm);
    }

         /******************************************************************************
        * ��������	:  Create
        * ��������	:  ���������ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbShmRepMgr::Create()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (m_pShm!=NULL)//�ظ�����
        {
            m_pShmRep = (TMdbShmRep*)m_pShm->GetBuffer();
            m_pShmRep->Clear();
            return 0;
        }

        if (TMdbNtcShareMem::CheckExist(m_strName.c_str(), ENV_QMDB_HOME_NAME))//�����ڴ��Ѿ�����
        {
            TADD_WARNING("ShareMem[%s] already exists.",m_strName.c_str());
            CHECK_RET(Attach(), "Attach to ShareMem[%s] Failed.",m_strName.c_str());
            m_pShmRep->Clear();
        }
        else
        {
            m_pShm = new(std::nothrow) TMdbNtcShareMem(m_strName.c_str(), sizeof(TMdbShmRep),ENV_QMDB_HOME_NAME);
            CHECK_OBJ(m_pShm);
            if (m_pShm->IsOK())
            {
                TADD_NORMAL("Create TShareMem[%s] OK, SHM_ID[%d].",m_strName.c_str(), m_pShm->GetShmID());
                m_pShmRep = (TMdbShmRep*)m_pShm->GetBuffer();
                m_pShmRep->Clear();
            }
            else
            {
                SAFE_DELETE(m_pShm);
                CHECK_RET(-1, "Create TShareMem[%s] failed. ErrorNo=%d, ErrorTest = %s", m_strName.c_str(),m_pShm->GetErrorNo(), m_pShm->GetErrorText().c_str());
            }

        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }
        /******************************************************************************
        * ��������	:  Attach
        * ��������	:  ���ӹ����ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbShmRepMgr::Attach()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (m_pShm!=NULL)
        {
            m_pShmRep=(TMdbShmRep*)m_pShm->GetBuffer();
            return iRet;
        }

        if (!TMdbNtcShareMem::CheckExist(m_strName.c_str(), ENV_QMDB_HOME_NAME))
        {
            CHECK_RET(-1, "ShareMem[%s] does not exist",m_strName.c_str());
        }

        m_pShm = new(std::nothrow) TMdbNtcShareMem(m_strName.c_str(), 0, ENV_QMDB_HOME_NAME);
        CHECK_OBJ(m_pShm);
        if (m_pShm->IsOK())
        {
            m_pShmRep = (TMdbShmRep*)m_pShm->GetBuffer();
            TADD_NORMAL("Attach TShareMem[%s] OK.",m_strName.c_str());
        }
        else
        {
            SAFE_DELETE(m_pShm);
            CHECK_RET(-1, "Attach TShareMem[%s] failed. ErrorNo=%d, ErrorTest = %s",m_strName.c_str(), m_pShm->GetErrorNo(), m_pShm->GetErrorText().c_str());
        }

        TADD_FUNC("Finish.");
        return iRet;
    }
        /******************************************************************************
        * ��������	:  Detach
        * ��������	:  �Ͽ��빲���ڴ������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbShmRepMgr::Detach()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        // QuickMDB::TShareMem�����Ĺ����ڴ�,����Detach.
        TADD_FUNC("Finish.");
        return iRet;
    }
        /******************************************************************************
        * ��������	:  Destroy
        * ��������	:  ���ٹ����ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbShmRepMgr::Destroy()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        if(!TMdbNtcShareMem::CheckExist(m_strName.c_str(), ENV_QMDB_HOME_NAME))
        {
            TADD_NORMAL("TShareMem[%s] not exist, not to destroy",m_strName.c_str());
            return iRet;
        }
        
        bool bRet = TMdbNtcShareMem::Destroy(m_strName.c_str(), ENV_QMDB_HOME_NAME);
        if(bRet)
        {
            TADD_NORMAL("Destroy TShareMem[%s] OK.",m_strName.c_str());
        }
        else
        {
            TADD_ERROR(-1,"Destroy TShareMem[%s] failed.",m_strName.c_str());
            bRet = -1;
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * ��������	:  WriteRoutingInfo
        * ��������	:  ��·����Ϣд�빲���ڴ�
        * ����		:  pData ���յ���·����Ϣ�� ��ʽ rID:hID1,hID2;rID2:hID3|rID:hID;rID2:hID|rID:rule;rID1:rule;rID2:rule2|hID:ip,port;hID2:ip,port
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbShmRepMgr::WriteRoutingInfo(const char *pData, int iLen)
    {
        TADD_FUNC("Start.");
        TADD_NORMAL("RoutingInfo:[%s]", pData);
        int iRet = 0;
        TMdbNtcSplit tSplitAll;
        tSplitAll.SplitString(pData, '|');
        if (tSplitAll.GetFieldCount()!=4)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "Invalid routing information format. ");
        }

        TMdbNtcSplit tSplitSemi;
        TMdbNtcSplit tSplitColon;
        TMdbNtcSplit tSplitComma;
        //����·�ɹ���ID  rID:rule;rID1:rule;rID2:rule2
        //QuickMDB::TIntMap tRuleIDMap;
        std::map< int, std::vector<int> > tRuleIDMap;
        tSplitSemi.SplitString(tSplitAll[2], ';');
        for (unsigned int i = 0; i<tSplitSemi.GetFieldCount();i++)
        {
            //QuickMDB::TMdbNtcAutoArray *ptIDArray = new(std::nothrow) QuickMDB::TMdbNtcAutoArray();
            std::vector<int> tIDArray;
            tSplitColon.SplitString(tSplitSemi[i], ':');
            if (tSplitColon.GetFieldCount()!=2)
            {
                CHECK_RET(ERR_APP_INVALID_PARAM, "Invalid routing information format. ");
            }
            CHECK_RET(TMdbRoutingTools::GetIDArrayFromRule(tSplitColon[1], tIDArray), "GetIDArrayFromRule failed.");
            tRuleIDMap.insert(std::pair< int, std::vector<int> >(atoi(tSplitColon[0]), tIDArray));
            
        }
        //��������ID hID:ip,port;hID2:ip,port
        tSplitSemi.SplitString(tSplitAll[3], ';');
        m_pShmRep->m_iRepHostCount = 0;
        for (unsigned int i = 0; i<tSplitSemi.GetFieldCount(); i++)
        {
            tSplitColon.SplitString(tSplitSemi[i], ':');
            if (tSplitColon.GetFieldCount()!=2)
            {
                CHECK_RET(ERR_APP_INVALID_PARAM, "Invalid routing information format. ");
            }
            tSplitComma.SplitString(tSplitColon[1], ',');
            if (tSplitComma.GetFieldCount()!=2)
            {
                CHECK_RET(ERR_APP_INVALID_PARAM, "Invalid routing information format. ");
            }
            m_pShmRep->m_arHosts[m_pShmRep->m_iRepHostCount].m_iHostID = atoi(tSplitColon[0]);
            SAFESTRCPY(m_pShmRep->m_arHosts[m_pShmRep->m_iRepHostCount].m_sIP, MAX_IP_LEN, tSplitComma[0]);
            m_pShmRep->m_arHosts[m_pShmRep->m_iRepHostCount].m_iPort = atoi(tSplitComma[1]);
            m_pShmRep->m_iRepHostCount++;
        }
        //������ӳ���ϵrID:hID1,hID2;rID2:hID3
        std::vector<int> vRoutingID;
        tSplitSemi.SplitString(tSplitAll[0], ';');      
        m_pShmRep->m_iRoutingIDCount=0;
        m_pShmRep->m_sRoutingList[0] = '\0';
        for (unsigned int i = 0; i<tSplitSemi.GetFieldCount(); i++)
        {
            tSplitColon.SplitString(tSplitSemi[i], ':');
            if (tSplitColon.GetFieldCount()==2)
            {
                tSplitComma.SplitString(tSplitColon[1], ',', true);

                // std::vector<int> tIDArray = tRuleIDMap.find(atoi(tSplitColon[0]))->second();
                vRoutingID = (tRuleIDMap.find(atoi(tSplitColon[0])))->second;//�ҵ�RuleID��Ӧ������·������
                std::vector<int>::iterator itor = vRoutingID.begin();
                for(; itor != vRoutingID.end(); ++itor)
                {
                    snprintf(m_pShmRep->m_sRoutingList+strlen(m_pShmRep->m_sRoutingList), MAX_REP_ROUTING_LIST_LEN-strlen(m_pShmRep->m_sRoutingList), "%d,", (*itor));
                    m_pShmRep->m_arRouting[m_pShmRep->m_iRoutingIDCount].m_iRoutingID = (*itor);
                    for (unsigned int k = 0; k< tSplitComma.GetFieldCount(); k++)
                    {
                        m_pShmRep->m_arRouting[m_pShmRep->m_iRoutingIDCount].m_aiHostID[k] =atoi(tSplitComma[k]);
                    }
                    
                    m_pShmRep->m_iRoutingIDCount++;
                }
            }
            else
            {
                //��·������ֻ���ڱ����ϣ������ݵ���������
                vRoutingID = (tRuleIDMap.find(atoi(tSplitColon[0])))->second;//�ҵ�RuleID��Ӧ������·������
                std::vector<int>::iterator itor = vRoutingID.begin();
                for(; itor != vRoutingID.end(); ++itor)
                {
                    snprintf(m_pShmRep->m_sRoutingList+strlen(m_pShmRep->m_sRoutingList), MAX_REP_ROUTING_LIST_LEN-strlen(m_pShmRep->m_sRoutingList), "%d,", (*itor));
                    m_pShmRep->m_arRouting[m_pShmRep->m_iRoutingIDCount].m_iRoutingID = (*itor);

                    m_pShmRep->m_iRoutingIDCount++;
                }
            }
            

/*            for(unsigned int j=0; j < pAutoArray->GetSize(); j++)
            {
                snprintf(m_pShmRep->m_sRoutingList, MAX_REP_ROUTING_LIST_LEN-strlen(m_pShmRep->m_sRoutingList), "%d,", pAutoArray->GetAt(i));
                m_pShmRep->m_arRouting[m_pShmRep->m_iRoutingIDCount].m_iRoutingID = pAutoArray->GetAt(i);
                for (unsigned int k = 0; k< tSplitComma.GetFieldCount(); k++)
                {
                    m_pShmRep->m_arRouting[m_pShmRep->m_iRoutingIDCount].m_aiHostID[k] =atoi(tSplitComma[i]);
                }
            }    */ 
        }

        TADD_DETAIL("m_pShmRep->m_iRoutingIDCount=%d, routing-id=%d", m_pShmRep->m_iRoutingIDCount,m_pShmRep->m_arRouting[0].m_iRoutingID);
        if(m_pShmRep->m_iRoutingIDCount ==1 && m_pShmRep->m_arRouting[0].m_iRoutingID == DEFALUT_ROUT_ID)
        {
            TADD_NORMAL("no-routing-id-rep mode.");
            m_pShmRep->m_bNoRtMode = true;
        }
        
        //�������ֻ�ӳ���ϵ rID:hID;rID2:hID
        tSplitSemi.SplitString(tSplitAll[1], ';');        
        for (unsigned int i = 0; i<tSplitSemi.GetFieldCount(); i++)
        {
            tSplitColon.SplitString(tSplitSemi[i], ':');
            if (tSplitColon.GetFieldCount()!=2)
            {
                CHECK_RET(-1, "Invalid routing information format. ");
            }

            vRoutingID = tRuleIDMap.find(atoi(tSplitColon[0]))->second;//�ҵ�RuleID��Ӧ������·������
            for(unsigned int j=0; j < vRoutingID.size(); j++)
            {
                for (int n =0; n<m_pShmRep->m_iRoutingIDCount; n++)
                {
                    if (m_pShmRep->m_arRouting[n].m_iRoutingID== vRoutingID[i])
                    {
                        m_pShmRep->m_arRouting[n].m_iRecoveryHostID = atoi(tSplitColon[1]);//·�ɶ�Ӧ�� ��������
                        break;
                    }
                }
            }     
        }

        TADD_FUNC("Finish.");
        return iRet;      
    }

    TMdbShmRepRouting* TMdbShmRepMgr::GetRepHosts(int iRoutingID)
    {
        for (int i = 0; i<MAX_REP_ROUTING_ID_COUTN; i++)
        {
            if (m_pShmRep->m_arRouting[i].m_iRoutingID == iRoutingID)
            {
                return &m_pShmRep->m_arRouting[i];
            }
        }
        return NULL;
    }

         /******************************************************************************
        * ��������	:  AddFailedRoutingList
        * ��������	:  ������ʧ�ܵ�·���б�д�빲���ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbShmRepMgr::AddFailedRoutingList(const char* sRoutingList)
    {
        //int iRet = 0;
        TADD_FUNC("TMdbShmRepMgr::AddFailedRoutingID(iRoutingID = %s", sRoutingList);
        CHECK_OBJ(sRoutingList);
        if (m_pShmRep->m_sFailedRoutingList[0] == '\0')
        {
            SAFESTRCPY(m_pShmRep->m_sFailedRoutingList, MAX_REP_ROUTING_LIST_LEN, sRoutingList);
        }
        else
        {
            snprintf(m_pShmRep->m_sFailedRoutingList, MAX_REP_ROUTING_LIST_LEN-strlen(m_pShmRep->m_sFailedRoutingList), ",%s", sRoutingList);
        }
        return 0;
    }

        /******************************************************************************
        * ��������	:  AddFailedRoutingID
        * ��������	:  ������ʧ�ܵ�·��IDд�빲���ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbShmRepMgr::AddFailedRoutingID(int iRoutingID)
    {
        //int iRet = 0;
        TADD_FUNC("TMdbShmRepMgr::AddFailedRoutingID(iRoutingID = %d)", iRoutingID);
        if (m_pShmRep->m_sFailedRoutingList[0] == '\0')
        {
            snprintf(m_pShmRep->m_sFailedRoutingList, MAX_REP_ROUTING_LIST_LEN-strlen(m_pShmRep->m_sFailedRoutingList), "%d", iRoutingID);
        }
        else
        {
            snprintf(m_pShmRep->m_sFailedRoutingList+strlen(m_pShmRep->m_sFailedRoutingList), MAX_REP_ROUTING_LIST_LEN-strlen(m_pShmRep->m_sFailedRoutingList), ",%d", iRoutingID);
        }
        return 0;
    }

    const char* TMdbShmRepMgr::GetFailedRoutingList()
    {
       return m_pShmRep->m_sFailedRoutingList;        
    }
        /******************************************************************************
        * ��������	:  IsIDInString
        * ��������	:  ָ��ID�Ƿ���ID�б�֮��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    bool TMdbRoutingTools::IsIDInString(int iID, const char* sIDList)
    {
        bool bRet = false;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(sIDList, ',');//IDlist �Զ��ŷָ�
        for (unsigned int i = 0; i<tSplit.GetFieldCount(); i++)
        {
            if (iID == atoi(tSplit[i]))
            {
                bRet = true;
                break;
            }
        }
        return bRet;
    }

    bool TMdbRoutingTools::IsIDInArray(int iID, std::vector<int> &tIDArray)
    {
        bool bRet = false;
       
        for (unsigned int i = 0; i<tIDArray.size(); i++)
        {
            if (iID == tIDArray[i])
            {
                bRet = true;
                break;
            }
        }
        return bRet;
    }

        /******************************************************************************
        * ��������	:  IsIDInString
        * ��������	:  ����ID�б���ID����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbRoutingTools::GetIntArrayFromStrList(const char* sIDList, std::vector<int> &tIDArray)
    {
        int iRet = 0;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(sIDList, ',', true);
        for (unsigned int i = 0; i<tSplit.GetFieldCount(); i++)
        {
            tIDArray.push_back(atoi(tSplit[i]));
        }
        return iRet;
    }

        /******************************************************************************
        * ��������	:  GetIDArrayFromRule
        * ��������	:  ���ݹ��򣬲���RoutingID����
        * ����		:  ���������֣�<rule ID="1"  value = "100" />        <rule ID="2"  value = "10,11,12,13" />        <rule ID="3"  value = "[100,200]" />
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbRoutingTools::GetIDArrayFromRule(const char* sRule, std::vector<int> &tIDArray)
    {
        int iRet = 0;
        tIDArray.clear();
        if (sRule[0] != '[')//�б��ʽ
        {
            TMdbNtcSplit tSplit;
            tSplit.SplitString(sRule, ',');
            for (unsigned int i = 0; i<tSplit.GetFieldCount(); i++)
            {
                tIDArray.push_back(atoi(tSplit[i]));
            }
        }
        else
        {
            std::string strRule(sRule, 1,strlen(sRule)-2);
            TMdbNtcSplit tSplit;
            tSplit.SplitString(strRule.c_str(), ',');
            if (tSplit.GetFieldCount() !=2)
            {
                CHECK_RET(-1, "Invalid Rule format.");
            }
            int iBegin = atoi(tSplit[0]);
            int iEnd = atoi((tSplit[1]));
            for (int i = iBegin; i<=iEnd; i++)
            {
                tIDArray.push_back(i);
            }
        }
        return iRet;
    }

    TMdbRepConfig::TMdbRepConfig()
    {

    }

    TMdbRepConfig::~TMdbRepConfig()
    {

    }

    int TMdbRepConfig::Init(const char* sDsn)
    {
        int iRet = 0;

        m_pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
        CHECK_OBJ(m_pConfig);

        m_sServerIP = m_pConfig->GetDSN()->sRepSvrIp;
        m_iServerPort = m_pConfig->GetDSN()->iRepSvrPort;
        m_sRepServerIP = m_pConfig->GetDSN()->sRepStandbySvrIp;
        m_iRepServerPort = m_pConfig->GetDSN()->iRepStandbySvrPort;
        m_sRepPath = m_pConfig->GetDSN()->sRepFilePath;
        m_iFileInvalidTime = m_pConfig->GetDSN()->iInvalidRepFileTime;      
        m_iMaxFileSize = m_pConfig->GetDSN()->iLogFileSize*1024*1024;
        m_iBackupInterval = m_pConfig->GetDSN()->iLogTime;

        m_sLocalIP = m_pConfig->GetDSN()->sLocalIP;
        m_iLocalPort = m_pConfig->GetDSN()->iRepLocalPort;

        TADD_DETAIL("Server IP=[%s],Port=[%d]", m_sServerIP.c_str(),m_iServerPort);
        TADD_DETAIL("Standby Server IP=[%s],Port=[%d]", m_sRepServerIP.c_str(),m_iRepServerPort);
        TADD_DETAIL("Local IP=[%s],Port=[%d]", m_sLocalIP.c_str(),m_iLocalPort);
        
        return iRet;
    }


	TMdbTableChangeOper::TMdbTableChangeOper()
	{
		m_iColPos = -1;
		m_iColType = -1;
		m_iColLen = -1;
		m_iChangeType = 0;
		m_iValue = 0;
		m_llValue = 0;
		memset(m_sValue, 0, sizeof(m_sValue));
	}
	
	TMdbTableChangeOper::~TMdbTableChangeOper()
	{
		
	}
	
	void TMdbTableChangeOper::Clear()
	{
		m_iColPos = -1;
		m_iColType = -1;
		m_iColLen = -1;
		m_iChangeType = 0;
		m_iValue = 0;
		m_llValue = 0;
		memset(m_sValue, 0, sizeof(m_sValue));
	}

	TMdbLoadTableRemoteStruct::TMdbLoadTableRemoteStruct()
	{
		Clear();
	}

	TMdbLoadTableRemoteStruct::~TMdbLoadTableRemoteStruct()
	{
		
	}

	void TMdbLoadTableRemoteStruct::Clear()
	{
		for(int i = 0; i < MAX_COLUMN_COUNTS; i++)
		{
			m_iIsLocalCol[i] = -1;
			m_iColPos[i] = -1;
		}
		m_iColCount = 0;
	}
//}
//}