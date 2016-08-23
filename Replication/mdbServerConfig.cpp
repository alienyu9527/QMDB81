/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbServerConfig.h		
*@Description： 统一配置服务的配置文件
*@Author:		jiang.lili
*@Date：	    2014/03/20
*@History:
******************************************************************************************/
#include "Replication/mdbServerConfig.h"
#include "Helper/mdbDateTime.h"

//namespace QuickMDB
//{
    TMdbServerConfig::TMdbServerConfig():m_ptDoc(NULL)
    {
        Clear();
    }

    TMdbServerConfig::~TMdbServerConfig()
    {
        SAFE_DELETE(m_ptDoc);
        Clear();
    }
    /******************************************************************************
    * 函数名称	:  ReadConfig
    * 函数描述	: 读取配置文件
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbServerConfig::ReadConfig()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbNtcStrFunc tStrFun;
        TMdbNtcString sConfigFile(MAX_PATH_NAME_LEN,"%s/%s", tStrFun.ReplaceEnv(MDB_SERVER_CONFIG_DIR), MDB_SERVER_CONFIG_NAME);//系统配置文件
        if( !TMdbNtcDirOper::IsFileExist(sConfigFile.c_str()) ) 
        {
            CHECK_RET(ERR_OS_NO_FILE,"Configuration file[%s] not exist.",sConfigFile.c_str());
        }

        //读取配置文件
        if (NULL == m_ptDoc)//第一次读取配置文件
        {
            m_ptDoc = new(std::nothrow) tinyxml2::XMLDocument();
            CHECK_OBJ(m_ptDoc);
        }
        else//重新读取
        {
            m_ptDoc->Clear();
        }
        
        tinyxml2::XMLError eErrorNo = m_ptDoc->LoadFile(sConfigFile.c_str());
        if (eErrorNo != tinyxml2::XML_NO_ERROR)
        {
            CHECK_RET(ERR_APP_CONFIG_FORMAT, "LoadXMLFile(name=%s) failed, XMLError = [%d].", sConfigFile.c_str(), eErrorNo);
        }
        tinyxml2::XMLElement* pRoot = m_ptDoc->FirstChildElement("mdbServer");
        if(NULL == pRoot)
        {
            CHECK_RET(ERR_APP_CONFIG_FORMAT, "MDBConfig node does not exist in the file[%s].", sConfigFile.c_str());
        }

        CHECK_RET(LoadSys(pRoot), "LoadRoot failed.");
        CHECK_RET(LoadHosts(pRoot), "LoadHosts failed.");
        CHECK_RET(LoadRules(pRoot), "LoadRules failed.");
        CHECK_RET(LoadGroups(pRoot), "LoadGroup failed.");
        CHECK_RET(LoadDisaster(pRoot), "LoadDisaster failed.");
        

        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  RereadConfig
    * 函数描述	: 重新读取配置文件
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbServerConfig::RereadConfig()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        Clear();       //清空已经赋值的变量
        iRet = ReadConfig();
        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * 函数名称	:  SyncLocalConfigFile
        * 函数描述	: 根据备机，同步配置文件（重写本地配置文件）
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbServerConfig::SyncLocalCfgFile(const char* pData)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //本地同步文件是否存在，如果存在，则先删除
        TMdbNtcStrFunc tStrFun;
        TMdbNtcString sConfigFile(MAX_PATH_NAME_LEN,"%s/%s", tStrFun.ReplaceEnv(MDB_SERVER_CONFIG_DIR), MDB_SERVER_CONFIG_NAME);//系统配置文件
        if(TMdbNtcDirOper::IsFileExist(sConfigFile.c_str())) 
        {
            TMdbNtcString sBakFile(MAX_PATH_NAME_LEN, "%s%s", sConfigFile.c_str(), "_bak");
            TMdbNtcFileOper::Remove(sBakFile.c_str());
            TMdbNtcFileOper::Rename(sConfigFile.c_str(), sBakFile.c_str());
            
            TADD_NORMAL("Rename the old file [%s] to [%s]", sConfigFile.c_str(), sBakFile.c_str());
        }

        //创建新的配置文件
        if (!TMdbNtcFileOper::MakeFile(sConfigFile.c_str()))
        {
            CHECK_RET(ERR_OS_CREATE_FILE, "Create new configuration file[%s] failed.", sConfigFile.c_str());
        }
        TADD_NORMAL("Create new [%s]", sConfigFile.c_str());
        //将备机配置文件内容复制到新文件中
        if (!TMdbNtcFileOper::WriteContent(sConfigFile.c_str(), pData))
        {
            CHECK_RET(ERR_OS_WRITE_FILE,"Write to configuration file[%s] failed.", sConfigFile.c_str());
        }
        //重新读取配置文件，并对文件中与备机不同的信息（主备机的host_ID）进行修改
        iRet = RereadConfig();
        if (iRet != ERROR_SUCCESS)
        {
            CHECK_RET(ERR_APP_CONFIG_FORMAT, "Read configuration file[%s] failed.", sConfigFile.c_str());
        }
        //主备机ID互换
        tinyxml2::XMLAttribute*pLocalID = NULL;
        tinyxml2::XMLAttribute*pRepID = NULL;
        tinyxml2::XMLElement* pESys = m_ptDoc->FirstChildElement("mdbServer")->FirstChildElement("sys");
        tinyxml2::XMLAttribute* pAttr      = NULL;
        tinyxml2::XMLAttribute* pAttrValue = NULL;
        for (tinyxml2::XMLElement* pSec=pESys->FirstChildElement("section"); pSec; pSec=pSec->NextSiblingElement("section"))
        {
            pAttr  = pSec->FirstAttribute();
            pAttrValue = pAttr->Next();
            if(pAttrValue != NULL)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0 
                    &&TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Name(), "host_ID") ==0 )
                {
                    if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(),"rep_server")==0)
                    {
                        pRepID = pAttrValue;
                    }
                    else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(),"Local_server")==0)
                    {
                        pLocalID = pAttrValue;
                    }
                }             
            }//end of if(pAttrValue != NULL)
        }//end of for

        //更改配置文件中的主备机信息
        int iTmpID = atoi(pLocalID->Value());
        pLocalID->SetAttribute(atoi(pRepID->Value()));
        m_iLoaclHostID = atoi(pLocalID->Value());
        pRepID->SetAttribute(iTmpID);
        m_iRepHostID = atoi(pRepID->Value());
        m_ptDoc->SaveFile(sConfigFile.c_str());

        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * 函数名称	:  GetConfigData
        * 函数描述	: 获取配置文件数据
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbServerConfig::GetConfigData(char* pDataBuf)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbNtcStrFunc tStrFun;
        TMdbNtcString sConfigFile(MAX_PATH_NAME_LEN,"%s/%s", tStrFun.ReplaceEnv(MDB_SERVER_CONFIG_DIR), MDB_SERVER_CONFIG_NAME);//系统配置文件
        if( !TMdbNtcDirOper::IsFileExist(sConfigFile.c_str())) 
        {
            CHECK_RET(ERR_OS_NO_FILE,"Configuration file[%s] not exist.",sConfigFile.c_str());
        }

        FILE* fp = fopen( sConfigFile.c_str(), "rb" );
        if ( !fp) 
        {
            CHECK_RET( ERR_OS_OPEN_FILE, "Open configuration file[%s] failed.", sConfigFile.c_str() );
        }

        fseek( fp, 0, SEEK_SET );
        fgetc( fp );
        if ( ferror( fp ) != 0 ) 
        {
            CHECK_RET( ERR_OS_READ_FILE, "Read configuration file[%s] failed.", sConfigFile.c_str() );
        }

        fseek( fp, 0, SEEK_END );
        size_t size = ftell( fp );
        fseek( fp, 0, SEEK_SET );

        if ( size == 0 ) 
        {
            CHECK_RET( ERR_OS_READ_FILE, "Configuration file[%s] is empty.", sConfigFile.c_str() );
        }

        size_t read = fread( pDataBuf, 1, size, fp );
        if ( read != size )
        {
            CHECK_RET( ERR_OS_READ_FILE, "Read configuration file[%s] failed.", sConfigFile.c_str() );
        }

        pDataBuf[size] = 0;
        fclose(fp);
        TADD_FUNC("Finish.");
        return iRet;
    }


        /*******************************************************************************
        * 函数名称	:  GetRoutingRequestData
        * 函数描述	: 处理路由请求信息, 格式：HostID, 回复格式： 格式 rID:hID1,hID2;rID2:hID3|rID:hID;rID2:hID|rID:rule;rID1:rule;rID2:rule2|hID:ip,port;hID2:ip,port
        * 输入		: 
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbServerConfig::GetRoutingRequestData(int iHostID, int iBufLen, char* pDataBuf)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pDataBuf);
        memset(pDataBuf, 0, iBufLen);
        int iGroupIndex = GetGroupIndex(iHostID);
        if (iGroupIndex < 0)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "Can not find a group including HostID[%d]", iHostID);
        }

       TMdbGroup *pGroup = &m_arGroup[iGroupIndex];
       std::map<int, std::vector<int> >::iterator  itorHostArray;
        int iRuleID = -1;
        std::vector<int> arRepHostID;//所有备机的ID
        std::vector<int> arRepRuleID;//所有的规则ID
        bool bFirst = true;

        for (unsigned int i = 0; i<pGroup->m_arRuleIDList.size(); i++)//拼接路由及其对应的备机
        {
             iRuleID = pGroup->m_arRuleIDList[i];
            itorHostArray = pGroup->m_tRoutingMap.find(iRuleID);//该路由ID对应的主机列表
            if (itorHostArray==pGroup->m_tRoutingMap.end())//路由ID不在任何主机上，（路由ID未使用）
            {
                continue;
            }

            if (TMdbRoutingTools::IsIDInArray(iHostID, itorHostArray->second))//路由在该主机上
            {
                arRepRuleID.push_back(iRuleID);
                bFirst = true;
                if (strlen(pDataBuf) == 0)
                {
                    snprintf(pDataBuf, iBufLen, "%d", iRuleID);
                }
                else
                {
                    snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ";%d", iRuleID);
                }
                for (unsigned int i = 0; i<itorHostArray->second.size(); i++)//获取路由对应的所有备机
                {
                    if (itorHostArray->second[i] != iHostID)//HostID不是本机，则为备机，拼接并保存
                    {
                        if (!TMdbRoutingTools::IsIDInArray(itorHostArray->second[i], arRepHostID))
                        {
                            arRepHostID.push_back(itorHostArray->second[i]);
                        }
                        if (bFirst)
                        {
                            snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ":%d", itorHostArray->second[i]);
                            bFirst = false;
                        }
                        else
                        {
                            snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ",%d", itorHostArray->second[i]);
                        }   
                    }
                }
            }
        }
        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "|");//分隔符
        TMdbDisasterHost *pHost;
        bFirst = true;
        
        for (unsigned int j =0; j<arRepRuleID.size(); j++)//拼接容灾机的信息
        {
            for (unsigned int i = 0; i<m_arRecoveryHost.size(); i++)
            {
                pHost = &m_arRecoveryHost[i];
                if (TMdbRoutingTools::IsIDInString(arRepRuleID[j], pHost->m_strRulelist.c_str()))
                {
                    if (bFirst)
                    {
                        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "%d:%d", arRepRuleID[j], pHost->m_iHostID);
						bFirst = false;
					}
                    else
                    {
                        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ";%d:%d", arRepRuleID[j], pHost->m_iHostID);
                    }  
                }
                arRepHostID.push_back(pHost->m_iHostID);
            }
        }
        //printf("###%s", pDataBuf);

        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "|");//分隔符
        std::map<int, TMdbRepRules>::iterator itorpRules;
        for (unsigned int i = 0; i<arRepRuleID.size(); i++)//拼接所有规则信息
        {
            itorpRules = m_AllRules.find(arRepRuleID[i]);
            
            if (i == 0)
            {
                snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "%d:%s", itorpRules->second.m_iRuleID, itorpRules->second.m_strRule.c_str());
            }
            else
            {
                snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ";%d:%s", itorpRules->second.m_iRuleID, itorpRules->second.m_strRule.c_str());
            }
        }

        //printf("###%s", pDataBuf);
        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "|");//分隔符
        std::map<int, TMdbRepHost>::iterator itorpHost;
        for (unsigned int i = 0; i<arRepHostID.size(); i++)//拼接所有备机信息
        {
            itorpHost = m_AllHosts.find(arRepHostID[i]);
            
            if (i == 0)
            {
                snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "%d:%s,%d", itorpHost->second.m_iHostID, itorpHost->second.m_strIP.c_str(), itorpHost->second.m_iPort);
            }
            else
            {
                snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ";%d:%s,%d", itorpHost->second.m_iHostID, itorpHost->second.m_strIP.c_str(), itorpHost->second.m_iPort);
            }
        }
        TADD_NORMAL("Routing request answer: [%s]", pDataBuf);

        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  UpdateRouterRep
    * 函数描述	: 更新配置文件路由信息
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbServerConfig::UpdateRouterRep()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetHostID
    * 函数描述	:  根据IP和端口号，获取主机ID
    * 输入		:  
    * 输出		:  
    * 返回值	:  主机在配置文件中的标识Host_ID
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbServerConfig::GetHostID(const char* pszIP, int iPort)
    {
        TADD_FUNC("Start.");

        int iHostID = -1;
        TMdbRepHost *pRepHost = NULL;
        std::map<int, TMdbRepHost>::iterator itor = m_AllHosts.begin();
        for (; itor!=m_AllHosts.end(); ++itor)//遍历查找
        {
            pRepHost = &itor->second;
            if (TMdbNtcStrFunc::StrNoCaseCmp(pszIP, pRepHost->m_strIP.c_str()) == 0 && iPort == pRepHost->m_iPort)//比较IP和端口号
            {
                iHostID = pRepHost->m_iHostID;
                break;
            }
        }

        TADD_FUNC("Finish.");
        return iHostID;
    }

            /******************************************************************************
        * 函数名称	:  GetPort
        * 函数描述	:  获取配置服务端口号
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        TMdbRepHost* TMdbServerConfig::GetLocalHost()
        {
            return &m_AllHosts.find(m_iLoaclHostID)->second;
        }
        /******************************************************************************
        * 函数名称	:  GetRepServerIP
        * 函数描述	:  获取配置服务备机的IP
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    TMdbRepHost* TMdbServerConfig::GetRepServer()
    {
        if (m_AllHosts.find(m_iRepHostID)!=m_AllHosts.end())
        {
            return &m_AllHosts.find(m_iRepHostID)->second;
        }
        else//不存在备机
        {
            return NULL;
        }
        
    }

        /******************************************************************************
        * 函数名称	:  GetRepServerIP
        * 函数描述	:  获取配置服务备机的IP
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbServerConfig::GetGroupIndex(int iHostID)
    {
        int iIndex = -1;
        TMdbGroup *pGroup;
        for (unsigned int i = 0; i< m_arGroup.size(); i++)
        {
            pGroup = &m_arGroup[i];
            if (TMdbRoutingTools::IsIDInArray(iHostID, pGroup->m_arHostIDList))
            {
                iIndex = i;
                break;
            }
        }
        return iIndex;
    }
    int TMdbServerConfig::LoadHosts(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        tinyxml2::XMLElement* pESec = NULL;
        const tinyxml2::XMLAttribute* pAttr = NULL;
        tinyxml2::XMLElement* pEHosts=pRoot->FirstChildElement("hosts");
        CHECK_OBJ(pEHosts);

        for (pESec = pEHosts->FirstChildElement("host"); pESec!=NULL; pESec = pESec->NextSiblingElement("host"))
        {
            TMdbRepHost tHost;
            for(pAttr=pESec->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ID") == 0)
                {
                    tHost.m_iHostID = atoi(pAttr->Value());
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ip") == 0)
                {
                    tHost.m_strIP = pAttr->Value();
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "port") == 0)
                {
                    tHost.m_iPort = atoi(pAttr->Value());
                }

            }

            //检查路由ID是否重复
            if (m_AllHosts.find(tHost.m_iHostID)!=m_AllHosts.end())
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "One more hosts identified by ID[%d].", tHost.m_iHostID);             
            }
            //检查主机是否重复
            if (!CheckHostInfo(tHost.m_strIP.c_str(), tHost.m_iPort))
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "One more hosts identified by IP[%s]:Port[%d].", tHost.m_strIP.c_str(), tHost.m_iHostID);             
            }
            m_AllHosts.insert(std::pair<int, TMdbRepHost>(tHost.m_iHostID, tHost));
        }

        /*printf("m_AllHosts: ");
        std::map<int, TMdbRepHost>::iterator itor = m_AllHosts.begin();
        for (; itor!=m_AllHosts.end(); ++itor)
        {
            printf("%d %d:%s;   ", itor->first, itor->second.m_iPort, itor->second.m_strIP.c_str());
        }
        printf("\n");*/

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbServerConfig::LoadRules(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        tinyxml2::XMLElement* pESec = NULL;
        const tinyxml2::XMLAttribute* pAttr = NULL;

        tinyxml2::XMLElement* pERule=pRoot->FirstChildElement("rules");
        CHECK_OBJ(pERule);
        for (pESec = pERule->FirstChildElement("rule"); pESec!=NULL; pESec = pESec->NextSiblingElement("rule"))
        {
            TMdbRepRules tRule;
            for(pAttr=pESec->FirstAttribute(); pAttr!=NULL; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ID") == 0)
                {
                    tRule.m_iRuleID = atoi(pAttr->Value());
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
                {
                    tRule.m_strRule = pAttr->Value();
                }
            }
            
            //检查路由ID是否重复
            if (m_AllRules.find(tRule.m_iRuleID)!=m_AllRules.end())
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "One more rules identified by ID[%d].", tRule.m_iRuleID);             
            }
            //检查规则配置是否合法
            CHECK_RET(CheckRoutingRule(tRule.m_strRule.c_str()), "Routing rules is invalid.");
            //保存规则
            m_AllRules.insert(std::pair<int, TMdbRepRules>(tRule.m_iRuleID, tRule));
        }

        //printf("m_AllRules: ");
        //std::map<int, TMdbRepRules>::iterator itor = m_AllRules.begin();
        //for (; itor!=m_AllRules.end(); ++itor)
        //{
        //    printf("ID:%d  rules: %s;", itor->first, itor->second.m_strRule.c_str());
        //}
        //printf("\n");
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbServerConfig::LoadGroups(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        tinyxml2::XMLElement* pESec = NULL;
        const tinyxml2::XMLAttribute* pAttr = NULL;

        for (tinyxml2::XMLElement* pEGroup=pRoot->FirstChildElement("group"); pEGroup; pEGroup=pEGroup->NextSiblingElement("group"))
        {
            TMdbGroup tGroup;
            //规则
            pESec = pEGroup->FirstChildElement("rule_ID");
            CHECK_OBJ(pESec);
            pAttr = pESec->FirstAttribute();
            if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "list") == 0)
            {
                TMdbRoutingTools::GetIntArrayFromStrList(pAttr->Value(), tGroup.m_arRuleIDList);
            }
            if (!CheckRoutingID(tGroup.m_arRuleIDList))//检查路由ID列表是否合法
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Rule ID list \"%s\" contains invalid rule ID", pAttr->Value());
            }

            //主机
            pESec = pEGroup->FirstChildElement("host_ID");
            CHECK_OBJ(pESec);
            pAttr = pESec->FirstAttribute();
            if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "list") == 0)
            {
                TMdbRoutingTools::GetIntArrayFromStrList(pAttr->Value(), tGroup.m_arHostIDList);
            }

            if (!CheckHostID(tGroup.m_arHostIDList))//检查主机ID列表是否合法
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Host ID list \"%s\" contains invalid host ID, not exist or included in other group.", pAttr->Value());
            }

            //映射
            pESec = pEGroup->FirstChildElement("routing_rep");
            CHECK_OBJ(pESec);
            const tinyxml2::XMLElement *pRouting = NULL;
            int iRuleID;
            TMdbNtcSplit tSplit;
            for (pRouting = pESec->FirstChildElement("routing"); pRouting !=NULL; pRouting = pRouting->NextSiblingElement("routing"))
            {
                std::vector<int> tHostIDArray;
                for(pAttr=pRouting->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "rule_ID") == 0)
                    {
                        iRuleID = atoi(pAttr->Value());
                    }
                    else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "host_ID_list") == 0)
                    {
                        TMdbRoutingTools::GetIntArrayFromStrList(pAttr->Value(), tHostIDArray);
                    }
                }

                if (!CheckRoutingRep(iRuleID, tHostIDArray, tGroup))//检查映射中的路由ID和主机ID是否在该组的路由和主机列表中
                {
                    CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Invalid rule ID or host ID list.");
                }

                tGroup.m_tRoutingMap.insert(std::pair<int, std::vector<int> >(iRuleID, tHostIDArray));
            }
            m_arGroup.push_back(tGroup);
        }

        //printf("m_arGroup: ");
        //std::vector<TMdbGroup>::iterator itor = m_arGroup.begin();
        //for (; itor!=m_arGroup.end(); ++itor)
        //{
        //    printf("hostlist:");
        //    for (int i=0; i<itor->m_arHostIDList.size();i++)
        //    {
        //        printf("%d,", itor->m_arHostIDList[i]);
        //    }
        //    printf("\n");

        //    printf("rulelist:");
        //    for (int i=0; i<itor->m_arRuleIDList.size();i++)
        //    {
        //        printf("%d,", itor->m_arRuleIDList[i]);
        //    }
        //    printf("\n");

        //    printf("rulemap: ");
        //    std::map<int, std::vector<int> >::iterator itormap = itor->m_tRoutingMap.begin();
        //    for (; itormap!= itor->m_tRoutingMap.end();  itormap++)
        //    {
        //        printf("%d:",  itormap->first);
        //        for (int i = 0; i< itormap->second.size();i++)
        //        {
        //            printf("%d,", itormap->second[i]);
        //        }
        //    }
        //    printf("\n");

        //}
        //printf("\n");

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbServerConfig::LoadDisaster(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
		tinyxml2::XMLElement* pERule = NULL;
        tinyxml2::XMLElement* pESec = NULL;
        const tinyxml2::XMLAttribute* pAttr = NULL;

        for (pERule=pRoot->FirstChildElement("disaster_recovery"); pERule; pERule=pERule->NextSiblingElement("disaster_recovery"))
        {        
        	for (pESec=pERule->FirstChildElement("host"); pESec; pESec=pESec->NextSiblingElement("host"))
        	{
				TMdbDisasterHost tHost;	            
	            for(pAttr=pESec->FirstAttribute(); pAttr; pAttr=pAttr->Next())
	            {
	                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ID") == 0)
	                {
	                    tHost.m_iHostID= atoi(pAttr->Value());
	                }
	                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "rule_ID_list") == 0)
	                {
	                    tHost.m_strRulelist= pAttr->Value();
	                }
	            }
				
				//检查主机ID是否合法
				if(m_AllHosts.find(tHost.m_iHostID) == m_AllHosts.end())
	            {
	                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "LoadDisaster,Invalid host ID [%d]", tHost.m_iHostID);
	            }

				//检查主机ID是否重复
				for (size_t i = 0; i<m_arRecoveryHost.size(); i++)
		        {
		            if (tHost.m_iHostID == m_arRecoveryHost[i].m_iHostID)
		            {
						CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "LoadDisaster,One more hosts identified by ID[%d].", tHost.m_iHostID); 			
		            }
		        }
				
				//检查路由ID列表是否合法
				std::vector<int> tmpArRuleIDList;
				TMdbRoutingTools::GetIntArrayFromStrList(tHost.m_strRulelist.c_str(), tmpArRuleIDList);
				if (!CheckRoutingID(tmpArRuleIDList))
	            {
	                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "LoadDisaster,Rule ID list \"%s\" contains invalid rule ID", pAttr->Value());
	            }
				
				m_arRecoveryHost.push_back(tHost);
        	}
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbServerConfig::LoadSys(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        tinyxml2::XMLElement* pESec = NULL;
        const tinyxml2::XMLAttribute* pAttr = NULL;
        tinyxml2::XMLElement *pESys = pRoot->FirstChildElement("sys");
        CHECK_OBJ(pESys);
        pESec = pESys->FirstChildElement("section");
        while(pESec!=NULL)
        {
                pAttr = pESec->FirstAttribute();
                if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"name")==0 && TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(),"rep_server") == 0 )
                {
                    pAttr = pAttr->Next();
                    if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "host_ID")==0)
                    {
                        m_iRepHostID = atoi(pAttr->Value());
                    }
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"name")==0 && TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(),"Local_server") == 0 )
                {
                    pAttr = pAttr->Next();
                    if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "host_ID")==0)
                    {
                        m_iLoaclHostID = atoi(pAttr->Value());
                    }
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"name")==0 && TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(),"HeartBeatWarning") == 0 )
                {
                    pAttr = pAttr->Next();
                    if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value")==0)
                    {
                        m_iHeartBeatWarning = atoi(pAttr->Value());
                    }
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"name")==0 && TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(),"HeartBeatFatal") == 0 )
                {
                    pAttr = pAttr->Next();
                    if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value")==0)
                    {
                        m_iHeartBeatFatal = atoi(pAttr->Value());
                    }
                }
            pESec = pESec->NextSiblingElement("section");
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    void TMdbServerConfig::Clear()
    {
        TADD_FUNC("Start.");
       
        m_arRecoveryHost.clear();
        m_arGroup.clear();
        m_AllHosts.clear();
        m_AllRules.clear();

        m_iHeartBeatFatal = 30;
        m_iHeartBeatWarning = 10;
        m_iRepHostID = MDB_REP_EMPTY_HOST_ID;
        m_iLoaclHostID = MDB_REP_EMPTY_HOST_ID;

        TADD_FUNC("Finish.");
    }

   /******************************************************************************
    * 函数名称	:  CheckRoutingRule
    * 函数描述	:  检查路由规则配置是否合法
    * 输入		:  
    * 输出		:  
    * 返回值	:  无
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbServerConfig::CheckRoutingRule(const char* sRules)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        static std::vector<TMdbRoutingCheck> tRoutingCheck;
        tRoutingCheck.clear();
        TMdbRoutingCheck tRouting;
        //校验路由分配是否合法，参考详设3.3.1
        if (sRules[0] != '[')//列表格式
        {
            TMdbNtcSplit tSplit;
            tSplit.SplitString(sRules, ',');
            int iTmp = 0;
            for (unsigned int i = 0; i<tSplit.GetFieldCount(); i++)
            {
                iTmp = atoi(tSplit[i]);
                if (CheckRange(iTmp, iTmp, tRoutingCheck))
                {
                    tRouting.iStart = iTmp;
                    tRouting.iEnd = iTmp;
                    tRoutingCheck.push_back(tRouting);
                }
                else
                {
                    CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Routing rule \"%s\" contains routing_id[%d] in another rule", sRules, iTmp);
                }
            }
        }
        else
        {
            std::string strRule(sRules, 1,strlen(sRules)-2);
            TMdbNtcSplit tSplit;
            tSplit.SplitString(strRule.c_str(), ',');
            if (tSplit.GetFieldCount() !=2)
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Invalid routing rule format.");
            }
            int iStart = atoi(tSplit[0]);
            int iEnd = atoi((tSplit[1]));
            if (CheckRange(iStart, iEnd, tRoutingCheck))
            {
                tRouting.iStart = iStart;
                tRouting.iEnd = iEnd;
                tRoutingCheck.push_back(tRouting);
            }
            else
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Routing rule [%s] contains routing_id  in another routing rule", sRules);
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  CheckRange
    * 函数描述	:  检查新增路由范围与已经配置的是否存在交叉
    * 输入		:  
    * 输出		:  
    * 返回值	:  无
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbServerConfig::CheckRange(int iStart, int iEnd, std::vector<TMdbRoutingCheck> &tRoutingCheck)
    {
        for (unsigned int i = 0; i<tRoutingCheck.size(); i++)
        {
            if ((iStart<tRoutingCheck[i].iStart&& iEnd<tRoutingCheck[i].iEnd)
                || (iStart>tRoutingCheck[i].iStart && iEnd>tRoutingCheck[i].iEnd))//范围不交叉
            {
                continue;
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    /******************************************************************************
    * 函数名称	:  CheckHostID
    * 函数描述	:  检查每个分组中的主机ID是否合法
    * 输入		:  
    * 输出		:  
    * 返回值	:  无
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbServerConfig::CheckHostID(std::vector<int>& vHostID)
    {
        
        for (unsigned int i = 0; i<vHostID.size(); i++)//检查主机ID是否存在
        {
            if(m_AllHosts.find(vHostID[i]) == m_AllHosts.end())
            {
                TADD_ERROR(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Invalid host ID [%d]", vHostID[i]);
                return false;
            }
            if (m_arGroup.size()>0)//检查主机ID是否已经在其他分组中
            {
                for (unsigned int k = 0; k<m_arGroup.size(); k++)
                {
                    for (unsigned int j = 0; j<m_arGroup[k].m_arHostIDList.size(); j++)
                    {
                        if (vHostID[i] == m_arGroup[k].m_arHostIDList[j])
                        {
                            return false;
                        }
                    }
                }
            }
        }
       
        return true;
    }

    /******************************************************************************
    * 函数名称	:  CheckRoutingID
    * 函数描述	:  检查每个分组中的路由ID是否合法
    * 输入		:  
    * 输出		:  
    * 返回值	:  无
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbServerConfig::CheckRoutingID(std::vector<int>& vRoutingID)
    {
        for (unsigned int i = 0; i<vRoutingID.size(); i++)
        {
            if(m_AllRules.find(vRoutingID[i]) == m_AllRules.end())
            {
                TADD_ERROR(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Invalid rule ID [%d]", vRoutingID[i]);
                return false;
            }
        }
        return true;
    }

    /******************************************************************************
    * 函数名称	:  CheckRoutingRep
    * 函数描述	:  检查每个分组中的路由分布是否合法
    * 输入		:  
    * 输出		:  
    * 返回值	:  无
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbServerConfig::CheckRoutingRep(int iRuleID, std::vector<int>&vHostID, TMdbGroup &tGroup)
    {
        unsigned int i = 0;
        //校验路由规则ID是否在该组的路由规则列表中
        for ( ; i<tGroup.m_arRuleIDList.size(); i++)
        {
            if (iRuleID == tGroup.m_arRuleIDList[i])
            {
                break;
            }
        }
        if(i == tGroup.m_arRuleIDList.size())//路由规则ID不在本组路由规则列表中
        {
            TADD_ERROR(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Invalid rule ID [%d]", iRuleID);
            return false;
        }

        //校验主机ID是否在该组的主机列表中
        unsigned int j = 0;
        for (j = 0; j < vHostID.size(); j++)
        {
            for (i=0; i < tGroup.m_arHostIDList.size(); i++)
            {
                if (vHostID[j] == tGroup.m_arHostIDList[i])
                {
                    break;
                }
            }
            if (i == tGroup.m_arHostIDList.size())//主机ID不在本组主机列表中
            {
                TADD_ERROR(ERR_APP_CONFIG_ITEM_VALUE_INVALID, "Invalid host ID[%d]", vHostID[j]);
                return false;
            }
        }

        return true;
    }

    /******************************************************************************
    * 函数名称	:  CheckHostInfo
    * 函数描述	:  检查主机信息是否已经存在
    * 输入		:  
    * 输出		:  
    * 返回值	:  无
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbServerConfig::CheckHostInfo(const char* sIP, int iPort)
    {
        bool bRet = true;
        for (unsigned int i = 0; i<m_AllHosts.size(); i++)
        {
        }
        std::map<int, TMdbRepHost>::iterator itorpHost = m_AllHosts.begin();
        for (; itorpHost != m_AllHosts.end(); ++itorpHost)
        {
            if (TMdbNtcStrFunc::StrNoCaseCmp(sIP, itorpHost->second.m_strIP.c_str()) == 0 && iPort == itorpHost->second.m_iPort)
            {
                return false;
            }
        }

        return bRet;
    }


    TShbRepLocalConfig::TShbRepLocalConfig()
    {
        m_mRoutingMap.clear();
        m_vRecoveryHost.clear();
        m_mRuleMap.clear();
        m_mHostMap.clear();
    }

    TShbRepLocalConfig::~TShbRepLocalConfig()
    {
        Clear();
    }

    void TShbRepLocalConfig::Clear()
    {

        m_mRoutingMap.clear();

        std::vector<TMdbDisasterHost*>::iterator itorRecV = m_vRecoveryHost.begin();
        for(; itorRecV != m_vRecoveryHost.end(); ++itorRecV)
        {
            TMdbDisasterHost* pRecvHost = *itorRecV;
            SAFE_DELETE(pRecvHost);
        }
        m_vRecoveryHost.clear();

        std::map<int ,TMdbRepRules*>::iterator itorRuleMap = m_mRuleMap.begin();
        for(; itorRuleMap != m_mRuleMap.end(); ++itorRuleMap)
        {
            TMdbRepRules* pRule = itorRuleMap->second;
            SAFE_DELETE(pRule);
        }
        m_mRuleMap.clear();

        std::map<int ,TMdbRepHost*>::iterator itorHost = m_mHostMap.begin();
        for(; itorHost != m_mHostMap.end(); ++itorHost)
        {
            TMdbRepHost* pHost = itorHost->second;
            SAFE_DELETE(pHost);
        }
        m_mHostMap.clear();
        
    }

    int TShbRepLocalConfig::ReadLocalConfig(const char* psDsn,char* pConfigBuf, int iBufLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(psDsn);
        CHECK_OBJ(pConfigBuf);

        char sDsn[MAX_NAME_LEN] ={0};
        SAFESTRCPY(sDsn, sizeof(sDsn), psDsn);
        TMdbNtcStrFunc::ToUpper(sDsn);

        TMdbNtcStrFunc tStrFun;
        char sConfigFile[MAX_FILE_NAME] = {0};
        snprintf(sConfigFile, sizeof(sConfigFile), "%s/.%s/.%s", tStrFun.ReplaceEnv(MDB_LOCAL_CONFIG_DIR), sDsn, MDB_LOCAL_CONFIG_FILE);

        if(!TMdbNtcFileOper::IsExist(sConfigFile))//配置文件不存在
        {
            CHECK_RET(ERR_APP_CONFIG_NOT_EXIST, "Local config file[%s] not exist", sConfigFile);
        }
        Clear();

        //读取配置文件
        tinyxml2::XMLDocument* ptDoc;
        ptDoc = new(std::nothrow) tinyxml2::XMLDocument();
        CHECK_OBJ(ptDoc);

        tinyxml2::XMLError eErrorNo = ptDoc->LoadFile(sConfigFile);
        if (eErrorNo != tinyxml2::XML_NO_ERROR)
        {
            CHECK_RET(ERR_APP_CONFIG_FORMAT, "LoadXMLFile(name=%s) failed, XMLError = [%d].", sConfigFile, eErrorNo);
        }
        tinyxml2::XMLElement* pRoot = ptDoc->FirstChildElement("MDBConfig");
        if(NULL == pRoot)
        {
            CHECK_RET(ERR_APP_CONFIG_FORMAT, "MDBConfig node does not exist in the file[%s].", sConfigFile);
        }

        CHECK_RET(LoadSys(pRoot), "LoadSys failed.");
        CHECK_RET(LoadHosts(pRoot), "LoadHosts failed.");
        CHECK_RET(LoadRules(pRoot), "LoadRules failed.");
        CHECK_RET(LoadStandBy(pRoot), "LoadGroup failed.");
        CHECK_RET(LoadDisaster(pRoot), "LoadDisaster failed.");

        // 生成配置文件信息串
        CHECK_RET(GenConfigStr(pConfigBuf, iBufLen),"Gen config string failed.");
        
        return iRet;
    }

    int TShbRepLocalConfig::GenConfigStr(char* pDataBuf, int iBufLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pDataBuf);
        memset(pDataBuf, 0, iBufLen);

        //std::vector<int> arRepHostID;//所有备机的ID
        std::vector<int> arRepRuleID;//所有的规则ID

        // 路由与备机映射关系
        bool bFirstRule = true;
        bool bFirstHost = true;
        std::vector<int>::iterator itorHost;
        std::map<int, std::vector<int> >::iterator itorRoutMap;
        std::map<int ,TMdbRepRules*>::iterator itorRule = m_mRuleMap.begin();
        for(; itorRule != m_mRuleMap.end(); ++itorRule)
        {
            itorRoutMap = m_mRoutingMap.find(itorRule->first);
            if(bFirstRule)
            {
                snprintf(pDataBuf, iBufLen, "%d", itorRoutMap->first);
                bFirstRule = false;
            }
            else
            {
                 snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ";%d", itorRoutMap->first);
            }

            arRepRuleID.push_back(itorRoutMap->first);

            bFirstHost = true;
            itorHost = itorRoutMap->second.begin();
            for(;itorHost != itorRoutMap->second.end(); ++itorHost)
            {
                if(bFirstHost)
                {
                    snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ":%d", (*itorHost));
                    bFirstHost = false;
                }
                else
                {
                     snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ",%d", (*itorHost));
                }
                //arRepHostID.push_back((*itorHost));
            }
        }

        TADD_DETAIL("after get ruleid and host ,DataBuff=[%s]", pDataBuf);

        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "|");//分隔符

        // 路由与容灾机映射关系
        TMdbDisasterHost *pHost = NULL;
        bool bFirstDisaster = true;
        for (unsigned int j =0; j<arRepRuleID.size(); j++)
        {
            for (unsigned int i = 0; i<m_vRecoveryHost.size(); i++)
            {
                pHost = m_vRecoveryHost[i];
                if (TMdbRoutingTools::IsIDInString(arRepRuleID[j], pHost->m_strRulelist.c_str()))
                {
                    if (bFirstDisaster)
                    {
                        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "%d:%d", arRepRuleID[j], pHost->m_iHostID);
                    }
                    else
                    {
                        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ";%d:%d", arRepRuleID[j], pHost->m_iHostID);
                    }  
                }
                //arRepHostID.push_back(pHost->m_iHostID);
            }
        }
        TADD_DETAIL("after get ruleid and disaster-host ,DataBuff=[%s]", pDataBuf);
        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "|");//分隔符

        // 路由规则ID 与路由ID 的映射关系
        std::map<int, TMdbRepRules*>::iterator itorpRules = m_mRuleMap.begin();
        for (; itorpRules!=m_mRuleMap.end(); itorpRules++)
        {
            if (itorpRules == m_mRuleMap.begin())
            {
                snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "%d:%s", itorpRules->second->m_iRuleID, itorpRules->second->m_strRule.c_str());
            }
            else
            {
                snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ";%d:%s", itorpRules->second->m_iRuleID, itorpRules->second->m_strRule.c_str());
            }
        }

        TADD_DETAIL("after get ruleid and routing-id ,DataBuff=[%s]", pDataBuf);
        snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "|");//分隔符

        // 主机ID 与主机的ip和port 映射关系
        std::map<int, TMdbRepHost*>::iterator itorpHost = m_mHostMap.begin();
        for (; itorpHost != m_mHostMap.end(); itorpHost++)
        {
            if (itorpHost == m_mHostMap.begin())
            {
                snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), "%d:%s,%d", itorpHost->second->m_iHostID, itorpHost->second->m_strIP.c_str(), itorpHost->second->m_iPort);
            }
            else
            {
                snprintf(pDataBuf+strlen(pDataBuf), iBufLen-strlen(pDataBuf), ";%d:%s,%d", itorpHost->second->m_iHostID, itorpHost->second->m_strIP.c_str(), itorpHost->second->m_iPort);
            }
        }
       
        TADD_NORMAL("%s", pDataBuf);

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TShbRepLocalConfig::LoadSys(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        tinyxml2::XMLElement* pESec = NULL;
        const tinyxml2::XMLAttribute* pAttr = NULL;
        tinyxml2::XMLElement* pESys=pRoot->FirstChildElement("Sys");
        CHECK_OBJ(pESys);

        pESec = pESys->FirstChildElement("Host");
        CHECK_OBJ(pESec);
        pAttr = pESec->FirstAttribute();
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ID") == 0)
        {
            m_iHostID = atoi(pAttr->Value());
        }

        pESec = pESys->FirstChildElement("Heartbeat");
        CHECK_OBJ(pESec);
        pAttr = pESec->FirstAttribute();
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "Warning") == 0)
        {
            m_iHeartbeat = atoi(pAttr->Value());
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TShbRepLocalConfig::LoadHosts(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        tinyxml2::XMLElement* pESec = NULL;
        const tinyxml2::XMLAttribute* pAttr = NULL;
        tinyxml2::XMLElement* pEHosts=pRoot->FirstChildElement("hosts");
        CHECK_OBJ(pEHosts);

        for (pESec = pEHosts->FirstChildElement("host"); pESec!=NULL; pESec = pESec->NextSiblingElement("host"))
        {
            TMdbRepHost *ptHost = new(std::nothrow) TMdbRepHost();
			CHECK_OBJ(ptHost);
            for(pAttr=pESec->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ID") == 0)
                {
                    ptHost->m_iHostID = atoi(pAttr->Value());
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ip") == 0)
                {
                    ptHost->m_strIP = pAttr->Value();
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "port") == 0)
                {
                    ptHost->m_iPort = atoi(pAttr->Value());
                }

            }
            m_mHostMap.insert(std::pair<int, TMdbRepHost*>(ptHost->m_iHostID, ptHost));
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TShbRepLocalConfig::LoadRules(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        tinyxml2::XMLElement* pESec = NULL;
        const tinyxml2::XMLAttribute* pAttr = NULL;

        tinyxml2::XMLElement* pERule=pRoot->FirstChildElement("rules");
        CHECK_OBJ(pERule);
        for (pESec = pERule->FirstChildElement("rule"); pESec!=NULL; pESec = pESec->NextSiblingElement("rule"))
        {
            TMdbRepRules* ptRule = new(std::nothrow) TMdbRepRules();
			CHECK_OBJ(ptRule);
            for(pAttr=pESec->FirstAttribute(); pAttr!=NULL; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ID") == 0)
                {
                    ptRule->m_iRuleID = atoi(pAttr->Value());
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
                {
                    ptRule->m_strRule = pAttr->Value();
                }

                m_mRuleMap.insert(std::pair<int, TMdbRepRules*>(ptRule->m_iRuleID, ptRule));
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TShbRepLocalConfig::LoadDisaster(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        tinyxml2::XMLElement* pESec = NULL;
        const tinyxml2::XMLAttribute* pAttr = NULL;

        for (tinyxml2::XMLElement* pERule=pRoot->FirstChildElement("disaster_recovery"); pERule; pERule=pERule->NextSiblingElement("disaster_recovery"))
        {
            TMdbDisasterHost *ptHost = new(std::nothrow) TMdbDisasterHost();
			CHECK_OBJ(ptHost);
            pESec = pERule->FirstChildElement("host");
            if(pESec == NULL) break; // 容灾机可能没有

            for(pAttr=pESec->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ID") == 0)
                {
                    ptHost->m_iHostID= atoi(pAttr->Value());
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "rule_ID_list") == 0)
                {
                    ptHost->m_strRulelist= pAttr->Value();
                }

                m_vRecoveryHost.push_back(ptHost);
            }
        }


        TADD_FUNC("Finish.");
        return iRet;
    }

    int TShbRepLocalConfig::LoadStandBy(tinyxml2::XMLElement* pRoot)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        const tinyxml2::XMLAttribute* pAttr = NULL;

        for (tinyxml2::XMLElement* pESec=pRoot->FirstChildElement("routing_rep"); pESec; pESec=pESec->NextSiblingElement("routing_rep"))
        {
            
            CHECK_OBJ(pESec);
            const tinyxml2::XMLElement *pRouting = NULL;
            int iRuleID;
            TMdbNtcSplit tSplit;
            for (pRouting = pESec->FirstChildElement("routing"); pRouting !=NULL; pRouting = pRouting->NextSiblingElement("routing"))
            {
                std::vector<int> tHostIDArray;
                for(pAttr=pRouting->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "rule_ID") == 0)
                    {
                        iRuleID = atoi(pAttr->Value());
                    }
                    else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "host_ID_list") == 0)
                    {
                        TMdbRoutingTools::GetIntArrayFromStrList(pAttr->Value(), tHostIDArray);
                    }
                }
                m_mRoutingMap.insert(std::pair<int, std::vector<int> >(iRuleID, tHostIDArray));
            }
            
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TShbRepLocalConfig::AnalyRepInfo(const char* pRepInfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        TMdbNtcSplit tSplitAll;
        tSplitAll.SplitString(pRepInfo, '|');
        if (tSplitAll.GetFieldCount()!=6)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "Invalid routing information format. ");
        }

        TMdbNtcSplit tSplitSemi;
        TMdbNtcSplit tSplitColon;
        TMdbNtcSplit tSplitComma;
        
        // 路由与备机
        tSplitSemi.SplitString(tSplitAll[0], ';');
        for (unsigned int i = 0; i<tSplitSemi.GetFieldCount(); i++)
        {
            std::vector<int> vHost;
            tSplitColon.SplitString(tSplitSemi[i], ':');
            if (tSplitColon.GetFieldCount()==2)//路由存在备机
            {
                CHECK_RET(TMdbRoutingTools::GetIDArrayFromRule(tSplitColon[1], vHost), "GetIDArrayFromRule failed.");
                m_mRoutingMap.insert(std::pair< int, std::vector<int> >(atoi(tSplitColon[0]), vHost));
            }
        }

        // 路由与容灾机
        tSplitSemi.SplitString(tSplitAll[1], ';');        
        int iHostId = 0;
        std::vector<TMdbDisasterHost*>::iterator itor;
        for (unsigned int i = 0; i<tSplitSemi.GetFieldCount(); i++)
        {   
            tSplitColon.SplitString(tSplitSemi[i], ':');
            iHostId = atoi(tSplitColon[1]); 
            itor = m_vRecoveryHost.begin();
            for(; itor != m_vRecoveryHost.end(); ++itor)
            {
                if((*itor)->m_iHostID == iHostId)
                {
                    if((*itor)->m_strRulelist.size() <=0)
                    {
                        (*itor)->m_strRulelist += tSplitColon[0];
                    }
                    else
                    {
                        (*itor)->m_strRulelist += ",";
                        (*itor)->m_strRulelist += tSplitColon[0];
                    }
                    break;
                }
            }

            if(itor == m_vRecoveryHost.end() )
            {
                TMdbDisasterHost* pDisasterHost = new(std::nothrow)  TMdbDisasterHost();
                pDisasterHost->m_iHostID = atoi(tSplitColon[1]);
                pDisasterHost->m_strRulelist += tSplitColon[0];
                m_vRecoveryHost.push_back(pDisasterHost);
            }
            
        }

        //处理路由规则ID
        tSplitSemi.SplitString(tSplitAll[2], ';');
        for (unsigned int i = 0; i<tSplitSemi.GetFieldCount();i++)
        {
            //std::vector<int> tIDArray;
            tSplitColon.SplitString(tSplitSemi[i], ':');
            TMdbRepRules* pRule = new TMdbRepRules();
            pRule->m_iRuleID = atoi(tSplitColon[0]);
            pRule->m_strRule = tSplitColon[1];
            //CHECK_RET(TMdbRoutingTools::GetIDArrayFromRule(tSplitColon[1], tIDArray), "GetIDArrayFromRule failed.");
            m_mRuleMap.insert(std::pair< int, TMdbRepRules* >(pRule->m_iRuleID, pRule));
        }
        
        //处理主机ID
        tSplitSemi.SplitString(tSplitAll[3], ';');
        for (unsigned int i = 0; i<tSplitSemi.GetFieldCount(); i++)
        {
            tSplitColon.SplitString(tSplitSemi[i], ':');
            tSplitComma.SplitString(tSplitColon[1], ',');
            TMdbRepHost *ptHost = new(std::nothrow) TMdbRepHost();
            
            ptHost->m_iHostID = atoi(tSplitColon[0]);
            ptHost->m_strIP= tSplitComma[0];
            ptHost->m_iPort = atoi(tSplitComma[1]);
            m_mHostMap.insert(std::pair<int, TMdbRepHost*>(ptHost->m_iHostID, ptHost));
        }

        m_iHostID = atoi(tSplitAll[4]);
        m_iHeartbeat = atoi(tSplitAll[5]);
        
        return iRet;
    }


    int TShbRepLocalConfig::SyncLocalConfig(const char* psDsn,const char* pRepInfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        CHECK_OBJ(psDsn);
        CHECK_OBJ(pRepInfo);

        char sDsn[MAX_NAME_LEN] ={0};
        SAFESTRCPY(sDsn, sizeof(sDsn), psDsn);
        TMdbNtcStrFunc::ToUpper(sDsn);

        TMdbNtcStrFunc tStrFun;
        char sConfigFile[MAX_FILE_NAME] = {0};
        snprintf(sConfigFile, sizeof(sConfigFile), "%s/.%s/.%s", tStrFun.ReplaceEnv(MDB_LOCAL_CONFIG_DIR), sDsn, MDB_LOCAL_CONFIG_FILE);

        //如果存在则先备份，然后重新生成
        if(TMdbNtcFileOper::IsExist(sConfigFile))
        {
            char sBakPath[MAX_PATH_NAME_LEN] ={0};
            snprintf(sBakPath, sizeof(sBakPath), "%s/.%s/.BAK/", tStrFun.ReplaceEnv(MDB_LOCAL_CONFIG_DIR), sDsn);
            char sCurTime[MAX_TIME_LEN] = {0};
            char sBakFile[MAX_PATH_NAME_LEN] = {0};
            char sTmpFile[MAX_PATH_NAME_LEN] = {0};
            TMdbDateTime::GetCurrentTimeStr(sCurTime);
            
            if(!TMdbNtcDirOper::IsExist(sBakPath))
            {
                if(!TMdbNtcDirOper::MakeFullDir(sBakPath))
                {
                    CHECK_RET(ERR_OS_CREATE_DIR, "create path[%s] failed.", sBakPath);
                }
            }
            snprintf(sBakFile,sizeof(sBakFile),"%s/%s",sBakPath, MDB_LOCAL_CONFIG_FILE);
            TMdbNtcFileOper::Copy(sConfigFile,sBakFile);
            snprintf(sTmpFile,sizeof(sTmpFile),"%s.%s",sBakFile,sCurTime);
            TMdbNtcFileOper::Rename(sBakFile,sTmpFile);
        }

        Clear();
        AnalyRepInfo(pRepInfo);

        MDBXMLDocument* pCfgDoc = new (std::nothrow)MDBXMLDocument();
        CHECK_OBJ(pCfgDoc); 
        pCfgDoc->Clear();

        MDBXMLElement* pConfigRoot = new(std::nothrow) MDBXMLElement("MDBConfig");
        CHECK_OBJ(pConfigRoot); 
        pCfgDoc->LinkEndChild(pConfigRoot);

        MDBXMLElement* pSys = new(std::nothrow) MDBXMLElement("Sys");
        CHECK_OBJ(pSys);
        pConfigRoot->LinkEndChild(pSys);
        AddSys(pSys, m_iHostID, m_iHeartbeat);

        // 添加hosts
        MDBXMLElement* pHost = new(std::nothrow) MDBXMLElement("hosts");
        CHECK_OBJ(pHost);
        pConfigRoot->LinkEndChild(pHost);
        std::map<int ,TMdbRepHost*>::iterator itorHostMap = m_mHostMap.begin();
        for(; itorHostMap != m_mHostMap.end(); ++itorHostMap)
        {
            AddHosts(pHost, itorHostMap->second);
        }

        // 添加rules
        MDBXMLElement* pRule = new(std::nothrow) MDBXMLElement("rules");
        CHECK_OBJ(pRule);
        pConfigRoot->LinkEndChild(pRule);
        std::map<int ,TMdbRepRules*>::iterator itorRuleMap = m_mRuleMap.begin();
        for(; itorRuleMap != m_mRuleMap.end(); ++itorRuleMap)
        {
            AddRules(pRule, itorRuleMap->second);
        }

        // 添加rule与主机映射关系
        MDBXMLElement* pRoutingRep = new(std::nothrow) MDBXMLElement("routing_rep");
        CHECK_OBJ(pRoutingRep);
        pConfigRoot->LinkEndChild(pRoutingRep);
        std::map<int, std::vector<int> >::iterator itorRoutingRepMap = m_mRoutingMap.begin();
        for(; itorRoutingRepMap != m_mRoutingMap.end(); ++itorRoutingRepMap)
        {
            AddStandBy(pRoutingRep, itorRoutingRepMap->first,itorRoutingRepMap->second);
        }

        // 添加容灾机
        MDBXMLElement* pDisaster = new(std::nothrow) MDBXMLElement("disaster_recovery");
        CHECK_OBJ(pDisaster);
        pConfigRoot->LinkEndChild(pDisaster);
        std::vector<TMdbDisasterHost*>::iterator  itorDisaster = m_vRecoveryHost.begin();
        for(; itorDisaster != m_vRecoveryHost.end(); ++itorDisaster)
        {
            AddDisaster(pDisaster, *itorDisaster);
        }

        pCfgDoc->SaveFile(sConfigFile);
        
        
        return iRet;
    }

    int TShbRepLocalConfig::AddSys(MDBXMLElement* pEle, int iHostID, int iHeartBeat)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        MDBXMLElement *pHostElement = new(std::nothrow) MDBXMLElement("Host");
        CHECK_OBJ(pHostElement);
        pEle->LinkEndChild(pHostElement);
        pHostElement->SetAttribute("ID", iHostID);
        MDBXMLElement *pHeartbeat = new(std::nothrow) MDBXMLElement("Heartbeat");
        CHECK_OBJ(pHeartbeat);
        pEle->LinkEndChild(pHeartbeat);
        pHeartbeat->SetAttribute("Warning", iHeartBeat);

        return iRet;
    }

    int TShbRepLocalConfig::AddHosts(MDBXMLElement* pEle, TMdbRepHost* pHostInfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        MDBXMLElement *pHostElement = new(std::nothrow) MDBXMLElement("host");
        CHECK_OBJ(pHostElement);
        pEle->LinkEndChild(pHostElement);
        pHostElement->SetAttribute("ID",pHostInfo->m_iHostID);
        pHostElement->SetAttribute("ip",pHostInfo->m_strIP.c_str());
        pHostElement->SetAttribute("port",pHostInfo->m_iPort);
        return iRet;
    }

    int TShbRepLocalConfig::AddRules(MDBXMLElement* pEle, TMdbRepRules* pRuleInfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        MDBXMLElement *pRuleElement = new(std::nothrow) MDBXMLElement("rule");
        CHECK_OBJ(pRuleElement);
        pEle->LinkEndChild(pRuleElement);
        pRuleElement->SetAttribute("ID",pRuleInfo->m_iRuleID);
        pRuleElement->SetAttribute("value",pRuleInfo->m_strRule.c_str());
        return iRet;
    }

    int TShbRepLocalConfig::AddStandBy(MDBXMLElement* pEle,int iRuleID, std::vector<int>& vHosts)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        MDBXMLElement *pRuleElement = new(std::nothrow) MDBXMLElement("routing");
        CHECK_OBJ(pRuleElement);
        pEle->LinkEndChild(pRuleElement);
        pRuleElement->SetAttribute("rule_ID",iRuleID);
        std::string sHostList;
        TMdbNtcStrFunc tStr;
        for(unsigned int i = 0 ; i < vHosts.size(); i++)
        {
            if(i == 0)
            {
                sHostList += tStr.IntToStr(vHosts[i]);
            }
            else
            {
                sHostList += ",";
                sHostList += tStr.IntToStr(vHosts[i]);
            }
        }
        pRuleElement->SetAttribute("host_ID_list",sHostList.c_str());
        
        return iRet;
    }

    int TShbRepLocalConfig::AddDisaster(MDBXMLElement* pEle, TMdbDisasterHost* pHostinfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        MDBXMLElement *pRuleElement = new(std::nothrow) MDBXMLElement("host");
        CHECK_OBJ(pRuleElement);
        pEle->LinkEndChild(pRuleElement);
        pRuleElement->SetAttribute("ID",pHostinfo->m_iHostID);
        pRuleElement->SetAttribute("rule_ID_list",pHostinfo->m_strRulelist.c_str());
        return iRet;
    }


//}
