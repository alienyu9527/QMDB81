/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbTableSpaceCtrl.h		
*@Description�� �������ĳ����ı�ռ���Ϣ����ȡһ������ҳ������ҳ
*@Author:		li.shugang
*@Date��	    2008��11��30��
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
    	int Init(const char * pszDsn,char* psTableSpaceName);//��ʼ��
    	int CreateTableSpace(TMdbTableSpace* pTableSpace, TMdbConfig* pConfig, bool bAddToFile);//������ռ�   
    	int GetFreePage(TMdbTable* pTable,TMdbPage * & pFreePage, bool bAddToFile = true);//   ��ȡһ������ҳ,���û������ҳ���򷵻�һ������ҳ 
    	int GetEmptyPage(TMdbTable* pTable,TMdbPage * & pEmptyPage,bool bAddToFile = true);   //��ȡһ������ҳ 
    	int GetEmptyPageByPageID(TMdbPage * & pEmptyPage,int iPageId, bool bAddToFile = true); //��ȡһ��ָ��ҳ�ŵĿ��пռ�
    	char* GetAddrByPageID(int iPageID);//����pageid��ȡpage��ַ
    	void PushBackPage(TMdbPage* pPage);//�ѵ�ǰҳ��黹����ռ䣬����ҳ����� 
    	int TablePageFreeToFull(TMdbTable * pTable,TMdbPage* pCurPage);
    	int TablePageFullToFree(TMdbTable * pTable,TMdbPage* pCurPage);
		long long GetFileSize(char * sFile);
		int AddToFile(TMdbTableSpace *pTableSpace,TMdbTSNode& node);
    	int AskNewPages(int iNewPageCount, bool bAddToFile = true);//��ϵͳ����һ����ҳ��
    	TMdbTSNode * GetNextNode(TMdbTSNode * pCurNode);//��ȡ��һ��node
    	int ReBuildFromDisk(TMdbTSFile * pTSFile);//�Ӵ������¹�����ռ� ��ҳ����
    	int ReBuildFromDiskOneByOne(TMdbTSFile * pTSFile);//�Ӵ������¹�����ռ� ����¼����
    	int SetPageDirtyFlag(int iPageID);//������ҳ��ʶ
    	int InitPage(char* pAddr, int iPageSID, int iPageFID, size_t iSize);//��ʼ�������ҳ��
    	int AddNode(TMdbTableSpace* pTSTmp, TMdbTSNode &nod);//�ѽڵ�����ռ䣬��������
    	int RemovePage(TMdbPage * pPageToRemove,int & iHeadPageId);//�Ƴ�һ��page
    	int AddPageToTop(TMdbPage * pPageToAdd,int & iHeadPageId);//��page��ӵ�ͷ
    	int GetPageFromTop(TMdbPage * &pPageToGet,int  iHeadPageId);//��listͷ��ȡһ��page
    	TMdbTSNode * GetNodeByPageID(int iPageID);//����pageid��ȡnode
    	int InsertDataFill(char* const &sTmp); //���Ҫ���������
    	int GetHasTableChange();
	int SetHasTableChange();	
    	int AddPageToCircle(TMdbPage * pPageToAdd,int & iHeadPageId);
        int RemovePageFromCircle(TMdbPage * pPageToRemove,int & iHeadPageId);
        int GetTableFreePageId(TMdbTable* pTable);
        bool IsFileStorage(){return (m_pTMdbTableSpace != NULL)?m_pTMdbTableSpace->m_bFileStorage:false;};
    private:
        char m_sTableSpaceName[MAX_NAME_LEN];  //��ռ�����
        TMdbTableSpace *m_pTMdbTableSpace;     //��ռ�ָ��        
        TMdbShmDSN     *m_pShmDSN;             //���ݿ���Ϣ
        TMdbDSN           *m_pDsn; 
		int m_iHasTableChange;//��ռ����Ƿ��б�ṹ�����仯
		FILE* m_pTSFile;
    };
//}

#endif //__ZX_MINI_DATABASE_TABLE_SPACE_H__

