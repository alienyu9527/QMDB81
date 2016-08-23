/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbDAOBase.cpp		
*@Description： 负责管理miniDB的动态DAO的控制
*@Author:		li.shugang
*@Date：	    2009年04月03日
*@History:
******************************************************************************************/
#include "Dbflush/mdbDAOBase.h"
#include "Helper/mdbBase.h"

//namespace QuickMDB{


    TMdbIntData::TMdbIntData()
    {
        piValue = NULL;
        memset(sName, 0, sizeof(sName));
        for(int i = 0;i<MAX_DATA_COUNTS;i++)
        {
            isNull[i] = 0;
        }
    }

    TMdbIntData::~TMdbIntData()
    {
        if(piValue!=NULL)
        {
            delete piValue;
            piValue = NULL;
        }
    }


    TMdbCharData::TMdbCharData()
    {
        psValue = NULL;
        for(int i = 0;i<MAX_DATA_COUNTS;i++)
        {
            ppsValue[i] = NULL;
            isNull[i] = 0;
        }
        memset(sName, 0, sizeof(sName));
        iLength = 0;
    }

    TMdbCharData::~TMdbCharData()
    {
        SAFE_DELETE_ARRAY(psValue);
        for(int i = 0;i<MAX_DATA_COUNTS;i++)
        {
            ppsValue[i] = NULL;
        }
    }


    TMdbDAOBase::TMdbDAOBase()
    {
        m_pQuery = NULL;
        memset(m_sSQL, 0, sizeof(m_sSQL));
		for(int i = 0; i < MAX_PARAM_COUNTS; i++)
		{
			m_tIntData[i] = NULL;
	        m_tCharData[i] = NULL;
	        m_tBlobData[i] = NULL;
	        m_tDaoData[i] = NULL;
		}
    }


    TMdbDAOBase::~TMdbDAOBase()
    {    
        SAFE_DELETE(m_pQuery);
		for(int i = 0; i < MAX_PARAM_COUNTS; i++)
		{
			SAFE_DELETE(m_tIntData[i]);
	        SAFE_DELETE(m_tCharData[i]);
	        SAFE_DELETE(m_tBlobData[i]);
	        SAFE_DELETE(m_tDaoData[i]);
		}
    }


    /******************************************************************************
    * 函数名称	:  SetOperType()
    * 函数描述	:  设置操作类型：0-Insert; 1-Delete; 2-Update....
    * 输入		:  pszSQL, SQL语句 
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbDAOBase::SetSQL(const char* pszSQL)
    {
        TADD_DETAIL("TMdbDAOBase::Execute() : pszSQL=%s", pszSQL);
        SAFESTRCPY(m_sSQL,sizeof(m_sSQL),pszSQL);
        ClearArrayData();
    }


    /******************************************************************************
    * 函数名称	:  GetCounts()
    * 函数描述	:  获取记录数
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  记录数
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbDAOBase::GetCounts()
    {
        return m_iCurCounts;
    }

    /******************************************************************************
    * 函数名称	:  Execute()
    * 函数描述	:  执行批处理命令
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0, 数据错误导致失败返回-1, Oracle链接断开导致失败返回1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbDAOBase::Execute(TMDBDBInterface* pDBLink)
    {
        TADD_FUNC("TMdbDAOBase::Execute() : Start.");
        
        if(m_iCurCounts <= 0)
        {
            TADD_FUNC("TMdbDAOBase::Execute() : Finish(No-Data).");
            return 0;
        }
        
        int iRet = 0;
        bool bRetFlag = false;
        
        try
        {
            //构造数据查询/更新对象
            if(m_pQuery == NULL)
            {
                m_pQuery = pDBLink->CreateDBQuery();
                CHECK_OBJ(m_pQuery);
            }      
            
            //设置SQL
            m_pQuery->Close();
            m_pQuery->SetSQL(m_sSQL);
            //绑定数据
            TADD_DETAIL("[%s : %d] : TMdbDAOBase::Execute() SQL=[%s]",__FILE__, __LINE__,m_sSQL);
            TADD_DETAIL("TMdbDAOBase::Execute() : m_TotalPos=%d, m_iCurCounts=%d.", m_TotalPos, m_iCurCounts);
            
            for(int i=0; i<m_TotalPos; ++i)
            {
                TADD_DETAIL("[%s : %d] : TMdbDAOBase::Execute() m_tDaoData[%d].m_sName=[%s]",__FILE__, __LINE__,i,m_tDaoData[i]->m_sName);
                if(m_tDaoData[i]->iType == DT_Int)
                {
                    m_pQuery->SetParamArray(m_tDaoData[i]->m_sName, m_tIntData[m_tDaoData[i]->iPos]->piValue, \
                        sizeof(long long), m_iCurCounts*sizeof(long long),m_tIntData[m_tDaoData[i]->iPos]->isNull);
                    for(int n=0; n<m_iCurCounts; ++n)
                    {
                        TADD_FLOW("m_iLong[%d][%d]=%d.\n", m_tDaoData[i]->iPos, n, m_tIntData[m_tDaoData[i]->iPos]->isNull[n]);
                    }
                }
                else if(m_tDaoData[i]->iType == DT_Blob)
                {
                    m_pQuery->SetBlobParamArray(m_tDaoData[i]->m_sName, (char*)m_tBlobData[m_tDaoData[i]->iPos]->psValue, \
                        m_tBlobData[m_tDaoData[i]->iPos]->iLength, m_iCurCounts,m_tBlobData[m_tDaoData[i]->iPos]->isNull); 
                }
                else
                {
                    TADD_DETAIL("TMdbDAOBase::Execute() : iType == DT_DateStamp/CHAR/VARCHAR, m_sName=%s, iPos=%d.\n", m_tDaoData[i]->m_sName, m_tDaoData[i]->iPos);
                    m_pQuery->SetParamArray(m_tDaoData[i]->m_sName, (char**)m_tCharData[m_tDaoData[i]->iPos]->psValue,
                            m_tCharData[m_tDaoData[i]->iPos]->iLength, m_tCharData[m_tDaoData[i]->iPos]->iLength, 
                            m_iCurCounts*m_tCharData[m_tDaoData[i]->iPos]->iLength,m_tCharData[m_tDaoData[i]->iPos]->isNull); 
                    for(int n=0; n<m_iCurCounts; ++n)
                    {
                        TADD_FLOW("TMdbDAOBase::Execute() : m_pString[%d][%d]=%d.\n", m_tDaoData[i]->iPos, n, m_tCharData[m_tDaoData[i]->iPos]->isNull[n]);
                        if(strlen(m_tCharData[m_tDaoData[i]->iPos]->ppsValue[n])!=0)
                        {
                            char *tempvalue= m_tCharData[m_tDaoData[i]->iPos]->ppsValue[n];
                            TADD_DETAIL("m_tCharData[m_tDaoData[i].iPos].iLength =%d\n",m_tCharData[m_tDaoData[i]->iPos]->iLength);
                            TADD_DETAIL("TMdbDAOBase::Execute() : m_pString[%d][%d]=%s.\n", m_tDaoData[i]->iPos, n,tempvalue);
                        }

                    }
                }
            }
            bRetFlag = m_pQuery->Execute(m_iCurCounts); 
			m_pQuery->Commit();
        }
        catch(TMDBDBExcpInterface &oe)
        {
            TADD_ERROR(ERROR_UNKNOWN," TDBException :%s\nSQL=[%s].", oe.GetErrMsg(), oe.GetErrSql());
            bRetFlag = false;
        }
        catch(TBaseException &oe)
        {        
            TADD_ERROR(ERROR_UNKNOWN,"TException :%s.", oe.GetErrMsg());
            bRetFlag = false;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Unknow Exception.");
            bRetFlag = false;
        }
        
        if(bRetFlag == false)
        {
            if(pDBLink->IsConnect() == false)
            {
                TADD_ERROR(ERROR_UNKNOWN,"No-Connect-Oracle.");
                iRet = 1;
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"Data-Error.");
                iRet = -1;
            }
        }

        ClearArrayData();    
        TADD_FUNC("TMdbDAOBase::Execute() : Finish.");
        
        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  ExecuteOne()
    * 函数描述  :  单条处理命令
    * 输入		:  pDBLink oracle链接
    * 返回值    :  成功返回0, 数据错误导致失败返回-1, Oracle链接断开导致失败返回1
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbDAOBase::ExecuteOne(TMDBDBInterface* pDBLink,TMdbLCR& tLCR)
    {
        TADD_FUNC("Start.");    
        int iRet = 0;
        bool bRetFlag = false;
        CHECK_OBJ(pDBLink);
        CHECK_OBJ(&tLCR);
        try
        {
            //构造数据查询/更新对象
            char sBlobValue[MAX_BLOB_LEN] = {0};
            if(m_pQuery == NULL)
            {
                m_pQuery = pDBLink->CreateDBQuery();
            }      
            
            //设置SQL
            m_pQuery->Close();
            m_pQuery->SetSQL(m_sSQL);
            //绑定数据
            TADD_DETAIL("SQL=[%s]",m_sSQL);
			std::vector<TLCRColm>::iterator itor = tLCR.m_vColms.begin();
	        for (; itor != tLCR.m_vColms.end(); ++itor)
	        {
				//NULL值处理
                if(itor->m_bNull)
                {
                    m_pQuery->SetParameterNULL(itor->m_sColmName.c_str());
                    continue;
                }
				
				if(itor->m_iType == DT_Int)
                {
                    m_pQuery->SetParameter(itor->m_sColmName.c_str(),\
                        TMdbNtcStrFunc::StrToInt(itor->m_sColmValue.c_str()));
                }
                else if(itor->m_iType == DT_Blob)
                {
                    memset(sBlobValue,0,sizeof(sBlobValue));
                    std::string encoded = itor->m_sColmValue;
                    std::string decoded = Base::base64_decode(encoded);
                    SAFESTRCPY(sBlobValue,sizeof(sBlobValue),decoded.c_str());
                    m_pQuery->SetParameter(itor->m_sColmName.c_str(),sBlobValue,int(decoded.length()));
                }
                else
                {
                    m_pQuery->SetParameter(itor->m_sColmName.c_str(),itor->m_sColmValue.c_str());
                }
        	}
			itor = tLCR.m_vWColms.begin();
	        for (; itor != tLCR.m_vWColms.end(); ++itor)
	        {
				//NULL值处理
                if(itor->m_bNull)
                {
                    m_pQuery->SetParameterNULL(itor->m_sColmName.c_str());
                    continue;
                }
				
				if(itor->m_iType == DT_Int)
                {
                    m_pQuery->SetParameter(itor->m_sColmName.c_str(),\
                        TMdbNtcStrFunc::StrToInt(itor->m_sColmValue.c_str()));
                }
                else if(itor->m_iType == DT_Blob)
                {
                    memset(sBlobValue,0,sizeof(sBlobValue));
                    std::string encoded = itor->m_sColmValue;
                    std::string decoded = Base::base64_decode(encoded);
                    SAFESTRCPY(sBlobValue,sizeof(sBlobValue),decoded.c_str());
                    m_pQuery->SetParameter(itor->m_sColmName.c_str(),sBlobValue,int(decoded.length()));
                }
                else
                {
                    m_pQuery->SetParameter(itor->m_sColmName.c_str(),itor->m_sColmValue.c_str());
                }
        	}
            bRetFlag = m_pQuery->Execute();  
            m_pQuery->Commit();
        }
        catch(TMDBDBExcpInterface &oe)
        {
            TADD_ERROR(ERROR_UNKNOWN,"TDBException :%s\nSQL=[%s].",oe.GetErrMsg(), oe.GetErrSql());
            bRetFlag = false;
        }
        catch(TBaseException &oe)
        {        
            TADD_ERROR(ERROR_UNKNOWN,"TException :%s.",oe.GetErrMsg());
            bRetFlag = false;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN," Unknow Exception.");
            bRetFlag = false;
        }
        
        if(bRetFlag == false)
        {
            if(pDBLink->IsConnect() == false)
            {
                TADD_ERROR(ERROR_UNKNOWN,"No-Connect-Oracle.");
                iRet = 1;
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"Data-Error.");
                iRet = -1;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ClearArrayData()
    * 函数描述	:  清空缓存记录链表
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbDAOBase::ClearArrayData()
    {
    	TADD_FUNC("TMdbDAOBase::ClearArrayData() : Start.");
        m_iStringPos = 0;
        m_iLongPos   = 0;
        m_TotalPos   = 0;
        m_iBlobPos = 0;    
        m_iCurCounts = 0;   
        TADD_FUNC("TMdbDAOBase::ClearArrayData() : Finish.");
    }


    /******************************************************************************
    * 函数名称	:  StartData()
    * 函数描述	:  开始添加数据
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbDAOBase::StartData()
    {
    	TADD_FUNC("TMdbDAOBase::StartData() : Start.");
    	TADD_FUNC("TMdbDAOBase::StartData() : m_iCurCounts=%d.", m_iCurCounts);
        m_iStringPos = 0;
        m_iLongPos   = 0;
        m_TotalPos   = 0;
        m_iBlobPos = 0;
        TADD_FUNC("TMdbDAOBase::StartData() : Finish.");
    }


    /******************************************************************************
    * 函数名称	:  AddData()
    * 函数描述	:  添加数据
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbDAOBase::AddData(TMdbData *pData)
    {
        TADD_FUNC("TMdbDAOBase::AddData() : Start.");
        if(pData->iType == DT_Int)
        {
			if(m_tDaoData[m_TotalPos] == NULL)
            {
                m_tDaoData[m_TotalPos] = new(std::nothrow) TMdbDAOData();
            }
            m_tDaoData[m_TotalPos]->iPos  = m_iLongPos;
			if(m_tIntData[m_iLongPos] == NULL)
            {
                m_tIntData[m_iLongPos] = new TMdbIntData();
            }
            if(m_tIntData[m_iLongPos]->piValue == NULL)
            {
                m_tIntData[m_iLongPos]->piValue = new long long[MAX_DATA_COUNTS];
            }
            m_tIntData[m_iLongPos]->piValue[m_iCurCounts] = TMdbNtcStrFunc::StrToInt(pData->sValue);
            SAFESTRCPY(m_tIntData[m_iLongPos]->sName,sizeof(m_tIntData[m_iLongPos]->sName),pData->sPName);
            m_tIntData[m_iLongPos]->isNull[m_iCurCounts] = pData->isNull;
            ++m_iLongPos;
            
        }
        else if(pData->iType == DT_Blob)
        {
			if(m_tDaoData[m_TotalPos] == NULL)
            {
                m_tDaoData[m_TotalPos] = new TMdbDAOData();
            }
            m_tDaoData[m_TotalPos]->iPos  = m_iBlobPos;
			if(m_tBlobData[m_iBlobPos] == NULL)
            {
                m_tBlobData[m_iBlobPos] = new TMdbCharData();
            }
            if(m_tBlobData[m_iBlobPos]->psValue == NULL)
            {
                m_tBlobData[m_iBlobPos]->psValue = new char[MAX_DATA_COUNTS*pData->iLen];
                for(int i = 0;i<MAX_DATA_COUNTS;i++)
                {
                    m_tBlobData[m_iBlobPos]->ppsValue[i]=m_tBlobData[m_iBlobPos]->psValue + (i*pData->iLen);
                }
            }
            std::string encoded = pData->sValue;
            std::string decoded = Base::base64_decode(encoded);
            m_tBlobData[m_iBlobPos]->iLength = decoded.length();
            SAFESTRCPY(m_tBlobData[m_iBlobPos]->ppsValue[m_iCurCounts],pData->iLen,decoded.c_str());
            SAFESTRCPY(m_tBlobData[m_iBlobPos]->sName,sizeof(m_tBlobData[m_iBlobPos]->sName),pData->sPName);
            m_tBlobData[m_iBlobPos]->isNull[m_iCurCounts] = pData->isNull;
            ++m_iBlobPos;    
        }
        else
        {
            if(pData->iType == DT_DateStamp)
            {
                pData->iLen = 15;
            }
			if(m_tDaoData[m_TotalPos] == NULL)
            {
                m_tDaoData[m_TotalPos] = new TMdbDAOData();
            }
            m_tDaoData[m_TotalPos]->iPos  = m_iStringPos;
			if(m_tCharData[m_iStringPos] == NULL)
            {
                m_tCharData[m_iStringPos] = new TMdbCharData();
            }
            if(m_tCharData[m_iStringPos]->psValue == NULL)
            {
                m_tCharData[m_iStringPos]->psValue = new char[MAX_DATA_COUNTS*pData->iLen];
                for(int i = 0;i<MAX_DATA_COUNTS;i++)
                {
                    m_tCharData[m_iStringPos]->ppsValue[i]=m_tCharData[m_iStringPos]->psValue + (i*pData->iLen);
                }
            }
            SAFESTRCPY(m_tCharData[m_iStringPos]->ppsValue[m_iCurCounts],pData->iLen,pData->sValue);
            SAFESTRCPY(m_tCharData[m_iStringPos]->sName,sizeof(m_tCharData[m_iStringPos]->sName),pData->sPName);
            m_tCharData[m_iStringPos]->iLength = pData->iLen;
            m_tCharData[m_iStringPos]->isNull[m_iCurCounts] = pData->isNull;
            ++m_iStringPos; 
        }
        SAFESTRCPY(m_tDaoData[m_TotalPos]->m_sName,sizeof(m_tDaoData[m_TotalPos]->m_sName),pData->sPName);
        m_tDaoData[m_TotalPos]->iType = pData->iType;
        ++m_TotalPos;
        
        TADD_FUNC("TMdbDAOBase::AddData() : Finish.");
        return;
    }
    /******************************************************************************
    * 函数名称	:  EndData()
    * 函数描述	:  结束添加数据
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbDAOBase::EndData()
    {
    	TADD_FUNC("TMdbDAOBase::EndData() : Start.");
    	
        m_iStringPos = 0;
        m_iLongPos   = 0;
        ++m_iCurCounts;
        
        TADD_FUNC("TMdbDAOBase::EndData() : m_iCurCounts=%d.", m_iCurCounts);
        
        TADD_FUNC("TMdbDAOBase::EndData() : Finish.");
    }


    //如果提交发现问题,写入文件日志，供以后处理
    /******************************************************************************
    * 函数名称	:  WriteError()
    * 函数描述	:  如果提交发现问题,写入文件日志，供以后处理
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbDAOBase::WriteError()
    {
        
    }      

//}


