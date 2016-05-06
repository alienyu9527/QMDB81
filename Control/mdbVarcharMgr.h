/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbVarcharMgr.h
*@Description： 负责管理QuickMDB的变长存储区控制接口
*@Author:		li.shugang
*@Date：	    2012年02月20日
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_VARCHAR_MANAGER_H__
#define __QUICK_MEMORY_DATABASE_VARCHAR_MANAGER_H__


#include "Helper/mdbStruct.h"
#include "Helper/mdbShm.h"
#include "Helper/mdbConfig.h"
#include "Helper/mdbDictionary.h"
#include "Control/mdbPageCtrl.h"

//namespace QuickMDB{

    class TMdbShmDSN;
#if 0
    class TMdbVarcharMgr
    {
    public:
        TMdbVarcharMgr();
        ~TMdbVarcharMgr();
    public:
        int SetConfig(TMdbShmDSN * pShmDSN);
        int AttachShm(int iPos, char* &pAddr);
        void Destroy();
        int  UpdateValue(int iShmNo, char* &pVarcharAddr, const char* pValue, int iLen, int iOffSet);
        int  GetVarcharValue(char *pResultValue ,int iPos,char* const pData);
        int  VarCharWhichStore(int iLen);
        //插入变长数据
        int InsertWhichPos(int iPos,const char *pszColoumValue, char*  const &sTmp, int iSize);
        int UpdateWhichPos(const char *pszColoumValue, char* const &pData, int  iPos);
        int DeleteWhichPos(int iPos,int iFlagPos, int iOffSet);

        int Lock();//变长存储区加锁
        int UnLock();//变长存储区解锁
        int GetStoragePos(const char *  pDataCell,int & iStorageFlag,long & lPos,long & lOffset);//获取存储pos
        bool HasValue(const char * pData,int iOffSet);//判断存储单元是否有值
    private:
        int GetFreeNode(int iMaxFreeNode, int& iNumFreeNode, int* pFreeNode, int & iFindPos,bool & bFound,int &iCurrentPos);//获取空闲节点
        int InsertWhichPosX(int iLen, const char* pszColumValue, int iMaxFreeNode, int& iNumFreeNode,
                                   int* pFreeNode, int &iCurrentPos, TvarcharNode* ptVarchar, char** pVarcharAddr);
        int DeleteWhichPosX(int iPos,int iFlagPos,int OffSet, int iLen, TvarcharNode* ptVarchar, char** pVarcharAddr,
                                   int &iCurrentPos, int& iMaxFreeNode, int* pFreeNode, int& iNumFreeNode);
       int  GetValue(int iShmNo, char* &pVarcharAddr, char* &pResultValue, int iLen, int iOffSet);
       int CreateShm(int len, int iPos, char* &pAddr);
      int GetDataSize(int iLen);
      int SetStoragePos(char *pDataCell,int iStorageFlag,long lPos,long lOffset);//设置varchar 存储

    private:
        TMdbDSN *m_pTMdbDSN;
        TvarcharBlock *m_pVarcharBlock; //变长数据存储地址
        char *m_pVarchar16Addr[1000];
        char *m_pVarchar32Addr[1000];
        char *m_pVarchar64Addr[1000];
        char *m_pVarchar128Addr[1000];
        char *m_pVarchar256Addr[1000];
        char *m_pVarchar512Addr[1000];
        char *m_pVarchar1024Addr[1000];
        char *m_pVarchar2048Addr[1000];
        char *m_pVarchar4096Addr[1000];
        char *m_pVarchar8192Addr[1000];
    };
#endif
class TMdbVarCharCtrl
{
public:
    TMdbVarCharCtrl();
    ~TMdbVarCharCtrl();
    int Init(const char* sDsn); //初始化key
    int GetFree();
    int CreateVarChar();//创建VARCHAR
    int CreateShm(TMdbVarchar *pVarChar,bool bAddToFile=true); //创建某个块的共享内存
    int VarCharWhichStore(int iLen,int& iPos);
    int AddNode(TMdbVarchar* pVarChar, TMdbTSNode &nod);//把节点管理起来
    TMdbTSNode * GetNextNode(TMdbTSNode * pCurNode);//获取下一个node
    int InitPage(char* pAddr, int iPageSID, int iPageFID, TMdbVarchar* pVarChar);//初始化申请的页面
    char* GetAddrByPageID(TMdbVarchar* pVarChar,int iPageID);//根据pageid获取page地址
    char* GetAddrByPageID(TMdbVarchar* pVarChar,int iPageID,bool& isNoPage);//根据pageid获取page地址
    int Insert(char* pValue,int& iWhichPos,unsigned int& iRowId,char cStorage,bool bBlob=false); //插入一条varchar数据
    int Update(char * pValue,int& iWhichPos,unsigned int& iRowId,char cStorage,bool bBlob=false); //更新一条varchar数据
    int Delete(int& iWhichPos, unsigned int& iRowId); //删除一条varchar数据
    int SetStorgePos(int iWhichPos, unsigned int iRowId,char* pAddr);
    int GetStoragePos(char* pAddr,int & iWhichPos,unsigned int & iRowId);//获取存储pos
    int  GetVarcharValue(char *pResultValue ,char* const pData, bool bBlob = false);
    int CreateOrAddFile();
    int GetFreePage(TMdbPage * & pFreePage);
    int GetValueSize(int iWhichPos);
    int RemovePage(TMdbPage * pPageToRemove,int & iHeadPageId);//移除一个page
    int AddPageToTop(TMdbPage * pPageToAdd,int & iHeadPageId);//将page添加到头
    int PageFreeToFull(TMdbPage* pCurPage);
    int PageFullToFree(TMdbPage* pCurPage);
    int SetPageDirtyFlag(int iPageID);//设置脏页标
    char* GetAddressRowId(TMdbRowID* pRowID);
    TMdbTSNode* GetNodeByPageId(int iPageID);
    bool NeedStorage();//是否需要文件存储
    int AddToFile(TMdbVarchar *pVarChar,TMdbTSNode& node);//将新申请的内存映射到文件中去
    long long GetFileSize(char* sFile);
    void SetStorageFlag();
    void SetVarchar(TMdbVarchar* pVarChar);
private:
    TMdbVarchar    *m_pVarChar; //VARCHAR段指针
    TMdbShmDSN     *m_pShmDSN;
    TMdbDSN        *m_pDsn;
    TMdbPageCtrl   m_mdbPageCtrl;
    FILE * m_pVarcharFile;
    bool m_bStorageFlag; //是否文件存储
};

//Varchar文件
//}

#endif //__QUICK_MEMORY_DATABASE_VARCHAR_MANAGER_H__

