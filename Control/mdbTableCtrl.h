/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbTableCtrl.h		
*@Description： 表管理
*@Author:		jin.shaohua
*@Date：	    2014/03/06
*@History:
******************************************************************************************/
#ifndef _MDB_TABLE_CTRL_H_
#define _MDB_TABLE_CTRL_H_
#include "Control/mdbMgrShm.h"
#include "Control/mdbTableSpaceCtrl.h"
//namespace QuickMDB{
	/**
	 * @brief 表管理
	 * 
	 */
	class TMdbTableCtrl
	{
	public:
		TMdbTableCtrl();
		~TMdbTableCtrl();
		int Init(const char * sDsn,const char * sTablename);//初始化
		int CreateTable(TMdbTable * pTable);//创建表
		int GetFreePage(TMdbPage * & pFreePage);//获取自由页
		int TablePageFreeToFull(TMdbPage* pCurPage);
		int TablePageFullToFree(TMdbPage* pCurPage);
	protected:
		int RemovePage(TMdbPage * pPageToRemove,int & iHeadPageId);//移除一个page
		int AddPageToTop(TMdbPage * pPageToAdd,int & iHeadPageId);//将page添加到头
	private:
		TMdbShmDSN * m_pShmDsn;//
		TMdbTable * m_pTable;//表管理
		TMdbTableSpaceCtrl m_tTSCtrl;//表空间管理
	};
//}





#endif