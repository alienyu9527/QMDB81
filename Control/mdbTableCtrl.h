/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbTableCtrl.h		
*@Description�� �����
*@Author:		jin.shaohua
*@Date��	    2014/03/06
*@History:
******************************************************************************************/
#ifndef _MDB_TABLE_CTRL_H_
#define _MDB_TABLE_CTRL_H_
#include "Control/mdbMgrShm.h"
#include "Control/mdbTableSpaceCtrl.h"
//namespace QuickMDB{
	/**
	 * @brief �����
	 * 
	 */
	class TMdbTableCtrl
	{
	public:
		TMdbTableCtrl();
		~TMdbTableCtrl();
		int Init(const char * sDsn,const char * sTablename);//��ʼ��
		int CreateTable(TMdbTable * pTable);//������
		int GetFreePage(TMdbPage * & pFreePage);//��ȡ����ҳ
		int TablePageFreeToFull(TMdbPage* pCurPage);
		int TablePageFullToFree(TMdbPage* pCurPage);
	protected:
		int RemovePage(TMdbPage * pPageToRemove,int & iHeadPageId);//�Ƴ�һ��page
		int AddPageToTop(TMdbPage * pPageToAdd,int & iHeadPageId);//��page��ӵ�ͷ
	private:
		TMdbShmDSN * m_pShmDsn;//
		TMdbTable * m_pTable;//�����
		TMdbTableSpaceCtrl m_tTSCtrl;//��ռ����
	};
//}





#endif