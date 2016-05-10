/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbStorageEngine.cpp		
*@Description： mdb存储引擎
*@Author:			jin.shaohua
*@Date：	    2013.7
*@History:
******************************************************************************************/
#include "Control/mdbStorageEngine.h"

#include "Control/mdbMgrShm.h"

#include "Control/mdbExecuteEngine.h"
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"


//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{

#define ArraySize(X) (sizeof(X)/sizeof(X[0]))

    /******************************************************************************
    * 函数名称  :  Clear
    * 函数描述  :  清理
    * 输入      :  
    * 输出      :  
    * 返回值    :  
    * 作者      :  dong.chun
    *******************************************************************************/
    void TMdbTSFileHead::Clear()
    {
        memset(m_sTSName, 0, sizeof(m_sTSName));
        m_iStartPage = 0;
        m_iEndPage = 0;
        m_iPageOffset = 0;
        m_iCheckPointLSN = -1;
        iVarcharID = -1;
        m_iPageSize = 0;
		m_iPageCount = 0;
    }

    /******************************************************************************
    * 函数名称  :  ToString
    * 函数描述  :  输出
    * 输入      :  
    * 输出      :  
    * 返回值    :  
    * 作者      :  dong.chun
    *******************************************************************************/
    std::string TMdbTSFileHead::ToString()
    {
        char sTemp[1024] = {0};
        sprintf(sTemp+strlen(sTemp),"TableSpace=[%s]\n",m_sTSName);
        sprintf(sTemp+strlen(sTemp),"Page offset=[%lld],StartPage = [%d],EndPage = [%d]\n",m_iPageOffset,m_iStartPage,m_iEndPage);
        std::string sRet = sTemp;
        return sRet;
    }

	StorageFileHandle::StorageFileHandle()
	{
		
	}

	StorageFileHandle::~StorageFileHandle()
	{
        
	}
	
	StorageFile::StorageFile()
	{
		memset(m_sTSName,0,sizeof(m_sTSName));
        m_iVarcharId = -1;
	}

	StorageFile::~StorageFile()
	{
	}
	
    void StorageFile::clear()
    {
        memset(m_sTSName,0,sizeof(m_sTSName));
        m_iVarcharId = -1;
        for(int i = 0;i < m_vStorageFile.size();i++)
        {
            FILE* pfp = m_vStorageFile[i].m_fFile;
            SAFE_CLOSE(pfp);
        }
		m_vStorageFile.clear();
    }


    /******************************************************************************
    * 函数名称	:  TMdbFileBuff
    * 函数描述	:  文件批量读取器
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  dong.chun
    *******************************************************************************/
    TMdbFileBuff::TMdbFileBuff()
    {
        m_pBuff = NULL;
        m_iSize = 0;
        m_pFile = NULL;
        m_iBuffSize = 0;
        iReadPageCount = 0;
    }

    TMdbFileBuff::~TMdbFileBuff()
    {
        SAFE_DELETE(m_pBuff);
    }

    int TMdbFileBuff::Init(int iSize,FILE* pFile)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pFile);
        m_pFile = pFile;
        m_iSize = iSize;
        SAFE_DELETE(m_pBuff);
        m_iBuffSize = 10000*(m_iSize);
        m_pBuff = new char[m_iBuffSize];
        CHECK_OBJ(m_pBuff);
        memset(m_pBuff,0,m_iBuffSize);
        iReadPageCount = 0;
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbFileBuff::ReadFromFile()
    {
        if(iReadPageCount > 0){return iReadPageCount;}
        if(feof(m_pFile))
        {//文件结束
            iReadPageCount = 0;
            return iReadPageCount;
        }
        iReadPageCount = fread(m_pBuff,m_iSize,10000,m_pFile);
        if(iReadPageCount <= 0)
        {
        	TADD_ERROR(ERR_APP_INVALID_PARAM, "fread failed,errno=%d,errstr=%s",errno,strerror(errno));
            return ERR_APP_INVALID_PARAM;
        }
        return iReadPageCount;
    }

    char* TMdbFileBuff::Next()
    {
        size_t iOffSet = (--iReadPageCount)*m_iSize;
		//TMdbPage * pPage = (TMdbPage *)(&m_pBuff[iOffSet]);
		//TADD_NORMAL("iReadPageCount = [%d], iOffSet = [%d], pPage->m_iPageID = [%d], pPage->m_iPageSize = [%d]", iReadPageCount, iOffSet, pPage->m_iPageID, pPage->m_iPageSize);
        if(iOffSet > m_iBuffSize-m_iSize)
        {
            TADD_ERROR(-1,"iOffSet[%d] error.,buff[%d],PageSize[%d]",iOffSet,m_iBuffSize,m_iSize);
            return NULL;
        }
        return &m_pBuff[iOffSet];
    }

    TMdbVarcharFile::TMdbVarcharFile()
    {
        m_pVarchar = NULL;
        m_pVarcharFile = NULL;
        m_pShmDSN = NULL;
    }

    /******************************************************************************
    * 函数名称	:  TMdbVarchar
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  dong.chun
    *******************************************************************************/
    TMdbVarcharFile::~TMdbVarcharFile()
    {
        SAFE_CLOSE(m_pVarcharFile);
    }
    /******************************************************************************
    * 函数名称	:  FlushFull
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbVarcharFile::LinkFile(const char * sFile)
    {
        int iRet = 0;
        CHECK_OBJ(sFile);
        if(TMdbNtcFileOper::IsExist(sFile) == false)
        {
            CHECK_RET(ERR_OS_NO_FILE,"not find file[%s]",sFile);
        }
        SAFE_CLOSE(m_pVarcharFile);
        m_pVarcharFile = fopen(sFile,"rb+");//不需要关闭，一直link住
        CHECK_OBJ(m_pVarcharFile);
        //读取文件头
        if(fread(&m_VarcharHead,sizeof(m_VarcharHead),1,m_pVarcharFile) != 1)
        {
            if(feof(m_pVarcharFile))
            {//文件结束
                CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
            }
            CHECK_RET(ERR_OS_READ_FILE,"fread(%s) errno=%d, errormsg=[%s].",sFile, errno, strerror(errno));
        }
        TADD_DETAIL("Link Varchar File[%d]",m_VarcharHead.iVarcharID);
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  FlushFull
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbVarcharFile::LinkMdb(const char * sDsn, TMdbVarchar * pVarchar)
    {
        int iRet = 0;
        CHECK_OBJ(sDsn);CHECK_OBJ(pVarchar);
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        m_pVarchar = pVarchar;
        CHECK_RET(m_VarCharCtrl.Init(sDsn),"Init failed.");
        CHECK_RET(m_tPageCtrl.SetDSN(sDsn),"Init failed.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  FlushFull
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbVarcharFile::FlushFull(int iStartPageId)
    {
        TADD_FUNC("Start");
        int iRet = 0;

        CHECK_OBJ(m_pShmDSN);
        CHECK_OBJ(m_pVarchar);
        if(m_pVarchar->iTotalPages <= 0)
        {
            return iRet;
        }
        long long iFileSize = 0;
		char sStorageFile[MAX_NAME_LEN] = {0};
        snprintf(sStorageFile,sizeof(sStorageFile),"%s/%s%d_%d%s",m_pShmDSN->GetInfo()->sStorageDir,VARCHAR_FILE_PREFIX,m_pVarchar->iVarcharID,m_pVarchar->iBlockId,TS_FILE_SUFFIX);
        if(TMdbNtcFileOper::IsExist(sStorageFile))
        {
            CHECK_RET(ERR_APP_FILE_IS_EXIST,"File[%s] is exist,cannot flush full.",sStorageFile);
        }
        TADD_NORMAL("Flush to [%s]",sStorageFile);
        SAFE_CLOSE(m_pVarcharFile);
        m_pVarcharFile = fopen(sStorageFile,"wb+");
        CHECK_OBJ(m_pVarcharFile);
		m_VarcharHead.Clear();
        m_VarcharHead.iVarcharID = m_pVarchar->iVarcharID;
        m_VarcharHead.m_iPageSize = m_pVarchar->iPageSize;
        m_VarcharHead.m_iStartPage = iStartPageId;
		m_VarcharHead.m_iPageCount = 0;
        m_VarcharHead.m_iPageOffset = sizeof(TMdbTSFileHead);
		CHECK_RET(fseek(m_pVarcharFile, sizeof(m_VarcharHead), SEEK_SET), "fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        int iPageID = iStartPageId;
        for(;iPageID <= m_pVarchar->iTotalPages; ++iPageID)
        {
            if(iFileSize + m_pVarchar->iPageSize < 1024*1024*1024)
            {
                TMdbPage * pPage = (TMdbPage *)m_VarCharCtrl.GetAddrByPageID(m_pVarchar,iPageID);
                CHECK_OBJ(pPage);
                m_tPageCtrl.Attach((char *)pPage,false,true);
                m_tPageCtrl.WLock();
                do{
                    if(fwrite(pPage, pPage->m_iPageSize, 1, m_pVarcharFile)!= 1 )
                    {
                        CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite(%s) errno=%d, errormsg=[%s].",sStorageFile, errno, strerror(errno));
                    }
                }while(0);
                m_tPageCtrl.UnWLock();
                m_VarcharHead.m_iEndPage = iPageID;
				m_VarcharHead.m_iPageCount++;
				iFileSize += m_pVarchar->iPageSize;
            }
            else
            {
                CHECK_RET(fseek(m_pVarcharFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
		        if(fwrite(&m_VarcharHead, sizeof(m_VarcharHead), 1, m_pVarcharFile)!= 1 )
		        {
		            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
		        }
                fflush(m_pVarcharFile);
                m_pVarchar->iBlockId++;
                TADD_NORMAL("Flush [%s] Finished",sStorageFile);
                CHECK_RET(FlushFull(iPageID),"FlushFull Failed,page[%d]",iPageID);
				SAFE_CLOSE(m_pVarcharFile);
                return iRet;
            }
        }
        CHECK_RET(fseek(m_pVarcharFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        if(fwrite(&m_VarcharHead, sizeof(m_VarcharHead), 1, m_pVarcharFile)!= 1 )
        {
            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
        fflush(m_pVarcharFile);
        m_pVarchar->iBlockId++;
		SAFE_CLOSE(m_pVarcharFile);
        TADD_NORMAL("Flush [%s] Finished",sStorageFile);
        TADD_FUNC("Finish");
        
        return iRet;
    }
    
    /******************************************************************************
    * 函数名称	:  FlushHead
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbVarcharFile::FlushHead()
    {
        int iRet = 0;
        CHECK_OBJ(m_pVarcharFile);
        //刷出文件头
        CHECK_RET(fseek(m_pVarcharFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        if(fwrite(m_pVarchar, sizeof(TMdbVarchar), 1, m_pVarcharFile)!= 1 )
        {
            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  FlushHead
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbVarcharFile::StartToReadPage()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pVarcharFile);
        CHECK_RET(fseek(m_pVarcharFile,sizeof(TMdbVarchar),SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        CHECK_RET(m_tFileBuff.Init(m_pVarchar->iPageSize, m_pVarcharFile),"m_tFileBuff Init Failed");
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  FlushHead
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbVarcharFile::GetNextPage(TMdbPage * & pMdbPage)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        iRet = m_tFileBuff.ReadFromFile();
        if(iRet <= 0) {return iRet;}//文件结束
        pMdbPage = NULL;
        pMdbPage = (TMdbPage*)m_tFileBuff.Next();
        if(pMdbPage == NULL)
        {
            TADD_ERROR(-1,"GetNextPage Failed");
            iRet = -1;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    
    /******************************************************************************
    * 函数名称	:  FlushHead
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbVarcharFile::FlushDirtyPage()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pVarchar);CHECK_OBJ(m_pVarcharFile);
        TMdbTSNode * pTSNode = &(m_pVarchar->tNode);
        CHECK_OBJ(pTSNode);
        //刷出文件头
        CHECK_RET(FlushHead(),"FlushHead failed.");
        //刷出页数据
        CHECK_RET(fseek(m_pVarcharFile,sizeof(TMdbVarchar),SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        int iPreFlushPageID = pTSNode->iPageStart - 1;//记录之前一次的页ID，用来做文件指针偏移
        //遍历所有表空间节点，找出脏页
        int iDirtyPageID = 0;
        int iMixPageLSN = -1;
        int iDirtyCount = 0;
        int iPageCount = 0;
        while(NULL != pTSNode)
        {
            int iRealBMSize = (pTSNode->iPageEnd - pTSNode->iPageStart)/8;
            iRealBMSize = iRealBMSize < MAX_BIT_MAP_SIZE ? iRealBMSize : MAX_BIT_MAP_SIZE;
            iPageCount = pTSNode->iPageEnd;
            int i = 0;
            unsigned char cTemp = (0x0);
            for(i = 0; i < iRealBMSize;++i)
            {//先一个字节一个字节的比较
                if(0 != (pTSNode->bmDirtyPage[i] | cTemp))
                {//若有脏页再按位比较
                    int j = 0;
                    for(j = 0;j < 8;++j)
                    {
                        cTemp =  (0x1<<j);
                        if(0 != (pTSNode->bmDirtyPage[i] & cTemp))
                        {//找到脏页
                            iDirtyCount ++;
                            iDirtyPageID = i*8+j+pTSNode->iPageStart;
                            TADD_NORMAL("Find dirty page=[%d]",iDirtyPageID);
                            TMdbPage *pPage = (TMdbPage *)m_VarCharCtrl.GetAddrByPageID(m_pVarchar,iDirtyPageID);
                            CHECK_OBJ(pPage);
                            //移动文件指针
                            TADD_NORMAL("iDirtyPageID=[%d],iPreFlushPageID=[%d]",iDirtyPageID,iPreFlushPageID);
                            CHECK_RET(fseek(m_pVarcharFile,(iDirtyPageID - iPreFlushPageID -1)*pPage->m_iPageSize,SEEK_CUR),
                                "fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
                            iPreFlushPageID = iDirtyPageID;
                            m_tPageCtrl.Attach((char *)pPage,false,true);
                            CHECK_RET(m_tPageCtrl.WLock(),"wlock failed.");
                            do{
                                if(fwrite(pPage, pPage->m_iPageSize, 1, m_pVarcharFile)!= 1 )
                                {
                                    CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
                                }
                                TADD_NORMAL("fwrite dirty page=[%d]",pPage->m_iPageID);
                            }while(0);
                            //取较小的page_LSN
                            if(iMixPageLSN < 0){
                                iMixPageLSN = pPage->m_iPageLSN;
                            }else{
                                iMixPageLSN = iMixPageLSN <  pPage->m_iPageLSN ? iMixPageLSN:pPage->m_iPageLSN;
                            }
                            pTSNode->bmDirtyPage[i] ^= cTemp;//清理脏页标识
                            m_tPageCtrl.UnWLock();
                        }
                    }
                }
            }
            pTSNode = m_VarCharCtrl.GetNextNode(pTSNode);
        }
        TADD_NORMAL("Flush VarChar[%d] DirtyPage=[%d],PageCount=[%d],iMixPageLSN=[%d]",m_pVarchar->iVarcharID,iDirtyCount,iPageCount,iMixPageLSN);
        CHECK_RET(FlushHead(),"FlushHead failed."); //刷出文件头
        fflush(m_pVarcharFile);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  FlushHead
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbVarcharFile::GetVarCharLen(int iPos,int& iVarCharLen)
    {
        int iRet = 0;
        switch(iPos)
        {
          case 0:
            iVarCharLen= 16;
            break;
          case 1:
            iVarCharLen= 32;
            break;
          case 2:
            iVarCharLen= 64;
            break;
          case 3:
            iVarCharLen= 128;
            break;
          case 4:
            iVarCharLen= 256;
            break;
          case 5:
            iVarCharLen= 512;
            break;
          case 6:
            iVarCharLen= 1024;
            break;
          case 7:
            iVarCharLen= 2048;
            break;
          case 8:
            iVarCharLen= 4096;
            break;
          case 9:
            iVarCharLen= 8192;
            break;
        default:
            CHECK_RET(ERR_DB_PRIVILEGE_INVALID,"ERROR VARCHAR POS[%d]",iPos);
            break;
        };
        return iRet;
    }

    TMdbTSFile::TMdbTSFile():
    m_pTSFile(NULL),
    m_pShmDSN(NULL),
    m_pMdbTS(NULL)
    {

    }
    /******************************************************************************
    * 函数名称  :  
    * 函数描述  :  
    * 输入      :  
    * 输出      :  
    * 返回值    :  
    * 作者      :  jin.shaohua
    *******************************************************************************/
    TMdbTSFile::~TMdbTSFile()
    {
        SAFE_CLOSE(m_pTSFile);
    }

    /******************************************************************************
    * 函数名称  :  LinkFile
    * 函数描述  :  读取文件头
    * 输入      :  
    * 输出      :  
    * 返回值    :  
    * 作者      :  jin.shaohua
    *******************************************************************************/
    int TMdbTSFile::LinkFile(const char * sFile)
    {
        int iRet = 0;
        CHECK_OBJ(sFile);
        if(TMdbNtcFileOper::IsExist(sFile) == false)
        {
            CHECK_RET(ERR_OS_NO_FILE,"not find file[%s]",sFile);
        }
        SAFE_CLOSE(m_pTSFile);
        m_pTSFile = fopen(sFile,"rb+");//不需要关闭，一直link住
        CHECK_OBJ(m_pTSFile);
        //读取文件头
        if(fread(&m_tTSFileHead,sizeof(m_tTSFileHead),1,m_pTSFile) != 1)
        {
            if(feof(m_pTSFile))
            {//文件结束
                CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
            }
            CHECK_RET(ERR_OS_READ_FILE,"fread(%s) errno=%d, errormsg=[%s].",sFile, errno, strerror(errno));
        }
        TADD_DETAIL("Link File[%s]",m_tTSFileHead.ToString().c_str());
        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  LinkMdb
    * 函数描述  :  链接mdb
    * 输入      :  
    * 输出      :  
    * 返回值    :  
    * 作者      :  jin.shaohua
    *******************************************************************************/
    int TMdbTSFile::LinkMdb(const char * sDsn,TMdbTableSpace *pMdbTS)
    {
        int iRet = 0;
        CHECK_OBJ(sDsn);CHECK_OBJ(pMdbTS);
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        m_pMdbTS    = pMdbTS;
        CHECK_RET(m_tTSCtrl.Init(sDsn,pMdbTS->sName),"Init failed.");
        CHECK_RET(m_tPageCtrl.SetDSN(sDsn),"Init failed.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  FlushFull
    * 函数描述  :  刷出表空间和表结构
    * 输入      :  
    * 输出      :  
    * 返回值    :  
    * 作者      :  dong.chun
    *******************************************************************************/
    int TMdbTSFile::FlushFull(int iStartPageId)
    {
        TADD_FUNC("Start");
        int iRet = 0;
        CHECK_OBJ(m_pMdbTS);
        char sStorageFile[MAX_FILE_NAME] = {0};
        snprintf(sStorageFile,sizeof(sStorageFile),"%s/%s%s_%d%s",m_pShmDSN->GetInfo()->sStorageDir,TS_FILE_PREFIX,m_pMdbTS->sName,m_pMdbTS->m_iBlockId,TS_FILE_SUFFIX);
        if(TMdbNtcFileOper::IsExist(sStorageFile))
        {
            CHECK_RET(ERR_APP_FILE_IS_EXIST,"File[%s] is exist,cannot flush full.",sStorageFile);
        }
        TADD_NORMAL("Flush to [%s]",sStorageFile);
        int iTotalPageCount = 1;
        long long iFileSize = 0;
        FILE* pTSFile = NULL;
        SAFE_CLOSE(pTSFile);
        pTSFile = fopen(sStorageFile,"wb+");
        CHECK_OBJ(pTSFile);
        m_tTSFileHead.Clear();
        //文件指针空出一个头文件位置
        CHECK_RET(fseek(pTSFile,sizeof(m_tTSFileHead),SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        CHECK_RET(m_pMdbTS->tEmptyMutex.Lock(true),"lock failed.");
        do{
        //记录表空间信息
        SAFESTRCPY(m_tTSFileHead.m_sTSName, sizeof(m_tTSFileHead.m_sTSName), m_pMdbTS->sName);
        TMdbTSNode * pTSNode = &(m_pMdbTS->tNode);
         do{
            iTotalPageCount = pTSNode->iPageEnd;//记录页个数
         }while((pTSNode = m_tTSCtrl.GetNextNode(pTSNode)) != NULL);
        //标记数据页偏移
        m_tTSFileHead.m_iPageOffset = sizeof(m_tTSFileHead);
        m_tTSFileHead.m_iPageSize = m_pMdbTS->iPageSize;
        m_tTSFileHead.m_iStartPage = iStartPageId;
		m_tTSFileHead.m_iPageCount = 0;
        }while(0);
        CHECK_RET(m_pMdbTS->tEmptyMutex.UnLock(true),"unlock failed.");
        //刷出所有页数据
        int iPageID = iStartPageId;
        for(;iPageID <= iTotalPageCount; ++iPageID)
        {
            if(iFileSize + m_pMdbTS->iPageSize < 1024*1024*1024)
            {
                TMdbPage * pPage = (TMdbPage *)m_tTSCtrl.GetAddrByPageID(iPageID);
                CHECK_OBJ(pPage);
                m_tPageCtrl.Attach((char *)pPage,false,true);
                CHECK_RET(m_tPageCtrl.WLock(),"wlock failed.");
                do{
                    if(fwrite(pPage, pPage->m_iPageSize, 1, pTSFile)!= 1 )
                    {
                        CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite(%s) errno=%d, errormsg=[%s].",sStorageFile, errno, strerror(errno));
                    }
                }while(0);
                m_tPageCtrl.UnWLock();
                m_tTSFileHead.m_iEndPage = iPageID;
                iFileSize+=m_pMdbTS->iPageSize;
				m_tTSFileHead.m_iPageCount++;
                TADD_DETAIL("pPage id = [%d],table_name=[%s]",pPage->m_iPageID,pPage->m_sTableName);
            }
            else
            {
                 //刷出本文件的头
                CHECK_RET(fseek(pTSFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
                if(fwrite(&m_tTSFileHead, sizeof(m_tTSFileHead), 1, pTSFile)!= 1 )
                {
                    CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
                }
                fflush(pTSFile);
                m_pMdbTS->m_iBlockId++;
                TADD_NORMAL("Flush [%s] Finished",m_tTSFileHead.ToString().c_str());
                CHECK_RET(FlushFull(iPageID),"FlushFull Failed,page[%d]",iPageID);
                SAFE_CLOSE(pTSFile);
                return iRet;
            }
        }
        CHECK_RET(fseek(pTSFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        if(fwrite(&m_tTSFileHead, sizeof(m_tTSFileHead), 1, pTSFile)!= 1 )
        {
            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
        fflush(pTSFile);
        m_pMdbTS->m_iBlockId++;
        SAFE_CLOSE(pTSFile);
        TADD_NORMAL("Flush [%s] Finished",m_tTSFileHead.ToString().c_str());
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  StartToReadPage
    * 函数描述	:  开始读取页数据
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbTSFile::StartToReadPage()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pTSFile);
        CHECK_RET(fseek(m_pTSFile,m_tTSFileHead.m_iPageOffset,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        CHECK_RET(m_tFileBuff.Init(m_tTSFileHead.m_iPageSize, m_pTSFile),"m_tFileBuff Init Failed");
        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  GetNextPage
    * 函数描述	:  获取下一页。
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbTSFile::GetNextPage(TMdbPage * &pMdbPage)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        iRet = m_tFileBuff.ReadFromFile();
        TADD_DETAIL("Cur_PageCount=[%d].",iRet);
        if(iRet <= 0) {return iRet;}//文件结束
        pMdbPage = NULL;
        pMdbPage = (TMdbPage*)m_tFileBuff.Next();
        if(pMdbPage == NULL)
        {
            TADD_ERROR(-1,"GetNextPage Failed");
            iRet = -1;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  FlushDirtyPage
    * 函数描述	:  刷出脏页,采用二次写方式
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbTSFile::FlushDirtyPage(int iDirtyPageID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //移动到数据头
        CHECK_RET(fseek(m_pTSFile,m_tTSFileHead.m_iPageOffset,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        TMdbPage *pPage = (TMdbPage *)m_tTSCtrl.GetAddrByPageID(iDirtyPageID);
        CHECK_OBJ(pPage);
        //移动文件指针
        CHECK_RET(fseek(m_pTSFile,(iDirtyPageID - m_tTSFileHead.m_iStartPage)*pPage->m_iPageSize,SEEK_CUR),
            "fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        m_tPageCtrl.Attach((char *)pPage,false,true);
        m_tPageCtrl.WLock();
        do{
            if(fwrite(pPage, pPage->m_iPageSize, 1, m_pTSFile)!= 1 )
            {
                CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
            }
        }while(0);
         m_tTSFileHead.m_iCheckPointLSN =  m_tTSFileHead.m_iCheckPointLSN >= pPage->m_iPageLSN ? m_tTSFileHead.m_iCheckPointLSN:pPage->m_iPageLSN;        
        CHECK_RET(FlushHead(),"FlushHead failed."); //刷出文件头
        fflush(m_pTSFile);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  FlushHead
    * 函数描述	:  刷出文件头
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbTSFile::FlushHead()
    {
        int iRet = 0;
        CHECK_OBJ(m_pTSFile);
        //刷出文件头
        CHECK_RET(fseek(m_pTSFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        if(fwrite(&m_tTSFileHead, sizeof(m_tTSFileHead), 1, m_pTSFile)!= 1 )
        {
            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
        return iRet;
    }

    TMdbCheckPoint::TMdbCheckPoint()
    {
        
    }

    TMdbCheckPoint::~TMdbCheckPoint()
    {
        m_vTSFile.clear();
        m_vVarCharFile.clear();
		m_vTSChangeFile.clear();
		m_vVarCharChangeFile.clear();
    }

    int TMdbCheckPoint::Init(char * sDsn)
    {
        int iRet = 0;
        CHECK_OBJ(sDsn);
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDSN);
        CHECK_RET(m_tPageCtrl.SetDSN(sDsn),"Init PageCtrl failed");
        CHECK_RET(m_VarCharCtrl.Init(sDsn),"Init failed.");
        m_vTSFile.clear();
        m_vVarCharFile.clear();
		m_vTSChangeFile.clear();
		m_vVarCharChangeFile.clear();
        TMdbDSN* pDSN = m_pShmDSN->GetInfo();
        m_iMaxLsn = pDSN->GetCurrentLSN();
        return iRet;
    }

    int TMdbCheckPoint::Init(TMdbTableSpace* pTS)
    {
        int iRet = 0;
        CHECK_OBJ(m_pShmDSN);
        CHECK_OBJ(pTS);
        m_pTS = pTS;
        CHECK_RET(m_TSCtrl.Init(m_pShmDSN->GetInfo()->sName,pTS->sName),"Init failed.");
        m_iCurPos = 0;
		m_iCurChangePos = 0;
        int iFileCount = m_vTSFile.size();
        for(; m_iCurPos<iFileCount;m_iCurPos++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_vTSFile[m_iCurPos].m_sTSName, pTS->sName) == 0){break;}
        }
		int iChangeFileCount = m_vTSChangeFile.size();
        for(; m_iCurChangePos<iChangeFileCount;m_iCurChangePos++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_vTSChangeFile[m_iCurChangePos].m_sTSName, pTS->sName) == 0){break;}
        }
        if(m_iCurPos >= iFileCount)
        {
            CHECK_RET(-1,"Not Find File,TS[%s]",pTS->sName)
        }
        if(m_iCurChangePos >= iChangeFileCount)
        {
            CHECK_RET(-1,"Not Find Change File,TS[%s]",pTS->sName)
        }
        return iRet;
    }

    int TMdbCheckPoint::Init(TMdbVarchar *pVarchar)
    {
        int iRet = 0;
        CHECK_OBJ(pVarchar);
        m_pVarchar = pVarchar;
        m_iCurPos = 0;
		m_iCurChangePos = 0;
        int iFileCount = m_vVarCharFile.size();
		if(iFileCount == 0) return iRet;
        for(; m_iCurPos<iFileCount;m_iCurPos++)
        {
            if(pVarchar->iVarcharID == m_vVarCharFile[m_iCurPos].m_iVarcharId){break;}
        }

        if(m_iCurPos >= iFileCount)
        {
            CHECK_RET(-1,"Not Find Varchar File[%d]",pVarchar->iVarcharID);
        }
        int iChangeFileCount = m_vVarCharChangeFile.size();
		if(iChangeFileCount == 0) return iRet;
        for(; m_iCurChangePos<iChangeFileCount;m_iCurChangePos++)
        {
            if(pVarchar->iVarcharID == m_vVarCharChangeFile[m_iCurChangePos].m_iVarcharId){break;}
        }

        if(m_iCurChangePos >= iChangeFileCount)
        {
            CHECK_RET(-1,"Not Find Varchar Change File[%d]",pVarchar->iVarcharID);
        }
        return iRet;
    }

	bool TMdbCheckPoint::NeedFlushFile()
    {
		bool bNeedFlush = false;
		char changeOKFile[MAX_NAME_LEN] = {0};
		snprintf(changeOKFile,sizeof(changeOKFile),"%s/%s",m_pShmDSN->GetInfo()->sStorageDir,"CHANGE.OK");
		if(TMdbNtcFileOper::IsExist(changeOKFile))
		{
			bNeedFlush = true;
		}
		return bNeedFlush;
	}
	
    bool TMdbCheckPoint::NeedLinkFile()
    {
        if(m_pShmDSN == NULL)
        {
            TADD_ERROR(-1,"m_pShmDSN is NULL");
            return true;
        }
        
        if(m_tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir) != 0)
        {
            TADD_ERROR(-1,"m_tFileList Init Failed");
            return true;
        }

        int iFileCount = 0;
        for(unsigned int i = 0; i< m_vTSFile.size();i++)
        {
            iFileCount += m_vTSFile[i].m_vStorageFile.size();
        }

        m_tFileList.GetFileList(1,0,TS_FILE_PREFIX,TS_FILE_SUFFIX);
        if(m_tFileList.GetFileCounts() != iFileCount)
        {
            return true;
        }

        iFileCount = 0;
        for(unsigned int i = 0; i< m_vVarCharFile.size();i++)
        {
            iFileCount += m_vVarCharFile[i].m_vStorageFile.size();
        }

        m_tFileList.GetFileList(1,0,VARCHAR_FILE_PREFIX,TS_FILE_SUFFIX);
        if(m_tFileList.GetFileCounts() != iFileCount)
        {
            return true;     
        }

		int iChangeFileCount = 0;
        for(unsigned int i = 0; i< m_vTSChangeFile.size();i++)
        {
            iChangeFileCount += m_vTSChangeFile[i].m_vStorageFile.size();
        }

        m_tFileList.GetFileList(1,0,TS_FILE_PREFIX,TS_CHANGE_FILE_SUFFIX);
        if(m_tFileList.GetFileCounts() != iChangeFileCount)
        {
            return true;
        }

        iChangeFileCount = 0;
        for(unsigned int i = 0; i< m_vVarCharChangeFile.size();i++)
        {
            iChangeFileCount += m_vVarCharChangeFile[i].m_vStorageFile.size();
        }

        m_tFileList.GetFileList(1,0,VARCHAR_FILE_PREFIX,TS_CHANGE_FILE_SUFFIX);
        if(m_tFileList.GetFileCounts() != iChangeFileCount)
        {
            return true;     
        }
        return false;
    }

    int TMdbCheckPoint::LinkStorageFile(const char * sTableSpaceName)
    {
        int iRet = 0;
        TADD_FUNC("Start");
        CHECK_OBJ(m_pShmDSN);
        for(unsigned int i = 0; i<m_vTSFile.size();i++)
        {
            m_vTSFile[i].clear();
        }
        m_vTSFile.clear();
        for(unsigned int i = 0; i<m_vVarCharFile.size();i++)
        {
            m_vVarCharFile[i].clear();
        }
        m_vVarCharFile.clear();
        CHECK_RET(m_tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
        m_tFileList.Clear();
        char sFileHead[MAX_NAME_LEN];
        char sFileName[MAX_NAME_LEN];
		char sName[MAX_NAME_LEN];
        //连接表空间文件
        TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            if(itor->m_bFileStorage == false){continue;}
            if(sTableSpaceName != NULL && TMdbNtcStrFunc::StrNoCaseCmp(sTableSpaceName,itor->sName) != 0)
            {//如果设置了表空间，就只连接这个表空间的文件
                continue;
            }
            StorageFile tempFile;
            SAFESTRCPY(tempFile.m_sTSName, sizeof(tempFile.m_sTSName), itor->sName);
            memset(sFileHead,0,sizeof(sFileHead));
			memset(sFileName, 0, sizeof(sFileName));
            snprintf(sFileHead,sizeof(sFileHead),"%s%s",TS_FILE_PREFIX,itor->sName);
            m_tFileList.GetFileList(1,0,sFileHead,TS_FILE_SUFFIX);
            while(m_tFileList.Next(sFileName) == 0)
            {
            	char *ptrFileName =  NULL;
				char *ptrPos =  NULL;
				memset(sName,0,sizeof(sName));

				if(m_tFileList.GetFileNameFromFullFileName(ptrFileName) != 0)
					continue;

				CHECK_OBJ(ptrFileName);
				ptrPos = strrchr(ptrFileName,'_');
				if(ptrPos == NULL)
					continue;
				int iLen = ptrPos-ptrFileName;
				if(iLen<sizeof(sName))
					strncpy(sName,ptrFileName,iLen);

				if(strcmp(sName,sFileHead) == 0)
				{
					StorageFileHandle tempSFHandle;
            		strncpy(tempSFHandle.m_sFileName, sFileName, sizeof(tempSFHandle.m_sFileName));
                	FILE * pFile = NULL;

                	pFile = fopen(sFileName,"rb+");//不需要关闭，一直link住
                	CHECK_OBJ(pFile);
					tempSFHandle.m_fFile = pFile;
                	tempFile.m_vStorageFile.push_back(tempSFHandle);
				}
            }
            m_vTSFile.push_back(tempFile);
            if(sTableSpaceName != NULL) {break;}//找到指定表空间，就退出
        }

        //连接varchar文件
        TShmList<TMdbVarchar>::iterator itor_var = m_pShmDSN->m_VarCharList.begin();
        for(;itor_var != m_pShmDSN->m_VarCharList.end();++itor_var)
        {
            StorageFile tempFile;
            tempFile.m_iVarcharId = itor_var->iVarcharID;
            memset(sFileHead,0,sizeof(sFileHead));
			memset(sFileName, 0, sizeof(sFileName));
            snprintf(sFileHead,sizeof(sFileHead),"%s%d",VARCHAR_FILE_PREFIX,itor_var->iVarcharID);
            m_tFileList.GetFileList(1,0,sFileHead,TS_FILE_SUFFIX);
            while(m_tFileList.Next(sFileName) == 0)
            {
                StorageFileHandle tempSFHandle;
            	strncpy(tempSFHandle.m_sFileName, sFileName, sizeof(tempSFHandle.m_sFileName));
                FILE * pFile = NULL;
                pFile = fopen(sFileName,"rb+");//不需要关闭，一直link住
                CHECK_OBJ(pFile);
				tempSFHandle.m_fFile = pFile;
                tempFile.m_vStorageFile.push_back(tempSFHandle);
            }
            m_vVarCharFile.push_back(tempFile);
        }

        TADD_NORMAL("clear end ts_size=[%d],varchar_size=[%d]",m_vTSFile.size(),m_vVarCharFile.size());
        return iRet;
    }

	int TMdbCheckPoint::LinkChangeFile(const char * sTableSpaceName)
    {
        int iRet = 0;
        TADD_FUNC("Start");
        CHECK_OBJ(m_pShmDSN);
		for(unsigned int i = 0; i<m_vTSChangeFile.size();i++)
        {
            m_vTSChangeFile[i].clear();
        }
        m_vTSChangeFile.clear();
        for(unsigned int i = 0; i<m_vVarCharChangeFile.size();i++)
        {
            m_vVarCharChangeFile[i].clear();
        }
        m_vVarCharChangeFile.clear();
        CHECK_RET(m_tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
        m_tFileList.Clear();
        char sFileHead[MAX_NAME_LEN];
        char sFileName[MAX_NAME_LEN];
		char sName[MAX_NAME_LEN];
        //连接表空间文件
		TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            if(itor->m_bFileStorage == false){continue;}
            if(sTableSpaceName != NULL && TMdbNtcStrFunc::StrNoCaseCmp(sTableSpaceName,itor->sName) != 0)
            {//如果设置了表空间，就只连接这个表空间的文件
                continue;
            }
            StorageFile tempFile;
            SAFESTRCPY(tempFile.m_sTSName, sizeof(tempFile.m_sTSName), itor->sName);
            memset(sFileHead,0,sizeof(sFileHead));
			memset(sFileName, 0, sizeof(sFileName));
            snprintf(sFileHead,sizeof(sFileHead),"%s%s",TS_FILE_PREFIX,itor->sName);
            m_tFileList.GetFileList(1,0,sFileHead,TS_CHANGE_FILE_SUFFIX);

            while(m_tFileList.Next(sFileName) == 0)
            {
            	char *ptrFileName =  NULL;
				char *ptrPos =  NULL;
				memset(sName,0,sizeof(sName));

				if(m_tFileList.GetFileNameFromFullFileName(ptrFileName) != 0)
					continue;

				CHECK_OBJ(ptrFileName);
				ptrPos = strrchr(ptrFileName,'_');
				if(ptrPos == NULL)
					continue;
				int iLen = ptrPos-ptrFileName;
				if(iLen<sizeof(sName))
					strncpy(sName,ptrFileName,iLen);
				
				if(strcmp(sName,sFileHead) == 0)
            	{
            		StorageFileHandle tempSFHandle;
            		strncpy(tempSFHandle.m_sFileName, sFileName, sizeof(tempSFHandle.m_sFileName));
                	FILE * pFile = NULL;
                	pFile = fopen(sFileName,"rb+");//不需要关闭，一直link住
                	CHECK_OBJ(pFile);
					tempSFHandle.m_fFile = pFile;
                	tempFile.m_vStorageFile.push_back(tempSFHandle);
				}
            }
            m_vTSChangeFile.push_back(tempFile);
            if(sTableSpaceName != NULL) {break;}//找到指定表空间，就退出
        }

        //连接varchar文件
        TShmList<TMdbVarchar>::iterator itor_var = m_pShmDSN->m_VarCharList.begin();
        for(;itor_var != m_pShmDSN->m_VarCharList.end();++itor_var)
        {
            StorageFile tempFile;
            tempFile.m_iVarcharId = itor_var->iVarcharID;
            memset(sFileHead,0,sizeof(sFileHead));
			memset(sFileName, 0, sizeof(sFileName));
            snprintf(sFileHead,sizeof(sFileHead),"%s%d",VARCHAR_FILE_PREFIX,itor_var->iVarcharID);
            m_tFileList.GetFileList(1,0,sFileHead,TS_CHANGE_FILE_SUFFIX);
            while(m_tFileList.Next(sFileName) == 0)
            {
                StorageFileHandle tempSFHandle;
            	strncpy(tempSFHandle.m_sFileName, sFileName, sizeof(tempSFHandle.m_sFileName));
                FILE * pFile = NULL;
                pFile = fopen(sFileName,"rb+");//不需要关闭，一直link住
                CHECK_OBJ(pFile);
				tempSFHandle.m_fFile = pFile;
                tempFile.m_vStorageFile.push_back(tempSFHandle);
            }
            m_vVarCharChangeFile.push_back(tempFile);
        }
        TADD_NORMAL("clear end ts_change_size=[%d],varchar_change_size=[%d]",m_vTSChangeFile.size(),m_vVarCharChangeFile.size());
        return iRet;
    }

    int TMdbCheckPoint::LinkFile(const char * sTableSpaceName)
    {
        int iRet = 0;
        TADD_FUNC("Start");
		CHECK_RET(LinkStorageFile(sTableSpaceName), "LinkStorageFile failed!");
		CHECK_RET(LinkChangeFile(sTableSpaceName), "LinkChangeFile failed!");
        return iRet;
    }

	int TMdbCheckPoint::WritePageArrayToFile(char* sArray, int & iCount)
	{
		int iRet = 0;
		if (iCount == 0) return iRet;
		int iSize = m_pTS->iPageSize*iCount;
		FILE* pFile = NULL;
        GetFlushChangeFile(pFile);
        CHECK_OBJ(pFile);
		CHECK_RET(fseek(pFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
		TMdbTSFileHead tTSChangeFileHead;
        if(fread(&tTSChangeFileHead,sizeof(tTSChangeFileHead),1,pFile) != 1)
        {
            if(feof(pFile))
            {//文件结束
                CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
            }
            CHECK_RET(ERR_OS_READ_FILE,"errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
		if(tTSChangeFileHead.m_iPageCount*m_pTS->iPageSize+iSize >= 1024*1024*1024)
		{
			char sStorageFile[MAX_FILE_NAME] = {0};
	        snprintf(sStorageFile,sizeof(sStorageFile),"%s/%s%s_%d%s",m_pShmDSN->GetInfo()->sStorageDir,TS_FILE_PREFIX,m_pTS->sName,m_pTS->m_iChangeBlockId,TS_CHANGE_FILE_SUFFIX);
	        if(TMdbNtcFileOper::IsExist(sStorageFile))
	        {
	            CHECK_RET(ERR_APP_FILE_IS_EXIST,"File[%s] is exist,cannot flush full.",sStorageFile);
	        }
	        pFile = fopen(sStorageFile,"wb+");
	        CHECK_OBJ(pFile);
			
            m_pTS->m_iChangeBlockId++;
			//
			StorageFileHandle tempSFHandle;
			strncpy(tempSFHandle.m_sFileName, sStorageFile, sizeof(tempSFHandle.m_sFileName));
			tempSFHandle.m_fFile = pFile;
			m_vTSChangeFile[m_iCurChangePos].m_vStorageFile.push_back(tempSFHandle);
			
	        tTSChangeFileHead.Clear();
	        do{
	        	//记录表空间信息
		        SAFESTRCPY(tTSChangeFileHead.m_sTSName, sizeof(tTSChangeFileHead.m_sTSName), m_pTS->sName);
		        tTSChangeFileHead.m_iPageOffset = sizeof(tTSChangeFileHead);
		        tTSChangeFileHead.m_iPageSize = m_pTS->iPageSize;
				tTSChangeFileHead.m_iStartPage = 0;
				tTSChangeFileHead.m_iEndPage = 0;
		        tTSChangeFileHead.m_iPageCount = 0;
		        tTSChangeFileHead.m_iCheckPointLSN= 0;
	        }while(0);
		}
		//TADD_NORMAL("tTSChangeFileHead.m_iPageCount = [%d]", tTSChangeFileHead.m_iPageCount);
		CHECK_RET(fseek(pFile,sizeof(tTSChangeFileHead)+tTSChangeFileHead.m_iPageCount*m_pTS->iPageSize,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));

		//TADD_NORMAL("write change page size [%d]", iCount*m_pTS->iPageSize);
		if(fwrite(sArray, iCount*m_pTS->iPageSize, 1, pFile)!= 1 )
        {
            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
		tTSChangeFileHead.m_iPageCount += iCount;
		CHECK_RET(fseek(pFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        if(fwrite(&tTSChangeFileHead, sizeof(tTSChangeFileHead), 1, pFile)!= 1 )
        {
            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
        fflush(pFile);
		return iRet;
	}

	int TMdbCheckPoint::WriteChangeFile()
    {
        int iRet = 0;
        TADD_FUNC("Start");
        TMdbTSNode * pTSNode = &(m_pTS->tNode);
        CHECK_OBJ(pTSNode);
        int iDirtyPageID = -1;
        int iDirtyCount = 0;
		char *sChangedPageArray = NULL;
		sChangedPageArray = new(std::nothrow) char[(m_pTS->iPageSize)*MAX_FLUSH_CHANGED_PAGE_COUNT+64];//64冗余
		if(sChangedPageArray == NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"no memory for new sChangedPageArray");
			return ERR_OS_NO_MEMROY;

		}
		memset(sChangedPageArray, 0, (m_pTS->iPageSize)*MAX_FLUSH_CHANGED_PAGE_COUNT+64);
		int iPos = 0;
		int iCount = 0;
		char bmDirtyPageTemp[MAX_BIT_MAP_SIZE] = {0};

        while(NULL != pTSNode)
        {
        	memset(bmDirtyPageTemp, 0, MAX_BIT_MAP_SIZE);
            int iRealBMSize = (pTSNode->iPageEnd - pTSNode->iPageStart)/8;
            iRealBMSize = iRealBMSize < MAX_BIT_MAP_SIZE ? iRealBMSize : MAX_BIT_MAP_SIZE;
            int i = 0;
            unsigned char cTemp = (0x0);

            for(i = 0; i <= iRealBMSize;++i)
            {//先一个字节一个字节的比较
                if(0 != (pTSNode->bmDirtyPage[i] | cTemp))
                {//若有脏页再按位比较
                    int j = 0;
                    for(j = 0;j < 8;++j)
                    {
                        cTemp =  (0x1<<j);
                        if(0 != (pTSNode->bmDirtyPage[i] & cTemp))
                        {//找到脏页
                        	bmDirtyPageTemp[i]|=cTemp;
                            iDirtyCount++;
                            iDirtyPageID = i*8+j+pTSNode->iPageStart;
							//添加到buff
							//TADD_NORMAL("iDirtyPageID = [%d], iPos = [%d], m_pTS->iPageSize = [%d]", iDirtyPageID, iPos, m_pTS->iPageSize);
							TMdbPage *pPage = (TMdbPage *)m_TSCtrl.GetAddrByPageID(iDirtyPageID);

							memcpy(sChangedPageArray+iPos, (char*)pPage, m_pTS->iPageSize);
							iPos += m_pTS->iPageSize;
							iCount++;

							//TADD_NORMAL("iCount = [%d]", iCount);
							if(iCount >= MAX_FLUSH_CHANGED_PAGE_COUNT)
							{
								CHECK_RET(WritePageArrayToFile(sChangedPageArray, iCount), "Write to file error!");
								memset(sChangedPageArray, 0, MAX_FLUSH_CHANGED_PAGE_COUNT*m_pTS->iPageSize+64);
								iPos = 0;
								iCount = 0;
							}
                        }
                    }
                }
            }
			CHECK_RET(WritePageArrayToFile(sChangedPageArray, iCount), "Write to file error!");
			memset(sChangedPageArray, 0, MAX_FLUSH_CHANGED_PAGE_COUNT*m_pTS->iPageSize+64);
			iPos = 0;
			iCount = 0;
            for(i = 0; i <= iRealBMSize;++i)
            {
				pTSNode->bmDirtyPage[i] ^= bmDirtyPageTemp[i];
        	}
            pTSNode = m_TSCtrl.GetNextNode(pTSNode);
        }
		
		SAFE_DELETE(sChangedPageArray);
        TADD_NORMAL("Flush[%s] to change file, iDirtyCount=[%d]",m_pTS->sName,iDirtyCount);
        return iRet;
    }

	int TMdbCheckPoint::WriteVarcharPageArrayToFile(char* sArray, int & iCount)
	{
		int iRet = 0;
		if (iCount == 0) return iRet;
		int iSize = m_pVarchar->iPageSize*iCount;
		FILE* pFile = NULL;
        GetFlushVarcharChangeFile(pFile);
        CHECK_OBJ(pFile);
		CHECK_RET(fseek(pFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
		TMdbTSFileHead tVarcharChangeFileHead;
        if(fread(&tVarcharChangeFileHead,sizeof(tVarcharChangeFileHead),1,pFile) != 1)
        {
            if(feof(pFile))
            {//文件结束
                CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
            }
            CHECK_RET(ERR_OS_READ_FILE,"errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
		if(tVarcharChangeFileHead.m_iPageCount*m_pVarchar->iPageSize+iSize >= 1024*1024*1024)
		{
			char sStorageFile[MAX_FILE_NAME] = {0};
	        snprintf(sStorageFile,sizeof(sStorageFile),"%s/%s%d_%d%s",m_pShmDSN->GetInfo()->sStorageDir,VARCHAR_FILE_PREFIX, m_pVarchar->iVarcharID,m_pVarchar->iChangeBlockId,TS_CHANGE_FILE_SUFFIX);
	        if(TMdbNtcFileOper::IsExist(sStorageFile))
	        {
	            CHECK_RET(ERR_APP_FILE_IS_EXIST,"File[%s] is exist,cannot flush full.",sStorageFile);
	        }
	        pFile = fopen(sStorageFile,"wb+");
	        CHECK_OBJ(pFile);
			
            m_pVarchar->iChangeBlockId++;
			//
			StorageFileHandle tempSFHandle;
			strncpy(tempSFHandle.m_sFileName, sStorageFile, sizeof(tempSFHandle.m_sFileName));
			tempSFHandle.m_fFile = pFile;
			m_vTSChangeFile[m_iCurChangePos].m_vStorageFile.push_back(tempSFHandle);
			
	        tVarcharChangeFileHead.Clear();
	        do{
	        	//记录表空间信息
				tVarcharChangeFileHead.iVarcharID = m_pVarchar->iVarcharID;
		        tVarcharChangeFileHead.m_iPageOffset = sizeof(tVarcharChangeFileHead);
		        tVarcharChangeFileHead.m_iPageSize = m_pVarchar->iPageSize;
		        tVarcharChangeFileHead.m_iPageCount= 0;
		        tVarcharChangeFileHead.m_iCheckPointLSN= 0;
	        }while(0);
	        //文件指针空出一个头文件位置
	        CHECK_RET(fseek(pFile,sizeof(tVarcharChangeFileHead),SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
		}
		else
		{
			//TADD_NORMAL("tVarcharChangeFileHead.m_iPageCount = [%d]", tVarcharChangeFileHead.m_iPageCount);
			CHECK_RET(fseek(pFile,sizeof(tVarcharChangeFileHead)+tVarcharChangeFileHead.m_iPageCount*m_pVarchar->iPageSize,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
		}
		//TADD_NORMAL("write change page size [%d]", iCount*m_pVarchar->iPageSize);
		if(fwrite(sArray, iCount*m_pVarchar->iPageSize, 1, pFile)!= 1 )
        {
            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
		tVarcharChangeFileHead.m_iPageCount += iCount;
		CHECK_RET(fseek(pFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        if(fwrite(&tVarcharChangeFileHead, sizeof(tVarcharChangeFileHead), 1, pFile)!= 1 )
        {
            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
        fflush(pFile);
		return iRet;
	}

	int TMdbCheckPoint::WriteVarcharChangeFile()
    {
        int iRet = 0;
        TADD_FUNC("Start");
        TMdbTSNode * pTSNode = &(m_pVarchar->tNode);
        CHECK_OBJ(pTSNode);
        int iDirtyPageID = -1;
        int iDirtyCount = 0;
		//char *sChangedPageArray = new char[(m_pVarchar->iPageSize)*MAX_FLUSH_CHANGED_PAGE_COUNT+64];
		//memset(sChangedPageArray, 0, (m_pVarchar->iPageSize)*MAX_FLUSH_CHANGED_PAGE_COUNT+64);
		if(m_pVarCharArray == NULL)
		{
			
			m_pVarCharArray = new(std::nothrow) char[(m_pVarchar->iPageSize)*MAX_FLUSH_CHANGED_PAGE_COUNT+64];
			if(m_pVarCharArray == NULL)
			{
				TADD_ERROR(ERR_OS_NO_MEMROY,"no memory for new m_pVarCharArray");
				return ERR_OS_NO_MEMROY;
			}
			
		}

		memset(m_pVarCharArray, 0, (m_pVarchar->iPageSize)*MAX_FLUSH_CHANGED_PAGE_COUNT+64);
		
		int iPos = 0;
		int iCount = 0;
		char bmDirtyPageTemp[MAX_BIT_MAP_SIZE] = {0};

        while(NULL != pTSNode)
        {
        	memset(bmDirtyPageTemp, 0, MAX_BIT_MAP_SIZE);
            int iRealBMSize = (pTSNode->iPageEnd - pTSNode->iPageStart)/8;
            iRealBMSize = iRealBMSize < MAX_BIT_MAP_SIZE ? iRealBMSize : MAX_BIT_MAP_SIZE;
            int i = 0;
            unsigned char cTemp = (0x0);
            for(i = 0; i <= iRealBMSize;++i)
            {//先一个字节一个字节的比较
                if(0 != (pTSNode->bmDirtyPage[i] | cTemp))
                {//若有脏页再按位比较
                    int j = 0;
                    for(j = 0;j < 8;++j)
                    {
                        cTemp =  (0x1<<j);
                        if(0 != (pTSNode->bmDirtyPage[i] & cTemp))
                        {//找到脏页
                        	bmDirtyPageTemp[i]|=cTemp;
                            iDirtyCount++;
                            iDirtyPageID = i*8+j+pTSNode->iPageStart;
							//添加到buff
							//TADD_NORMAL("iDirtyPageID = [%d], iPos = [%d], m_pVarchar->iPageSize = [%d]", iDirtyPageID, iPos, m_pVarchar->iPageSize);
							TMdbPage *pPage = (TMdbPage *)m_VarCharCtrl.GetAddrByPageID(m_pVarchar,iDirtyPageID);
							memcpy(m_pVarCharArray+iPos, (char*)pPage, m_pVarchar->iPageSize);
							iPos += m_pVarchar->iPageSize;
							iCount++;
							//TADD_NORMAL("iCount = [%d]", iCount);
							if(iCount >= MAX_FLUSH_CHANGED_PAGE_COUNT)
							{
								CHECK_RET(WriteVarcharPageArrayToFile(m_pVarCharArray, iCount), "Write to file error!");
								memset(m_pVarCharArray, 0, MAX_FLUSH_CHANGED_PAGE_COUNT*m_pVarchar->iPageSize+64);
								iPos = 0;
								iCount = 0;
							}
                        }
                    }
                }
            }
			CHECK_RET(WriteVarcharPageArrayToFile(m_pVarCharArray, iCount), "Write to file error!");
			memset(m_pVarCharArray, 0, MAX_FLUSH_CHANGED_PAGE_COUNT*m_pVarchar->iPageSize+64);
			iPos = 0;
			iCount = 0;
            for(i = 0; i <= iRealBMSize;++i)
            {
				pTSNode->bmDirtyPage[i] ^= bmDirtyPageTemp[i];
        	}
            pTSNode = m_VarCharCtrl.GetNextNode(pTSNode);
        }
		
		//SAFE_DELETE(sChangedPageArray);
        TADD_NORMAL("Flush varchar[%d] to change file, iDirtyCount=[%d]",m_pVarchar->iVarcharID,iDirtyCount);
        return iRet;
    }

	int TMdbCheckPoint::FlushChangeFile(const char * sTableSpaceName)
	{
        TADD_FUNC("Start");
		int iRet = 0;
		char changeStartFile[MAX_NAME_LEN] = {0};
		char changeOKFile[MAX_NAME_LEN] = {0};
		snprintf(changeStartFile,sizeof(changeStartFile),"%s/%s",m_pShmDSN->GetInfo()->sStorageDir,"CHANGE.START");
		snprintf(changeOKFile,sizeof(changeOKFile),"%s/%s",m_pShmDSN->GetInfo()->sStorageDir,"CHANGE.OK");
		TMdbNtcFileOper::Remove(changeOKFile);
		TMdbNtcFileOper::MakeFile(changeStartFile);
		bool bIsFileStorage = false;
		TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            if(sTableSpaceName != NULL && TMdbNtcStrFunc::StrNoCaseCmp(sTableSpaceName,itor->sName) != 0)
            {//如果设置了表空间，就只连接这个表空间的文件
                continue;
            }
            if(itor->m_bFileStorage == false){continue;}
			bIsFileStorage = true;
            TMdbTableSpace* pTS = &(*itor);
            CHECK_RET(Init(pTS),"Init Failed");
			CHECK_RET(WriteChangeFile(),"WriteChangeFile Failed");//将脏页写入临时文件
            if(sTableSpaceName != NULL) {break;}//找到指定表空间，就退出
        }

		if (!bIsFileStorage) return iRet;
		
        //刷新varchar页
        m_pVarCharArray = NULL;
        TShmList<TMdbVarchar>::iterator itor_var = m_pShmDSN->m_VarCharList.begin();
        for(;itor_var != m_pShmDSN->m_VarCharList.end();++itor_var)
        {
            TMdbVarchar* pVarchar = &(*itor_var);
            CHECK_RET(Init(pVarchar),"Init Failed");
			CHECK_RET(WriteVarcharChangeFile(),"WriteVarcharChangeFile Failed");//将脏页写入临时文件
        }
		SAFE_DELETE(m_pVarCharArray);
		TMdbNtcFileOper::Remove(changeStartFile);
		TMdbNtcFileOper::MakeFile(changeOKFile);
		return iRet;
	}

	int TMdbCheckPoint::FlushStorageFile(const char * sTableSpaceName)
	{
        TADD_FUNC("Start");
		int iRet = 0;
		char flushStartFile[MAX_NAME_LEN] = {0};
		char flushOKFile[MAX_NAME_LEN] = {0};
		snprintf(flushStartFile,sizeof(flushStartFile),"%s/%s",m_pShmDSN->GetInfo()->sStorageDir,"FLUSH.START");
		snprintf(flushOKFile,sizeof(flushOKFile),"%s/%s",m_pShmDSN->GetInfo()->sStorageDir,"FLUSH.OK");
		TMdbNtcFileOper::Remove(flushOKFile);
		TMdbNtcFileOper::MakeFile(flushStartFile);
		bool bIsFileStorage = false;
		TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            if(sTableSpaceName != NULL && TMdbNtcStrFunc::StrNoCaseCmp(sTableSpaceName,itor->sName) != 0)
            {//如果设置了表空间，就只连接这个表空间的文件
                continue;
            }
            if(itor->m_bFileStorage == false){continue;}
			bIsFileStorage = true;
            TMdbTableSpace* pTS = &(*itor);
            CHECK_RET(Init(pTS),"Init Failed");
            CHECK_RET(FlushDirtyPage(),"FlushDirtyPage Failed");
            if(sTableSpaceName != NULL) {break;}//找到指定表空间，就退出
        }

		if (!bIsFileStorage) return iRet;
		
        //刷新varchar页
        TShmList<TMdbVarchar>::iterator itor_var = m_pShmDSN->m_VarCharList.begin();
        for(;itor_var != m_pShmDSN->m_VarCharList.end();++itor_var)
        {
            TMdbVarchar* pVarchar = &(*itor_var);
            CHECK_RET(Init(pVarchar),"Init Failed");
            CHECK_RET(FlushVarcharDirtyPage(),"FlushVarcharDirtyPage Failed");
        }
		TMdbNtcFileOper::Remove(flushStartFile);
		TMdbNtcFileOper::MakeFile(flushOKFile);
		return iRet;
	}

    int TMdbCheckPoint::DoCheckPoint(const char * sTableSpaceName)
    {
        TADD_FUNC("Start");
        int iRet = 0;
        //脏页写入临时文件
		CHECK_RET(FlushChangeFile(sTableSpaceName),"FlushChangeFile Failed");
		//临时文件刷入存储文件
		CHECK_RET(FlushStorageFile(sTableSpaceName),"FlushStorageFile Failed");
		//指定表空间，不能清理 redo
        if(sTableSpaceName != NULL) {return iRet;}
        //清理redolog文件
        CHECK_RET(ClearRedoLog(),"ClearRedoLog Failed");
        return iRet;
    }

    int TMdbCheckPoint::FlushDirtyPage()
    {
        int iRet = 0;
        TADD_FUNC("Start");
        TMdbTSNode * pTSNode = &(m_pTS->tNode);
        CHECK_OBJ(pTSNode);
		FILE* pChangeFile = NULL;
		int iDirtyCount = 0;
		int iChangeFileCount = m_vTSChangeFile[m_iCurChangePos].m_vStorageFile.size();
		for(int i = 0; i < iChangeFileCount; i++)
		{
		    TADD_NORMAL("m_vTSChangeFile[%d].m_vStorageFile[%d].m_sFileName = [%s]", m_iCurChangePos, i, m_vTSChangeFile[m_iCurChangePos].m_vStorageFile[i].m_sFileName);
			pChangeFile = NULL;
			pChangeFile = m_vTSChangeFile[m_iCurChangePos].m_vStorageFile[i].m_fFile;
			CHECK_OBJ(pChangeFile);
			CHECK_RET(fseek(pChangeFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
			TMdbTSFileHead tTSChangeFileHead;
	        if(fread(&tTSChangeFileHead,sizeof(tTSChangeFileHead),1,pChangeFile) != 1)
	        {
	            if(feof(pChangeFile))
	            {//文件结束
	                CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
	            }
	            CHECK_RET(ERR_OS_READ_FILE,"errno=%d, errormsg=[%s].", errno, strerror(errno));
	        }
			//CHECK_RET(fseek(pChangeFile,sizeof(tTSChangeFileHead),SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
			
			int iDataSize = tTSChangeFileHead.m_iPageCount*m_pTS->iPageSize;
			//TADD_NORMAL("tTSChangeFileHead.m_iPageCount = [%d], iDataSize = [%d]", tTSChangeFileHead.m_iPageCount, iDataSize);
			char sPages[1024*1024] = {0};
			int iReadLen = 0;
			int iReadPos = sizeof(tTSChangeFileHead);
			TMdbPage * pPage = NULL;
			int iPageCount = 0;
			FILE* pFile = NULL;
			while(iDataSize > 0)
			{
				iReadPos += iReadLen;
				//TADD_NORMAL("iReadPos = [%d], iReadLen = [%d]", iReadPos, iReadLen);
				CHECK_RET(fseek(pChangeFile,iReadPos,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
				iReadLen = (sizeof(sPages)/m_pTS->iPageSize)>=(iDataSize/m_pTS->iPageSize)?iDataSize:(m_pTS->iPageSize*(sizeof(sPages)/m_pTS->iPageSize));
				memset(sPages, 0, sizeof(sPages));
				if(fread(sPages,iReadLen,1,pChangeFile) != 1)
		        {
		            if(feof(pChangeFile))
		            {//文件结束
		                CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
		            }
		            CHECK_RET(ERR_OS_READ_FILE,"errno=%d, errormsg=[%s].", errno, strerror(errno));
		        }
				pPage = NULL;
				pFile = NULL;
				iPageCount = iReadLen/m_pTS->iPageSize;
				//TADD_NORMAL("iPageCount = [%d]", iPageCount);
				for(int j = 0; j < iPageCount; j++)
				{
					pPage = (TMdbPage*)(sPages + m_pTS->iPageSize*j);
				    CHECK_OBJ(pPage);
					pFile = NULL;
					//TADD_NORMAL("pPage->m_iPageID = [%d]", pPage->m_iPageID);
	                if(GetFlushFile(pPage->m_iPageID, pFile) != 0)
	                {
	                    //脏页找不到，有可能是文件中还没有该页
	                    continue;
	                }
	                else
	                {
	                    CHECK_OBJ(pFile);
				        //移动文件指针
				        CHECK_RET(fseek(pFile,sizeof(TMdbTSFileHead),SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
				        //TADD_NORMAL("m_tTSFileHead.m_iStartPage = [%d]", m_tTSFileHead.m_iStartPage);
				        CHECK_RET(fseek(pFile,(pPage->m_iPageID - m_tTSFileHead.m_iStartPage)*m_pTS->iPageSize,SEEK_CUR),
				            "fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
				        do{
				            if(fwrite(pPage, m_pTS->iPageSize, 1, pFile)!= 1 )
				            {
				                CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
				            }
				            if(m_iMaxLsn == 0)
				            {
				                m_iMaxLsn = pPage->m_iPageLSN;
				            }
				            else
				            {
				                m_iMaxLsn = m_iMaxLsn < pPage->m_iPageLSN?pPage->m_iPageLSN:m_iMaxLsn;
				            }
				        }while(0);
				        fflush(pFile);
						iDirtyCount++;
	                }
				}
				iDataSize -= iReadLen;
			}
			//删除临时文件
			fclose(pChangeFile);
			TADD_NORMAL("Remove file[%s]", m_vTSChangeFile[m_iCurChangePos].m_vStorageFile[i].m_sFileName);
			TMdbNtcFileOper::Remove(m_vTSChangeFile[m_iCurChangePos].m_vStorageFile[i].m_sFileName);
		}
		StorageFile* sf = &(m_vTSChangeFile[m_iCurChangePos]);
		sf->m_vStorageFile.clear();
        m_pTS->m_iChangeBlockId = 0;
        TADD_NORMAL("Flush[%s] iDirtyCount=[%d]",m_pTS->sName,iDirtyCount);
        return iRet;
    }

    int TMdbCheckPoint::FlushVarcharDirtyPage()
    {
        int iRet = 0;
        TADD_FUNC("Start");
        TMdbTSNode * pTSNode = &(m_pVarchar->tNode);
        CHECK_OBJ(pTSNode);
		FILE* pChangeFile = NULL;
        int iDirtyCount = 0;

		
		int iChangeFileCount = m_vVarCharChangeFile[m_iCurChangePos].m_vStorageFile.size();
		for(int i = 0; i < iChangeFileCount; i++)
		{
			TADD_NORMAL("m_vVarCharChangeFile[%d].m_vStorageFile[%d].m_sFileName = [%s]", m_iCurChangePos, i, m_vVarCharChangeFile[m_iCurChangePos].m_vStorageFile[i].m_sFileName);
			pChangeFile = NULL;
			pChangeFile = m_vVarCharChangeFile[m_iCurChangePos].m_vStorageFile[i].m_fFile;
			CHECK_OBJ(pChangeFile);
			CHECK_RET(fseek(pChangeFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
			TMdbTSFileHead tVarCharChangeFileHead;
	        if(fread(&tVarCharChangeFileHead,sizeof(tVarCharChangeFileHead),1,pChangeFile) != 1)
	        {
	            if(feof(pChangeFile))
	            {//文件结束
	                CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
	            }
	            CHECK_RET(ERR_OS_READ_FILE,"errno=%d, errormsg=[%s].", errno, strerror(errno));
	        }
			//CHECK_RET(fseek(pChangeFile,sizeof(tVarCharChangeFileHead),SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
			
			int iDataSize = tVarCharChangeFileHead.m_iPageCount*m_pVarchar->iPageSize;
			//TADD_NORMAL("tVarCharChangeFileHead.m_iPageCount = [%d], iDataSize = [%d]", tVarCharChangeFileHead.m_iPageCount, iDataSize);
			char sPages[1024*1024] = {0};
			int iReadLen = 0;
			int iReadPos = sizeof(tVarCharChangeFileHead);
			TMdbPage * pPage = NULL;
			int iPageCount = 0;
			FILE* pFile = NULL;
			while(iDataSize > 0)
			{
				iReadPos += iReadLen;
				//TADD_NORMAL("iReadPos = [%d], iReadLen = [%d]", iReadPos, iReadLen);
				CHECK_RET(fseek(pChangeFile,iReadPos,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
				iReadLen = (sizeof(sPages)/m_pVarchar->iPageSize)>=(iDataSize/m_pVarchar->iPageSize)?iDataSize:(m_pVarchar->iPageSize*(sizeof(sPages)/m_pVarchar->iPageSize));
				memset(sPages, 0, sizeof(sPages));
				if(fread(sPages,iReadLen,1,pChangeFile) != 1)
		        {
		            if(feof(pChangeFile))
		            {//文件结束
		                CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
		            }
		            CHECK_RET(ERR_OS_READ_FILE,"errno=%d, errormsg=[%s].", errno, strerror(errno));
		        }
				pPage = NULL;
				pFile = NULL;
				iPageCount = iReadLen/m_pVarchar->iPageSize;
				//TADD_NORMAL("iPageCount = [%d]", iPageCount);
				for(int j = 0; j < iPageCount; j++)
				{
					pPage = (TMdbPage*)(sPages + m_pVarchar->iPageSize*j);
				    CHECK_OBJ(pPage);
					
					pFile = NULL;
					//TADD_NORMAL("pPage->m_iPageID = [%d], pPage->m_iPageSize = [%d]", pPage->m_iPageID, pPage->m_iPageSize);
	                if(GetVarcharFlushFile(pPage->m_iPageID, pFile) != 0)
	                {
	                    //脏页找不到，有可能是文件中还没有该页
	                    continue;
	                }
	                else
	                {
	                    CHECK_OBJ(pFile);
				        //移动文件指针
				        CHECK_RET(fseek(pFile,sizeof(TMdbTSFileHead),SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
				        //TADD_NORMAL("m_tTSFileHead.m_iStartPage = [%d]", m_tTSFileHead.m_iStartPage);
				        CHECK_RET(fseek(pFile,(pPage->m_iPageID - m_tTSFileHead.m_iStartPage)*(m_pVarchar->iPageSize),SEEK_CUR),
				            "fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
				        do{
				            if(fwrite(pPage, m_pVarchar->iPageSize, 1, pFile)!= 1 )
				            {
				                CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
				            }
				            if(m_iMaxLsn == 0)
				            {
				                m_iMaxLsn = pPage->m_iPageLSN;
				            }
				            else
				            {
				                m_iMaxLsn = m_iMaxLsn < pPage->m_iPageLSN?pPage->m_iPageLSN:m_iMaxLsn;
				            }
				        }while(0);
				        fflush(pFile);
						iDirtyCount++;
	                }
				}
				iDataSize -= iReadLen;
			}
			//删除临时文件
			SAFE_CLOSE(pChangeFile);
			TADD_NORMAL("Remove file[%s]", m_vVarCharChangeFile[m_iCurChangePos].m_vStorageFile[i].m_sFileName);
			TMdbNtcFileOper::Remove(m_vVarCharChangeFile[m_iCurChangePos].m_vStorageFile[i].m_sFileName);
        }
		StorageFile* sf = &(m_vVarCharChangeFile[m_iCurChangePos]);
		sf->m_vStorageFile.clear();
        m_pVarchar->iChangeBlockId = 0;
        TADD_NORMAL("Flush Varchar [%d] iDirtyCount=[%d]",m_pVarchar->iVarcharID,iDirtyCount);
        return iRet;
    }

	int TMdbCheckPoint::GetFlushChangeFile(FILE* &pFile)
    {
        int iRet = 0;
        TADD_FUNC("Start");
        pFile = NULL;
        int iChangeFileCount = m_vTSChangeFile[m_iCurChangePos].m_vStorageFile.size();
		if(iChangeFileCount == 0)
		{
			char sStorageFile[MAX_FILE_NAME] = {0};
	        snprintf(sStorageFile,sizeof(sStorageFile),"%s/%s%s_%d%s",m_pShmDSN->GetInfo()->sStorageDir,TS_FILE_PREFIX,m_pTS->sName,m_pTS->m_iChangeBlockId,TS_CHANGE_FILE_SUFFIX);
	        if(TMdbNtcFileOper::IsExist(sStorageFile))
	        {
	            CHECK_RET(ERR_APP_FILE_IS_EXIST,"File[%s] is exist,cannot flush full.",sStorageFile);
	        }
	        pFile = fopen(sStorageFile,"wb+");
	        CHECK_OBJ(pFile);
			//
            m_pTS->m_iChangeBlockId++;
			StorageFileHandle tempSFHandle;
			strncpy(tempSFHandle.m_sFileName, sStorageFile, sizeof(tempSFHandle.m_sFileName));
			tempSFHandle.m_fFile = pFile;
			m_vTSChangeFile[m_iCurChangePos].m_vStorageFile.push_back(tempSFHandle);
			//写入临时文件头部信息
			TMdbTSFileHead tTSChangeFileHead;
			tTSChangeFileHead.Clear();
	        m_pTS->tEmptyMutex.Lock(true);
	        do{
	        	//记录表空间信息
		        SAFESTRCPY(tTSChangeFileHead.m_sTSName, sizeof(tTSChangeFileHead.m_sTSName), m_pTS->sName);
		        tTSChangeFileHead.m_iPageOffset = sizeof(tTSChangeFileHead);
		        tTSChangeFileHead.m_iPageSize = m_pTS->iPageSize;
				tTSChangeFileHead.m_iStartPage = 0;
				tTSChangeFileHead.m_iEndPage = 0;
		        tTSChangeFileHead.m_iPageCount = 0;
		        tTSChangeFileHead.m_iCheckPointLSN= 0;
	        }while(0);
	        m_pTS->tEmptyMutex.UnLock(true);
			CHECK_RET(fseek(pFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
	        if(fwrite(&tTSChangeFileHead, sizeof(tTSChangeFileHead), 1, pFile)!= 1 )
	        {
	            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
	        }
	        fflush(pFile);
		}
		else
		{
			pFile = m_vTSChangeFile[m_iCurChangePos].m_vStorageFile[iChangeFileCount-1].m_fFile;
        	CHECK_OBJ(pFile);
		}
        
        return iRet;
    }
    
    int TMdbCheckPoint::GetFlushFile(int iPageId,FILE* &pFile)
    {
        int iRet = 0;
        TADD_FUNC("Start");
        pFile = NULL;
        int iFileCount = m_vTSFile[m_iCurPos].m_vStorageFile.size();
        for(int i = 0; i<iFileCount;i++)
        {
            pFile = m_vTSFile[m_iCurPos].m_vStorageFile[i].m_fFile;
            CHECK_OBJ(pFile);
            CHECK_RET(fseek(pFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
            if(fread(&m_tTSFileHead,sizeof(m_tTSFileHead),1,pFile) != 1)
            {
                if(feof(pFile))
                {//文件结束
                    CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
                }
                CHECK_RET(ERR_OS_READ_FILE,"errno=%d, errormsg=[%s].", errno, strerror(errno));
            }
			//TADD_NORMAL("m_vTSFile[%d].m_vStorageFile[%d]: m_tTSFileHead.m_iStartPage = [%d], m_tTSFileHead.m_iEndPage = [%d]", m_iCurPos, i, m_tTSFileHead.m_iStartPage, m_tTSFileHead.m_iEndPage);
            if(iPageId >= m_tTSFileHead.m_iStartPage && iPageId <= m_tTSFileHead.m_iEndPage)
            {
                return iRet;
            }
        }
        CHECK_RET(-1,"Not Find Page[%d] in File[%s].",iPageId,m_pTS->sName);
        return iRet;
    }

	int TMdbCheckPoint::GetFlushVarcharChangeFile(FILE* &pFile)
    {
        int iRet = 0;
        TADD_FUNC("Start");
        pFile = NULL;
        int iChangeFileCount = m_vVarCharChangeFile[m_iCurChangePos].m_vStorageFile.size();
		if(iChangeFileCount == 0)
		{
			char sStorageFile[MAX_FILE_NAME] = {0};
	        snprintf(sStorageFile,sizeof(sStorageFile),"%s/%s%d_%d%s",m_pShmDSN->GetInfo()->sStorageDir,VARCHAR_FILE_PREFIX,m_pVarchar->iVarcharID,m_pVarchar->iChangeBlockId,TS_CHANGE_FILE_SUFFIX);
	        if(TMdbNtcFileOper::IsExist(sStorageFile))
	        {
	            CHECK_RET(ERR_APP_FILE_IS_EXIST,"File[%s] is exist,cannot flush full.",sStorageFile);
	        }
	        pFile = fopen(sStorageFile,"wb+");
	        CHECK_OBJ(pFile);
			//
            m_pVarchar->iChangeBlockId++;
			StorageFileHandle tempSFHandle;
			strncpy(tempSFHandle.m_sFileName, sStorageFile, sizeof(tempSFHandle.m_sFileName));
			tempSFHandle.m_fFile = pFile;
			m_vVarCharChangeFile[m_iCurChangePos].m_vStorageFile.push_back(tempSFHandle);
			TMdbTSFileHead tVarcharChangeFileHead;
			tVarcharChangeFileHead.Clear();
	        do{
	        	//记录表空间信息
				tVarcharChangeFileHead.iVarcharID = m_pVarchar->iVarcharID;
		        tVarcharChangeFileHead.m_iPageOffset = sizeof(tVarcharChangeFileHead);
		        tVarcharChangeFileHead.m_iPageSize = m_pVarchar->iPageSize;
		        tVarcharChangeFileHead.m_iPageCount= 0;
		        tVarcharChangeFileHead.m_iCheckPointLSN= 0;
	        }while(0);
			CHECK_RET(fseek(pFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
	        if(fwrite(&tVarcharChangeFileHead, sizeof(tVarcharChangeFileHead), 1, pFile)!= 1 )
	        {
	            CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
	        }
	        fflush(pFile);
		}
		else
		{
			pFile = m_vVarCharChangeFile[m_iCurChangePos].m_vStorageFile[iChangeFileCount-1].m_fFile;
        	CHECK_OBJ(pFile);
		}
        
        return iRet;
    }

    int TMdbCheckPoint::GetVarcharFlushFile(int iPageId, FILE * &pFile)
    {
        int iRet = 0;
        TADD_FUNC("Start");
        pFile = NULL;
        int iFileCount = m_vVarCharFile[m_iCurPos].m_vStorageFile.size();
        for(int i = 0; i<iFileCount;i++)
        {
            pFile = m_vVarCharFile[m_iCurPos].m_vStorageFile[i].m_fFile;
            CHECK_OBJ(pFile);
            CHECK_RET(fseek(pFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
            if(fread(&m_tTSFileHead,sizeof(m_tTSFileHead),1,pFile) != 1)
            {
                if(feof(pFile))
                {//文件结束
                    CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
                }
                CHECK_RET(ERR_OS_READ_FILE,"errno=%d, errormsg=[%s].", errno, strerror(errno));
            }
			//TADD_NORMAL("m_vVarCharFile[%d].m_vStorageFile[%d]: m_tTSFileHead.m_iStartPage = [%d], m_tTSFileHead.m_iEndPage = [%d]", m_iCurPos, i, m_tTSFileHead.m_iStartPage, m_tTSFileHead.m_iEndPage);
            if(iPageId >= m_tTSFileHead.m_iStartPage && iPageId <= m_tTSFileHead.m_iEndPage)
            {
                return iRet;
            }
        }
        CHECK_RET(-1,"Not Find Page[%d] in Varchar File[%d].",iPageId,m_pVarchar->iVarcharID);
        return iRet;
    }
/*
	int TMdbCheckPoint::CopyChangePage(FILE* pFile,char & bmDirtyPage, unsigned char cTemp)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //移动到数据头
        CHECK_RET(fseek(pFile,m_tTSFileHead.m_iPageOffset,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        TMdbPage *pPage = (TMdbPage *)m_TSCtrl.GetAddrByPageID(iPageId);
        CHECK_OBJ(pPage);
        //移动文件指针
        CHECK_RET(fseek(pFile,(iPageId - m_tTSFileHead.m_iStartPage)*pPage->m_iPageSize,SEEK_CUR),
            "fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        m_tPageCtrl.Attach((char *)pPage,false,true);
        m_tPageCtrl.WLock();
        do{
            if(fwrite(pPage, pPage->m_iPageSize, 1, pFile)!= 1 )
            {
                CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
            }
            if(m_iMaxLsn == 0)
            {
                m_iMaxLsn = pPage->m_iPageLSN;
            }
            else
            {
                m_iMaxLsn = m_iMaxLsn < pPage->m_iPageLSN?pPage->m_iPageLSN:m_iMaxLsn;
            }
        }while(0);
        bmDirtyPage ^= cTemp;
        m_tPageCtrl.UnWLock();
        fflush(pFile);
        
        
        TADD_FUNC("Finish.");
        return iRet;
    }
*/
    /*int TMdbCheckPoint::CopyPage(FILE* pFile,int iPageId,char & bmDirtyPage, unsigned char cTemp)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //移动到数据头
        CHECK_RET(fseek(pFile,m_tTSFileHead.m_iPageOffset,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        TMdbPage *pPage = (TMdbPage *)m_TSCtrl.GetAddrByPageID(iPageId);
        CHECK_OBJ(pPage);
        //移动文件指针
        CHECK_RET(fseek(pFile,(iPageId - m_tTSFileHead.m_iStartPage)*pPage->m_iPageSize,SEEK_CUR),
            "fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        m_tPageCtrl.Attach((char *)pPage,false,true);
        m_tPageCtrl.WLock();
        do{
            if(fwrite(pPage, pPage->m_iPageSize, 1, pFile)!= 1 )
            {
                CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
            }
            if(m_iMaxLsn == 0)
            {
                m_iMaxLsn = pPage->m_iPageLSN;
            }
            else
            {
                m_iMaxLsn = m_iMaxLsn < pPage->m_iPageLSN?pPage->m_iPageLSN:m_iMaxLsn;
            }
        }while(0);
        bmDirtyPage ^= cTemp;
        
        m_tPageCtrl.UnWLock();
        fflush(pFile);
        
        
        TADD_FUNC("Finish.");
        return iRet;
    }*/

	/*int TMdbCheckPoint::CopyPage(FILE* pFile,int iPageId)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //移动到数据头
        CHECK_RET(fseek(pFile,m_tTSFileHead.m_iPageOffset,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        TMdbPage *pPage = (TMdbPage *)m_TSCtrl.GetAddrByPageID(iPageId);
        CHECK_OBJ(pPage);
        //移动文件指针
        CHECK_RET(fseek(pFile,(iPageId - m_tTSFileHead.m_iStartPage)*pPage->m_iPageSize,SEEK_CUR),
            "fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        m_tPageCtrl.Attach((char *)pPage,false,true);
        m_tPageCtrl.WLock();
        do{
            if(fwrite(pPage, pPage->m_iPageSize, 1, pFile)!= 1 )
            {
                CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
            }
            if(m_iMaxLsn == 0)
            {
                m_iMaxLsn = pPage->m_iPageLSN;
            }
            else
            {
                m_iMaxLsn = m_iMaxLsn < pPage->m_iPageLSN?pPage->m_iPageLSN:m_iMaxLsn;
            }
        }while(0);
        
        m_tPageCtrl.UnWLock();
        fflush(pFile);
        
        
        TADD_FUNC("Finish.");
        return iRet;
    }*/

    /*int TMdbCheckPoint::CopyVarcharPage(FILE* pFile,int iPageId,char & bmDirtyPage, unsigned char cTemp)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //移动到数据头
        CHECK_RET(fseek(pFile,m_tTSFileHead.m_iPageOffset,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        TMdbPage *pPage = (TMdbPage *)m_VarCharCtrl.GetAddrByPageID(m_pVarchar,iPageId);
        CHECK_OBJ(pPage);
        //移动文件指针
        CHECK_RET(fseek(pFile,(iPageId - m_tTSFileHead.m_iStartPage)*pPage->m_iPageSize,SEEK_CUR),
            "fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        m_tPageCtrl.Attach((char *)pPage,false,true);
        m_tPageCtrl.WLock();
        do{
            if(fwrite(pPage, pPage->m_iPageSize, 1, pFile)!= 1 )
            {
                CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
            }
        }while(0);
        bmDirtyPage ^= cTemp;
        m_tPageCtrl.UnWLock();
        fflush(pFile);
        TADD_FUNC("Finish.");
        return iRet;
    }*/

    int TMdbCheckPoint::ClearRedoLog()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_iMaxLsn == 0)
        {
            TADD_NORMAL("Needn't Clear RedoLog");
            return iRet;
        }
        CHECK_OBJ(m_pShmDSN);
        TMdbConfig *m_pConfig = TMdbConfigMgr::GetMdbConfig(m_pShmDSN->GetInfo()->sName);
        CHECK_OBJ(m_pConfig);
        m_tFileList.Init(m_pConfig->GetDSN()->sRedoDir);
        m_tFileList.GetFileList(1,0,"Redo.",".OK");
        char sFileName[MAX_FILE_NAME] = {0};
        TMdbRedoLogParser tParser;
		char* ptrFileName =  NULL;
        while(m_tFileList.Next(sFileName) == 0)
        {
        	ptrFileName =  NULL;
        	if(m_tFileList.GetFileNameFromFullFileName(ptrFileName) != 0)
				continue;
            // m_tSplit.SplitString(sFileName,'.');
             m_tSplit.SplitString(ptrFileName,'.');
             long long iLsn = TMdbNtcStrFunc::StrToInt(m_tSplit[2]);
             if(iLsn <= m_iMaxLsn)
            {
                TADD_NORMAL("Remove FileName=[%s],lsn=[%lld],maxLsn=[%lld]",sFileName,iLsn,m_iMaxLsn);
                TMdbNtcFileOper::Remove(sFileName);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    
    TMdbLoadFromDisk::TMdbLoadFromDisk()
    {
        
    }

    TMdbLoadFromDisk::~TMdbLoadFromDisk()
    {
        
    }

    int TMdbLoadFromDisk::Init(char * sDsn)
    {
        int iRet = 0;
        CHECK_OBJ(sDsn);
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDSN);
        CHECK_RET(m_tPageCtrl.SetDSN(sDsn),"PageCtrl SetDSN[%s] Failed.",sDsn);
        CHECK_RET(m_tVarCharCtrl.Init(sDsn),"Init failed.");
        m_vTSFile.clear();
        m_vVarCharFile.clear();
        return iRet;
    }

    int TMdbLoadFromDisk::LinkFile()
    {
        int iRet = 0;
        TADD_FUNC("Start");
        CHECK_OBJ(m_pShmDSN);
        for(unsigned int i = 0; i<m_vTSFile.size();i++)
        {
            m_vTSFile[i].clear();
        }
        m_vTSFile.clear();
        for(unsigned int i = 0; i<m_vVarCharFile.size();i++)
        {
            m_vVarCharFile[i].clear();
        }
        CHECK_RET(m_tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
        m_tFileList.Clear();
        StorageFile tempFile;
        char sFileHead[MAX_NAME_LEN];
        char sFileName[MAX_NAME_LEN];
		char sName[MAX_NAME_LEN];
        //连接表空间文件
        TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            if(itor->m_bFileStorage == false){continue;}
            StorageFile tempFile;
            SAFESTRCPY(tempFile.m_sTSName, sizeof(tempFile.m_sTSName), itor->sName);
            memset(sFileHead,0,sizeof(sFileHead));
            snprintf(sFileHead,sizeof(sFileHead),"%s%s",TS_FILE_PREFIX,itor->sName);
            m_tFileList.GetFileList(1,0,sFileHead,TS_FILE_SUFFIX);
            if (m_tFileList.GetFileCounts() == 0)
            {
                TADD_ERROR(ERR_APP_STORAGE_NOT_EXIST, "NO Storage file found.");
                return ERR_APP_STORAGE_NOT_EXIST;
            }
            while(m_tFileList.Next(sFileName) == 0)
            {

				char *ptrFileName =  NULL;
				char *ptrPos =  NULL;
				memset(sName,0,sizeof(sName));

				if(m_tFileList.GetFileNameFromFullFileName(ptrFileName) != 0)
					continue;

				CHECK_OBJ(ptrFileName);
				ptrPos = strrchr(ptrFileName,'_');
				if(ptrPos == NULL)
					continue;
				int iLen = ptrPos-ptrFileName;
				if(iLen<sizeof(sName))
					strncpy(sName,ptrFileName,iLen);

				if(strcmp(sName,sFileHead) == 0)
            	{
            		StorageFileHandle tempSFHandle;
            		strncpy(tempSFHandle.m_sFileName, sFileName, sizeof(tempSFHandle.m_sFileName));
                	FILE * pFile = NULL;
                	pFile = fopen(sFileName,"rb+");//不需要关闭，一直link住
                	CHECK_OBJ(pFile);
					tempSFHandle.m_fFile = pFile;
                	tempFile.m_vStorageFile.push_back(tempSFHandle);
				}
            }
            m_vTSFile.push_back(tempFile);
        }

        //连接varchar文件
        TShmList<TMdbVarchar>::iterator itor_var = m_pShmDSN->m_VarCharList.begin();
        for(;itor_var != m_pShmDSN->m_VarCharList.end();++itor_var)
        {
            StorageFile tempFile;
            tempFile.m_iVarcharId = itor_var->iVarcharID;
            memset(sFileHead,0,sizeof(sFileHead));
            snprintf(sFileHead,sizeof(sFileHead),"%s%d",VARCHAR_FILE_PREFIX,itor_var->iVarcharID);
            m_tFileList.GetFileList(1,0,sFileHead,TS_FILE_SUFFIX);
            if(m_tFileList.GetFileCounts() == 0){continue;}
            while(m_tFileList.Next(sFileName) == 0)
            {
                StorageFileHandle tempSFHandle;
            	strncpy(tempSFHandle.m_sFileName, sFileName, sizeof(tempSFHandle.m_sFileName));
                FILE * pFile = NULL;
                pFile = fopen(sFileName,"rb+");//不需要关闭，一直link住
                CHECK_OBJ(pFile);
				tempSFHandle.m_fFile = pFile;
                tempFile.m_vStorageFile.push_back(tempSFHandle);
            }
            m_vVarCharFile.push_back(tempFile);
        }
        return iRet;
    }

    int TMdbLoadFromDisk::LoadNormalData()
    {
        int iRet = 0;
        TADD_FUNC("Start");
        TMdbExecuteEngine m_tExecEngine;
        long long iMaxLsn = 0;
        for(unsigned int i = 0;i < m_vTSFile.size();++i)
        {
            StorageFile * pTSFile = &m_vTSFile[i];
            TMdbTableSpace* pTS = m_pShmDSN->GetTableSpaceAddrByName(pTSFile->m_sTSName);
            CHECK_OBJ(pTS);
            CHECK_RET(m_TSCtrl.Init(m_pShmDSN->GetInfo()->sName,pTSFile->m_sTSName),"Init failed.");
            CHECK_RET(m_TSCtrl.SetHasTableChange(),"SetHasTableChange failed.");
            for(unsigned int m = 0; m<pTSFile->m_vStorageFile.size();++m)
            {
                FILE* pFile=pTSFile->m_vStorageFile[m].m_fFile;
                CHECK_OBJ(pFile);
                TMdbPage * pFilePage = NULL;
                CHECK_RET(StartReadPage(pFile),"StartToReadPage failed.");
                while((iRet = GetNextPage(pFilePage)) > 0)
                {
                    TADD_DETAIL("TS=[%s],sTableName=[%s],pageid=[%d]",pTSFile->m_sTSName,pFilePage->m_sTableName,pFilePage->m_iPageID);
                    TMdbTable * pTable = m_pShmDSN->GetTableByName(pFilePage->m_sTableName);
                    if(pTable == NULL){continue;}
                    CHECK_RET(StorageCopyCtrl.Init(m_pShmDSN,pTable, pTS,&m_tPageCtrl,&m_TSCtrl),"StorageCopyCtrl Init Failed.");
                    CHECK_RET(StorageCopyCtrl.Load(pFilePage),"StorageCopyCtrl Load Failed,PageID[%d],table_name[%s]",pFilePage->m_iPageID,pFilePage->m_sTableName);
                    iMaxLsn = iMaxLsn < pFilePage->m_iPageLSN?pFilePage->m_iPageLSN:iMaxLsn;
                }
            }
            TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Load TableSpace[%s].",pTSFile->m_sTSName);
        }
        m_pShmDSN->GetInfo()->SetLsn(iMaxLsn);
        TADD_NORMAL("system lsn=[%lld]",iMaxLsn);
        //重新构建索引段
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
            TMdbTable * pTable = &(*itor);
            if(pTable->bIsSysTab){continue;}//跳过系统表
            CHECK_RET(m_tExecEngine.ReBuildTableFromPage(m_pShmDSN->GetInfo()->sName,pTable),"ReBuildTableFromPage failed.");
        }
        TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Rebuild Index.");
        TADD_FUNC("end.");
        return iRet;
    }

    int TMdbLoadFromDisk::LoadVarcharData()
    {
        int iRet = 0;
        TADD_FUNC("Start");
        char* pAddr = NULL;
        bool isNoPage = false;
        for(unsigned int i = 0;i < m_vVarCharFile.size();++i)
        {
            StorageFile * pVarcharFile = &m_vVarCharFile[i];
            TMdbVarchar* pVarChar = m_pShmDSN->GetVarchar(pVarcharFile->m_iVarcharId);
            CHECK_OBJ(pVarChar);
            for(unsigned int m = 0; m<pVarcharFile->m_vStorageFile.size();++m)
            {
                FILE* pFile=pVarcharFile->m_vStorageFile[m].m_fFile;
                CHECK_OBJ(pFile);
                TMdbPage * pFilePage = NULL;
                CHECK_RET(StartReadPage(pFile),"StartToReadPage failed.");
                while((iRet = GetNextPage(pFilePage)) > 0)
                {
                    pAddr = NULL;
                    isNoPage = false;
                    while(true)
                    {
                        if(pFilePage->m_iPageID <=0)
                        {
                            CHECK_RET(-1,"m_iPageID[%d] is invalid",pFilePage->m_iPageID);
                        }
                        
                        pAddr = m_tVarCharCtrl.GetAddrByPageID(pVarChar,pFilePage->m_iPageID,isNoPage);
                        if(isNoPage == true)
                        {
                            CHECK_RET(m_tVarCharCtrl.CreateShm(pVarChar,false),"CreateShm Failed");
                            continue;
                        }
                        else
                        {
                            CHECK_OBJ(pAddr);
                        }
                        break;
                    }
                    if(((TMdbPage*)pAddr)->m_iPageSize != pFilePage->m_iPageSize)
                    {
                        CHECK_RET(ERR_APP_INVALID_PARAM,"Mem PageSize[%d] != File PageSize[%d].",((TMdbPage*)pAddr)->m_iPageSize,pFilePage->m_iPageSize);
                    }
                    //这里只拷贝页的数据部分，不拷贝页头，页头的整理，到后面统一处理
                    int iPrePageID = ((TMdbPage*)pAddr)->m_iPrePageID;
                    int iNextPageID = ((TMdbPage*)pAddr)->m_iNextPageID;
                    memcpy(pAddr,pFilePage,pFilePage->m_iPageSize);
                    ((TMdbPage*)pAddr)->m_iPrePageID = iPrePageID;
                    ((TMdbPage*)pAddr)->m_iNextPageID = iNextPageID;

                    //回收页面里面非文件存储的记录
                    int m_iNode = ((TMdbPage*)pAddr)->m_iFullPageNode;
                    while(m_iNode > 0)
                    {
                        TMdbPageNode * pPageNode = (TMdbPageNode *)(pAddr + m_iNode);
                        if(pPageNode->cStorage == 'N')
                        {
                            int iOffSet = m_iNode+sizeof(TMdbPageNode);
                            CHECK_RET(((TMdbPage*)pAddr)->PushBack(iOffSet),"PushBack Failed");
                            m_iNode = ((TMdbPage*)pAddr)->m_iFullPageNode;
                        }
                        else
                        {
                           m_iNode = pPageNode->iNextNode;
                        }
                    }
                }
            }

             
             pVarChar->iBlockId = pVarcharFile->m_vStorageFile.size();
             TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Load Varchar[%d],iBlockId[%d]",pVarcharFile->m_iVarcharId,pVarChar->iBlockId);
        }
        //所有的页加载完了，重新整理free链和full链
        TShmList<TMdbVarchar>::iterator itor = m_pShmDSN->m_VarCharList.begin();
        for(;itor != m_pShmDSN->m_VarCharList.end();++itor)
        {
            TMdbVarchar * pMdbVarchar = &(*itor);
            m_tVarCharCtrl.SetVarchar(pMdbVarchar);
            for(int iPageId = 1; iPageId <= itor->iTotalPages;iPageId++)
            {
                bool isNoPage = false;
                TMdbPage* pPage = (TMdbPage*)(m_tVarCharCtrl.GetAddrByPageID(pMdbVarchar, iPageId,isNoPage));
                if(isNoPage == true)
                {
                     CHECK_RET(ERR_APP_INVALID_PARAM,"not find page[%d] in  varchar[%d]",iPageId,pMdbVarchar->iVarcharID);
                }

                if(TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState,"full") == 0)
                {
                    CHECK_RET(m_tVarCharCtrl.PageFreeToFull(pPage),"PageFreeToFull Faild");
                }
            }
        }
        TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Load Varchar Finish");
        return iRet;
    }

    int TMdbLoadFromDisk::LoadRedoLog()
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        CHECK_OBJ(m_pShmDSN);
        TMdbFlushEngine tMdbFlushEngine;
        CHECK_RET(tMdbFlushEngine.Init(m_pShmDSN->GetInfo()->sName),"tMdbFlushEngine Init failed.");
        TMdbConfig * m_pConfig = TMdbConfigMgr::GetMdbConfig(m_pShmDSN->GetInfo()->sName);
        CHECK_OBJ(m_pConfig);
        TMdbFileList tFileList;
        tFileList.Init(m_pConfig->GetDSN()->sRedoDir);
        tFileList.GetFileList(1,0,"Redo.","OK");
        char sFileName[MAX_FILE_NAME] = {0};
        char* sRecord = NULL;
        int iLen = 0;
        TMdbRedoLogParser tParser;
        TMdbLCR tLCR;
        int Count = 0;
        int FailedCount = 0;
		int iDropIndex[128] = {0};
		int paramIndex = 0;
		bool bIsInvalidUpdate = true;
        while(tFileList.Next(sFileName) == 0)
        {
            if(tParser.ParseFile(sFileName) != 0)
            {
                TADD_ERROR(-1,"ParseFile Failed[%s]",sFileName);
                continue;
            }
            Count = 0;
            FailedCount = 0;
            while(true)
            {
                iRet = tParser.NextRecord(sRecord,iLen);
                if(iRet != 0) {continue;}//记录有问题尝试跳过该记录
                if(sRecord == NULL){break;}//结束            
                if(tParser.Analyse(sRecord,tLCR) != 0)
                {//如果解析失败则跳过该记录继续处理
                    TADD_ERROR(-1,"Analyse Failed[%s]",sRecord);
                    continue;
                }

				bIsInvalidUpdate = true;
				paramIndex = 0;
				for(int i = 0; i < 128; i++)
				{
					iDropIndex[i] = 0;
				}
				switch(tLCR.m_iSqlType)
				{
				case TK_INSERT:
					{
						std::vector<TLCRColm>::iterator itor = tLCR.m_vColms.begin();
						for (; itor != tLCR.m_vColms.end(); ++itor)
						{
							TADD_DETAIL("tLCR.m_sTableName.c_str() = [%s], itor->m_sColmName.c_str() = [%s]", tLCR.m_sTableName.c_str(), itor->m_sColmName.c_str());
							if(m_pConfig->IsDropColumn(tLCR.m_sTableName.c_str(), itor->m_sColmName.c_str()))
							{
								iDropIndex[paramIndex++] = 1;
								continue;
							}
							paramIndex++;
						}
						break;
					}
				case TK_UPDATE:
					{
						std::vector<TLCRColm>::iterator itor = tLCR.m_vColms.begin();
						for(; itor != tLCR.m_vColms.end(); ++itor)
						{
							if(m_pConfig->IsDropColumn(tLCR.m_sTableName.c_str(), itor->m_sColmName.c_str()))
							{
								iDropIndex[paramIndex++] = 1;
								continue;
							}
							bIsInvalidUpdate = false;
							paramIndex++;
						}
						break;
					}
				}
				
                if(tMdbFlushEngine.Excute(tLCR,QUERY_NO_ORAFLUSH |QUERY_NO_REDOFLUSH|QUERY_NO_SHARDFLUSH, iDropIndex, bIsInvalidUpdate) != 0){TADD_ERROR(-1,"Excute Failed");FailedCount++;continue;}
                Count++;
            }
            TADD_NORMAL("Load File[%s] Successful Count=[%d] FailedCount=[%d]",sFileName,Count,FailedCount);
        }
        TADD_FUNC("end.");
        return iRet;
    }

    int TMdbLoadFromDisk::StartReadPage(FILE* pFile)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pFile);
        CHECK_RET(fseek(pFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
        if(fread(&m_tTSFileHead,sizeof(m_tTSFileHead),1,pFile) != 1)
        {
            if(feof(pFile))
            {//文件结束
                CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
            }
            CHECK_RET(ERR_OS_READ_FILE,"errno=%d, errormsg=[%s].",errno, strerror(errno));
        }
		TADD_NORMAL("m_tTSFileHead.iVarcharID = [%d], m_tTSFileHead.m_iStartPage = [%d], m_tTSFileHead.m_iEndPage = [%d]",m_tTSFileHead.iVarcharID, m_tTSFileHead.m_iStartPage, m_tTSFileHead.m_iEndPage);
        CHECK_RET(m_tFileBuff.Init(m_tTSFileHead.m_iPageSize, pFile),"m_tFileBuff Init Failed");
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbLoadFromDisk::GetNextPage(TMdbPage * & pMdbPage)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        iRet = m_tFileBuff.ReadFromFile();
        TADD_DETAIL("Cur_PageCount=[%d].",iRet);
        if(iRet <= 0) {return iRet;}//文件结束
        pMdbPage = NULL;
        pMdbPage = (TMdbPage*)m_tFileBuff.Next();
        if(pMdbPage == NULL)
        {
            TADD_ERROR(-1,"GetNextPage Failed");
            iRet = -1;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  TMdbStorageEngine
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbStorageEngine::TMdbStorageEngine():
    m_pShmDSN(NULL)
    {
        m_pConfig = NULL;
        Clear();
    }
    /******************************************************************************
    * 函数名称	:  ~TMdbStorageEngine
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbStorageEngine::~TMdbStorageEngine()
    {
        Clear();
    }
    /******************************************************************************
    * 函数名称	:  Clear
    * 函数描述	:  清理
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    void TMdbStorageEngine::Clear()
    {
       size_t i = 0;
       for(i = 0;i < m_vTSFile.size();i++)
       {
            SAFE_DELETE(m_vTSFile[i]);
       }
       m_vTSFile.clear();

       for(i = 0;i < m_vVarCharFile.size();i++)
       {
            SAFE_DELETE(m_vVarCharFile[i]);
       }
       m_vVarCharFile.clear();
    }

    /******************************************************************************
    * 函数名称	:  Attach
    * 函数描述	:  链接共享内存
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbStorageEngine::Attach(const char * sDsn)
    {
        int iRet = 0;
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDSN);
        //检测目录
        char * sStorageDir = m_pShmDSN->GetInfo()->sStorageDir;
        if(false == TMdbNtcDirOper::IsExist(sStorageDir))
        {
             if(false == TMdbNtcDirOper::MakeFullDir(sStorageDir))
             {
                CHECK_RET(ERR_OS_CREATE_DIR,"can not mkdir[%s]",sStorageDir);
             }
        }
        return iRet;
    }

	
    int TMdbStorageEngine::BackupFile()
    {
        TADD_FUNC("Start");
        int iRet = 0;
        
        char bakStartFile[MAX_NAME_LEN] = {0};
		char bakOKFile[MAX_NAME_LEN] = {0};
		snprintf(bakStartFile,sizeof(bakStartFile),"%s/%s",m_pShmDSN->GetInfo()->sStorageDir,"BACKUP.START");
		snprintf(bakOKFile,sizeof(bakOKFile),"%s/%s",m_pShmDSN->GetInfo()->sStorageDir,"BACKUP.OK");
		TMdbNtcFileOper::Remove(bakOKFile);
		TMdbNtcFileOper::MakeFile(bakStartFile);

		//重命名备份
		char sFileName[MAX_NAME_LEN] = {0};
		char sBakFileName[MAX_NAME_LEN] = {0};
		CHECK_RET(m_tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
		m_tFileList.Clear();
		m_tFileList.GetFileList(1,0,TS_FILE_PREFIX,TS_FILE_SUFFIX);
		if (m_tFileList.GetFileCounts() == 0)
		{
			TADD_NORMAL("No tablespace storage file found.");
		}
		while(m_tFileList.Next(sFileName) == 0)
		{
			memset(sBakFileName,0,sizeof(sBakFileName));
			snprintf(sBakFileName, sizeof(sBakFileName), "%s%s", sFileName, ".bak");
			if(!TMdbNtcFileOper::Rename(sFileName,sBakFileName)) return -1;
		}

		TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            TMdbTableSpace * pTS = &(*itor);
            if(pTS->sName[0] != 0)
            {
            	if(!pTS->m_bFileStorage) continue;
				pTS->m_iBlockId = 0;
            }
        }
		
		m_tFileList.Clear();
		m_tFileList.GetFileList(1,0,VARCHAR_FILE_PREFIX,TS_FILE_SUFFIX);
		if (m_tFileList.GetFileCounts() == 0)
		{
			TADD_NORMAL("No varchar storage file found.");
		}
		while(m_tFileList.Next(sFileName) == 0)
		{
			memset(sBakFileName,0,sizeof(sBakFileName));
			snprintf(sBakFileName, sizeof(sBakFileName), "%s%s", sFileName, ".bak");
			if(!TMdbNtcFileOper::Rename(sFileName,sBakFileName)) return -1;
		}

		TShmList<TMdbVarchar>::iterator itor_var = m_pShmDSN->m_VarCharList.begin();
        for(;itor_var != m_pShmDSN->m_VarCharList.end();++itor_var)
        {
        	TMdbVarchar * pVarchar = &(*itor_var);
			if(pVarchar->iVarcharID< VC_16 || pVarchar->iVarcharID > VC_8192) continue;
			pVarchar->iBlockId = 0;
        }
		
		TMdbNtcFileOper::Remove(bakStartFile);
		TMdbNtcFileOper::MakeFile(bakOKFile);
        TADD_FUNC("Finish");
        
        return iRet;
    }

    int TMdbStorageEngine::RemoveBakFile()
    {
        TADD_FUNC("Start");
        int iRet = 0;

		char sFileName[MAX_NAME_LEN] = {0};
		CHECK_RET(m_tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
		m_tFileList.Clear();
		m_tFileList.GetFileList(1,0,TS_FILE_PREFIX,BAK_FILE_SUFFIX);
		if (m_tFileList.GetFileCounts() == 0)
		{
			TADD_NORMAL("No tablespace backup file found.");
		}
		while(m_tFileList.Next(sFileName) == 0)
		{
			if(!TMdbNtcFileOper::Remove(sFileName)) return -1;
		}

		m_tFileList.Clear();
		m_tFileList.GetFileList(1,0,VARCHAR_FILE_PREFIX,BAK_FILE_SUFFIX);
		if (m_tFileList.GetFileCounts() == 0)
		{
			TADD_NORMAL("No varchar backup file found.");
		}
		while(m_tFileList.Next(sFileName) == 0)
		{
			if(!TMdbNtcFileOper::Remove(sFileName)) return -1;
		}
		
        TADD_FUNC("Finish");
        
        return iRet;
    }
	
    /******************************************************************************
    * 函数名称	:  FlushFull
    * 函数描述	:  刷出所有数据，作为起始点
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbStorageEngine::FlushFull()
    {
        int iRet = 0;

		CHECK_RET(BackupFile(), "BackupFile failed!");
        bool bFileSys = false;

		char bakFFStartFile[MAX_NAME_LEN] = {0};
		char bakFFOKFile[MAX_NAME_LEN] = {0};
		snprintf(bakFFStartFile,sizeof(bakFFStartFile),"%s/%s",m_pShmDSN->GetInfo()->sStorageDir,"FLUSHFULL.START");
		snprintf(bakFFOKFile,sizeof(bakFFOKFile),"%s/%s",m_pShmDSN->GetInfo()->sStorageDir,"FLUSHFULL.OK");
		TMdbNtcFileOper::Remove(bakFFOKFile);
		TMdbNtcFileOper::MakeFile(bakFFStartFile);
		
        //取出所有表空间
        TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            TMdbTableSpace * pTS = &(*itor);
            if(pTS->sName[0] != 0)
            {
            	  if(!pTS->m_bFileStorage) continue;
                TMdbTSFile tTSFile;
                CHECK_RET(tTSFile.LinkMdb(m_pShmDSN->GetInfo()->sName,pTS),"LinkMdb failed.");//链接mdb
                CHECK_RET(tTSFile.FlushFull(),"FlushFull failed."); //刷出所有数据
                bFileSys = true;
            }
        }
		
        if(!bFileSys) 
        {
            return iRet;
        }
		//刷新出varchar页

		TShmList<TMdbVarchar>::iterator itor_var = m_pShmDSN->m_VarCharList.begin();
        for(;itor_var != m_pShmDSN->m_VarCharList.end();++itor_var)
        {
        	TMdbVarchar * pVarchar = &(*itor_var);
			if(pVarchar->iVarcharID< VC_16 || pVarchar->iVarcharID > VC_8192) continue;
        	TMdbVarcharFile tVarcharFile;
            CHECK_RET(tVarcharFile.LinkMdb(m_pShmDSN->GetInfo()->sName, pVarchar),"Init Failed");
            CHECK_RET(tVarcharFile.FlushFull(),"FlushFull Failed");
        }

		TMdbNtcFileOper::Remove(bakFFStartFile);
		TMdbNtcFileOper::MakeFile(bakFFOKFile);

		CHECK_RET(RemoveBakFile(), "RemoveBakFile failed!");
        return iRet;
    }

    int TMdbStorageEngine::RemoveNormalFile()
    {
        int iRet = 0;
        char sFileName[MAX_NAME_LEN];
        TMdbFileList m_tFileList;
        CHECK_RET(m_tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
        m_tFileList.GetFileList(1,0,TS_FILE_PREFIX,TS_FILE_SUFFIX);
        while(m_tFileList.Next(sFileName) == 0)
        {
            TADD_NORMAL("Remove FileName=[%s]",sFileName);
            TMdbNtcFileOper::Remove(sFileName);
        }

		TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            TMdbTableSpace * pTS = &(*itor);
            if(pTS->sName[0] != 0)
            {
            	if(!pTS->m_bFileStorage) continue;
				pTS->m_iBlockId = 0;
            }
        }
        return iRet;
    }

    int TMdbStorageEngine::RemoveVarcharFile()
    {
        int iRet = 0;
        char sFileName[MAX_NAME_LEN];
        TMdbFileList m_tFileList;
        CHECK_RET(m_tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
        m_tFileList.GetFileList(1,0,VARCHAR_FILE_PREFIX,TS_FILE_SUFFIX);
        while(m_tFileList.Next(sFileName) == 0)
        {
            TADD_NORMAL("Remove Varchar FileName=[%s]",sFileName);
            TMdbNtcFileOper::Remove(sFileName);
        }

		TShmList<TMdbVarchar>::iterator itor_var = m_pShmDSN->m_VarCharList.begin();
        for(;itor_var != m_pShmDSN->m_VarCharList.end();++itor_var)
        {
        	TMdbVarchar * pVarchar = &(*itor_var);
			if(pVarchar->iVarcharID< VC_16 || pVarchar->iVarcharID > VC_8192) continue;
			pVarchar->iBlockId = 0;
        }
        return iRet;
    }

	int TMdbStorageEngine::RemoveChangeFile()
    {
        int iRet = 0;
        char sFileName[MAX_NAME_LEN];
        TMdbFileList m_tFileList;
        CHECK_RET(m_tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
        m_tFileList.GetFileList(1,0,TS_FILE_PREFIX,TS_CHANGE_FILE_SUFFIX);
        while(m_tFileList.Next(sFileName) == 0)
        {
            TADD_NORMAL("Remove FileName=[%s]",sFileName);
            TMdbNtcFileOper::Remove(sFileName);
        }

		TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            TMdbTableSpace * pTS = &(*itor);
            if(pTS->sName[0] != 0)
            {
            	if(!pTS->m_bFileStorage) continue;
				pTS->m_iChangeBlockId = 0;
            }
        }
		
		m_tFileList.Clear();
		m_tFileList.GetFileList(1,0,VARCHAR_FILE_PREFIX,TS_CHANGE_FILE_SUFFIX);
        while(m_tFileList.Next(sFileName) == 0)
        {
            TADD_NORMAL("Remove Varchar FileName=[%s]",sFileName);
            TMdbNtcFileOper::Remove(sFileName);
        }

		TShmList<TMdbVarchar>::iterator itor_var = m_pShmDSN->m_VarCharList.begin();
        for(;itor_var != m_pShmDSN->m_VarCharList.end();++itor_var)
        {
        	TMdbVarchar * pVarchar = &(*itor_var);
			if(pVarchar->iVarcharID< VC_16 || pVarchar->iVarcharID > VC_8192) continue;
			pVarchar->iChangeBlockId = 0;
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  FlushVarchar
    * 函数描述	:  刷出所有数据，作为起始点
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    /*
    int TMdbStorageEngine::FlushVarchar()
    {
        int iRet = 0;
        //取出所有表空间
        TShmList<TMdbVarchar>::iterator itor = m_pShmDSN->m_VarCharList.begin();
        for(;itor != m_pShmDSN->m_VarCharList.end();++itor)
        {
            TMdbVarchar * pVarchar = &(*itor);
            TMdbVarcharFile tVarcharFile;
            CHECK_RET(tVarcharFile.LinkMdb(m_pShmDSN->GetInfo()->sName,pVarchar),"LinkMdb failed.");//链接mdb
            CHECK_RET(tVarcharFile.FlushFull(),"FlushFull failed."); //刷出所有数据
        }
        return iRet;
    }
    */
    /******************************************************************************
    * 函数名称	:  LinkDisk
    * 函数描述	:  链接磁盘，读取磁盘数据
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*
    int TMdbStorageEngine::LinkDiskToMdb()
    {
        int iRet = 0;
        TADD_FUNC("Start");
        CHECK_OBJ(m_pShmDSN);
        TMdbFileList tFileList;
        CHECK_RET(tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
        Clear();//清理先
<<<<<<< .mine
        TSFile tTempTSFile;
        char sHead[128]={0};
        TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
=======
        char sTempFile[MAX_FILE_NAME] = {0};
        tFileList.GetFileList(1,0,TS_FILE_PREFIX,TS_FILE_SUFFIX);
        while(tFileList.Next(sTempFile) == 0)
>>>>>>> .r540
        {
            if(itor->m_bFileStorage == false){continue;}
            tTempTSFile.clear();
            SAFESTRCPY(tTempTSFile.m_sTSName, sizeof(tTempTSFile.m_sTSName), itor->sName)
            memset(sHead,0,sizeof(sHead));
            sprintf(sHead,"%s%s",TS_FILE_PREFIX,itor->sName);
            tFileList.GetFileList(1,0,sHead,TS_FILE_SUFFIX);
            char sTempFile[MAX_FILE_NAME] = {0};
            while(tFileList.Next(sTempFile) == 0)
            {
                TMdbTSFile * pTSFile = new TMdbTSFile();
                CHECK_RET(pTSFile->LinkFile(sTempFile),"LinkFile[%s] failed.",sTempFile);
                TMdbTableSpace * pTS = m_pShmDSN->GetTableSpaceAddrByName(pTSFile->m_tTSFileHead.m_sTSName);
                CHECK_OBJ(pTS);
                CHECK_RET(pTSFile->LinkMdb(m_pShmDSN->GetInfo()->sName,pTS),"LinkMdb failed.");
                tTempTSFile.m_vTSFile.push_back(pTSFile);
            }
            m_vTSFile.push_back(tTempTSFile);
        }
        
        tFileList.GetFileList(1,0,VARCHAR_FILE_PREFIX,TS_FILE_SUFFIX);
        while(tFileList.Next(sTempFile) == 0)
        {
            TMdbVarcharFile * pVarcharFile = new TMdbVarcharFile();
            CHECK_RET(pVarcharFile->LinkFile(sTempFile),"LinkFile[%s] failed.",sTempFile);
            TMdbVarchar * pVarchar = m_pShmDSN->GetVarchar(pVarcharFile->m_VarcharHead.iVarcharID);
            CHECK_OBJ(pVarchar);
            CHECK_RET(pVarcharFile->LinkMdb(m_pShmDSN->GetInfo()->sName,pVarchar),"LinkMdb failed.");
            m_vVarCharFile.push_back(pVarcharFile);
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }
    */

    /******************************************************************************
    * 函数名称	:  LoadDataFromDisk
    * 函数描述	:  从磁盘上载数据
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    /*
    int TMdbStorageEngine::LoadDataFromDisk()
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TMdbTableSpaceCtrl tTSCtrl;
        TMdbExecuteEngine tExecEngine;
        size_t i = 0;
		char sTempPath[MAX_PATH_NAME_LEN] = {0}, sTempFile[MAX_NAME_LEN] = {0}, sTempFullPath[MAX_PATH_NAME_LEN] = {0};
        for(i = 0;i < m_vTSFile.size();++i)
        {
            TMdbTSFile * pTSFile = m_vTSFile[i];
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(pTSFile->m_tTSFileHead.m_sTSName, SYS_TABLE_SPACE) )
            {//跳过系统表,系统表无需重新创建
                continue;
            }
            CHECK_RET(tTSCtrl.Init(m_pShmDSN->GetInfo()->sName,pTSFile->m_tTSFileHead.m_sTSName),"Init failed.");
            CHECK_RET(tTSCtrl.ReBuildFromDisk(pTSFile),"ReBuildFromDisk failed.");//重新创建表空间
            TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Load TableSpace[%s].",pTSFile->m_tTSFileHead.m_sTSName);
			//对没有按原有页序加载的表空间文件进行重新刷新
			TMdbConfig* m_pConfig = TMdbConfigMgr::GetMdbConfig(m_pShmDSN->GetInfo()->sName);
			TIntAlterAttr* pIntAlterAttr = NULL;
			if(m_pConfig->IsPageSizeAlter(pTSFile->m_tTSFileHead.m_sTSName, pIntAlterAttr) || StructHasChange(tTSCtrl.GetHasTableChange(),STRUCT_TABLE_IN_TABLESPACE_CHANGE))
			{
				memset(sTempPath, 0, sizeof(sTempPath));
				memset(sTempFile, 0, sizeof(sTempFile));
				memset(sTempFullPath, 0, sizeof(sTempFullPath));
				SAFESTRCPY(sTempPath,sizeof(sTempPath),m_pShmDSN->GetInfo()->sStorageDir);
			    if(sTempPath[strlen(sTempPath)-1] != '/')
			    {
			        sTempPath[strlen(sTempPath)] = '/';  
			    }
				sprintf(sTempFile, "%s%s%s", TS_FILE_PREFIX, pTSFile->m_tTSFileHead.m_sTSName, TS_FILE_SUFFIX);
				sprintf(sTempFullPath, "%s%s", sTempPath, sTempFile);
				if(TMdbNtcFileOper::IsExist(sTempFullPath))
		        {
		            TMdbNtcFileOper::Remove(sTempFullPath);
		        }
                //CHECK_RET(pTSFile->FlushFull(),"FlushFull failed."); //刷出所有数据
				TMdbTSFile tTSFile;
				CHECK_RET(tTSFile.LinkMdb(m_pShmDSN->GetInfo()->sName,pTSFile->m_pMdbTS),"LinkMdb failed.");//链接mdb
				CHECK_RET(tTSFile.FlushFull(),"FlushFull failed."); //刷出所有数据
			}
        }
		
        //重新构建索引段
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
            TMdbTable * pTable = &(*itor);
            if(pTable->bIsSysTab){continue;}//跳过系统表
            CHECK_RET(tExecEngine.ReBuildTableFromPage(m_pShmDSN->GetInfo()->sName,pTable),"ReBuildTableFromPage failed.");
        }
        TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Rebuild Index.");
        TADD_FUNC("end.");
        return iRet;
    }
    */

    /******************************************************************************
    * 函数名称	:  LoadDataFromDisk
    * 函数描述	:  从磁盘上载数据
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    /*
    int TMdbStorageEngine::LoadVarcharDataFromDisk()
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        CHECK_OBJ(m_pShmDSN);
        TMdbVarCharCtrl tVarCharCtrl;
        CHECK_RET(tVarCharCtrl.Init(m_pShmDSN->GetInfo()->sName),"Init failed.");
        TMdbExecuteEngine tExecEngine;
        bool isNoPage = false;
        size_t i = 0;
        for(i = 0;i < m_vVarCharFile.size();++i)
        {
            TMdbPage * pFilePage = NULL;
            TMdbVarcharFile * pVarCharFile = m_vVarCharFile[i];
            CHECK_OBJ(pVarCharFile);
            TMdbVarchar* pVarChar = m_pShmDSN->GetVarchar(pVarCharFile->m_VarcharHead.iVarcharID);
            CHECK_RET(pVarCharFile->StartToReadPage(),"StartToReadPage failed.");
            while((iRet = pVarCharFile->GetNextPage(pFilePage)) > 0)
            {
                CHECK_OBJ(pFilePage);
                char* pAddr = NULL;
                while(true)
                {
                    if(pFilePage->m_iPageID <=0)
                    {
                        return -1;
                    }
                    pAddr = tVarCharCtrl.GetAddrByPageID(pVarChar,pFilePage->m_iPageID,isNoPage);
                    if(isNoPage == true)
                    {
                        CHECK_RET(tVarCharCtrl.CreateShm(pVarChar,false),"CreateShm Failed");
                        continue;
                    }
                    else
                    {
                        CHECK_OBJ(pAddr);
                    }
                    break;
                }
                if(((TMdbPage*)pAddr)->m_iPageSize != pFilePage->m_iPageSize)
                {
                    CHECK_RET(ERR_APP_INVALID_PARAM,"Mem PageSize[%d] != File PageSize[%d].",((TMdbPage*)pAddr)->m_iPageSize,pFilePage->m_iPageSize);
                }
                memcpy(pAddr,pFilePage,pFilePage->m_iPageSize);

                //回收页面里面非文件存储的记录
                int m_iNode = ((TMdbPage*)pAddr)->m_iFullPageNode;
                while(m_iNode > 0)
                {
                    TMdbPageNode * pPageNode = (TMdbPageNode *)(pAddr + m_iNode);
                    if(pPageNode->cStorage == 'N')
                    {
                        int iOffSet = m_iNode+sizeof(TMdbPageNode);
                        CHECK_RET(((TMdbPage*)pAddr)->PushBack(iOffSet),"PushBack Failed");
                        m_iNode = ((TMdbPage*)pAddr)->m_iFullPageNode;
                    }
                    else
                    {
                       m_iNode = pPageNode->iNextNode;
                    }
                }
            }
            TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Load Varchar[%d].",pVarCharFile->m_VarcharHead.iVarcharID);
        }
		//重新生成varchar存储文件
		TMdbFileList tFileList;
        CHECK_RET(tFileList.Init(m_pShmDSN->GetInfo()->sStorageDir),"Init failed.");
        char sTempFile[MAX_FILE_NAME] = {0};
		tFileList.GetFileList(1,0,VARCHAR_FILE_PREFIX,TS_FILE_SUFFIX);
        while(tFileList.Next(sTempFile) == 0)
        {
            if(TMdbNtcFileOper::IsExist(sTempFile))
	        {
	            TMdbNtcFileOper::Remove(sTempFile);
	        }
        }
		TShmList<TMdbVarchar>::iterator itor = m_pShmDSN->m_VarCharList.begin();
        for(;itor != m_pShmDSN->m_VarCharList.end();++itor)
        {
            TMdbVarchar * pVarchar = &(*itor);
            TMdbVarcharFile tVarcharFile;
            CHECK_RET(tVarcharFile.LinkMdb(m_pShmDSN->GetInfo()->sName,pVarchar),"LinkMdb failed.");//链接mdb
            CHECK_RET(tVarcharFile.FlushFull(),"FlushFull failed."); //刷出所有数据
        }
        
        TADD_FUNC("end.");
        return iRet;
    }
    */

/******************************************************************************
* 函数名称	:  LoadDataFromRedoLog
* 函数描述	:  从redolog上载数据
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  dong.chun
*******************************************************************************/
/*
int TMdbStorageEngine::LoadDataFromRedoLog()
{
    int iRet = 0;
    TADD_FUNC("Start.");
    CHECK_OBJ(m_pShmDSN);
    TMdbFlushEngine tMdbFlushEngine;
    CHECK_RET(tMdbFlushEngine.Init(m_pShmDSN->GetInfo()->sName),"tMdbFlushEngine Init failed.");
    m_pConfig = TMdbConfigMgr::GetMdbConfig(m_pShmDSN->GetInfo()->sName);
    CHECK_OBJ(m_pConfig);
    TMdbFileList tFileList;
    tFileList.Init(m_pConfig->GetDSN()->sRedoDir);
    tFileList.GetFileList(1,0,"Redo.","");
    char sFileName[MAX_FILE_NAME] = {0};
    char* sRecord = NULL;
    int iLen = 0;
    TMdbRedoLogParser tParser;
    TMdbLCR tLCR;
    while(tFileList.Next(sFileName) == 0)
    {
        if(tParser.ParseFile(sFileName) != 0)
        {
            TADD_ERROR(-1,"ParseFile Failed[%s]",sFileName);
            continue;
        }
        while(true)
        {
            iRet = tParser.NextRecord(sRecord,iLen);
            if(iRet != 0) {continue;}//记录有问题尝试跳过该记录
            if(sRecord == NULL){break;}//结束            
            if(tParser.Analyse(sRecord,tLCR) != 0)
            {//如果解析失败则跳过该记录继续处理
                TADD_ERROR(-1,"Analyse Failed[%s]",sRecord);
                continue;
            }
            if(tMdbFlushEngine.Excute(tLCR) != 0){TADD_ERROR(-1,"Excute Failed");continue;}
        }
    }
    TADD_FUNC("end.");
    return iRet;
}
*/
    /******************************************************************************
    * 函数名称	:  ClearRedoLog
    * 函数描述	: 清理过期的redo日志
    * 输入		:  清理 <= iChkLSN 的日志
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    /*
    int TMdbStorageEngine::ClearRedoLog(int iChkLSN)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pShmDSN);
        m_pConfig = TMdbConfigMgr::GetMdbConfig(m_pShmDSN->GetInfo()->sName);
        CHECK_OBJ(m_pConfig);
        TMdbFileList tFileList;
        tFileList.Init(m_pConfig->GetDSN()->sRedoDir);
        tFileList.GetFileList(1,0,"Redo.","");
        char sFileName[MAX_FILE_NAME] = {0};
        TMdbRedoLogParser tParser;
        char* sRecBuff = NULL;
        int iLen;
        while(tFileList.Next(sFileName) == 0)
        {
            TADD_NORMAL("FileName=[%s]",sFileName);
            CHECK_RET(tParser.ParseFile(sFileName),"ParseFile error[%s]",sFileName);
            CHECK_RET(tParser.NextRecord(sRecBuff,iLen),"NextRecord failed.");
            if(0 == iLen){
            CHECK_RET(ERR_APP_INVALID_PARAM,"len = 0 ");
            }
            CHECK_OBJ(sRecBuff);
            TADD_NORMAL("FileName=[%s],lsn=[%lld]",sFileName,tParser.GetPageLSN(sRecBuff));
            if(tParser.GetPageLSN(sRecBuff) < iChkLSN)
            {//TODO:这边先粗略的估算下，将该文件认定为过期文件,只取第一条记录是错误的。
                TMdbNtcFileOper::Remove(sFileName);
                TADD_NORMAL("Remove[%s]",sFileName);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    */
    /******************************************************************************
    * 函数名称	:  TMdbRedoLogParser
    * 函数描述	:  redo日志解析器
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  dong.chun
    *******************************************************************************/
    TMdbRedoLogParser::TMdbRedoLogParser()
    {
        m_pCurFile = NULL;
        m_pFileBuff = NULL;
        m_iBuffSize = 0;
        m_iCurPos = 0;
        memset(m_sRout, 0, sizeof(m_sRout));
        int iSize = sizeof(m_iFactor)/sizeof(int);
        for(int i = 0; i < iSize; i++)
        {
            m_iFactor[i] = 1;
            int j = 0;
            for(j = 0; j < i; j++)
            {
                m_iFactor[i] *= 10;
            }
        }
    }
    TMdbRedoLogParser::~TMdbRedoLogParser()
    {
        SAFE_DELETE(m_pFileBuff);
        SAFE_CLOSE(m_pCurFile);
    }
    /******************************************************************************
    * 函数名称  :  ParseFile
    * 函数描述  :  解析文件
    * 输入      :  sFile - 待解析的文件
    * 输入      :  
    * 输出      :  
    * 返回值    :  0 - 成功!0 -失败
    * 作者      :  dong.chun
    *******************************************************************************/
    int TMdbRedoLogParser::ParseFile(const char * sFile)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(sFile);
        if(TMdbNtcFileOper::IsExist(sFile) == false)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"file[%s] is not exist.",sFile);
        }
        SAFE_CLOSE(m_pCurFile);
        m_pCurFile = fopen(sFile,"rb");
        CHECK_OBJ(m_pCurFile);
        long long iFileSize = GetFileSize(sFile);
        if(iFileSize < 0)
        {
            TADD_ERROR("GetFileSize() failed.", __FILE__, __LINE__, sFile);
            return iFileSize;    
        }

        if(iFileSize > m_iBuffSize)
        {
            SAFE_DELETE(m_pFileBuff);
            m_pFileBuff = new(std::nothrow) char[iFileSize+100]; 
            CHECK_OBJ(m_pFileBuff);
            m_iBuffSize = iFileSize;
        }

        memset(m_pFileBuff,0,m_iBuffSize+100);

        if(fread(m_pFileBuff,iFileSize,1,m_pCurFile)!=1)
        {
            TADD_ERROR(-1,"Read File[%s] Failed.", sFile);
            return ERR_OS_OPEN_FILE;   
        }
        m_iCurPos = 0;
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称  :  NextRecord
    * 函数描述  :  下一条记录
    * 输入      :  
    * 输入      :  
    * 输出      :  
    * 返回值    :  0 - 成功!0 -失败
    * 作者      :  dong.chun
    *******************************************************************************/
    int TMdbRedoLogParser::NextRecord(char* &sRecordBuff,int & iLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //读文件头检测是否正确
        iLen = 0;
        sRecordBuff = NULL;
        if(m_iCurPos >= m_iBuffSize) //读完了
        {
            return iRet;
        }
        if('^' != m_pFileBuff[m_iCurPos] || '^' != m_pFileBuff[m_iCurPos+1])
        {
        	if(m_pFileBuff[m_iCurPos] == 0)
			{
				return iRet;
			}
            m_iCurPos++;
            CHECK_RET(ERR_APP_INVALID_PARAM,"head is not ^,m_iCurPos[%d],m_iBuffSize=[%d]",m_iCurPos,m_iBuffSize);
        }

        iLen = GetNum(&m_pFileBuff[m_iCurPos+2], 4);
        if(iLen <= 0 || iLen > MAX_VALUE_LEN || m_iCurPos+iLen > m_iBuffSize)
        {
            m_iCurPos++;
            CHECK_RET(ERR_APP_INVALID_PARAM,"Invalid Data! Len=[%d],m_iCurPos[%d],m_iBuffSize=[%d]",iLen,m_iCurPos,m_iBuffSize);
        }
        //检测文件尾
        if('#' !=m_pFileBuff[m_iCurPos+iLen -2] || '#' != m_pFileBuff[m_iCurPos+iLen -1])
        {
            m_iCurPos++;
            CHECK_RET(ERR_APP_INVALID_PARAM,"sRecordBuff = [%s][%d] is error,m_iCurPos[%d],m_iBuffSize=[%d]",sRecordBuff,iLen,m_iCurPos,m_iBuffSize);
        }

        sRecordBuff = &m_pFileBuff[m_iCurPos];
        m_iCurPos+=iLen;
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称  :  Analyse
    * 函数描述  :  解析一条记录
    * 输入      :  
    * 输入      :  
    * 输出      :  
    * 返回值    :  0 - 成功!0 -失败
    * 作者      :  dong.chun
    *******************************************************************************/
    int TMdbRedoLogParser::Analyse(const char * sRecord, TMdbLCR & tMdbLcr)
    {
        TADD_FUNC("Start.");
        
        int iRet = 0;
        int iPos = 0;
        
        tMdbLcr.Clear();
        //解析记录头信息
        iPos+=2; //跳过^^
        //iLen [4 bytes]
        tMdbLcr.m_iLen = GetNum(sRecord+iPos, 4);
        iPos+=4;
        // Version [1 bytes]
        tMdbLcr.m_iVersion = GetNum(sRecord + iPos, 1);
        iPos += 1;

        // SourceId [1 bytes]
        tMdbLcr.m_iSyncFlag =  GetNum(sRecord + iPos, 4);
        iPos += 4;

        // LSN [20 bytes]
        tMdbLcr.m_lLsn = GetNum(sRecord + iPos, 20);
        iPos += 20;

        // TimeStamp [10 bytes]
        tMdbLcr.m_iTimeStamp = GetNum(sRecord + iPos, 10);
        iPos += 10;

        // RowID [8 bytes]
        tMdbLcr.m_lRowId = GetNum(sRecord + iPos, 8);
        iPos += 8;

        // SQL Type [2 bytes]
        tMdbLcr.m_iSqlType = GetNum(sRecord + iPos, 2);
        iPos += 2;

        // Routing ID [4 bytes]
        tMdbLcr.m_iRoutID = GetNum(sRecord + iPos, 4);
        iPos += 4;

        // Column Name Length [4 bytes]
        tMdbLcr.m_iColmLen = GetNum(sRecord + iPos, 4);
        iPos += 4;

        //解析表名、字段名、字段值
        char m_sColName[MAX_VALUE_LEN];
        memcpy(m_sColName,sRecord+iPos,tMdbLcr.m_iColmLen);
        m_sColName[tMdbLcr.m_iColmLen] = 0;
        m_tSplitWhere.SplitString(m_sColName,'|');
        int iActCnt = 0;
        if(m_tSplitWhere.GetFieldCount() == 1)
        {
            m_tSplitCol.SplitString(m_sColName,',');
            tMdbLcr.m_sTableName = m_tSplitCol[0];
            for(unsigned int i = 1; i < m_tSplitCol.GetFieldCount(); i++)
            {
                tLcrColumn.Clear();
                tLcrColumn.m_sColmName = m_tSplitCol[i];
                tMdbLcr.m_vColms.push_back(tLcrColumn); 
            }
			iActCnt += m_tSplitCol.GetFieldCount() - 1;
        }
        else if(m_tSplitWhere.GetFieldCount() == 2)
        {
            //解析column列
            m_tSplitCol.SplitString(m_tSplitWhere[0],',');
            tMdbLcr.m_sTableName = m_tSplitCol[0];
            for(unsigned int i = 1; i < m_tSplitCol.GetFieldCount();i++)
            {
                tLcrColumn.Clear();
                tLcrColumn.m_sColmName = m_tSplitCol[i];
                tMdbLcr.m_vColms.push_back(tLcrColumn); 
            }
			iActCnt += m_tSplitCol.GetFieldCount() - 1;
            //解析where列
            m_tSplitCol.SplitString(m_tSplitWhere[1],',');
            for(unsigned int i = 0; i < m_tSplitCol.GetFieldCount();i++)
            {
                tLcrColumn.Clear();
                tLcrColumn.m_sColmName = m_tSplitCol[i];
                tMdbLcr.m_vWColms.push_back(tLcrColumn); 
            }
			iActCnt += m_tSplitCol.GetFieldCount();
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"more thran one '|', invalid column name string");
        }
        
        iPos += tMdbLcr.m_iColmLen;
        int iColmCnt = GetNum(sRecord+iPos, 4);
        iPos += 4;
        if(iColmCnt != iActCnt)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"Column Count Error,[%d|%d]",iColmCnt,iActCnt);
        }
        
        int iColmLenPos = iPos;  // 列值长度起始偏移
        int iValuePos = iColmLenPos + iColmCnt * 4; // 列值起始偏移
        int iColumLen = 0; // 列值长度
        char sValue[MAX_VALUE_LEN] = {0};
        std::vector<TLCRColm>::iterator itor = tMdbLcr.m_vColms.begin();
        for(; itor != tMdbLcr.m_vColms.end(); itor++)
        {
            iColumLen = GetNum(sRecord+iColmLenPos,4);
            iColmLenPos += 4;
            
            if(iColumLen < 0)
            {
                itor->m_bNull = true;
                iValuePos += 0;
            }
            else
            {
                memcpy(sValue,sRecord+iValuePos,iColumLen);
                sValue[iColumLen] = 0;
                itor->m_sColmValue = sValue;
                iValuePos += iColumLen;
            }
        }

        if(tMdbLcr.m_vWColms.size() > 0)
        {
            std::vector<TLCRColm>::iterator itorW = tMdbLcr.m_vWColms.begin();
            for(; itorW != tMdbLcr.m_vWColms.end(); itorW++)
            {
                iColumLen = GetNum(sRecord+iColmLenPos, 4);
                iColmLenPos += 4;
                
                if(iColumLen < 0)
                {
                    itorW->m_bNull = true;
                    iValuePos += 0;
                }
                else
                {
                    memcpy(sValue,sRecord+iValuePos,iColumLen);
                    sValue[iColumLen] = 0;
                    itorW->m_sColmValue = sValue;
                    iValuePos += iColumLen;
                }
            }
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称  :  NextRecord
    * 函数描述  :  下一条记录
    * 输入      :  
    * 输入      :  
    * 输出      :  
    * 返回值    :  0 - 成功!0 -失败
    * 作者      :  jin.shaohua
    *******************************************************************************/
    MDB_INT64 TMdbRedoLogParser::GetPageLSN(char * sRecordBuff)
    {
           char sTemp[32] = {0};
           memcpy(sTemp,sRecordBuff+11,20);
           return TMdbNtcStrFunc::StrToInt(sTemp);
    }

    /******************************************************************************
    * 函数名称  :  GetFileSize
    * 函数描述  :  获取文件大小
    * 输入      :  
    * 输入      :  
    * 输出      :  
    * 返回值    :  0 - 成功!0 -失败
    * 作者      :  dong.chun
    *******************************************************************************/
    long long TMdbRedoLogParser::GetFileSize(const char * pFile)
    {
        struct stat f_stat;
        if(stat(pFile, &f_stat) == -1) 
        {
            return ERR_OS_NO_FILE;
        }
        return (long long)f_stat.st_size;
    }

    int TMdbRedoLogParser::GetNum(const  char* pNum,int iNumSize)
    {
        int iRetNum = 0;
        int i = 0;
        for(i = 0; i<iNumSize; i++)
        {
            iRetNum += m_iFactor[iNumSize - i -1]*(pNum[i] - '0');
        }
        return iRetNum;
    }
    
    /******************************************************************************
    * 函数名称  :  GetTableName
    * 函数描述  :  获取表名
    * 输入      :  
    * 输入      :  
    * 输出      :  
    * 返回值    :  0 - 成功!0 -失败
    * 作者      :  dong.chun
    *******************************************************************************/
    int TMdbRedoLogParser::GetTableName(char * sRecord, std::string& sTableName)
    {
        int iRet = 0;
        int iColumnLen = GetNum(sRecord+55,4);
        char sColName[MAX_VALUE_LEN];
        memcpy(sColName,sRecord+59,iColumnLen);
        sColName[iColumnLen] = 0;
        m_tSplitWhere.SplitString(sColName,'|');
        if(m_tSplitWhere.GetFieldCount() == 1)
        {
             m_tSplitCol.SplitString(sColName,',');
        }
        else if(m_tSplitWhere.GetFieldCount() == 2)
        {
            //解析column列
            m_tSplitCol.SplitString(m_tSplitWhere[0],',');
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"more thran one '|', invalid column name string");
        }
        sTableName = m_tSplitCol[0];
        return iRet;
    }

    int TMdbRedoLogParser::GetWhereValue(char * sRecord)
    {
        return 0;
    }

    char* TMdbRedoLogParser::GetSQL(TMdbLCR & tMdbLcr)
    {
        return NULL;
    }

    int TMdbRedoLogParser::GetRoutingID(char * sRecord)
    {
        memset(m_sRout, 0, sizeof(m_sRout));
        strncpy(m_sRout, sRecord + 51, 4);
        return atoi(m_sRout);
    }

    char TMdbRedoLogParser::GetVersion(char * sRecord)
    {
        char cVersion = *(sRecord+6);
        if(cVersion != VERSION_DATA_SYNC && cVersion != VERSION_DATA_CAPTURE && cVersion == '0')
        {
            cVersion = VERSION_DATA_12;
        }
        return cVersion;
    }

    int TMdbRedoLogParser::GetSyncFlag(char *sRecord)
    {
        memset(m_sSyncFlag, 0, sizeof(m_sSyncFlag));
        strncpy(m_sSyncFlag, sRecord + 7, 4);
        return atoi(m_sSyncFlag);
    }
    /******************************************************************************
    * 函数名称	:  TMdbStorageCopy
    * 函数描述	:  从文件读取数据到内存
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  dong.chun
    *******************************************************************************/
    TMdbStorageCopy::TMdbStorageCopy()
    {
        m_pTable = NULL;
        m_pTMdbTableSpace = NULL;
        m_pInsertBlock = NULL;
        m_pTableSpaceCtrl = NULL;
        m_PageCtrl = NULL;
        m_pShmDSN = NULL;
        m_pConfig = NULL;
        m_pOldTable = NULL;
        m_pVarcharCtrl = NULL;
		m_iStructChangeType = 0;
    }

    TMdbStorageCopy::~TMdbStorageCopy()
    {
        SAFE_DELETE_ARRAY(m_pInsertBlock);
        SAFE_DELETE(m_pVarcharCtrl);
    }

    int TMdbStorageCopy::Init(TMdbShmDSN* pShmDSN,TMdbTable * pTable,TMdbTableSpace* pTMdbTableSpace,TMdbPageCtrl*pPageCtrl,TMdbTableSpaceCtrl* pTableSpaceCtrl)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pTable);
        CHECK_OBJ(pTMdbTableSpace);
        CHECK_OBJ(pTableSpaceCtrl);
        CHECK_OBJ(pPageCtrl);
        CHECK_OBJ(pShmDSN);
        m_pTMdbTableSpace = pTMdbTableSpace;
        m_pTableSpaceCtrl = pTableSpaceCtrl;
        m_PageCtrl = pPageCtrl;
        m_pTable = pTable;
        m_pShmDSN = pShmDSN;
        m_pConfig = TMdbConfigMgr::GetMdbConfig(m_pShmDSN->GetInfo()->sName);
        //这里后续和接口对接
        CHECK_RET(SetStructChangeType(),"SetStructChangeType Failed");
        SAFE_DELETE_ARRAY(m_pInsertBlock);
        m_pInsertBlock = new(std::nothrow) char[m_pTable->iOneRecordSize];
        CHECK_OBJ(m_pInsertBlock);
        m_pOldTable = m_pConfig->GetOldTableStruct(m_pTable->sTableName);
        if(m_pOldTable == NULL)
        {//如果没有老的，则说明表没有变化
            m_pOldTable = m_pTable;
        }
        CHECK_RET(m_OldRowCtrl.Init(m_pShmDSN->GetInfo()->sName, m_pOldTable),"m_OldRowCtrl.Init");
        CHECK_RET(m_NewRowCtrl.Init(m_pShmDSN->GetInfo()->sName, m_pTable),"m_NewRowCtrl.Init");

        if(m_pVarcharCtrl == NULL)
        {
            m_pVarcharCtrl = new(std::nothrow)TMdbVarCharCtrl();
            CHECK_OBJ(m_pVarcharCtrl);
            CHECK_RET(m_pVarcharCtrl->Init(m_pShmDSN->GetInfo()->sName),"m_pVarcharCtrl.Init");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  Load
    * 函数描述	:  加载页
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbStorageCopy::Load(TMdbPage * pPage)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(StructHasChange(m_iStructChangeType,STRUCT_TABLESPACE_CHANGE))
        {
        	CHECK_RET(CopyByRecord(pPage),"CopyByRecord Failed");
	        TADD_FUNC("Finish.");
	        return iRet;
        }
		else
		{
			if(StructHasChange(m_iStructChangeType,STRUCT_TABLE_IN_TABLESPACE_CHANGE))
			{
				if(StructHasChange(m_iStructChangeType,STRUCT_TABLE_CHANGE))
				{
					CHECK_RET(CopyByRecord(pPage),"CopyByRecord Failed");
    		        TADD_FUNC("Finish.");
    		        return iRet;
				}
				else
				{
					CHECK_RET(CopyByPage(pPage),"CopyByPageOrder Failed");
    		        TADD_FUNC("Finish.");
    		        return iRet;
				}
			}
			else
			{
				CHECK_RET(CopyByPageOrder(pPage),"CopyByPageOrder Failed");
    	        TADD_FUNC("Finish.");
    	        return iRet;
			}
		}
        CHECK_RET(ERR_APP_INVALID_PARAM,"Struct type[%d] is error.",m_iStructChangeType);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SetStructChangeType
    * 函数描述	:  获取表结构和表空间变化
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbStorageCopy::SetStructChangeType()
    {
        CHECK_OBJ(m_pConfig);
		TIntAlterAttr* pIntAlterAttr = NULL;
		//表空间变化判断逻辑未完整
		StructSetChange(m_iStructChangeType,STRUCT_NO_CHANGE);
		if(StructHasChange(m_pTableSpaceCtrl->GetHasTableChange(), STRUCT_TABLE_IN_TABLESPACE_CHANGE))
		{
			StructAddChange(m_iStructChangeType,STRUCT_TABLE_IN_TABLESPACE_CHANGE);
		}
        if(m_pConfig->IsTableAlter(m_pTable->sTableName))
        {
            StructAddChange(m_iStructChangeType,STRUCT_TABLE_CHANGE);
        }
        if(m_pConfig->IsPageSizeAlter(m_pTable->m_sTableSpace, pIntAlterAttr))
        {
            StructAddChange(m_iStructChangeType,STRUCT_TABLESPACE_CHANGE);
        }
        return 0;
    }

	/******************************************************************************
    * 函数名称	:  CopyByPage
    * 函数描述	:  页拷贝，文件中页号与内存中页号不对应，该方式加载后需重新落地文件
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbStorageCopy::CopyByPage(TMdbPage * pPage)
    {
        TADD_FUNC("Start.");
        //TADD_NORMAL("CopyByPage.");
        int iRet = 0;
        CHECK_OBJ(pPage);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState,"empty") == 0)
        {
            return iRet;
        }
        CHECK_OBJ(m_pTableSpaceCtrl);
        CHECK_OBJ(m_pTable);
        TMdbPage * pMdbPage = NULL;
		CHECK_RET(m_pTableSpaceCtrl->GetEmptyPage(m_pTable,pMdbPage,false),"Get empty page failed!");
        CHECK_OBJ(pMdbPage);
		int iPageID = pMdbPage->m_iPageID;
        if(pMdbPage->m_iPageSize != pPage->m_iPageSize)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"Mem PageSize[%d] != File PageSize[%d].",pMdbPage->m_iPageSize,pPage->m_iPageSize);
        }
        
        if(TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState,"full") == 0)
        {
            memcpy(pMdbPage,pPage,pPage->m_iPageSize);
			pMdbPage->m_iPageID = iPageID;
			pMdbPage->m_iPrePageID = -1;
			pMdbPage->m_iNextPageID = -1;
            CHECK_RET(m_pTable->tFullMutex.Lock(true, &m_pShmDSN->GetInfo()->tCurTime),"[%s].tFreeMutex.Lock() failed.",m_pTable->sTableName);
        	m_pTableSpaceCtrl->AddPageToTop(pMdbPage,m_pTable->iFullPageID);
            m_pTable->iFullPages++;
        	m_pTable->tFullMutex.UnLock(true, m_pShmDSN->GetInfo()->sCurTime);
        }else if(TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState,"free") == 0)
        {
            memcpy(pMdbPage,pPage,pPage->m_iPageSize);
			pMdbPage->m_iPageID = iPageID;
			pMdbPage->m_iPrePageID = -1;
			pMdbPage->m_iNextPageID = -1;
            CHECK_RET(m_pTable->tFreeMutex.Lock(true, &m_pShmDSN->GetInfo()->tCurTime),"[%s].tFreeMutex.Lock() failed.",m_pTable->sTableName);
        	m_pTableSpaceCtrl->AddPageToCircle(pMdbPage,m_pTable->iFreePageID);
            m_pTable->iFreePages++;
        	m_pTable->tFreeMutex.UnLock(true, m_pShmDSN->GetInfo()->sCurTime);
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"Page state[%s] is error.",pPage->m_sState);
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }
    

    /******************************************************************************
    * 函数名称	:  CopyByPageOrder
    * 函数描述	:  页拷贝，文件中页号与内存中页号完全对应，该方式加载后无需重新落地文件
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbStorageCopy::CopyByPageOrder(TMdbPage * pPage)
    {
        TADD_FUNC("Start.");
        //TADD_NORMAL("CopyByPageOrder.");
        int iRet = 0;
        CHECK_OBJ(pPage);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState,"empty") == 0)
        {
            return iRet;
        }
        CHECK_OBJ(m_pTableSpaceCtrl);
        CHECK_OBJ(m_pTable);
        TMdbPage * pMdbPage = NULL;
        CHECK_RET(m_pTableSpaceCtrl->GetEmptyPageByPageID(pMdbPage,pPage->m_iPageID),"GetEmptyPageByPageID[%d] Failed",pPage->m_iPageID);
        CHECK_OBJ(pMdbPage);

        if(pMdbPage->m_iPageSize != pPage->m_iPageSize)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"Mem PageSize[%d] != File PageSize[%d].",pMdbPage->m_iPageSize,pPage->m_iPageSize);
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState,"full") == 0)
        {
            memcpy(pMdbPage,pPage,pPage->m_iPageSize);
            CHECK_RET(m_pTable->tFullMutex.Lock(true, &m_pShmDSN->GetInfo()->tCurTime),"[%s].tFreeMutex.Lock() failed.",m_pTable->sTableName);
        	m_pTableSpaceCtrl->AddPageToTop(pMdbPage,m_pTable->iFullPageID);
            m_pTable->iFullPages++;
        	m_pTable->tFullMutex.UnLock(true, m_pShmDSN->GetInfo()->sCurTime);
        }else if(TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState,"free") == 0)
        {
            memcpy(pMdbPage,pPage,pPage->m_iPageSize);
            CHECK_RET(m_pTable->tFreeMutex.Lock(true, &m_pShmDSN->GetInfo()->tCurTime),"[%s].tFreeMutex.Lock() failed.",m_pTable->sTableName);
        	m_pTableSpaceCtrl->AddPageToCircle(pMdbPage,m_pTable->iFreePageID);
            m_pTable->iFreePages++;
        	m_pTable->tFreeMutex.UnLock(true, m_pShmDSN->GetInfo()->sCurTime);
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"Page state[%s] is error.",pPage->m_sState);
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }
    
    
    /******************************************************************************
    * 函数名称	:  CopyByPageOrder
    * 函数描述	:  单挑记录拷贝，文件中页号与内存中页号不对应，加载完成后需要重新落地文件
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbStorageCopy::CopyByRecord(TMdbPage * pPage)
    {
        TADD_FUNC("Start.");
	    //TADD_NORMAL("CopyByRecord.");
        int iRet = 0;
        CHECK_OBJ(pPage);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState,"empty") == 0)
        {
            return iRet;
        }
        CHECK_OBJ(m_pTableSpaceCtrl);
        CHECK_OBJ(m_pTable);
        CHECK_OBJ(m_pInsertBlock);
        CHECK_OBJ(m_PageCtrl);
        //获取文件页中的一条记录
        char* pDataAddr = NULL;
		char* pNextDataAddr = NULL;
        char* pMemDataAddr=NULL;
        int iDataOffSet = 0;
        while(pPage->GetNextDataAddr(pDataAddr, iDataOffSet,pNextDataAddr) != NULL)
        {
            CHECK_RET(FillData(pDataAddr),"FillData Failed");
            while(true)
            {
                TMdbPage * pFreePage= NULL;
                CHECK_RET(m_pTableSpaceCtrl->GetFreePage(m_pTable,pFreePage,false),"GetFreePage failed.");
                CHECK_RET(m_PageCtrl->Attach((char *)pFreePage, m_pTable->bReadLock, m_pTable->bWriteLock),"Can't Attach to page.");
				iRet = m_PageCtrl->InsertData((unsigned char*)m_pInsertBlock, m_pTable->iOneRecordSize, m_rowID,pMemDataAddr);
                if(ERR_PAGE_NO_MEMORY == iRet)
                {
                    //对于找到的自由页满了而无法插入数据，则找下一个自由页
                    TADD_DETAIL("Current page is Full.");
                    CHECK_RET(m_pTableSpaceCtrl->TablePageFreeToFull(m_pTable,pFreePage),"FreeToFull() error.iRet=[%d]",iRet);
                    continue;
                }
                break;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbStorageCopy::ColumnCopyByDataTypeChange(TMdbColumn * pNewColumn,TMdbColumn * pOldColumn,char* pMemAddr)
    {
        int iRet = 0;
        if(pNewColumn->iDataType == DT_VarChar && pOldColumn->iDataType == DT_Char)
        {
                int iWhichPos = -1;
                unsigned int iRowId = 0;
                CHECK_RET(m_pVarcharCtrl->Insert(&pMemAddr[pOldColumn->iOffSet], iWhichPos, iRowId,'Y'),"Insert Varchar Failed,ColoumName=[%s],iVarCharlen[%d]",pNewColumn->sName,strlen(&pMemAddr[pOldColumn->iOffSet]));
                m_pVarcharCtrl->SetStorgePos(iWhichPos, iRowId, &m_pInsertBlock[pNewColumn->iOffSet]);
        }
        else if(pNewColumn->iDataType == DT_Char && pOldColumn->iDataType == DT_VarChar)
        {
            CHECK_RET(m_pVarcharCtrl->GetVarcharValue(&m_pInsertBlock[pNewColumn->iOffSet], &pMemAddr[pOldColumn->iOffSet]),"GetVarcharValue ERROR");
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"ERROR DataType New[%d],Old[%d].",pNewColumn->iDataType,pOldColumn->iDataType);
        }
        return iRet;
    }

    int TMdbStorageCopy::ColumnCopyByDateTimeChange(TMdbColumn * pNewColumn,TMdbColumn * pOldColumn,char* pMemAddr)
    {
        int iRet = 0;
        if(pNewColumn->iColumnLen == sizeof(int) && pOldColumn->iColumnLen >=14)
        {
            int * pInt = (int*)&m_pInsertBlock[pNewColumn->iOffSet];
            if(TMdbDateTime::BetweenDate(&pMemAddr[pOldColumn->iOffSet],"19700000000000","20370000000000"))
            {
                *pInt = (int)TMdbDateTime::StringToTime(&pMemAddr[pOldColumn->iOffSet],m_pShmDSN->GetInfo()->m_iTimeDifferent);
                TADD_DETAIL("after date zip Fill Column[%d].",*pInt);
            }
            else
            {
                CHECK_RET(ERR_SQL_FILL_MDB_INFO,"column[%s] invalid zip-time value [%s],not in[19700000000000,20370000000000].",pOldColumn->sName,(&pMemAddr[pOldColumn->iOffSet]));
            }
        }
        else if(pNewColumn->iColumnLen == sizeof(long long) && pOldColumn->iColumnLen >=14)
        {
            long long * pLonglong = (long long*)&m_pInsertBlock[pNewColumn->iOffSet];
             * pLonglong = TMdbDateTime::StringToTime(&pMemAddr[pOldColumn->iOffSet],m_pShmDSN->GetInfo()->m_iTimeDifferent);
            TADD_DETAIL("after date zip Fill Column[%lld].",* pLonglong);
        }
        else if(pNewColumn->iColumnLen >=14  && pOldColumn->iColumnLen == sizeof(int))
        {

                int* pInt = (int*)&pMemAddr[pOldColumn->iOffSet];
                TMdbDateTime::TimeToString(*pInt,&m_pInsertBlock[pNewColumn->iOffSet]);
        }
        else if(pNewColumn->iColumnLen >=14  && pOldColumn->iColumnLen == sizeof(long long))
        {
            long long * pLong= (long long*)&pMemAddr[pOldColumn->iOffSet];
            TMdbDateTime::TimeToString(*pLong,&m_pInsertBlock[pNewColumn->iOffSet]);
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"ERROR ClumnLen New[%d],Old[%d].",pNewColumn->iColumnLen,pOldColumn->iColumnLen);
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  GetNextCopySize
    * 函数描述	:  如果表结构发生变化，需要分段进行记录拷贝，获取分段拷贝大小，>0 表示需要拷贝的长度，<=0表示拷贝完成
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbStorageCopy::GetNextCopySize(int iSize)
    {
        //等表结构变动接口确定再填写
        return 0;
    }

    int TMdbStorageCopy::FillData(char* pMemAddr)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pMemAddr);
        CHECK_OBJ(m_pInsertBlock);
        memset(m_pInsertBlock,0,m_pTable->iOneRecordSize);
        /*if(m_pTable->iColumnCounts < m_pOldTable->iColumnCounts)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"NewTable[%s] ColumCounts[%d] < OldTable[%s] ColumnCount[%d].",m_pTable->sTableName,m_pTable->iColumnCounts,m_pOldTable->sTableName,m_pOldTable->iColumnCounts);
        }*/
        TMdbColumn * pNewColumn = NULL;
        TMdbColumn * pOldColumn = NULL;
        //先把老的结构拷贝进去
        for(int i = 0; i<m_pTable->iColumnCounts;i++)
        {
        	for(int j = 0; j<m_pOldTable->iColumnCounts; j++)
        	{
	            pNewColumn =  &(m_pTable->tColumn[i]);
	            pOldColumn  = &(m_pOldTable->tColumn[j]);
	            if(TMdbNtcStrFunc::StrNoCaseCmp(pNewColumn->sName, pOldColumn->sName) == 0)
	            {
	                //设置字段的空值
	                if(m_OldRowCtrl.IsColumnNull(pOldColumn,pMemAddr) == true)
	                {// NULL值
	                    m_NewRowCtrl.SetColumnNull(pNewColumn, m_pInsertBlock);
	                    break;
	                }
	                else
	                {
	                    m_NewRowCtrl.ClearColumnNULL(pNewColumn, m_pInsertBlock);
	                }

	                //如果列的数据类型变了比如从char变化到varachar等，则要重新复制
	                if(pNewColumn->iDataType != pOldColumn->iDataType)
	                {
	                    CHECK_RET(ColumnCopyByDataTypeChange(pNewColumn,pOldColumn,pMemAddr),"ColumnCopyByDataTypeChange Failed");
	                    break;
	                }

	                //如果是时间类型，并且长度变了，则需要重新赋值
	                if((pNewColumn->iColumnLen != pOldColumn->iColumnLen) && pNewColumn->iDataType == DT_DateStamp)
	                {
	                    CHECK_RET(ColumnCopyByDateTimeChange(pNewColumn,pOldColumn,pMemAddr),"ColumnCopyByDataTypeChange Failed");
	                    break;
	                }

	                switch(pNewColumn->iDataType)
	                {
	                    case DT_Int:  //Integer
	                    case DT_Char://Char
	                    case DT_DateStamp:
	                    {
	                        memcpy(&m_pInsertBlock[pNewColumn->iOffSet],&pMemAddr[pOldColumn->iOffSet],pOldColumn->iColumnLen);
	                        break;
	                    }
	                    case DT_Blob:
	                    case DT_VarChar:  //VarChar
	                    {
	                         memcpy(&m_pInsertBlock[pNewColumn->iOffSet],&pMemAddr[pOldColumn->iOffSet],1+sizeof(long)*2);
	                        break;
	                    }
	                    default:
	                        CHECK_RET(ERR_SQL_TYPE_INVALID,"column[%s]DataType=%d invalid.", pNewColumn->sName,pNewColumn->iDataType);
	                        break;
	                }
					break;
	            }
	            else if(j == m_pOldTable->iColumnCounts-1)
	            {
	                //如果是新增列，则判断是否有默认值，或者NULL等
		            if(pNewColumn->bIsDefault == true)
		            {
		                switch(pNewColumn->iDataType)
		                {
		                    case DT_Int:  //Integer
		                    {
		                        long long * pLonglong = (long long*)&m_pInsertBlock[pNewColumn->iOffSet];
		                        * pLonglong = TMdbNtcStrFunc::StrToInt(pNewColumn->iDefaultValue);
		                        break;
		                    }
		                    case DT_Char://Char
		                    {
		                        SAFESTRCPY(&m_pInsertBlock[pNewColumn->iOffSet],pNewColumn->iColumnLen,pNewColumn->iDefaultValue);
		                        break;
		                    }
		                    case DT_DateStamp:
		                    {
		                        if(pNewColumn->iColumnLen == sizeof(int))
		                        {
		                            int * pInt = (int*)&m_pInsertBlock[pNewColumn->iOffSet];
		                            *pInt = (int)TMdbDateTime::StringToTime(pNewColumn->iDefaultValue,m_pShmDSN->GetInfo()->m_iTimeDifferent);
		                        }
		                        else if(pNewColumn->iColumnLen == sizeof(long long))
		                        {
		                            long long * pLonglong = (long long*)&m_pInsertBlock[pNewColumn->iOffSet];
		                             * pLonglong = TMdbDateTime::StringToTime(pNewColumn->iDefaultValue,m_pShmDSN->GetInfo()->m_iTimeDifferent);
		                        }
		                        else if(pNewColumn->iColumnLen >= 14)
		                        {
		                            SAFESTRCPY(&m_pInsertBlock[pNewColumn->iOffSet],pNewColumn->iColumnLen,pNewColumn->iDefaultValue);
		                        }
		                        break;
		                    }
		                    case DT_Blob:
		                    case DT_VarChar:  //VarChar
		                    {
		                        int iWhichPos = -1;
		                        unsigned int iRowId = 0;
		                        CHECK_RET(m_pVarcharCtrl->Insert(pNewColumn->iDefaultValue, iWhichPos, iRowId,'Y'),"Insert Varchar Failed,ColoumName=[%s],iVarCharlen[%d]",
		                            pNewColumn->sName,strlen(pNewColumn->iDefaultValue));
		                        m_pVarcharCtrl->SetStorgePos(iWhichPos, iRowId, &m_pInsertBlock[pNewColumn->iOffSet]);
		                        break;
		                    }
		                    default:
		                        CHECK_RET(ERR_SQL_TYPE_INVALID,"column[%s]DataType=%d invalid.", pNewColumn->sName,pNewColumn->iDataType);
		                        break;
		                }
		                m_NewRowCtrl.ClearColumnNULL(pNewColumn, m_pInsertBlock);
		            }
		            else if(pNewColumn->m_bNullable == true)
		            {
		                m_NewRowCtrl.SetColumnNull(pNewColumn, m_pInsertBlock);
		            }
		            else
		            {
		                CHECK_RET(ERR_SQL_TYPE_INVALID,"NewColumn[%s] not support default and not support null,can't load from disk.", pNewColumn->sName);
		            }
	            }
	        }
			
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    
//}

