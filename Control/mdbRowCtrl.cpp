/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbRowCtrl.h
*@Description： 内存数据库的记录级管理
*@Author:		jin.shaohua
*@Date：	    2013年1月29日
*@History:
******************************************************************************************/

#include "Control/mdbRowCtrl.h"
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    /******************************************************************************
    * 函数名称	:  TMdbColumnNullFlag
    * 函数描述	:  构造
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbColumnNullFlag::TMdbColumnNullFlag()
    {


    }
     TMdbColumnNullFlag::~TMdbColumnNullFlag()
     {

     }
     /******************************************************************************
     * 函数名称  :  CalcNullFlag
     * 函数描述  :  计算null标识
     * 输入      :
     * 输入      :
     * 输出      :
     * 返回值    :  0 - 成功!0 -失败
     * 作者      :
     *******************************************************************************/
    int TMdbColumnNullFlag::CalcNullFlag(int iColumnPos)
    {
        m_iColumnPos = iColumnPos;
        m_iNullFlagOffset = iColumnPos/MDB_CHAR_SIZE;
        char sMask[] = {128,64,32,16,8,4,2,1};
        m_cNullFlag = sMask[iColumnPos%MDB_CHAR_SIZE];
        return 0; 
    }

    /******************************************************************************
    * 函数名称	:  TMdbRowCtrl
    * 函数描述	:  构造
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbRowCtrl::TMdbRowCtrl():
        m_pMdbTable(NULL),
        m_pShmDsn(NULL),
        m_arrColNullFlag(NULL)
    {
        int i = 0;
        for(i = 0;i < MAX_COLUMN_COUNTS;++i)
        {
            m_pArrColValueBlock[i] = NULL;
        }
    }
    /******************************************************************************
    * 函数名称	:  ~TMdbRowCtrl
    * 函数描述	:  析构
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbRowCtrl::~TMdbRowCtrl()
    {
        SAFE_DELETE_ARRAY(m_arrColNullFlag);
        ClearColValueBlock();
    }
    /******************************************************************************
    * 函数名称	:  AttachTable
    * 函数描述	:  链接表
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    int TMdbRowCtrl::Init(const char * sDsn,const char * sTableName)//初始化
    {
        int iRet = 0;
        m_pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDsn);
        m_pMdbTable = m_pShmDsn->GetTableByName(sTableName);
        CHECK_OBJ(m_pMdbTable);
        CHECK_RET(m_tVarcharCtrl.Init(sDsn),"varchar Init faild.");
        //计算null flag
        SAFE_DELETE_ARRAY(m_arrColNullFlag);
        m_arrColNullFlag = new TMdbColumnNullFlag[m_pMdbTable->iColumnCounts];
        int i =0;
        for(i = 0;i < m_pMdbTable->iColumnCounts;++i)
        {
            if(i != m_pMdbTable->tColumn[i].iPos)
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"column[%s] of table[%s] pos=[%d] is error,should be [%d]",
                    m_pMdbTable->tColumn[i].sName,m_pMdbTable->sTableName,m_pMdbTable->tColumn[i].iPos,i);
            }
            m_arrColNullFlag[i].CalcNullFlag(i);
        }
        //清理临时区
        ClearColValueBlock();
        return iRet;
    }

    int TMdbRowCtrl::Init(const char * sDsn,TMdbTable* pTable)//初始化
    {
        int iRet = 0;
        m_pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDsn);
        CHECK_OBJ(pTable);
        m_pMdbTable = pTable;		
        CHECK_RET(m_tVarcharCtrl.Init(sDsn),"varchar Init faild.");
        //计算null flag
        SAFE_DELETE_ARRAY(m_arrColNullFlag);
        m_arrColNullFlag = new TMdbColumnNullFlag[m_pMdbTable->iColumnCounts];
        int i =0;
        for(i = 0;i < m_pMdbTable->iColumnCounts;++i)
        {
            if(i != m_pMdbTable->tColumn[i].iPos)
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"column[%s] of table[%s] pos=[%d] is error,should be [%d]",
                    m_pMdbTable->tColumn[i].sName,m_pMdbTable->sTableName,m_pMdbTable->tColumn[i].iPos,i);
            }
            m_arrColNullFlag[i].CalcNullFlag(i);
        }
        //清理临时区
        ClearColValueBlock();
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ClearColValueBlock
    * 函数描述	:  清理临时区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbRowCtrl::ClearColValueBlock()
    {
        int i = 0;
        for(i = 0;i < MAX_COLUMN_COUNTS;++i)
        {
            SAFE_DELETE_ARRAY(m_pArrColValueBlock[i]);
        }
        return 0;
    }
    /******************************************************************************
    * 函数名称	:  GetColValueBlockByPos
    * 函数描述	:  根据column -pos 获取记录临时区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    char * TMdbRowCtrl::GetColValueBlockByPos(int iPos)
    {
        if(iPos < 0 || iPos >= MAX_COLUMN_COUNTS){return NULL;}
        if(NULL == m_pArrColValueBlock[iPos])
        {
            m_pArrColValueBlock[iPos] = new char[MAX_BLOB_LEN];
        }
        m_pArrColValueBlock[iPos][0] = 0;
        return m_pArrColValueBlock[iPos];
    }


    /******************************************************************************
    * 函数名称	:  SetColumnNull
    * 函数描述	:  设置null值
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    int TMdbRowCtrl::SetColumnNull(TMdbColumn * const & pColumn,char* const & pDataAddr)
    {
        TADD_FUNC("Start.column[%s].",pColumn->sName);
        int iRet = 0;
        CHECK_OBJ(pColumn);
        CHECK_OBJ(pDataAddr);
        CHECK_OBJ(m_arrColNullFlag);
        if(false == pColumn->m_bNullable)
        {
            CHECK_RET(ERR_SQL_DATA_TYPE_INVALID,"column [%s] not support null value",pColumn->sName);
        }
       // char sMask[] = {128,64,32,16,8,4,2,1};
       // char * sNull = pDataAddr + m_pMdbTable->iOneRecordNullOffset + pColumn->iPos/MDB_CHAR_SIZE;
       // *sNull |=sMask[pColumn->iPos%MDB_CHAR_SIZE];
        char * sNull  = pDataAddr + m_pMdbTable->iOneRecordNullOffset + m_arrColNullFlag[pColumn->iPos].m_iNullFlagOffset;
        *sNull |= m_arrColNullFlag[pColumn->iPos].m_cNullFlag;
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ClearColumnNULL
    * 函数描述	:  清理Null数据
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    int TMdbRowCtrl::ClearColumnNULL(TMdbColumn * const & pColumn,char* const & pDataAddr)
    {
        TADD_FUNC("Start.column[%s].",pColumn->sName);
        int iRet = 0;
        CHECK_OBJ(pColumn);
        CHECK_OBJ(pDataAddr);
         CHECK_OBJ(m_arrColNullFlag);
        if(m_pMdbTable->iOneRecordNullOffset <= 0)
        {
            TADD_DETAIL("table[%s] not support null value",m_pMdbTable->sTableName);
            return 0;
        }
        //char sMask[] = {127,191,223,239,247,251,253,254};
        //char * sNull = pDataAddr + m_pMdbTable->iOneRecordNullOffset + pColumn->iPos/MDB_CHAR_SIZE;
        //*sNull &=sMask[pColumn->iPos%MDB_CHAR_SIZE];

        char * sNull  = pDataAddr + m_pMdbTable->iOneRecordNullOffset + m_arrColNullFlag[pColumn->iPos].m_iNullFlagOffset;
        *sNull &= (m_arrColNullFlag[pColumn->iPos].m_cNullFlag^(char)255);
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  IsColumnNull
    * 函数描述	:  是否是null数据
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    bool TMdbRowCtrl::IsColumnNull(TMdbColumn * const & pColumn,const char*  pDataAddr)
    {
        TADD_FUNC("Start.column[%s].",pColumn->sName);
        if(NULL == pColumn || NULL == pDataAddr || false == pColumn->m_bNullable || NULL == m_arrColNullFlag)
        {
            TADD_DETAIL("IsNULL(%s)","false");
            return false;
        }
        if(m_pMdbTable->iOneRecordNullOffset <= 0)
        {
            TADD_DETAIL("table[%s] not support null value",m_pMdbTable->sTableName);
            return false;
        }
        //char sMask[] = {128,64,32,16,8,4,2,1};
        //const char * sNull = pDataAddr + m_pMdbTable->iOneRecordNullOffset + pColumn->iPos/MDB_CHAR_SIZE;
        //return 0 != (*sNull & sMask[pColumn->iPos%MDB_CHAR_SIZE]);
        const char * sNull = pDataAddr + m_pMdbTable->iOneRecordNullOffset + m_arrColNullFlag[pColumn->iPos].m_iNullFlagOffset;
        TADD_DETAIL("IsNULL(%s)",0 != (*sNull & m_arrColNullFlag[pColumn->iPos].m_cNullFlag) ? "true":"false");
        return 0 != (*sNull & m_arrColNullFlag[pColumn->iPos].m_cNullFlag);
    }
    /******************************************************************************
    * 函数名称	:  FillOneColumn
    * 函数描述	:  填充某列
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    int TMdbRowCtrl::FillOneColumn(char* const & pDataAddr,TMdbColumn * const & pColumn,ST_MEM_VALUE * const & pstMemValue,int iFillType)
    {
        int iRet = 0;
        CHECK_OBJ(pColumn);
        TADD_FUNC("Start.pDataAddr=[%p],Column[%s],type[%d],offset[%d]",pDataAddr,pColumn->sName,pColumn->iDataType,pColumn->iOffSet);
        int iOffSet = pColumn->iOffSet;
        //null值判定
        if(true == pstMemValue->IsNull())
        {
            CHECK_RET(SetColumnNull(pColumn,pDataAddr),"[%s]SetColumnNull failed.",pColumn->sName);
			switch(pColumn->iDataType)
		    {
			    case DT_Int:
			    case DT_Char:
			    case DT_DateStamp:
			    {
				    pDataAddr[iOffSet] = 0;
			    }
			    break;
			    case DT_Blob:
			    case DT_VarChar:
			    {
		        
			    }
			    break;
			    default:
			        CHECK_RET(ERR_SQL_FILL_MDB_INFO,"table[%s],pColumn[%s] type[%d] is error.",
			                    m_pMdbTable->sTableName,pColumn->sName,pColumn->iDataType);
			        break;
			}
            return 0;//NULL设置完就结束了。
        }
        else
        {
            //不是Null需要清空Null判定
            CHECK_RET(ClearColumnNULL(pColumn,pDataAddr),"[%s]ClearColumnNULL failed.",pColumn->sName);
        }
        switch(pColumn->iDataType)
        {
        case DT_Int:
        {
            long long* pInt = (long long*)&pDataAddr[iOffSet];
            *pInt = pstMemValue->lValue;
            TADD_DETAIL("Fill Column[%lld].",pstMemValue->lValue);
        }
        break;
        case DT_Char:
        {
            SAFESTRCPY(&pDataAddr[iOffSet], MAX_VALUE_LEN-iOffSet ,pstMemValue->sValue);
            TADD_DETAIL("Fill Column[%s].",pstMemValue->sValue);
        }
        break;
        case DT_DateStamp:
        {
            TADD_DETAIL("Fill Column[%s].",pstMemValue->sValue);
            if(false == TMdbNtcStrFunc::IsDateTime(pstMemValue->sValue))
            {//时间校验
                 CHECK_RET(ERR_SQL_FILL_MDB_INFO,"column[%s] invalid type time value [%s].",
                                                    pColumn->sName,pstMemValue->sValue);
            }
            switch(m_pMdbTable->m_cZipTimeType)
            {
                case 'Y':
                    { //int压缩时间存储
                        int * pInt = (int*)&pDataAddr[iOffSet];
                        if(TMdbDateTime::BetweenDate(pstMemValue->sValue,"19700000000000","20370000000000"))
                        {
                            *pInt = (int)TMdbDateTime::StringToTime(pstMemValue->sValue,m_pShmDsn->GetInfo()->m_iTimeDifferent);
                            TADD_DETAIL("after date zip Fill Column[%d].",*pInt);
                        }
                        else
                        {
                            CHECK_RET(ERR_SQL_FILL_MDB_INFO,"column[%s] invalid zip-time value [%s],not in[19700000000000,20370000000000].",
                                                            pColumn->sName,pstMemValue->sValue);
                        }
                    }
                    break;
                case 'L':
                   { //long long 压缩时间
                        long long * pLonglong = (long long*)&pDataAddr[iOffSet];
                         * pLonglong = TMdbDateTime::StringToTime(pstMemValue->sValue,m_pShmDsn->GetInfo()->m_iTimeDifferent);
                        TADD_DETAIL("after date zip Fill Column[%lld].",* pLonglong);
                    }
                    break;
                default://普通方式非压缩
                    SAFESTRCPY(&pDataAddr[iOffSet],pstMemValue->iSize,pstMemValue->sValue);
                    break;
            }
        }
        break;
        case DT_Blob:
        case DT_VarChar:
        {
            
            TADD_DETAIL("Fill Column[%s].",pstMemValue->sValue);
            do
            {
                int iWhichPos = -1;
                unsigned int iRowId = 0;
                m_tVarcharCtrl.GetStoragePos(pDataAddr+iOffSet, iWhichPos, iRowId);
                TMdbTableSpace * pTablespace = m_pShmDsn->GetTableSpaceAddrByName(m_pMdbTable->m_sTableSpace);
                char cStorage = pTablespace->m_bFileStorage?'Y':'N';
                if(iWhichPos >= VC_16 && iWhichPos <=VC_8192)
                {//有数据，更新该位置上的数据
                	
					
                    CHECK_RET(m_tVarcharCtrl.Update(pstMemValue->sValue, iWhichPos, iRowId,cStorage),"Update Varchar Faild,ColoumName=[%s],iVarCharlen[%d]",pstMemValue->pColumnToSet->sName,strlen(pstMemValue->sValue))
                    //whichpos 和 rowid可能发生改变，因此这里需要重新设置一下
                    m_tVarcharCtrl.SetStorgePos(iWhichPos, iRowId, pDataAddr+iOffSet);
                }
                else
                {
                    CHECK_RET(m_tVarcharCtrl.Insert(pstMemValue->sValue, iWhichPos, iRowId,cStorage),"Insert Varchar Faild,ColoumName=[%s],iVarCharlen[%d]",pstMemValue->pColumnToSet->sName,strlen(pstMemValue->sValue))
                    m_tVarcharCtrl.SetStorgePos(iWhichPos, iRowId, pDataAddr+iOffSet);
                }
                
            }while(0);
            break;
        }
        default:
            CHECK_RET(ERR_SQL_FILL_MDB_INFO,"pColumn[%s] type[%d] is error.",pColumn->sName,pColumn->iDataType);
            break;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetOneColumnValue
    * 函数描述	:  获取某列值
    * 输入		:  iResultType - Mem_Str,Mem_Int,Mem_NULL
    * 输入		:iValueSize <=  ->尽量使用内部值
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    int TMdbRowCtrl::GetOneColumnValue(char* pDataAddr,TMdbColumn * const & pColumn,long long & llValue,
                                                                       char * & sValue,int iValueSize,int & iResultType)
    {
        TADD_FUNC("Start.column[%s].",pColumn->sName);
        int iRet = 0;
        CHECK_OBJ(pColumn);
        if(IsColumnNull(pColumn,pDataAddr) == true)
        {//空值
            iResultType = MEM_Null;
            return 0;
        }
        int iOffset = pColumn->iOffSet;
        //根据数据类型，转换成对应的字符串
        switch(pColumn->iDataType)
        {
        case DT_Int:  //Integer
        {
            llValue = *(long long*)&pDataAddr[iOffset];
            iResultType = MEM_Int;
            TADD_FLOW("column[%s].value=[%lld]",pColumn->sName,llValue);
        }
        break;
        case DT_Char:     //Char
        {
            if(iValueSize <= 0)
            {
                sValue = pDataAddr + iOffset;
            }
            else
            {
                SAFESTRCPY(sValue,iValueSize,&pDataAddr[iOffset]);
            }
            iResultType = MEM_Str;
            TADD_FLOW("column[%s].value=[%s]",pColumn->sName,sValue);
            break;
        }
        case DT_VarChar:  //VarChar
        case DT_Blob:
        {
            if(iValueSize <=0 ){sValue = GetColValueBlockByPos(pColumn->iPos);}
            //如果地址信息为空，则不处理
            /*if(sValue[0] == 0) 
            {
                TADD_WARNING("varchar address = [%s]",sValue);
                break;
            }*/
            CHECK_RET(m_tVarcharCtrl.GetVarcharValue(sValue,pDataAddr+iOffset),"column[%s],offset[%d] GetVarcharValue failed.",
                            pColumn->sName,iOffset);
            iResultType = MEM_Str;
            TADD_FLOW("column[%s].value=[%s]",pColumn->sName,sValue);
            break;
        }
        case DT_DateStamp: //时间
        {
            if(iValueSize <= 0)
            {//没有值
                if('Y' ==m_pMdbTable->m_cZipTimeType )
                {
                    sValue = GetColValueBlockByPos(pColumn->iPos);
                    int* pInt = (int*)&pDataAddr[iOffset];
                    TMdbDateTime::TimeToString(*pInt,sValue);
                }
                else if('L' ==m_pMdbTable->m_cZipTimeType )
                {
                    sValue = GetColValueBlockByPos(pColumn->iPos);
                    long long * pLong= (long long*)&pDataAddr[iOffset];
                    TMdbDateTime::TimeToString(*pLong,sValue);
                }
                else
                {
                    sValue = pDataAddr + iOffset;
                }
            }
            else
            {
                if('Y' ==m_pMdbTable->m_cZipTimeType )
                {
                    int* pInt = (int*)&pDataAddr[iOffset];
                    TMdbDateTime::TimeToString(*pInt,sValue);
                }
                else if('L' ==m_pMdbTable->m_cZipTimeType )
                {
                    long long * pLong = (long long *)&pDataAddr[iOffset];
                    TMdbDateTime::TimeToString(*pLong,sValue);
                }
                else
                {
                    SAFESTRCPY(sValue,DATE_TIME_SIZE,&pDataAddr[iOffset]);
                    if(0 == sValue[0])
                    {//若是空值，可能这条记录被删除了给一个默认日期值
                        TADD_WARNING("date value is empty,may be record is deleted,use date[19890101010101]");
                        SAFESTRCPY(sValue,DATE_TIME_SIZE,"19890101010101");
                    }
                }
            }
            iResultType = MEM_Str;
            TADD_FLOW("column[%s].value=[%s]",pColumn->sName,sValue);
            break;
        }
        default:
            CHECK_RET(ERR_SQL_TYPE_INVALID,"column[%s] DataType=%d invalid.",pColumn->sName,pColumn->iDataType);
            break;
        }
        return iRet;
    }

    int TMdbRowCtrl::SetTimeStamp(char* const & pDataAddr, int iOffSet,long long iTimeStamp)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

		if(NULL == pDataAddr)
		{
			return iRet;
		}
		
        if(iOffSet <= 0)
        {
            iRet = ERROR_UNKNOWN;
            TADD_ERROR(iRet, "TimeStamp[%d] offset invalid", iOffSet);
            return iRet;
        }

        long long* piTimeStamp = (long long*)&pDataAddr[iOffSet];
        *piTimeStamp = iTimeStamp;
        TADD_FLOW("set timestamp[%lld].",iTimeStamp);

        return iRet;
    }

    int TMdbRowCtrl::GetTimeStamp(char* pDataAddr, int iOffSet,long long & iTimeStamp)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

		if(NULL == pDataAddr)
		{
			return iRet;
		}
		
        if(iOffSet <= 0)
        {
            iRet = ERROR_UNKNOWN;
            TADD_ERROR(iRet, "TimeStamp[%d] offset invalid", iOffSet);
            return iRet;
        }

        iTimeStamp = *(long long*)&pDataAddr[iOffSet];
        TADD_FLOW("iTimeStamp=[%lld]",iTimeStamp);

        return iRet;
    }

    
//}


