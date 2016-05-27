/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbDictionary.cpp		
*@Description： mdb字典相关
*@Author:		jin.shaohua
*@Date：	    2014/03/06
*@History:
******************************************************************************************/
//#include "BillingSDK.h"
#include "Helper/mdbDictionary.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"

//using namespace ZSmart::BillingSDK;
//namespace QuickMDB{

    #define BOOL_TO_CHAR(VALUE)  ((VALUE)?"Y":"N")


	//获取页号
        int TMdbRowID::GetPageID(){
            unsigned int  iPageID;
            iPageID = m_iRowID;
            return (int)(iPageID>>11);
        }
        //获取数据偏移
        int TMdbRowID::GetDataOffset()
        {
            return (int)(m_iRowID&MAX_MDB_PAGE_RECORD_COUNT);
        }
        //设置pageid
        int TMdbRowID::SetPageID(int iPageID)
        {
            int iRet = 0;
            if(iPageID > MAX_MDB_PAGE_COUNT || iPageID <= 0)
            {
                CHECK_RET(ERR_APP_INVALID_PARAM,"iPageID[%d] > MAX_MDB_PAGE_COUNT[%d]",iPageID,MAX_MDB_PAGE_COUNT);
            }
            iPageID = iPageID << 11;
            m_iRowID &= MAX_MDB_PAGE_RECORD_COUNT;//先清理
            m_iRowID |= iPageID;
            return iRet;
        }
        int TMdbRowID::SetDataOffset(int iDataOffset)
        {
            int iRet =0;
            if(iDataOffset > MAX_MDB_PAGE_RECORD_COUNT || iDataOffset < 0)
            {
                CHECK_RET(ERR_APP_INVALID_PARAM,"iDataOffset[%d] > MAX_MDB_PAGE_RECORD_COUNT[%d]",
                    iDataOffset,MAX_MDB_PAGE_RECORD_COUNT);
            }
            m_iRowID &= (MAX_MDB_PAGE_COUNT << 11);//先清理
            m_iRowID |= iDataOffset;
            return iRet;
        }

        

	void TMdbPage::Init(int iPageID, int iPageSize)
        {
            m_iPageID = iPageID;
            TMdbDateTime::GetCurrentTimeStr(m_sUpdateTime);
            m_iRecordCounts = 0;       //使用的记录数
            m_iPageSize     = iPageSize;   //页大小,默认为4096
            m_iFreePageNode = -1;      //第一个空闲的页节点  0-没有空闲页
            m_iFullPageNode = -1;
            m_iFreeOffSet   = sizeof(TMdbPage);//空闲偏移
            m_iFreeSize = m_iPageSize -  sizeof(TMdbPage);//空闲大小
            m_iRecordSize  = 0;
            memset(m_sState, 0, sizeof(m_sState));
            SAFESTRCPY(m_sState,sizeof(m_sState),"empty");
            memset(m_sTableName,0,sizeof(m_sTableName));
            //m_iTableID = -1;
            m_iPageLSN = 0;
        }


	void TMdbPage::Init(int iPageID, int iPageSize, int iVarcharID)
        {
            m_iPageID = iPageID;
            TMdbDateTime::GetCurrentTimeStr(m_sUpdateTime);
            m_iRecordCounts = 0;       //使用的记录数
            m_iPageSize     = iPageSize;   //页大小,默认为4096
            m_iFreePageNode = -1;      //第一个空闲的页节点  0-没有空闲页
            m_iFullPageNode = -1;
            m_iFreeOffSet   = sizeof(TMdbPage);//空闲偏移
            m_iFreeSize = m_iPageSize -  sizeof(TMdbPage);//空闲大小
            switch (iVarcharID)
        	{
				case 0:
					m_iRecordSize = 16;
					break;
				case 1:
					m_iRecordSize = 32;
					break;
				case 2:
					m_iRecordSize = 64;
					break;
				case 3:
					m_iRecordSize = 128;
					break;
				case 4:
					m_iRecordSize = 256;
					break;
				case 5:
					m_iRecordSize = 512;
					break;
				case 6:
					m_iRecordSize = 1024;
					break;
				case 7:
					m_iRecordSize = 2048;
					break;
				case 8:
					m_iRecordSize = 4096;
					break;
				case 9:
					m_iRecordSize = 8192;
					break;
				default:
					m_iRecordSize  = 0;
					//TADD_ERROR("Invalid varchar ID [%d].",iVarcharID);
					break;
        	}            
            memset(m_sState, 0, sizeof(m_sState));
            SAFESTRCPY(m_sState,sizeof(m_sState),"empty");
            memset(m_sTableName,0,sizeof(m_sTableName));
            //m_iTableID = -1;
            m_iPageLSN = 0;
        }
        //输出
        std::string TMdbPage::ToString()
        {
            char sTemp[1024] = {0};
            sprintf(sTemp,"TableName=[%s],PageID=[%d],m_iPrePageID=[%d],m_iNextPageID=[%d],"\
                "m_iFreePageNode=[%d],m_iFreeOffSet=[%d],m_iPageSize=[%d],"\
                "m_sState=[%s],m_sUpdateTime=[%s],this=[%p]",m_sTableName,m_iPageID,m_iPrePageID,m_iNextPageID,
                m_iFreePageNode,m_iFreeOffSet,m_iPageSize,m_sState,m_sUpdateTime,this);
            std::string sRet = sTemp;
            return sRet;
        }

        //获取一个空闲的记录空间
        /*
        内存示意图
        +------------------+
        |TMdbPage         |
        +------------------+
        |TMdbPageNode  |
        +------------------+
        |DATA                |
        |                        |
        +------------------+
        |TMdbPageNode  |
        +------------------+
        |DATA                |
        |                        |
        +------------------+
        |TMdbPageNode  |
        +------------------+
        |DATA                |
        |                        |
        +------------------+
        使用pagenode可以将回收的空闲区串连起来
        */
        //记录位置转data offset
        int TMdbPage::RecordPosToDataOffset(int iRecordPos)
        {
            return sizeof(TMdbPage)+ iRecordPos*(m_iRecordSize + sizeof(TMdbPageNode))+sizeof(TMdbPageNode);
        }
        //data offset 转记录位置
        int TMdbPage::DataOffsetToRecordPos(int iDataOffset)
        {
            return (iDataOffset - sizeof(TMdbPage)-sizeof(TMdbPageNode))/(m_iRecordSize + sizeof(TMdbPageNode));
        }

        int TMdbPage::GetFreeRecord(int & iNewDataOffset,int iDataSize)
        {
            TADD_FUNC("Start.pPageAddr = [%p],iDataSize = [%d]",this,iDataSize);
            int iRet = 0;
            if(m_iRecordSize <= 0)
            {//哪边忘记设置recordsize
                TADD_WARNING("Boy! you forget sth.");
                m_iRecordSize = iDataSize;
            }
            if(iDataSize != m_iRecordSize)
            {//定长记录不匹配
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"m_iRecordSize[%d] != iDataSize[%d],Page=[%s]",m_iRecordSize,iDataSize,ToString().c_str());
            }
            if(m_iFreePageNode > 0)
            {//有空闲节点
                
                iNewDataOffset = m_iFreePageNode + sizeof(TMdbPageNode);
                TMdbPageNode * pFreePageNode = (TMdbPageNode *)((char *)this + m_iFreePageNode);//空闲节点
				if(pFreePageNode->iNextNode > 0)
				{
					TMdbPageNode * pNextFreePageNode = (TMdbPageNode *)((char *)this + pFreePageNode->iNextNode);
					pNextFreePageNode->iPreNode = -1;
					pFreePageNode->iPreNode = -1;
					if(m_iFullPageNode > 0)
					{
						TMdbPageNode * pFullPageNode = (TMdbPageNode *)((char *)this + m_iFullPageNode);
						pFullPageNode->iPreNode = m_iFreePageNode;
					}
	                m_iFreePageNode = pFreePageNode->iNextNode;
	                pFreePageNode->iNextNode = m_iFullPageNode;
	                m_iFullPageNode = iNewDataOffset - sizeof(TMdbPageNode);
				}
				else
				{
					pFreePageNode->iPreNode = -1;
					if(m_iFullPageNode > 0)
					{
						TMdbPageNode * pFullPageNode = (TMdbPageNode *)((char *)this + m_iFullPageNode);
						pFullPageNode->iPreNode = m_iFreePageNode;
					}
	                m_iFreePageNode = -1;
	                pFreePageNode->iNextNode = m_iFullPageNode;
	                m_iFullPageNode = iNewDataOffset - sizeof(TMdbPageNode);
				}                
            }
            else if(m_iFreeOffSet + m_iRecordSize + (int)sizeof(TMdbPageNode) < m_iPageSize)
            {//获取新位置
                
                iNewDataOffset = m_iFreeOffSet + sizeof(TMdbPageNode);
                TMdbPageNode * pFreePageNode = (TMdbPageNode *)((char *)this + m_iFreeOffSet);//占用该节点
                if(m_iFullPageNode > 0)
            	{
					TMdbPageNode * pFullPageNode = (TMdbPageNode *)((char *)this + m_iFullPageNode);
	                pFreePageNode->iNextNode = m_iFullPageNode;
					pFreePageNode->iPreNode = -1;
					pFullPageNode->iPreNode = m_iFreeOffSet;
	                m_iFullPageNode = m_iFreeOffSet;
	                m_iFreeOffSet += m_iRecordSize + sizeof(TMdbPageNode);
            	}
                else
            	{
					pFreePageNode->iNextNode = -1;
					pFreePageNode->iPreNode = -1;
	                m_iFullPageNode = m_iFreeOffSet;
	                m_iFreeOffSet += m_iRecordSize + sizeof(TMdbPageNode);
            	}
            }
            else 
            {
                //不足以放一个record,寻找下一个
                return ERR_PAGE_NO_MEMORY;
            }
            m_iFreeSize -= iDataSize;
            return iRet;
        }
        /*
        //是否是合法的dataOffset
        bool TMdbPage::IsValidDataOffset(int iDataOffset)
        {
            return false == (iDataOffset >= m_iFreeOffSet || iDataOffset < (int)sizeof(TMdbPage) + (int)sizeof(TMdbPageNode));
        }
        */
        /******************************************************************************
        * 函数名称	:  GetNextDataAddr
        * 函数描述	:  获取下一个数据地址
        * 输入		:  
        * 输入		:  
        * 输出		:  
        * 返回值	:  NULL - 失败 !NULL -成功
        * 作者		:  dong.chun
        *******************************************************************************/
        char * TMdbPage::GetNextDataAddr(char * &pDataAddr,int & iDataOffset,char *&pNextDataAddr)
        {
            if(m_iFullPageNode <=0){return NULL;}
            if(NULL == pDataAddr)
            {//第一次获取
                pDataAddr = (char * )this + m_iFullPageNode + sizeof(TMdbPageNode);//第一个数据地址
                iDataOffset = pDataAddr - (char * )this;

				TMdbPageNode* node = (TMdbPageNode*)(pDataAddr - sizeof(TMdbPageNode));
                if(node->iNextNode > 0)
                {
                    pNextDataAddr = (char*)this + node->iNextNode + sizeof(TMdbPageNode);
                }
				else
				{
					pNextDataAddr = NULL;
				}
				
                return pDataAddr;
            }
            else 
            {
            	pDataAddr = pNextDataAddr;
				if(NULL == pDataAddr)
				{
					return NULL;
				}
				iDataOffset = pDataAddr - (char * )this;
				
                TMdbPageNode* node = (TMdbPageNode*)(pDataAddr - sizeof(TMdbPageNode));
                if(node->iNextNode > 0)
                {
                    pNextDataAddr = (char*)this + node->iNextNode + sizeof(TMdbPageNode);
                }
				else
				{
					pNextDataAddr = NULL;
				}

				return pDataAddr;
				
            }
            return NULL;//遍历结束
        }

        //归还一个记录空间
        int TMdbPage::PushBack(int iDataOffset)
        {
            TADD_FUNC("Start.pPageAddr = [%p],iDataOffset = [%d]",this,iDataOffset);
            int iRet = 0;
            if(IsValidDataOffset(iDataOffset) == false)
            {//归还的记录位置不在数据区
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"iDataOffset=[%d],Page=[%s]",iDataOffset,ToString().c_str());
            }
            else 
            {
                int iBlockOffset = iDataOffset - sizeof(TMdbPageNode);
                TMdbPageNode * pCurPageNode = (TMdbPageNode *)((char *)this + iBlockOffset);//空闲节点
                TMdbPageNode * pFreePageNode = NULL;
                if(m_iFreePageNode > 0)
	            {
	             	pFreePageNode = (TMdbPageNode *)((char *)this + m_iFreePageNode);
                }
				
				if(pCurPageNode->iPreNode > 0)
				{
					TMdbPageNode * pPreFullPageNode = (TMdbPageNode *)((char *)this + pCurPageNode->iPreNode);
					pPreFullPageNode->iNextNode = pCurPageNode->iNextNode;
				}
				else
				{
					m_iFullPageNode = pCurPageNode->iNextNode;
				}
				if(pCurPageNode->iNextNode > 0)
				{
					TMdbPageNode * pNextFullPageNode = (TMdbPageNode *)((char *)this + pCurPageNode->iNextNode);
					pNextFullPageNode->iPreNode = pCurPageNode->iPreNode;
				}
				
				if(m_iFreePageNode > 0 && pFreePageNode != NULL)
	            {
					pCurPageNode->iPreNode = -1;
					pFreePageNode->iPreNode = iBlockOffset;
	                pCurPageNode->iNextNode = m_iFreePageNode;
	                m_iFreePageNode = iBlockOffset;
	                memset((char *)this+iDataOffset,0,m_iRecordSize);//清空数据区
	            }
	            else
	            {
					pCurPageNode->iPreNode = -1;
					pCurPageNode->iNextNode = -1;
					m_iFreePageNode = iBlockOffset;
	                memset((char *)this+iDataOffset,0,m_iRecordSize);//清空数据区
	            }
            }
            m_iFreeSize += m_iRecordSize;
            return iRet;
        }

	void TMdbColumn::Clear()
        {
            memset(sName, 0, sizeof(sName));
            memset(iDefaultValue, 0, sizeof(iDefaultValue));
            iDataType  = DT_Unknown;
            iColumnLen = -1;
            iPos       = -1;
            isInOra    = false;
            iOffSet = -1;             //数据的偏移量
            iHashPos = -1;
            bIsDefault = false; 
            m_bNullable = false;
        	//iDefaultValue = 0;	
            //nHashB = -1;
           // bExists = false;
        }
        
        //是否是增量更新列
        bool TMdbColumn::bIncrementalUpdate()
        {
            if(DT_Int == iDataType && false == m_bNullable)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        //是否是字符串类型的字段
        bool TMdbColumn::IsStrDataType()
        {
            if(DT_Char == iDataType || DT_VarChar == iDataType)
            {

                return true;
            }
            return false;
        }

    int TMdbTable::GetInnerTableLevel(const char* psTabLevel)
    {
        int iTableLevl = 0;
        if(NULL == psTabLevel) return -1;
        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psTabLevel, "TINY"))
        {
            iTableLevl = TAB_TINY;
        }
        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psTabLevel, "MINI"))
        {
            iTableLevl = TAB_MINI;
        }
        else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psTabLevel, "SMALL"))
        {
            iTableLevl = TAB_SMALL;
        }
        else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psTabLevel, "LARGE"))
        {
            iTableLevl = TAB_LARGE;
        }
        else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psTabLevel, "HUGE"))
        {
            iTableLevl = TAB_HUGE;
        }
        else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psTabLevel, "ENOR") || 0 == TMdbNtcStrFunc::StrNoCaseCmp(psTabLevel, "ENORMOUS"))
        {
            iTableLevl = TAB_ENORMOUS;
        }
        else
        {
            iTableLevl = -1;
        }

        return iTableLevl;
    }

    void TMdbTable::Clear()
    {
        memset(sTableName,  0, sizeof(sTableName));
        memset(sCreateTime, 0, sizeof(sCreateTime));
        memset(sUpdateTime, 0, sizeof(sUpdateTime));
        m_iTableId = -1;
        cState = Table_unused;  
        iRecordCounts = 10000;
        iExpandRecords = 10000;
        
        iColumnCounts = 0;
        int i = 0;
        for(i=0; i<MAX_COLUMN_COUNTS; ++i)
        {
            tColumn[i].Clear();
            m_tableAttr[i].Clear();
        }
        
        iIndexCounts = 0;                 //索引个数
        for(i=0; i<MAX_INDEX_COUNTS; ++i)
        {
            tIndex[i].Clear();
            tParameter[i].Clear();
        }    
        iParameterCount = 0;
        m_tPriKey.Clear();
        memset(m_sFilterSQL, 0, sizeof(m_sFilterSQL));
        memset(m_sLoadSQL, 0, sizeof(m_sLoadSQL));
        memset(m_sFlushSQL, 0, sizeof(m_sFlushSQL));
        memset(sViewSQL,     0, sizeof(sViewSQL));
        
        iFreePageID = -1;
        iCounts     = 0;
        iCINCounts  = 0;
        iFullPages  = 0;
        iFreePages  = 0;
        bFixOffset  = true;
        bFixedLength = true;
        bIsView = false;
        //m_bIsZipTime = false;
        m_cZipTimeType = 'N';
        bIsCheckPriKey = true;
        bIsNeedLoadFromOra = true;
        iLoadType = 1;
        bRollBack = true;

        bReadLock = false;
        bWriteLock = true;
        bIsPerfStat = false;
    	
        lTotalCollIndexNodeCounts = 0;     //全部的冲突索引节点数
        lLeftCollIndexNodeCounts  = 0;     //空闲的冲突索引节点数
        tCollIndexNode.tData.Clear();  
        tCollIndexNode.iNextPos = -1;
        iOneRecordSize = 0;//每条记录的大小
        iOneRecordNullOffset = -1;
        m_iTimeStampOffset = -1;
        iInsertCounts = 0;
        iDeleteCounts = 0;
        iUpdateCounts = 0;
        iQueryCounts = 0;
        iInsertFailCounts = 0;
        iDeleteFailCounts = 0;
        iUpdateFailCounts = 0;
        iQueryFailCounts = 0;
        iMagic_n = 0;
        //  tFreeMutex.Destroy();
        //   tTableMutex.Destroy();
        bIsSysTab = false;

        memset(m_sTableSpace, 0, sizeof(m_sTableSpace));

        m_bShardBack = false;
        m_cStorageType = MDB_DS_TYPE_NO;
    }

    TMdbColumn* TMdbTable::GetColumnByName(const char * sColumnName)
    {
        int i = 0;
        for(; i<MAX_COLUMN_COUNTS;i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(tColumn[i].sName,sColumnName) == 0)
            {
                break;
            }
        }

        if(i <MAX_COLUMN_COUNTS)
        {
            return &tColumn[i];
        }
        return NULL;
    }

    int TMdbTable::GetFreePageRequestCount()
    {
        int iCount = 0;
        switch(iTableLevel)
        {
            case TAB_TINY:
            case TAB_MINI:
                iCount =5;
                break;
            case TAB_SMALL:
                iCount = 10;
                break;
            case TAB_LARGE:
                iCount = 20;
                break;
            case TAB_HUGE:
                iCount = 50;
                break;
            case TAB_ENORMOUS:
                iCount =100;
                break;
            default:
                iCount = 1;
                break;
        }
		iCount += 20;
        return iCount;
    }

    /*char* TMdbTable::GetStorageType()
    {
        std::string sStorageType;
        
        switch(m_cStorageType)
        {
            case STORAGE_ORACLE:
                sStorageType = "ORACLE";
                break;
            case STORAGE_MYSQL:
                sStorageType =  "MYSQL";
                break;
            case STORAGE_FILE:
                sStorageType =  "FILE";
                break;
            case STORAGE_MDB:
                sStorageType =  "QMDB";
                break;
            case STORAGE_UNKOWN:
                sStorageType =  "UNKOWN";
                break;
        }

        return sStorageType.c_str();
        
    }
    */

    int TMdbTable::Init(TMdbTable * pSrcTable)
    {
        int iRet = 0;
        CHECK_OBJ(pSrcTable);
        Clear();
         //初始化表
        memcpy(this, pSrcTable, sizeof(TMdbTable));
         /*
        for(int j=0; j<iIndexCounts; ++j)
        {
            tIndex[j].tMutex.Create();
        }*/
        TMdbDateTime::GetCurrentTimeStr(sCreateTime);
        TMdbDateTime::GetCurrentTimeStr(sUpdateTime);
        cState      = Table_unused;
        iCounts     = 0;      //实际记录数
        iCINCounts  = 0;      //冲突索引数
        iFreePageID = -1;
        iFullPageID = -1;
        iFullPages  = 0;      //已满页数
        iFreePages  = 0;      //自由页数
        tFreeMutex.Create();  //创建自由页的锁
        tFullMutex.Create();  //创建满页锁
        tTableMutex.Create(); //创建表信息的锁
        return iRet;
    }

    void TMdbTable::Show(bool bIfMore)
    {
        int i = 0;
        char buf[500];
        char indexbuf[500] = {0};
        memset(buf,0,500);
        if(bIfMore == true)
        {
            for(i=0; i<m_tPriKey.iColumnCounts; ++i)
            {
                if(i == 0)
                {
                    SAFESTRCPY(buf,sizeof(buf),"Column-No =[");
                    sprintf(buf+strlen(buf),"%d",m_tPriKey.iColumnNo[i]);
                    continue;
                }
                else
                {
                    strcat(buf,",");
                    sprintf(buf+strlen(buf),"%d",m_tPriKey.iColumnNo[i]);
                }
            }
            strcat(buf,"]");
            printf("Table=[ %s ] Start\n", sTableName);
            printf("|    Table-Space      = %-20s\n", m_sTableSpace);
            printf("|    sCreateTime   = %-20s |  sUpdateTime   = %s\n", sCreateTime,sUpdateTime);
            printf("|    iFullPages    = %-20d |  iFreePages    = %d\n", iFullPages,iFreePages);
            printf("|    iFullPageID   = %-20d |  iFreePageID   = %d\n", iFullPageID,iFreePageID);
            printf("|    iRecordCounts(Real) = %-14d |  iRecordCounts(Set)  = %d\n", iCounts,iRecordCounts);
            printf("|    iTableLevel = %-14d |  iTabLevelCnts  = %d\n", iTableLevel, iTabLevelCnts);
            printf("|    bReadLock     = %-20s |  bWriteLock    = %s\n", BOOL_TO_CHAR(bReadLock),BOOL_TO_CHAR(bWriteLock));
            printf("|    bIsView       = %-20s |  bRollBack     = %s\n", BOOL_TO_CHAR(bIsView),BOOL_TO_CHAR(bRollBack));
            printf("|    bIsZipTime    = %-20c |  bIsCheckPriKey  = %s\n", m_cZipTimeType,BOOL_TO_CHAR(bIsCheckPriKey));
            printf("|    bIsPerfStat   = %-20s |  iLoadType       = %d\n", BOOL_TO_CHAR(bIsPerfStat),iLoadType);
            printf("|    cState        = %c(%c-unused, %c-using, %c-loading, %c-repping, %c-waitingRep)\n", cState, Table_unused, Table_running, Table_loading, Table_repping, Table_watingRep);
            printf("|    iCINCounts(real)     = %d |iOneRecordSize = %d\n", iCINCounts,iOneRecordSize);
            printf("|    iOneRecordNullOffset = %d |HaveNullableColumn=%s\n", iOneRecordNullOffset,HaveNullableColumn()?"Y":"N");
            printf("|    m_iTimeStampOffset = %d\n", m_iTimeStampOffset);
            printf("|    Primary-Key as like: %s\n",buf);
            printf("|    iIndexCounts  =%d\n", iIndexCounts);
            printf("|    Shard-Backup  =%-20s\n", BOOL_TO_CHAR(m_bShardBack));
            printf("|    Storage-Type  =%-20c\n", m_cStorageType);
            printf("|    +Index Info : \n");
            printf("|    |     \n");
            for(i=0;i<iIndexCounts;i++)
            {
                memset(indexbuf,0,sizeof(indexbuf));
                for(int iIndexPos = 0; iIndexPos< MAX_INDEX_COLUMN_COUNTS;iIndexPos++)
                {
                    if(tIndex[i].iColumnNo[iIndexPos] < 0)
                    {
                        continue;
                    }
                    snprintf(indexbuf+strlen(indexbuf),sizeof(indexbuf)-strlen(indexbuf),\
                        "%d,",tIndex[i].iColumnNo[iIndexPos]);
                }
                TMdbNtcStrFunc::Trim(indexbuf,',');
                printf("|    ---Index-Name=[%s], Column-No=[%s], iPriority=[%d],AlgoType=[%d]\n",\
                    tIndex[i].sName,indexbuf,tIndex[i].iPriority, tIndex[i].m_iAlgoType);
            }
            
            printf("|    iColumnCounts = %d\n", iColumnCounts);  
            if(iColumnCounts%2==0 )
            {
                for(i=0;i<iColumnCounts;i=i+2)
                {
                    printf("|    +Column Info(%-2d)---------------------| +Column Info(%-2d) \n",i+1,i+2);
                    printf("|    |                                      | \n");
                    printf("|    ---sName      =  %-20s| ---sName      =  %s\n",tColumn[i].sName,tColumn[i+1].sName);   
                    printf("|    ---iDataType  =  %-20d| ---iDataType  =  %d\n",tColumn[i].iDataType,tColumn[i+1].iDataType);
                    printf("|    ---iColumnLen =  %-20d| ---iColumnLen =  %d\n",tColumn[i].iColumnLen,tColumn[i+1].iColumnLen);
                    printf("|    ---iPos       =  %-20d| ---iPos       =  %d\n",tColumn[i].iPos,tColumn[i+1].iPos);
                    printf("|    ---NullAble   =  %-20s| ---NullAble   =  %s\n",BOOL_TO_CHAR(tColumn[i].m_bNullable),BOOL_TO_CHAR(tColumn[i+1].m_bNullable));
                }
            }
            else
            {   //如果是奇数列
                for(i=0;i<iColumnCounts-1;i=i+2)
                {   
                    printf("|    +Column Info(%-2d)---------------------| +Column Info(%-2d) \n",i+1,i+2);
                    printf("|    |                                      | \n");
                    printf("|    ---sName      =  %-20s| ---sName      =  %s\n",tColumn[i].sName,tColumn[i+1].sName);   
                    printf("|    ---iDataType  =  %-20d| ---iDataType  =  %d\n",tColumn[i].iDataType,tColumn[i+1].iDataType);
                    printf("|    ---iColumnLen =  %-20d| ---iColumnLen =  %d\n",tColumn[i].iColumnLen,tColumn[i+1].iColumnLen);
                    printf("|    ---iPos       =  %-20d| ---iPos       =  %d\n",tColumn[i].iPos,tColumn[i+1].iPos);
                    printf("|    ---NullAble   =  %-20s| ---NullAble   =  %s\n",BOOL_TO_CHAR(tColumn[i].m_bNullable),BOOL_TO_CHAR(tColumn[i+1].m_bNullable));
                }
                if(i == iColumnCounts-1)
                {
                    printf("|    +Column Info(%-2d)\n",i+1);
                    printf("|    |                                      \n");
                    printf("|    ---sName      =  %-20s\n",tColumn[i].sName);   
                    printf("|    ---iDataType  =  %-20d\n",tColumn[i].iDataType);
                    printf("|    ---iColumnLen =  %-20d\n",tColumn[i].iColumnLen);
                    printf("|    ---iPos       =  %-20d\n",tColumn[i].iPos);
                    printf("|    ---NullAble   =  %-20s\n",tColumn[i].m_bNullable?"Y":"N");
                }
            }
            
            printf("|    DataType:(0-Unknown,1-Int,2-Char,3-VarChar,4-DateStamp,5-Float)\n");
            printf("|    RepAttr:(0-Column_From_Ora,1-Column_To_Ora,2-Column_To_Rep,3-Column_Ora_Rep,4-Column_No_Rep)\n");    
            
            printf("Table=[ %s ] Finish\n", sTableName);
        }
        else
        {
            char sHead[500];
            static bool bfirst = false;
            memset(sHead,0,500);
            if(bfirst == false)
            {
                sprintf(sHead,"%-32s %-32s %-16s %-11s %-4s %-12s %-11s %-14s\n",\
                    "Name","TableSpace","Cretime","RecdCounts","Stat","ColumnCount","IndexCount","PriKeyCount");
                printf("\n");
                printf("[ Table-Info ]:\n");
                printf("%s",sHead);
                bfirst = true;
            }
            if(sTableName[0]!=0)
            {
                printf("%-32s %-32s %-16s %-11d %-4c %-12d %-11d %-14d\n",sTableName,m_sTableSpace,sCreateTime,iCounts,cState,iColumnCounts,iIndexCounts,m_tPriKey.iColumnCounts);
            }
        }
    }

    void TMdbTable::Print()
    {
        TADD_NORMAL("======Table=[%s] Start======", sTableName);
        TADD_NORMAL("    sCreateTime   = %s", sCreateTime);
        TADD_NORMAL("    sUpdateTime   = %s", sUpdateTime);
        TADD_NORMAL("    cState        = %c(%c-unused, %c-using, %c-loading, %c-repping, %c-waitingRep)", cState, Table_unused, Table_running, Table_loading, Table_repping, Table_watingRep);
        TADD_NORMAL("    TableSpace = %s", m_sTableSpace);
        TADD_NORMAL("    iColumnCounts = %d", iColumnCounts);
        TADD_NORMAL("    iFullPageID   = %d", iFullPageID);
        TADD_NORMAL("    iFreePageID   = %d", iFreePageID);
        TADD_NORMAL("    iFullPages    = %d", iFullPages);
        TADD_NORMAL("    iFreePages    = %d", iFreePages);
        TADD_NORMAL("    iRecordCounts(Real) = %d", iCounts);
        TADD_NORMAL("    iRecordCounts(Set)  = %d", iRecordCounts);
        TADD_NORMAL("    iTableLevel  = %d", iTableLevel);
        TADD_NORMAL("    iTabLevelCnts  = %d", iTabLevelCnts);
        TADD_NORMAL("    iCINCounts(real)    = %d", iCINCounts);
        TADD_NORMAL("    iOneRecordSize      = %d", iOneRecordSize);
        TADD_NORMAL("    Coloum Msg as like:");
        int i = 0;
        for(i=0; i<iColumnCounts; ++i)
       	{
       		TADD_NORMAL("    Column-Name =[%s].", tColumn[i].sName);
       		TADD_NORMAL("    Column-Type =[%d].", tColumn[i].iDataType);
       		TADD_NORMAL("    Column-Leng =[%d].", tColumn[i].iColumnLen);
       	}
       	TADD_NORMAL("    Primary-Key as like:");
        for(i=0; i<m_tPriKey.iColumnCounts; ++i)
        {
            TADD_NORMAL("    Column-No =[%d].", m_tPriKey.iColumnNo[i]);
        }
        
        TADD_NORMAL("======Table=[%s] Finish======\n", sTableName);
        
    }


    //数据库中的的进程信息
    void TMdbProc::Clear()
    {
        cState     = PSTOP;
        iPid       = -1;
        //bRestart   = false;
        memset(sStartTime, 0, sizeof(sStartTime));
        memset(sStopTime,  0, sizeof(sStopTime));
        memset(sUpdateTime,  0, sizeof(sUpdateTime));
        iLogLevel = 0;
        bIsMonitor = true;
        memset(sName, 0, sizeof(sName));
    }

    void TMdbProc::Show(bool bIfMore)
    {
        char sHead[500];
        static bool bfirst = false;
        memset(sHead,0,500);
        
        if(bfirst == false)
        {
            sprintf(sHead,"%-11s %-50s %-14s %-14s %-6s %-10s %-9s\n",\
            "Id","Name","StartTime","UpdateTime","State","LogLevel","IsMonitor");
            printf("\n");
            printf("[ Process-Info ]:\n");
            printf("%s",sHead);
            bfirst = true;
        }
        
        if(sName[0]!=0)
        {
    	    printf("%-11d %-50s %-14s %-14s %-7c %-9d %-9s\n",\
    	    iPid,sName,sStartTime,sUpdateTime,cState,iLogLevel,bIsMonitor?"TRUE":"FALSE");
        }
            
    }

    void TMdbProc::Print()
    {
        TADD_NORMAL("==============Process-Info=================");
        TADD_NORMAL("    sName      = %s.", sName);
        TADD_NORMAL("    cState     = %c(%c-stop, %c-free, %c-busy, %c-kill).", cState, PSTOP, PFREE, PBUSY, PKILL);
        TADD_NORMAL("    iPid       = %d.", iPid);
        TADD_NORMAL("    sStartTime = %s.", sStartTime);
        TADD_NORMAL("    sStopTime  = %s.", sStopTime);
        TADD_NORMAL("    sUpdateTime= %s.", sUpdateTime);
        TADD_NORMAL("    iLogLevel  = %d.", iLogLevel);
        TADD_NORMAL("    bIsMonitor = %s.", bIsMonitor?"TRUE":"FALSE");
        TADD_NORMAL("==============Process-Info=================");
    }
    /******************************************************************************
    * 函数名称	: Serialize 
    * 函数描述	:  序列化
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbProc::Serialize(rapidjson::PrettyWriter<TMdbStringStream> & writer)
    {
        writer.String("name");
        writer.String(sName);
        
        writer.String("State");
        writer.String(&cState,1);
        
        writer.String("Pid");
        writer.Int(iPid);
        
        
        writer.String("sStartTime");
        writer.String(sStartTime);
        
        writer.String("sStopTime");
        writer.String(sStopTime);
        
        writer.String("sUpdateTime");
        writer.String(sUpdateTime);
        
        writer.String("iLogLevel");
        writer.Int(iLogLevel);
        
        writer.String("bIsMonitor");
        writer.String(bIsMonitor?"TRUE":"FALSE");
        return 0;
    }
    
    void TMdbLocalLink::Clear()
    {
        iPID      = -1;
        iTID      = 0;
        iSocketID = -1;
        cState    = Link_down;
        memset(sStartTime, 0, sizeof(sStartTime));
        memset(sFreshTime, 0, sizeof(sFreshTime));
        iLogLevel = 0;
        cAccess   = MDB_WRITE;
        iSQLPos   = -1;
        
        iQueryCounts = 0;  //查询次数
        iQueryFailCounts = 0;  //查询次数
        
        iInsertCounts = 0; //插入次数
        iInsertFailCounts = 0; //插入次数
        
        iUpdateCounts = 0; //更新次数
        iUpdateFailCounts = 0; //更新次数
        
        iDeleteCounts = 0; //删除次数
        iDeleteFailCounts = 0; //删除次数
        
        
        memset(sProcessName,0,sizeof(sProcessName));

		iRBAddrOffset = 0;
    }    
    //是否是当前线程的链接
    bool TMdbLocalLink::IsCurrentThreadLink()
    {
        if(TMdbOS::GetPID() == iPID  && TMdbOS::GetTID() == iTID)
        {
            return true;
        }
        return false;
    }

    void TMdbLocalLink::Show(bool bIfMore)
    {
        char sHead[500];
        static bool bfirst = false;
        memset(sHead,0,500);
        if(bfirst == false)
        {
            sprintf(sHead,"%-11s %-22s %-8s %-14s %-14s %-6s %-10s %-7s SQLPos\n",\
            "Pid","Tid","SocketID","StartTime","FreshTime","State","LogLevel","Access");
            printf("\n");
            printf("[ Local-Link-Info ]:\n");
            printf("%s",sHead);
            bfirst = true;
        }
        if(iPID > 0)
        {
            printf("%-11d %-22lu %-8d %-14s %-14s %-6c %-10d %-7c %d\n",\
            iPID,iTID,iSocketID,sStartTime,sFreshTime,cState,iLogLevel,cAccess, iSQLPos);
        }   
    }

	
    void TMdbLocalLink::ShowRBUnits()
    {
		if(m_RBList.empty()) return;
		
		int iMaxShow = 100;
		int i = 0;
		TShmList<TRBRowUnit>::iterator itor = m_RBList.begin();
        for(;itor != m_RBList.end();++itor)
        {
        	if(i>100)
			{
				printf("Only Show %d RBUnits.\n",iMaxShow);
			}
			printf("[%d]--",i);
			itor->Show();
			i++;
        }
	}
    void TMdbLocalLink::Print()
    {
        TADD_NORMAL("==============Local-Link=================");
        TADD_NORMAL("    iPID       = %d.", iPID);
        TADD_NORMAL("    iTID       = %lu.", iTID);
        TADD_NORMAL("    SocketID   = %d.", iSocketID);
        TADD_NORMAL("    cState     = %c(%c-USE, %c-DOWN).", cState, Link_use, Link_down);
        TADD_NORMAL("    sStartTime = %s.", sStartTime);
        TADD_NORMAL("    sFreshTime = %s.", sFreshTime);
        TADD_NORMAL("    iLogLevel  = %d.", iLogLevel);
        TADD_NORMAL("    cAccess    = %c.", cAccess);
        TADD_NORMAL("    iSQLPos    = %d.", iSQLPos);
        TADD_NORMAL("    iQueryCounts    = %d.", iQueryCounts);
        TADD_NORMAL("    iQueryFailCounts    = %d.", iQueryFailCounts);
        TADD_NORMAL("    iInsertCounts    = %d.", iInsertCounts);
        TADD_NORMAL("    iInsertFailCounts    = %d.", iInsertFailCounts);
        TADD_NORMAL("    iUpdateCounts    = %d.", iUpdateCounts);
        TADD_NORMAL("    iUpdateFailCounts    = %d.", iUpdateFailCounts);
        TADD_NORMAL("    iDeleteCounts    = %d.", iDeleteCounts);
        TADD_NORMAL("    iDeleteFailCounts    = %d.", iDeleteFailCounts);
        TADD_NORMAL("==============Local-Link=================");
    }
	


	void TMdbLocalLink::Commit()
	{
		if(m_RBList.empty()) return;
		
		TADD_NORMAL("TMdbLocalLink::Commit\n");
			
		//正向遍历
		TShmList<TRBRowUnit>::iterator itorB = m_RBList.begin();
		while(!m_RBList.empty())
		{		
			if(itorB->Commit()!=0)break;	
			itorB= m_RBList.erase(itorB);
		}
		
	}

	int gdb_shut()
	{		
		while(getchar()!='\n');
		return 1;
	}
	
	
	void  TMdbLocalLink::RollBack()
	{
	
	//gdb_shut();
		if(m_RBList.empty()) return;
		
		TADD_NORMAL("TMdbLocalLink::RollBack\n");
		
		
		//反向遍历
		TShmList<TRBRowUnit>::iterator itor = m_RBList.end();
		while(!m_RBList.empty())
		{
			--itor;	
			itor->Show();
			if(itor->RollBack()!=0)break;	
			itor = m_RBList.erase(itor);
		}
	}

	int TMdbLocalLink::AddNewRBRowUnit(TRBRowUnit* pRBRowUnit)
	{
		int iRet = 0;
		TADD_NORMAL("TMdbLocalLink::AddNewRBRowUnit\n");
		TShmList<TRBRowUnit>::iterator itorNew =  m_RBList.insert(m_RBList.end());
        if(itorNew != m_RBList.end())
        {//分配成功
            TRBRowUnit* pNewRBRowUnit = &(*itorNew);
            pNewRBRowUnit->SQLType = pRBRowUnit->SQLType;
            pNewRBRowUnit->iRealRowID = pRBRowUnit->iRealRowID;
            pNewRBRowUnit->iVirtualRowID = pRBRowUnit->iVirtualRowID;
        }
        else
        {//分配失败
            CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for RBRowUnit");
        }
		return iRet;
}
    bool TMdbRemoteLink::IsCurrentThreadLink()
    {

        return false;
    }

    void TMdbRemoteLink::Clear()
    {
        memset(sIP, 0, sizeof(sIP));
        iHandle = -1;
        cState = Link_down;
        memset(sStartTime, 0, sizeof(sStartTime));
        memset(sFreshTime, 0, sizeof(sFreshTime));
        iLogLevel = 0;
        cAccess   = MDB_WRITE;
        iPID = -1;
        iTID = 0;
        memset(sUser, 0, sizeof(sUser));
        memset(sPass, 0, sizeof(sPass));
        memset(sDSN,  0, sizeof(sDSN));
        iLowPriority = 0;
        iSQLPos   = -1;
        memset(sProcessName,0,sizeof(sProcessName));
    }    

    void TMdbRemoteLink::Show(bool bIfMore)
    {   
        char sHead[500];
        static bool bfirst = false;
        memset(sHead,0,500);
        if(bfirst == false)
        {
            sprintf(sHead,"%-15s %-14s %-14s %-5s %-8s %-6s %-6s %-7s %-7s %-7s %-11s %-22s %-8s %-11s\n",\
            "IP","StartTime","FreshTime","State","LogLevel","Access","Handle","User","Pass","DSN","PID","TID","PRIORITY","SQLPos");
            printf("\n");
            printf("[ Remote-Link-Info ]:\n");
            printf("%s",sHead);
            bfirst = true;
        }
        if(sIP[0] != 0)
        {
            printf("%-15s %-14s %-14s %-5c %-8d %-6c %-6d %-7s %-7s %-7s %-11d %-22lu %-8d %-8d\n",\
            sIP,sStartTime,sFreshTime,cState,iLogLevel,cAccess,iHandle,sUser,sPass,sDSN,iPID,iTID,iLowPriority,iSQLPos);
        }
        
    }


    void TMdbRemoteLink::Print()
    {
        TADD_NORMAL("==============Remote-Link=================");
        TADD_NORMAL("    sIP        = %s.", sIP);
        TADD_NORMAL("    iHandle    = %d.", iHandle);
        TADD_NORMAL("    iPID       = %d.", iPID);
        TADD_NORMAL("    iTID       = %lu.", iTID);
        TADD_NORMAL("    cState     = %c(%c-USE, %c-DOWN).", cState, Link_use, Link_down);
        TADD_NORMAL("    sStartTime = %s.", sStartTime);
        TADD_NORMAL("    sFreshTime = %s.", sFreshTime);
        TADD_NORMAL("    iLogLevel  = %d.", iLogLevel);
        TADD_NORMAL("    cAccess    = %c.", cAccess);
        TADD_NORMAL("    sUser      = %s.", sUser);
        TADD_NORMAL("    iPriority  = %s.", iLowPriority);
        TADD_NORMAL("    sPass      = %s.", sPass);
        TADD_NORMAL("    sDSN       = %s.", sDSN);
        TADD_NORMAL("    iSQLPos    = %d.", iSQLPos);
        TADD_NORMAL("==============Remote-Link=================");
    }

    void TMdbRepLink::Clear()
    {
        iSocketID = -1;
        iPID       = -1;
        memset(sStartTime, 0, sizeof(sStartTime));
        memset(sIP,0,sizeof(sIP));
        iPort = -1;
        sState= 'U';
        sLink_Type = 'U';
    }

    void TMdbRepLink::Print()
    {
        TADD_NORMAL("==============Rep-Link=================");
        TADD_NORMAL("    SocketID   = %d.", iSocketID);
        TADD_NORMAL("    iPid       = %d.", iPID);
        TADD_NORMAL("    sIP        = %s.", sIP);
        TADD_NORMAL("    iPort      = %d.", iPort);
        TADD_NORMAL("    sStartTime = %s.", sStartTime);
        TADD_NORMAL("    sState     = %c.", sState);
        TADD_NORMAL("    sLink_Type = %c.", sLink_Type);
        TADD_NORMAL("==============Rep-Link=================");
    }    

    void TMdbRepLink::Show()
    {   
        char sHead[500];
        static bool bfirst = false;
        memset(sHead,0,500);
        if(bfirst == false)
        {
            sprintf(sHead,"%-15s %-11s %-14s %-20s %-11s %-11s %-11s\n",\
            "IP","Port","SocketId","StartTime","PID","State","Link_Type");
            printf("\n");
            printf("[ Rep-Link-Info ]:\n");
            printf("%s",sHead);
            bfirst = true;
        }
        if(sIP[0] != 0)
        {
            printf("%-15s %-11d %-14d %-20s %-11d %-11c %-11c\n",\
            sIP,iPort,iSocketID,sStartTime,iPID,sState,sLink_Type);
        }
        
    }

    TMdbJob::TMdbJob():
        m_iInterval(0),
        m_iRateType(0),
        m_iExcCount(0)
    {
        Clear();
    }
    //清理
    int TMdbJob::Clear()
    {
        memset(m_sExecuteDate,0,sizeof(m_sExecuteDate));
        memset(m_sSQL,0,sizeof(m_sSQL));
        memset(m_sNextExecuteDate,0,sizeof(m_sNextExecuteDate));
        memset(m_sStartTime,0,sizeof(m_sStartTime));
        memset(m_sName,0,sizeof(m_sName));
        m_iInterval = 0;
        m_iRateType = 0;
        m_iState = JOB_STATE_NONE;
        return  0;
    }

    /******************************************************************************
    * 函数名称	:  SetRateType
    * 函数描述	:  设置RateType
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbJob::SetRateType(const char * sRateType)
    {
        int iRet = 0;
        CHECK_OBJ(sRateType);
        m_iRateType = JOB_PER_NONE;
        int iCount = 6;
        int i = 0;
        const   char * sRateTypeArr[] = {"","year","month","day","hour","min","sec"};
        for(i = 1;i <= iCount;++i)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(sRateTypeArr[i],sRateType) == 0)
            {
                m_iRateType = i;
                break;
            }
        }
        if(JOB_PER_NONE == m_iRateType || 0 == sRateType[0])
        {//not find
            CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"RateType[%s] error.",sRateType);
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetRateType
    * 函数描述	:  获取RateType
    * 输入		:  iLen频率类型长度
    * 输入		:  
    * 输出		:  pRateType 频率类型
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbJob::GetRateType(char*pRateType,const int iLen)
    {
        int iRet = 0;
        CHECK_OBJ(pRateType);
        switch(m_iRateType)
        {   
        case JOB_PER_YEAR:
            {
                SAFESTRCPY(pRateType,iLen,"year");
                break;     
            } 
        case JOB_PER_MONTH:
            {
                SAFESTRCPY(pRateType,iLen,"month");
                break;     
            } 
        case JOB_PER_DAY:
            {
                SAFESTRCPY(pRateType,iLen,"day");
                break;     
            } 
        case JOB_PER_HOUR:
            {
                SAFESTRCPY(pRateType,iLen,"hour");
                break;     
            } 
        case JOB_PER_MIN:
            {
                SAFESTRCPY(pRateType,iLen,"min");
                break;     
            } 
        case JOB_PER_SEC:
            {
                SAFESTRCPY(pRateType,iLen,"sec");
                break;     
            } 
        default:
            {
                SAFESTRCPY(pRateType,iLen," ");
                break;
            }
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ToString
    * 函数描述	:  打印
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    std::string TMdbJob::ToString()
    {
        char sTemp[10240] = {0};
        const   char * sRateTypeArr[] = {"","year","month","day","hour","min","sec"};
        sprintf(sTemp,"name=[%s] startTime=[%s] execDate=[%s] nextDate=[%s] sql=[%s] RateType=[%s] interval=[%d] ExecCount=[%d]",
            m_sName,m_sStartTime,m_sExecuteDate,m_sNextExecuteDate,m_sSQL,sRateTypeArr[m_iRateType],m_iInterval,m_iExcCount);
        std::string sRet = sTemp;
        return sRet;
    }

    void TMemSeq::Clear()
    {
        memset(sSeqName, 0, sizeof(sSeqName));
        iStart = -1;
        iEnd   = -1;
        iCur   = -1;
        iStep  = -1;
        
    }
    void TMemSeq::Init()
    {
        Clear();
        //tMutex.Destroy();
        tMutex.Create();
    }

    void TMemSeq::Show(bool bIfMore)
    {
        char sHead[500];
        static bool bfirst = false;
        memset(sHead,0,500);
        if(bfirst == false)
        {
            sprintf(sHead,"%-22s %-14s %-14s %-14s %-10s\n",\
            "Name","Start","End","Cur","Step");
            printf("\n");
            printf("[ MemSeq-Info ]:\n");
            printf("%s",sHead);
            bfirst = true;
        }
        if(sSeqName[0] != 0)
        printf("%-22s %-14lld %-14lld %-14lld %-10lld\n",\
        sSeqName,iStart,iEnd,iCur,iStep);
    }

    void TMemSeq::Print()
    {
        TADD_NORMAL("==============Sequence-Info=================");
        TADD_NORMAL("    sSeqName = %s.", sSeqName);
        TADD_NORMAL("    iStart   = %ld.", iStart);
        TADD_NORMAL("    iEnd     = %ld.", iEnd);
        TADD_NORMAL("    iCur     = %ld.", iCur);
        TADD_NORMAL("    iStep    = %ld.", iStep);    
        TADD_NORMAL("==============Sequence-Info================="); 
    }

    void TMdbSyncArea::Clear()
    {
        m_iSAType = -1;
        memset(m_sName,0,sizeof(m_sName));
        m_iShmID  = INITVAl;
        m_iShmKey = INITVAl;
        m_iShmSize = 256;
        for(int i = 0; i<SA_MAX;i++)
        {
            memset(m_sDir[i],0,MAX_PATH_NAME_LEN);
        }
        m_iFileSize = 0;
    }
    //设置文件和大小
    void TMdbSyncArea::SetFileAndSize(const char * sFile,int iSize,int iSyncType)
    {
         SAFESTRCPY(m_sDir[iSyncType],MAX_PATH_NAME_LEN,sFile);
         m_iFileSize = iSize;
    }

    void TMdbDSN::Clear()
    {
        SAFESTRCPY(sVersion,sizeof(sVersion),MDB_VERSION);
        memset(sName, 0, sizeof(sName));
        cState       = DB_unused;
        iRepAttr     = DSN_No_Rep;
        iRoutingID = -1;
        iTableCounts = 0;
        iMutexCounts = MAX_MUTEX_COUNTS;
        memset(sCreateTime, 0, sizeof(sCreateTime));
        memset(sCurTime, 0, sizeof(sCurTime));
        tCurTime.tv_sec  = 0;
        tCurTime.tv_usec = 0;
        
        iShmCounts = 0;
        iBaseIndexShmCounts = 0;
        iConflictIndexShmCounts = 0;
        iMHashBaseIdxShmCnt = 0;
        iMHashMutexShmCnt = 0;
        iMHashConfIdxShmCnt = 0;
        iMHashLayerIdxShmCnt = 0;
		iTrieBranchIdxShmCnt = 0;
		iTrieConfIdxShmCnt = 0;
		
        m_iDataSize = 0;
        for(int i=0; i<MAX_SHM_ID; ++i)
        {
            iShmID[i]  = INITVAl;
            iBaseIndexShmID[i] = INITVAl;
            iConflictIndexShmID[i] = INITVAl;
            iMHashBaseIdxShmID[i] = INITVAl;
            iMHashMutexShmID[i]=INITVAl;
            
            iShmKey[i] = INITVAl;   
            iBaseIndexShmKey[i]     = INITVAl;
            iConflictIndexShmKey[i] = INITVAl;
            iMHashBaseIdxShmKey[i] = INITVAl;
            iMHashMutexShmKey[i] = INITVAl;
            
        }

        iMhashConfMgrShmId = INITVAl;
        iMhashConfMgrShmKey = INITVAl;
        iMhashLayerMgrShmId = INITVAl;
        iMhashLayerMgrShmKey = INITVAl;
        iMHashConfAddr= 0;
        iMHashLayerAddr = 0;

        for(int i =0 ; i < MAX_MHASH_SHMID_COUNT; ++i)
        {
            iMHashConfIdxShmID[i] = INITVAl;
            iMHashLayerIdxShmID[i] = INITVAl;
            iMHashConfIdxShmKey[i] = INITVAl;
            iMHashLayerIdxShmKey[i] = INITVAl;
        }

		iTrieBranchMgrShmId = INITVAl;
		iTrieBranchMgrShmKey = INITVAl;
		iTrieConfMgrShmId = INITVAl;
		iTrieConfMgrShmKey = INITVAl;
		
		for(int i =0 ; i < MAX_BRIE_SHMID_COUNT; ++i)
        {
            iTrieBranchIdxShmID[i] = INITVAl;
            iTrieBranchIdxShmKey[i] = INITVAl;
            iTrieConfIdxShmID[i] = INITVAl;
            iTrieConfIdxShmKey[i] = INITVAl;
        }
		


		
        for(int i=0;i<MAX_VARCHAR_SHM_ID;i++)
        {
        	iVarCharShmID[i] = INITVAl;
        	iVarCharShmKey[i] = INITVAl;
        }
        m_arrSyncArea.Clear();
        
        iProcAddr       = 0;

        iPageMutexAddr  = 0;
        iUserTableAddr  = 0;
        iLocalLinkAddr = 0;
        iRemoteLinkAddr = 0;
        iTableSpaceAddr = 0;
        iSeqAddr        = 0;             //序列管理区偏移量
        iSQLAddr        = 0;
        iRepLinkAddr= 0;
        iJobAddr  = 0;//
        iVarcharAddr = 0;
        
        
        

        memset(sDataStore, 0, sizeof(sDataStore));
        memset(sStorageDir,0,sizeof(sStorageDir));


        iPermSize       = -1;                 //只有在表信息不确定的情况下，才需要设定

        
        memset(sOracleID,  0, sizeof(sOracleID));
        memset(sOracleUID, 0, sizeof(sOracleUID));
        memset(sOraclePWD, 0, sizeof(sOraclePWD));
        
        memset(sLocalIP, 0, sizeof(sLocalIP));
        memset(sPeerIP, 0, sizeof(sPeerIP));

        memset(m_sRepFileName,0,sizeof(m_sRepFileName));
        m_iRepFileDataPos = -1;
        memset(m_sRepSecondFileName,0,sizeof(m_sRepSecondFileName));
        m_iRepSecondFileDataPos = -1;
        
        iLocalPort     = DEFAULT_PORT;
        iPeerPort      = DEFAULT_PORT;  
		for(int i = 0; i<MAX_AGENT_PORT_COUNTS; i++)
		{
			iAgentPort[i]     = -1;
		}

        iLogLevel       = 0;
        bDiskSpaceStat = true;
        m_bIsOraRep = false;
        m_bIsRep = false;
        m_bLoadFromDisk = false;
        m_bLoadFromDiskOK = false;

        memset(m_arrRouterToCapture,0,sizeof(m_arrRouterToCapture));
        m_iLSN = 0;
        m_iTimeDifferent = 0;
        m_iOraRepCounts = 1;
        m_bDiscDisasterLink = false;

		m_iSessionID = 0;
		
    }

    void TMdbDSN::Show(bool bIfMore)
    {
        if(bIfMore == true)
        {//详细信息
            printf("DSN =[ %s ] Start\n", sName);
            printf("|    sVersion      = %-20s |  sName          = %s \n", sVersion,sName);
            printf("|    sCreateTime   = %-20s |  sCurTime       = %s\n", sCreateTime,sCurTime);
            printf("|    iRepAttr      = %-20d |  iRoutingID     = %d\n", iRepAttr,iRoutingID);
            printf("|    iTableCounts  = %-20d |  iMutexCounts   = %d\n", iTableCounts,iMutexCounts);

            printf("|    cState        = %c(0-DB_unused, U-DB_running, L-DB_loading, R-DB_repping, S-DB_stop)\n", cState);
            printf("|    iLogLevel     = %d\n ", iLogLevel);
            
            printf("|\n");
            printf("|    iProcAddr     = %-20zd \n", iProcAddr);
            printf("|    iPageMutexAddr= %-20zd |  iUserTableAddr = %zd\n", iPageMutexAddr,iUserTableAddr);
            printf("|    TableSpaceAddr= %-20zd  \n", iTableSpaceAddr);
            printf("|    iLocalLinkAddr= %-20zd    |  iRemoteLinkAddr      = %zd\n", iLocalLinkAddr,iRemoteLinkAddr);
            printf("|    iSeqAddr      = %-20zd \n", iSeqAddr);
            
            printf("|\n");
            printf("|    bIsOraRep           = %-12s \n", m_bIsOraRep?"Y":"N");
            printf("|    bIsRep              = %-12s \n", m_bIsRep?"Y":"N");
            printf("|    bIsCaptureRouter    = %-12s \n", m_bIsCaptureRouter?"Y":"N");
            
            printf("|\n");
            printf("|    sDataStore    = %-40s |  iPermSize        = %d\n", sDataStore,iPermSize);
            TMdbSyncArea & tSA = m_arrSyncArea;
            printf("|[%-10s]Dir       = %-40s |  FileSize     = %d\n",tSA.m_sName,tSA.m_sDir,tSA.m_iFileSize);
            printf("|    sStorageDir   = %-40s \n", sStorageDir);
            printf("|\n");
            printf("|    sOracleID     = %-20s \n", sOracleID);
            printf("|    sOracleUID    = %-20s |  sOraclePWD     = %s\n", sOracleUID,sOraclePWD);
            printf("|\n");
            printf("|    sLocalIP      = %-20s |  iLocalPort     = %d\n", sLocalIP,iLocalPort);
            printf("|    sPeerIP       = %-20s |  iPeerPort      = %d\n", sPeerIP,iPeerPort);
			for(int i = 0; i<MAX_AGENT_PORT_COUNTS; i++)
			{
	    		printf("|    iAgentPort[%d]    = %-20d \n", i, iAgentPort[i]);
			}
            //printf("|    iAgentPort    = %-20d \n",iAgentPort);
            printf("|    m_iLSN       = %-20lld | m_iTimeDifferent=%d \n",m_iLSN,m_iTimeDifferent);
            printf("|\n");
            

            printf("Note:\n");
            printf("|    RepAttr: (0-DSN_No_Rep,1-DSN_Ora_Rep,2-DSN_Rep,3-DSN_Two_Rep)\n");
            printf("|    State:   (0-DB_unused, U-DB_running, L-DB_loading, R-DB_repping, S-DB_stop)\n");    
            printf("DSN =[ %s ] Finish\n", sName);
        }
        else
        {
            char sHead[500];
            static bool bfirst = false;
            memset(sHead,0,500);
            if(bfirst == false)
            {
                sprintf(sHead,"%-8s %-5s %-7s %-7s %-8s %-14s %-8s %-11s %-9s %-7s\n",\
                "Name","State","RepAttr","AccAttr","TabCount","CTime","ShmCount","BINShmCount","ProcCount","LeftAdd");
                printf("\n");
                printf("[ DSN-Info ]:\n");
                printf("%s",sHead);
                bfirst = true;
            }
            if(sName[0] !=0 )
            {
                printf("%-8s %-5c %-7d %-7d %-8d %-14s %-8d %-11d\n",\
                    sName,cState,iRepAttr,iRoutingID,iTableCounts,\
                    sCreateTime,iShmCounts,iBaseIndexShmCounts
                    );
            }
                
        }
    }

	int TMdbDSN::GetSessionID()
	{
		m_SessionMutex.Lock(true);
		if(m_iSessionID < 0)
			m_iSessionID=0;

		int iTmp = m_iSessionID;
		m_iSessionID++;
		m_SessionMutex.UnLock(true);
		
		return iTmp;
	}
	
    void TMdbTableSpace::Clear()
    {
        m_bFileStorage = false;
        m_iBlockId = 0;
		m_iChangeBlockId = 0;
        memset(sName, 0, sizeof(sName));
        iPageSize = 16*1024;
        iRequestCounts = 10000;
        iEmptyPages    = 0;
        iTotalPages    = 0;
        iEmptyPageID   = -1;
        cState         = TableSpace_unused;
        memset(sCreateTime, 0, sizeof(sCreateTime));
        tNode.Clear();
    }

    TMdbVarchar::TMdbVarchar()
    {
        Clear();
    }
    TMdbVarchar::~TMdbVarchar()
    {
    
    }
    void TMdbVarchar::Clear()
    {
        iVarcharID= -1;
        iPageSize= 0;
        iTotalPages = 0;
        iFreePageId = 0;
        iFullPageId = 0;
        iBlockId = 0;
		iChangeBlockId = 0;
        tNode.Clear();
    }

    void TMdbTableSpace::Show(bool bIfMore)
    {
        char sHead[500];
        static bool bfirst = false;
        memset(sHead,0,500);
        
        if(bfirst == false)
        {
            sprintf(sHead,"%-22s %-7s %-7s %-3s %-11s %-12s %-12s %-14s %-6s\n",\
            "Name","Size","Counts","Stat","EmptyPages","TotalPages","EmptyPageID","CreateTime", "IsFS");
            printf("\n");
            printf("[ TableSpace-Info ]:\n");
            printf("%s",sHead);
            bfirst = true;
        }
        
        if(sName[0]!=0)
        {
        	char sTemp[22];
        	memset(sTemp,0,22);
        	sprintf(sTemp,"'%s'",sName);
        	printf("%-22s %-7zd %-7d %-4c %-11d %-12d %-12d %-14s %-4s\n",\
        	sTemp,iPageSize,iRequestCounts,cState,iEmptyPages,iTotalPages,iEmptyPageID,sCreateTime, m_bFileStorage?"TRUE":"FALSE");
        }   
        
    }

    void TMdbTableSpace::Print()
    {
        TADD_NORMAL("==============TableSpace-Info=================");
        TADD_NORMAL("    sName          = %s.", sName);
        TADD_NORMAL("    FileStorage          = %s.", m_bFileStorage?"TRUE":"FALSE");
        TADD_NORMAL("    iPageSize      = %d.", iPageSize);
        TADD_NORMAL("    iRequestCounts = %d.", iRequestCounts);
        TADD_NORMAL("    iEmptyPages    = %d.", iEmptyPages);    
        TADD_NORMAL("    iTotalPages    = %d.", iTotalPages);
        TADD_NORMAL("    iEmptyPageID   = %d.", iEmptyPageID);
        TADD_NORMAL("    sCreateTime    = %s.", sCreateTime);
        TADD_NORMAL("    cState         = %c(%c-unused, %c-using, %c-creating, %c-destroying).", cState, TableSpace_unused, TableSpace_using, TableSpace_creating, TableSpace_destroying);
        TADD_NORMAL("==============TableSpace-Info================="); 
    }

    //获取剩余空间
    int TMdbMemQueue::GetFreeSpace()
    {
        if(iPushPos == iPopPos)
        {//全部空
            return iEndPos - iStartPos;
        }
        if(iPushPos > iPopPos)
        {
            return (iEndPos - iStartPos) - (iPushPos - iPopPos);
        }
        if(iPushPos< iPopPos)
        {
            return iPopPos - iPushPos;
        }
        return 0;
    }

    TMdbTSNode::TMdbTSNode()
    {
        Clear(); 
    }


    void TMdbTSNode::Clear()
    {
        iPageStart = -1;  //起始页号
        iPageEnd   = -1;    //结束页号
        iShmID     = INITVAl;      //所属共享内存ID
    	iOffSet    = 0;  //起始页在共享内存中的偏移量
        iNext      = 0;       //下一个节点偏移量   
        memset(bmDirtyPage,0,sizeof(bmDirtyPage));
    }

    void TMdbIndex::Clear()
    {
        memset(sName, 0, sizeof(sName));
        
        for(int i=0; i<MAX_INDEX_COLUMN_COUNTS; ++i)
        {
            iColumnNo[i] = -1;    
        }
        
        iPriority = -1;
        
        m_iIndexType = HT_Unknown;
        m_iAlgoType = INDEX_HASH;
        iMaxLayer = 1;
    }

    const char* TMdbIndex::GetAlgoType()
    {
        std::string sAlgoType;
        switch(m_iAlgoType)
        {
        case INDEX_HASH:
            sAlgoType = "HASH";
            break;
        case INDEX_M_HASH:
            sAlgoType = "Multistep-Hash";
            break;
        case INDEX_TRIE:
            sAlgoType = "Trie";
            break;
        default:
            sAlgoType = "UNKNOWN";
            break;
        }

        return sAlgoType.c_str();
    }

    int TMdbIndex::GetInnerAlgoType(const char* psAlgoName)
    {
        int iAlgoType = 0;
        if(NULL == psAlgoName) return -1;
        
        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psAlgoName, "HASH"))
        {
            iAlgoType = INDEX_HASH;
        }
        else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psAlgoName, "MHASH")
            || 0 == TMdbNtcStrFunc::StrNoCaseCmp(psAlgoName, "Multistep-Hash"))
        {
            iAlgoType = INDEX_M_HASH;
        }
        else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psAlgoName, "Trie"))
        {
            iAlgoType = INDEX_TRIE;
        }
        else
        {
            iAlgoType = -1;
        }

        return iAlgoType;
    }
    void TMdbIndex::Print()
    {
        TADD_NORMAL("===INDEX Start===");
        TADD_NORMAL("    Name     = %s", sName);
        TADD_NORMAL("    Algorithm Type = %d(%d-HT_Unknown, %d-Hash index, %d-multistep hash index, %d-Trie index)", m_iAlgoType, INDEX_UNKOWN, INDEX_HASH, INDEX_M_HASH, INDEX_TRIE);
        TADD_NORMAL("    Type = %d(%d-HT_Unknown, %d-HT_Int, %d-HT_Char)", m_iIndexType, HT_Unknown, HT_Int, HT_Char);
        
        char sName[32] = {0};
        for(int i=0; i<MAX_INDEX_COLUMN_COUNTS; ++i)
        {
            if(iColumnNo[i] > -1)
            {
                if(i == 0)
                    sprintf(sName, "%d", iColumnNo[i]);
                else
                    sprintf(sName+strlen(sName), ", %d",iColumnNo[i]);   
             } 
        }
        TADD_NORMAL("    ColumnNo = %s", sName);
        TADD_NORMAL("    Priority = %d", iPriority);
        TADD_NORMAL("    MaxLayer = %d", iMaxLayer);
        TADD_NORMAL("===INDEX End=====");
    }

    void TMdbPrimaryKey::Clear()
    {
        iColumnCounts = 0;                 //主键需要的列数

        //主键对应的列号
        for(int i=0; i<MAX_PRIMARY_KEY_CC; ++i)
        {
            iColumnNo[i] = -1;
        } 
        memset(m_sTableName, 0, sizeof(m_sTableName));
    }

    void TMdbPrimaryKey::Print()
        {
        TADD_NORMAL("===PrimaryKey Start===");
        TADD_NORMAL("    Table      = %s.", m_sTableName);
        TADD_NORMAL("    iColumnCounts = %d.", iColumnCounts);
        for(int i=0; i<iColumnCounts; ++i)
        {
            TADD_NORMAL("         Column_No = %d.", iColumnNo[i]);
        } 
        TADD_NORMAL("===PrimaryKey End=====");
    }

    void TMdbParameter::Clear()
    {
        iDataType = DT_Unknown;
        memset(sName, 0, sizeof(sName));
        memset(sValue, 0, sizeof(sValue));
        iParameterType = -1;
    }

    void TMdbMTableAttr::Clear()
    {
        memset(sAttrName, 0, sizeof(sAttrName));
        memset(sAttrValue,0,sizeof(sAttrValue));
        bIsExist = false;
    }

    void TvarcharBlock::Clear()
    {
    	memset(sName,0,128);
    	
    	for(int i=0; i<MAX_VARCHAR_SHM_ID; i++)
    	{
    		iShmID[i] = 0;
    	}
    	
    	for(int i=0; i<1000; i++)
    	{
    		tVarchar16[i].Clear();
    		tVarchar32[i].Clear();
    		tVarchar64[i].Clear();
    		tVarchar128[i].Clear();
    		tVarchar256[i].Clear();
    		tVarchar512[i].Clear();
    		tVarchar1024[i].Clear();
    		tVarchar2048[i].Clear();
    		tVarchar4096[i].Clear();
                  tVarchar8192[i].Clear();
    		
    		m_iFreeNode16[i] = -1;
    		m_iFreeNode32[i] = -1;
    		m_iFreeNode64[i] = -1;
    		m_iFreeNode128[i] = -1;
    		m_iFreeNode256[i] = -1;
    		m_iFreeNode512[i] = -1;
    		m_iFreeNode1024[i] = -1;
    		m_iFreeNode2048[i] = -1;
    		m_iFreeNode4096[i] = -1;
                m_iFreeNode8192[i] = -1;
    	}
    	
    	m_iMaxFreeNode16 = 0;
    	m_iMaxFreeNode32 = 0;
    	m_iMaxFreeNode64= 0;
    	m_iMaxFreeNode128 = 0;
    	m_iMaxFreeNode256= 0;
    	m_iMaxFreeNode512= 0;
    	m_iMaxFreeNode1024= 0;
    	m_iMaxFreeNode2048= 0;
    	m_iMaxFreeNode4096= 0;
            m_iMaxFreeNode8192 = 0;
    	
    	m_iNumFreeNode16 = 0;
    	m_iNumFreeNode32 = 0;
    	m_iNumFreeNode64 = 0;
    	m_iNumFreeNode128 = 0;
    	m_iNumFreeNode256 = 0;
    	m_iNumFreeNode512 = 0;
    	m_iNumFreeNode1024 = 0;
    	m_iNumFreeNode2048 = 0;
    	m_iNumFreeNode4096 = 0;	
           m_iNumFreeNode8192 = 0;
    	
    	m_iCurrentPos16 = 0;
    	m_iCurrentPos32 = 0;
    	m_iCurrentPos64 = 0;
    	m_iCurrentPos128 = 0;
    	m_iCurrentPos256 = 0;
    	m_iCurrentPos512 = 0;
    	m_iCurrentPos1024 = 0;
    	m_iCurrentPos2048 = 0;
    	m_iCurrentPos4096 = 0;
           m_iCurrentPos8192 = 0;
    }

    TLCRColm::TLCRColm()
    {
        Clear();
    }

    TLCRColm::~TLCRColm()
    {
        
    }

    void TLCRColm::Clear()
    {
        m_bNull = false;
        m_iType = DT_Unknown;
        m_sColmName.clear();
        m_sColmValue.clear();
    }

    TMdbLCR::TMdbLCR()
    {
        Clear();
    }

    TMdbLCR::~TMdbLCR()
    {
        
    }

    void TMdbLCR::Clear()
    {
        m_iRoutID = DEFALUT_ROUT_ID;
        m_iSqlType = -1;
        m_iLen = 0;
        m_iTimeStamp = 0;
        m_lLsn = 0;
        m_iVersion = 0;
        //m_iSourceId = 0;
        m_iColmLen = 0;
        m_lRowId = 0;
        m_vColms.clear();
        m_vWColms.clear();
        m_sTableName.clear();
        memset(m_sSQL,0,sizeof(m_sSQL));
    }

	int TMdbLCR::GetSelectSQL(TMdbTable* pTable)
	{
		int iRet = 0;
		CHECK_OBJ(pTable);
		memset(m_sSelSQL,0,sizeof(m_sSelSQL));
		char sColm[MAX_SQL_LEN] = {0};
		for(int i = 0; i < pTable->m_tPriKey.iColumnCounts; i++)
		{
			int colNum = pTable->m_tPriKey.iColumnNo[i];
			if(DT_DateStamp == pTable->tColumn[colNum].iDataType)
			{
				snprintf(sColm+strlen(sColm),sizeof(sColm) - strlen(sColm), " %s = to_date(:%s, 'YYYYMMDDHH24MISS') and",pTable->tColumn[colNum].sName,pTable->tColumn[colNum].sName);
			}
			else
			{
				snprintf(sColm+strlen(sColm),sizeof(sColm) - strlen(sColm), " %s = :%s and",pTable->tColumn[colNum].sName,pTable->tColumn[colNum].sName);
			}
		}

		sColm[strlen(sColm) -4] = '\0';//去除最后一个，
		snprintf(m_sSelSQL,sizeof(m_sSelSQL),"select * from %s where %s",m_sTableName.c_str(),sColm);
		TADD_DETAIL("m_sSelSQL = [%s]", m_sSelSQL);
		return iRet;
	}


    int TMdbLCR::GetSQL(TMdbTable* pTable)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        TMdbColumn* pColumn = NULL;
        memset(m_sSQL,0,sizeof(m_sSQL));
        switch(m_iSqlType)
        {
        case TK_INSERT:
            {
                char sColm[MAX_SQL_LEN] = {0};
                char sValue[MAX_VALUE_LEN]= {0};
                std::vector<TLCRColm>::iterator itor = m_vColms.begin();
                for (; itor != m_vColms.end(); ++itor)
                {
                    sprintf(sColm+strlen(sColm)," %s,",itor->m_sColmName.c_str());
                    pColumn = pTable->GetColumnByName(itor->m_sColmName.c_str());
                    CHECK_OBJ(pColumn);
                    if(DT_DateStamp == pColumn->iDataType)
                    {
                        sprintf(sValue+strlen(sValue)," to_date(:%s, 'YYYYMMDDHH24MISS'),",pColumn->sName);
                    }
                    else
                    {
                        sprintf(sValue+strlen(sValue)," :%s,",pColumn->sName);
                    }
                    itor->m_iType = pColumn->iDataType;
                }

                sColm[strlen(sColm) -1] = '\0';//去除最后一个，
                sValue[strlen(sValue) -1]   = '\0';
                sprintf(m_sSQL,"insert into %s (%s) values (%s)",m_sTableName.c_str(),sColm,sValue);

                break;
            }
        case TK_UPDATE:
            {
                char sWhere[MAX_SQL_LEN] = {0};
                char sSet[MAX_SQL_LEN] ={0};
                std::vector<TLCRColm>::iterator itor = m_vColms.begin();
                for(; itor != m_vColms.end(); ++itor)
                {
                    pColumn = pTable->GetColumnByName(itor->m_sColmName.c_str());
                    CHECK_OBJ(pColumn);
                    if(DT_DateStamp == pColumn->iDataType)
                    {
                        sprintf(sSet+strlen(sSet),"  %s=to_date(:%s, 'YYYYMMDDHH24MISS'),",itor->m_sColmName.c_str(),itor->m_sColmName.c_str());
                    }
                    else
                    {
                         sprintf(sSet+strlen(sSet),"  %s=:%s,",itor->m_sColmName.c_str(),itor->m_sColmName.c_str());
                    }
                    itor->m_iType = pColumn->iDataType;
                }

                std::vector<TLCRColm>::iterator itor_where = m_vWColms.begin();
                for(; itor_where != m_vWColms.end(); itor_where++)
                {
                    pColumn = pTable->GetColumnByName(itor_where->m_sColmName.c_str());
                    CHECK_OBJ(pColumn);
                    if(DT_DateStamp == pColumn->iDataType)
                    {
                        sprintf(sWhere+strlen(sWhere),"  %s=to_date(:%s, 'YYYYMMDDHH24MISS') and",itor_where->m_sColmName.c_str(),itor_where->m_sColmName.c_str());
                    }
                    else
                    {
                        sprintf(sWhere+strlen(sWhere),"  %s=:%s and",itor_where->m_sColmName.c_str(),itor_where->m_sColmName.c_str());
                    }
                    itor_where->m_iType = pColumn->iDataType;
                }

                sSet[strlen(sSet) -1]  = '\0';
                sWhere[strlen(sWhere) -4]  = '\0';
                sprintf(m_sSQL,"update %s set %s where %s",m_sTableName.c_str(),sSet,sWhere);
               
                break;
            }
        case TK_DELETE:
            {
                char sWhere[MAX_SQL_LEN] = {0};
                std::vector<TLCRColm>::iterator itor_where = m_vWColms.begin();
                for(; itor_where != m_vWColms.end(); itor_where++)
                {
                    pColumn = pTable->GetColumnByName(itor_where->m_sColmName.c_str());
                    CHECK_OBJ(pColumn);
                    if(DT_DateStamp == pColumn->iDataType)
                    {
                        sprintf(sWhere+strlen(sWhere),"  %s=to_date(:%s, 'YYYYMMDDHH24MISS') and",itor_where->m_sColmName.c_str(),itor_where->m_sColmName.c_str());
                    }
                    else
                    {
                        sprintf(sWhere+strlen(sWhere),"  %s=:%s and",itor_where->m_sColmName.c_str(),itor_where->m_sColmName.c_str());
                    }
                    itor_where->m_iType = pColumn->iDataType;
                }
                sWhere[strlen(sWhere) -4]  = '\0';
                sprintf(m_sSQL,"delete from %s where %s",m_sTableName.c_str(),sWhere);
                break;
            }
        default:
            {
                TADD_ERROR(-1,"Unknown SQL type.");
                return -1;
            }  
        }

        return iRet;
    }

	int TMdbLCR::GetInsertSQL(TMdbTable* pTable, int reptNum, char * sSQL)
	{
		int iRet = 0;
        TMdbColumn* pColumn = NULL;
		char sColm[MAX_SQL_LEN] = {0};
        char sValue[MAX_VALUE_LEN]= {0};
		for (int i = 0; i <= reptNum; i++)
		{
			strncpy(sValue+strlen(sValue), "(", sizeof("("));
        	std::vector<TLCRColm>::iterator itor = m_vColms.begin();
			for (; itor != m_vColms.end(); ++itor)
			{
				pColumn = pTable->GetColumnByName(itor->m_sColmName.c_str());
				CHECK_OBJ(pColumn);
				if(i == 0)
				{
					snprintf(sColm+strlen(sColm),sizeof(sColm)-strlen(sColm)," %s,",itor->m_sColmName.c_str());
					if(DT_DateStamp == pColumn->iDataType)
					{
						snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," to_date(:%s, 'YYYYMMDDHH24MISS'),",pColumn->sName);
					}
					else
					{
						snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," :%s,",pColumn->sName);
					}
				}
				else
				{
					if(DT_DateStamp == pColumn->iDataType)
					{
						snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," to_date(:%s%d, 'YYYYMMDDHH24MISS'),",pColumn->sName,i);
					}
					else
					{
						snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," :%s%d,",pColumn->sName,i);
					}
				}
				itor->m_iType = pColumn->iDataType;
			}
        	sValue[strlen(sValue) -1]   = '\0';
			strncpy(sValue+strlen(sValue), "),", sizeof("),"));
		}                
        sColm[strlen(sColm) -1] = '\0';//去除最后一个，
        sValue[strlen(sValue) -1]   = '\0';
        snprintf(sSQL, MAX_SQL_LEN-strlen(sSQL), "insert into %s (%s) values %s",m_sTableName.c_str(),sColm,sValue);
		return iRet;
	}

	int TMdbLCR::GetUpdateSQL(TMdbTable* pTable, int reptNum, char * sSQL)
	{
		int iRet = 0;
        TMdbColumn* pColumn = NULL;
		char sColm[MAX_SQL_LEN] = {0};
        char sValue[MAX_VALUE_LEN]= {0};
		char sUpdate[MAX_VALUE_LEN]= {0};
		for (int i = 0; i <= reptNum; i++)
		{
			strncpy(sValue+strlen(sValue), "(", sizeof("("));
			
            std::vector<TLCRColm>::iterator itor_where = m_vWColms.begin();
            for(; itor_where != m_vWColms.end(); itor_where++)
            {
                pColumn = pTable->GetColumnByName(itor_where->m_sColmName.c_str());
                CHECK_OBJ(pColumn);
				if(i == 0)
				{
					snprintf(sColm+strlen(sColm),sizeof(sColm)-strlen(sColm)," %s,",itor_where->m_sColmName.c_str());
					if(DT_DateStamp == pColumn->iDataType)
                    {
                        snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," to_date(:%s, 'YYYYMMDDHH24MISS'),",itor_where->m_sColmName.c_str());
                    }
                    else
                    {
                        snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," :%s,",itor_where->m_sColmName.c_str());
                    }
				}
				else
				{
					if(DT_DateStamp == pColumn->iDataType)
                    {
                        snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," to_date(:%s%d, 'YYYYMMDDHH24MISS'),",itor_where->m_sColmName.c_str(),i);
                    }
                    else
                    {
                        snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," :%s%d,",itor_where->m_sColmName.c_str(),i);
                    }
				}
                
                itor_where->m_iType = pColumn->iDataType;
            }
			
        	std::vector<TLCRColm>::iterator itor = m_vColms.begin();
			for (; itor != m_vColms.end(); ++itor)
			{
				pColumn = pTable->GetColumnByName(itor->m_sColmName.c_str());
				CHECK_OBJ(pColumn);
				if(i == 0)
				{
					snprintf(sColm+strlen(sColm),sizeof(sColm)-strlen(sColm)," %s,",itor->m_sColmName.c_str());
					if(DT_DateStamp == pColumn->iDataType)
					{
						snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," to_date(:%s, 'YYYYMMDDHH24MISS'),",pColumn->sName);
					}
					else
					{
						snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," :%s,",pColumn->sName);
					}
					snprintf(sUpdate+strlen(sUpdate),sizeof(sUpdate)-strlen(sUpdate)," %s=values(%s),",pColumn->sName,pColumn->sName);
				}
				else
				{
					if(DT_DateStamp == pColumn->iDataType)
					{
						snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," to_date(:%s%d, 'YYYYMMDDHH24MISS'),",pColumn->sName,i);
					}
					else
					{
						snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," :%s%d,",pColumn->sName,i);
					}
				}
				
				itor->m_iType = pColumn->iDataType;
			}
        	sValue[strlen(sValue) -1]   = '\0';
			strncpy(sValue+strlen(sValue), "),", sizeof("),"));
		}                
        sColm[strlen(sColm) -1] = '\0';//去除最后一个，
        sValue[strlen(sValue) -1]   = '\0';
		sUpdate[strlen(sUpdate) -1] = '\0';
        snprintf(sSQL, MAX_SQL_LEN-strlen(sSQL),"insert into %s (%s) values %s on duplicate key update %s",m_sTableName.c_str(),sColm,sValue,sUpdate);
		return iRet;
	}


    int TMdbLCR::GetDeleteSQL(TMdbTable* pTable, int reptNum, char * sSQL)
    {
        int iRet = 0;
        TMdbColumn* pColumn = NULL;
        char sColm[MAX_SQL_LEN] = {0};
        char sValue[MAX_VALUE_LEN]= {0};
        for (int i = 0; i <= reptNum; i++)
        {
            strncpy(sValue+strlen(sValue), "(", sizeof("("));

            std::vector<TLCRColm>::iterator itor_where = m_vWColms.begin();
            for(; itor_where != m_vWColms.end(); itor_where++)
            {
                pColumn = pTable->GetColumnByName(itor_where->m_sColmName.c_str());
                CHECK_OBJ(pColumn);
                if(i == 0)
                {
                    snprintf(sColm+strlen(sColm),sizeof(sColm)-strlen(sColm)," %s,",itor_where->m_sColmName.c_str());
                    if(DT_DateStamp == pColumn->iDataType)
                    {
                        snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," to_date(:%s, 'YYYYMMDDHH24MISS'),",itor_where->m_sColmName.c_str());
                    }
                    else
                    {
                        snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," :%s,",itor_where->m_sColmName.c_str());
                    }
                }
                else
                {
                    if(DT_DateStamp == pColumn->iDataType)
                    {
                        snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," to_date(:%s%d, 'YYYYMMDDHH24MISS'),",itor_where->m_sColmName.c_str(),i);
                    }
                    else
                    {
                        snprintf(sValue+strlen(sValue),sizeof(sValue)-strlen(sValue)," :%s%d,",itor_where->m_sColmName.c_str(),i);
                    }
                }

                itor_where->m_iType = pColumn->iDataType;
            }

            sValue[strlen(sValue) -1]   = '\0';
            strncpy(sValue+strlen(sValue), "),", sizeof("),"));
        }                
        sColm[strlen(sColm) -1] = '\0';//去除最后一个，
        sValue[strlen(sValue) -1]   = '\0';
        snprintf(sSQL, MAX_SQL_LEN-strlen(sSQL),"delete from %s where (%s) in (%s)",m_sTableName.c_str(),sColm,sValue);
        return iRet;
    }

	int TMdbLCR::GetSQL(TMdbTable* pTable, int reptNum, char * sSQL)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
		memset(sSQL, 0, MAX_SQL_LEN);

        switch(m_iSqlType)
        {
        case TK_INSERT:
            {
                GetInsertSQL(pTable, reptNum, sSQL);
                break;
            }
        case TK_UPDATE:
            {
                GetUpdateSQL(pTable, reptNum, sSQL);
                break;
            }
        case TK_DELETE:
            {
                GetDeleteSQL(pTable, reptNum, sSQL);
                break;
            }
        default:
            {
                TADD_ERROR(-1,"Unknown SQL type.");
                return -1;
            }  
        }
		
        return iRet;
    }

    TMdbDAONode::TMdbDAONode()
    {
        memset(m_sSQL,0,sizeof(m_sSQL));
        m_pQuery = NULL;
    }

    TMdbDAONode::~TMdbDAONode()
    {
        SAFE_DELETE(m_pQuery);
    }

    
    void TMdbDAONode::Clear()
    {
        memset(m_sSQL,0,sizeof(m_sSQL));
        SAFE_DELETE(m_pQuery);
    }
    
    TMdbQuery* TMdbDAONode::CreateDBQuery(char * sSQL, TMdbDatabase * pDatabase,int iFlag)
    {
        if(m_pQuery == NULL)
        {
            int iLen = strlen(sSQL);
            memcpy(m_sSQL,sSQL,iLen);
            m_sSQL[iLen]=0;

            try
            {
                m_pQuery = pDatabase->CreateDBQuery();
                m_pQuery->SetSQL(sSQL,iFlag,0);
            }
            catch(TMdbException& e)
            {
                SAFE_DELETE(m_pQuery);
                TADD_ERROR(ERR_DB_NOT_CONNECTED,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
                TADD_FUNC("Finish.");
                return NULL;
            }
            catch(...)
            {
                SAFE_DELETE(m_pQuery);
                TADD_ERROR(ERR_DB_NOT_CONNECTED,"Unknown error!\n");
                TADD_FUNC("Finish.");
                return NULL;
            }
        }

        return m_pQuery;
    }

    
    TMdbFlushDao::TMdbFlushDao()
    {
        for(int i = 0; i<MAX_FLUSN_DAO_COUNTS;i++)
        {
            m_arrDaoNode[i] = NULL;
        }
    }

    TMdbFlushDao::~TMdbFlushDao()
    {
        for(int i = 0; i<MAX_FLUSN_DAO_COUNTS;i++)
        {
            SAFE_DELETE(m_arrDaoNode[i]);
        }
    }

    TMdbQuery* TMdbFlushDao::CreateDBQuery(int iSqlType, char * sSQL, TMdbDatabase * pDatabase,int iFlag)
    {
        int iPos = 0;       
        if(iSqlType == TK_INSERT)
        {
            iPos = 0;
        }
        else if(iSqlType  == TK_DELETE)
        {
            iPos = 1;
            
        }
        else if(iSqlType == TK_UPDATE)
        {//update 从2开始
            int i = 0;
            TMdbDAONode * pstNode = NULL;
            for(i = 2;i<MAX_FLUSN_DAO_COUNTS;i++)
            {
                pstNode = m_arrDaoNode[i];
                if(NULL != pstNode)
                {
                    if(0 != pstNode->m_sSQL[0] && TMdbNtcStrFunc::StrNoCaseCmp(pstNode->m_sSQL,sSQL) == 0)
                    {//这条sql语句已被构造
                        break;
                    }
                }
                else
                {//找不到，新建一个
                    break;
                }
            }
            if(i >= MAX_FLUSN_DAO_COUNTS)
            {//满了,随机取一个节点清空掉
                srand((unsigned)time(0));
                i = rand()%(MAX_FLUSN_DAO_COUNTS-3) +2;
                SAFE_DELETE(m_arrDaoNode[i]);
            }
            iPos = i;
        }

        if(NULL == m_arrDaoNode[iPos])
        {
            m_arrDaoNode[iPos] = new TMdbDAONode();
            if(NULL == m_arrDaoNode[iPos])
            {
                TADD_ERROR(-1,"new dao node failed.");
                return  NULL;
            }
        }
        return m_arrDaoNode[iPos]->CreateDBQuery(sSQL,pDatabase,iFlag);
    }

    TMdbFlushEngine::TMdbFlushEngine()
    {
        m_pDatabase = NULL;
    }

    TMdbFlushEngine::~TMdbFlushEngine()
    {
        SAFE_DELETE(m_pDatabase);
        m_tMapDao.Clear();
    }

    int TMdbFlushEngine::Init(char * sDsn)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        m_tMapDao.SetAutoRelease(true);
        try
        {
            m_pDatabase = new(std::nothrow) TMdbDatabase();
            CHECK_OBJ(m_pDatabase);
            if(m_pDatabase->ConnectAsMgr(sDsn) == false)
            {
                CHECK_RET(ERR_DB_NOT_CONNECTED,"ConnectAsMgr[%s] error.",sDsn);
            }
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERR_DB_NOT_CONNECTED,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
        }
        catch(...)
        {
            CHECK_RET(ERR_DB_NOT_CONNECTED,"Unknown error!\n");
        } 
        TADD_FUNC("Finish.");
        return iRet;
    }

    TMdbQuery* TMdbFlushEngine::CreateDBQuery(int iSqlType, const char* sTableName,char * sSQL,int iFlag)
    {
        if(m_pDatabase == NULL)
        {
            TADD_ERROR(-1,"db link error.");
            return NULL;
        }
        TMdbFlushDao *pFlushDao = (TMdbFlushDao *)m_tMapDao.FindData(sTableName);
        if(pFlushDao == NULL )
        {
            pFlushDao= new(std::nothrow)TMdbFlushDao();
            if(pFlushDao == NULL)
            {
                TADD_ERROR(-1,"new flush dao failed.");
                return NULL;
            }
            m_tMapDao.Add(sTableName, pFlushDao);
        }        
        return pFlushDao->CreateDBQuery(iSqlType,sSQL,m_pDatabase,iFlag);
    }

    int TMdbFlushEngine::Excute(TMdbLCR & tMdbLcr,int iFlag, int * iDropIndex, bool bIsInvalidUpdate)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TMdbQuery* pMdbQuery = NULL;
        char sSQL[MAX_SQL_LEN];
        iRet = GetSQL(sSQL,tMdbLcr,iDropIndex, bIsInvalidUpdate);
		if(iRet == -1)
		{
			TADD_ERROR(iRet,"GetSQL Faild");
			return iRet;
		}
		else if(iRet == 1)
		{
			return 0;
		}
		
        pMdbQuery = CreateDBQuery(tMdbLcr.m_iSqlType, tMdbLcr.m_sTableName.c_str(), sSQL,iFlag);
        CHECK_OBJ(pMdbQuery);
        CHECK_RET(SetParameter(pMdbQuery,tMdbLcr,iDropIndex),"SetParameter Faild");
        try
        {
            pMdbQuery->Execute();
            pMdbQuery->Commit();
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(-1,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
            iRet = ERROR_UNKNOWN;
        }
        catch(...)
        {
            TADD_ERROR(-1, "Unknown error!\n");   
            iRet = ERROR_UNKNOWN;    
        } 
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbFlushEngine::GetSQL(char* sSQL, TMdbLCR & tMdbLcr, int * iDropIndex, bool bIsInvalidUpdate)
    {
        int iRet = 0;
        memset(sSQL,0,MAX_SQL_LEN);
		int paramIndex = 0;
        switch(tMdbLcr.m_iSqlType)
        {
        case TK_INSERT:
            {
                char sColm[MAX_SQL_LEN] = {0};
                char sValue[MAX_VALUE_LEN]= {0};
                std::vector<TLCRColm>::iterator itor = tMdbLcr.m_vColms.begin();
                for (; itor != tMdbLcr.m_vColms.end(); ++itor)
                {
                	if(iDropIndex[paramIndex++] == 1)
            		{
						continue;
            		}
					snprintf(sColm+strlen(sColm), sizeof(sColm) - strlen(sColm)," %s,",itor->m_sColmName.c_str());
					snprintf(sValue+strlen(sValue), sizeof(sValue) - strlen(sValue)," :%s,",itor->m_sColmName.c_str());
                }

                sColm[strlen(sColm) -1] = '\0';//去除最后一个，
                sValue[strlen(sValue) -1]   = '\0';
                sprintf(sSQL, "insert into %s (%s)values(%s)",tMdbLcr.m_sTableName.c_str(),sColm,sValue);
				TADD_DETAIL("sSQL = [%s]", sSQL);
                break;
            }
        case TK_UPDATE:
            {
				
                if(bIsInvalidUpdate) return 1;
                char sWhere[MAX_SQL_LEN] = {0};
                char sSet[MAX_SQL_LEN] ={0};
                std::vector<TLCRColm>::iterator itor = tMdbLcr.m_vColms.begin();
                for(; itor != tMdbLcr.m_vColms.end(); ++itor)
                {
                	if(iDropIndex[paramIndex++] == 1)
            		{
						continue;
            		}
                    snprintf(sSet+strlen(sSet), sizeof(sSet) - strlen(sSet),"  %s=:%s,",itor->m_sColmName.c_str(),itor->m_sColmName.c_str());
                }

                std::vector<TLCRColm>::iterator itor_where = tMdbLcr.m_vWColms.begin();
                for(; itor_where != tMdbLcr.m_vWColms.end(); itor_where++)
                {
                    if(strlen(sWhere) > 0)
                    {
                        snprintf(sWhere+strlen(sWhere), sizeof(sWhere) - strlen(sWhere),"  and %s=:%s ",itor_where->m_sColmName.c_str(),itor_where->m_sColmName.c_str());
                    }
                    else
                    {
                        snprintf(sWhere+strlen(sWhere), sizeof(sWhere) - strlen(sWhere),"  %s=:%s ",itor_where->m_sColmName.c_str(),itor_where->m_sColmName.c_str());
                    }
                    
                }

                sSet[strlen(sSet) -1]  = '\0';
                sprintf(sSQL,"update %s set %s where %s",tMdbLcr.m_sTableName.c_str(),sSet,sWhere);
				TADD_DETAIL("sSQL = [%s]", sSQL);
                break;
            }
        case TK_DELETE:
            {
                char sWhere[MAX_SQL_LEN] = {0};
                std::vector<TLCRColm>::iterator itor_where = tMdbLcr.m_vWColms.begin();
                for(; itor_where != tMdbLcr.m_vWColms.end(); itor_where++)
                {
                    if(strlen(sWhere) > 0)
                    {
                        snprintf(sWhere+strlen(sWhere), sizeof(sWhere) - strlen(sWhere)," and %s=:%s ",itor_where->m_sColmName.c_str(),itor_where->m_sColmName.c_str());
                    }
                    else
                    {
                        snprintf(sWhere+strlen(sWhere), sizeof(sWhere) -strlen(sWhere),"  %s=:%s ",itor_where->m_sColmName.c_str(),itor_where->m_sColmName.c_str());
                    }
                    
                }
                //sWhere[strlen(sWhere) -1]  = '\0';
                sprintf(sSQL, "delete from %s where %s",tMdbLcr.m_sTableName.c_str(),sWhere);
				TADD_DETAIL("sSQL = [%s]", sSQL);
                break;
            }
        default:
            {
                TADD_ERROR(-1,"Unknown SQL type.");
                return -1;
            }  
        }

        return iRet;
    }

    int TMdbFlushEngine::SetParameter(TMdbQuery* pMdbQuery,TMdbLCR & tMdbLcr,int * iDropIndex)
    {
        int iRet = 0;
        int iCount = 0;
		int iParamIndex = 0;
        
        try
        {
            std::vector<TLCRColm>::iterator itor = tMdbLcr.m_vColms.begin();
            for(; itor != tMdbLcr.m_vColms.end(); ++itor )
            {
            	if(iDropIndex[iParamIndex++] == 1)
        		{
					continue;
        		}
                if(itor->m_bNull)
				{
				  pMdbQuery->SetParameterNULL(iCount);
				}
				else
				{
				  pMdbQuery->SetParameter(iCount,itor->m_sColmValue.c_str());
				}
				iCount ++;
            }

            itor = tMdbLcr.m_vWColms.begin();
            for(; itor != tMdbLcr.m_vWColms.end(); ++itor )
            {
                if(itor->m_bNull)
                  {
                      
                      pMdbQuery->SetParameterNULL(iCount);
                  }
                  else
                  {
                      pMdbQuery->SetParameter(iCount,itor->m_sColmValue.c_str());
                  }
                  iCount ++;
            }
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
            iRet = ERROR_UNKNOWN;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN, "Unknown error!\n");   
            iRet = ERROR_UNKNOWN;    
        } 
        return iRet;
    }
//}
