/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbTableSpaceCtrl.h		
*@Description： 负责管理某个表的表空间信息：获取一个自由页、空闲页
*@Author:		li.shugang
*@Date：	    2008年11月30日
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_TABLE_SPACE_H__
#define __ZX_MINI_DATABASE_TABLE_SPACE_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbDictionary.h"

//namespace QuickMDB{

   

    class TMdbConfig;
    class TMdbShmDSN;
    class TMdbTSFile;
    class TMdbTableSpaceCtrl
    {
    public:
    	TMdbTableSpaceCtrl();
    	~TMdbTableSpaceCtrl();
    	int Init(const char * pszDsn,char* psTableSpaceName);//初始化
    	int CreateTableSpace(TMdbTableSpace* pTableSpace, TMdbConfig* pConfig, bool bAddToFile);//创建表空间   
    	int GetFreePage(TMdbTable* pTable,TMdbPage * & pFreePage, bool bAddToFile = true);//   获取一个自由页,如果没有自由页，则返回一个空闲页 
    	int GetEmptyPage(TMdbTable* pTable,TMdbPage * & pEmptyPage,bool bAddToFile = true);   //获取一个空闲页 
    	int GetEmptyPageByPageID(TMdbPage * & pEmptyPage,int iPageId, bool bAddToFile = true); //获取一个指定页号的空闲空间
    	char* GetAddrByPageID(int iPageID);//根据pageid获取page地址
    	void PushBackPage(TMdbPage* pPage);//把当前页面归还给表空间，进行页面回收 
    	int TablePageFreeToFull(TMdbTable * pTable,TMdbPage* pCurPage);
    	int TablePageFullToFree(TMdbTable * pTable,TMdbPage* pCurPage);
		long long GetFileSize(char * sFile);
		int AddToFile(TMdbTableSpace *pTableSpace,TMdbTSNode& node);
    	int AskNewPages(int iNewPageCount, bool bAddToFile = true);//向系统申请一批新页面
    	TMdbTSNode * GetNextNode(TMdbTSNode * pCurNode);//获取下一个node
    	int ReBuildFromDisk(TMdbTSFile * pTSFile);//从磁盘重新构建表空间 按页拷贝
    	int ReBuildFromDiskOneByOne(TMdbTSFile * pTSFile);//从磁盘重新构建表空间 按记录拷贝
    	int SetPageDirtyFlag(int iPageID);//设置脏页标识
    	int InitPage(char* pAddr, int iPageSID, int iPageFID, size_t iSize);//初始化申请的页面
    	int AddNode(TMdbTableSpace* pTSTmp, TMdbTSNode &nod);//把节点插入表空间，管理起来
    	int RemovePage(TMdbPage * pPageToRemove,int & iHeadPageId);//移除一个page
    	int AddPageToTop(TMdbPage * pPageToAdd,int & iHeadPageId);//将page添加到头
    	int GetPageFromTop(TMdbPage * &pPageToGet,int  iHeadPageId);//从list头获取一个page
    	TMdbTSNode * GetNodeByPageID(int iPageID);//根据pageid获取node
    	int InsertDataFill(char* const &sTmp); //填充要插入的数据
    	int GetHasTableChange();
	int SetHasTableChange();	
    	int AddPageToCircle(TMdbPage * pPageToAdd,int & iHeadPageId);
        int RemovePageFromCircle(TMdbPage * pPageToRemove,int & iHeadPageId);
        int GetTableFreePageId(TMdbTable* pTable);
        bool IsFileStorage(){return (m_pTMdbTableSpace != NULL)?m_pTMdbTableSpace->m_bFileStorage:false;};
    private:
        char m_sTableSpaceName[MAX_NAME_LEN];  //表空间名称
        TMdbTableSpace *m_pTMdbTableSpace;     //表空间指针        
        TMdbShmDSN     *m_pShmDSN;             //数据库信息
        TMdbDSN           *m_pDsn; 
		int m_iHasTableChange;//表空间下是否有表结构发生变化
		FILE* m_pTSFile;
    };
//}

#endif //__ZX_MINI_DATABASE_TABLE_SPACE_H__

