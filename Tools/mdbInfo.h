/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbInfo.h       
*@Description�� �����ӡ�ڴ����ݿ�ĸ�����Ϣ
*@Author:       li.shugang
*@Date��        2009��03��10��
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_INFORMATION_H__
#define __MINI_DATABASE_INFORMATION_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{


    class TMdbInfo
    {
    public:
        TMdbInfo(bool bDetail);
        TMdbInfo();
        ~TMdbInfo();
        
        /******************************************************************************
        * ��������  :  Connect()
        * ��������  :  ����ĳ��DSN�����ǲ��ڹ�����ע���κ���Ϣ    
        * ����      :  pszDSN, ��������������DSN 
        * ���      :  ��
        * ����ֵ    :  �ɹ�����0, ʧ�ܷ���-1
        * ����      :  li.shugang
        *******************************************************************************/
        int Connect(const char* pszDSN);   
        

        /******************************************************************************
        * ��������  :  PrintLink()
        * ��������  :  ��ӡ������Ϣ    
        * ����      :  iFlag, 1-��ӡ��������, 2-��ӡԶ������, 0-��ӡ��������
        * ���      :  ��
        * ����ֵ    :  ��
        * ����      :  li.shugang
        *******************************************************************************/
        void PrintLink(int iFlag=0, int iPid=0);
        

        /******************************************************************************
        * ��������  :  PrintProc()
        * ��������  :  ��ӡ������Ϣ    
        * ����      :  pszProc, ��������
        * ���      :  ��
        * ����ֵ    :  ��
        * ����      :  li.shugang
        *******************************************************************************/
        void PrintProc(const char* pszProc=NULL);
        

        /******************************************************************************
        * ��������  :  PrintMem()
        * ��������  :  ��ӡ�ڴ����Ϣ    
        * ����      :  pszShmID, Share-Memory-ID
        * ���      :  ��
        * ����ֵ    :  ��
        * ����      :  li.shugang
        *******************************************************************************/
        int PrintMem(const char* pszShmID=NULL);
        

        /******************************************************************************
        * ��������  :  PrintProc()
        * ��������  :  ��ӡ��ռ���Ϣ    
        * ����      :  pszTableSpace, ��ռ�����
        * ���      :  ��
        * ����ֵ    :  ��
        * ����      :  li.shugang
        *******************************************************************************/
        void PrintTableSpace(const char* pszTableSpace=NULL);
        

        /******************************************************************************
        * ��������  :  PrintTable()
        * ��������  :  ��ӡ����Ϣ    
        * ����      :  pszTable, ������
        * ���      :  ��
        * ����ֵ    :  ��
        * ����      :  li.shugang
        *******************************************************************************/
        void PrintTable(const char* pszTable=NULL);
        
        /******************************************************************************
        * ��������  :  PrintSQL()
        * ��������  :  ��ӡϵͳSQL��Ϣ    
        * ����      :  iPos, SQLλ�ã����Ϊ-1��ʾ��ӡȫ��
        * ���      :  ��
        * ����ֵ    :  ��
        * ����      :  li.shugang
        *******************************************************************************/
        void PrintSQL(int iPos=-1);
        
        void PrintDSN(const char* pszDSN);
        void PrintSeq(const char* pszSeq=NULL);
        void PrintJob(const char * sJobName = NULL);//��ӡjob�б�
        int SetRouterToCapture(const char * sRouter);//����Ҫ�����·��
    	/******************************************************************************
    	* ��������	: PrintUser 
    	* ��������	: ��ӡ�û��б�(��Ҫ��Ϊ�˲�ѯ�û���������)
    	* ����		:  sUserName �û�����all
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  cao.peng
    	*******************************************************************************/
    	void PrintUser(const char * sUserName);
    	int EstimateTableCostMemory(TMdbTable * pTable);//Ԥ�����ڴ�����
    	void PrintUsageOfLock();//��ӡ��ʹ�����
        void PrintRoutingRep();//��ӡ��Ƭ����·����Ϣ
        void PrintNotLoadFromDBInfo();//���Ʊ�����ݿ���صĸ���������Ϣ
		void PrintVarcharPageList();
	private:
        TMdbConfig *m_pConfig;
        
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn;     
        TMdbTable  *m_pMdbTable;
        TMdbTableSpace *m_pMdbTableSpace;
        TMdbProc       *m_pMdbProc;
        TMdbLocalLink  *m_pMdbLocalLink;
        //TMdbRemoteLink *m_pMdbRemoteLink;
        bool m_bMore;

		
		TShmAlloc m_tMgrShmAlloc;//�����ڴ������
		char *  m_pMgrAddr;

        
    };
//}
//���ݿ���ڴ���Ϣ
class TMdbSizeInfo
{
public:
    class TResourceSize
    {//��Դ��Ϣ
    public:
        double  dTotalSize;
        double  dUsedSize;
        char    sDataType[32];
    public:
        void Clear()
        {
            dTotalSize      = 0.0;
            dUsedSize       = 0.0;
            sDataType[0]    = 0;
        }
        void Print()
        {
            printf("%-32s %15.1fM %15.1fM %15.1fM\n", 
                sDataType, 
                dTotalSize, 
                dUsedSize,
                dTotalSize - dUsedSize);
        }
    };
    class TTableSize
    {//����Ϣ
    public:
        double  dDataSize;//����
        double  dIndexSize;//����
        int     iTotalCount;//��¼����
        int     iUsedCount;//��ʹ��
        char    sTableName[MAX_NAME_LEN];
    public:
        void Clear()
        {
            dDataSize       = 0.0;
            dIndexSize      = 0.0;
            iTotalCount     = 0;
            iUsedCount      = 0;
            sTableName[0]   = 0;
        }
        void Print()
        {
            printf("%-32s %15.1fM %15.1fM %16d %16d\n",
                sTableName,
                dDataSize,
                dIndexSize,
                iTotalCount,
                iUsedCount);
        }
    };
public:
    TMdbSizeInfo();
    ~TMdbSizeInfo();
    
    int Init(const char * sDsn);
    void PrintResourceInfo(bool bDetail);//��ӡ��Դ���ڴ�ռ��
    void PrintTableInfo(const char *sTableName,int iCount);//��ӡqmdb����ڴ�ռ��
private:
    void GetMgrSize(TResourceSize & data);
    void GetDataBlockSize(TResourceSize & data);
    void GetIndexBlockSize(TResourceSize & data);
    void GetHashIndexSize(TResourceSize & data);
    void GetMHashIndexSize(TResourceSize & data);
    void GetMHashOtherSize(TResourceSize & data);
    void GetVarcharBlockSize(TResourceSize & data);
    void GetSyncSize(TResourceSize & data);
	void GetSBSyncSize(int iHostID, TResourceSize &data);
    void GetOneTableSize(TMdbTable * pTable,TTableSize &tTableSize);
    
private:
    TMdbConfig      *m_pConfig;
    TMdbShmDSN      *m_pShmDSN;
    TMdbDSN         *m_pDsn; 
	TMdbShmRepMgr   *m_pShmMgr;
};


#endif //__MINI_DATABASE_INFORMATION_H__



