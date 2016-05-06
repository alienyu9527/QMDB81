/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbVarcharMgr.h
*@Description�� �������QuickMDB�ı䳤�洢�����ƽӿ�
*@Author:		li.shugang
*@Date��	    2012��02��20��
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
        //����䳤����
        int InsertWhichPos(int iPos,const char *pszColoumValue, char*  const &sTmp, int iSize);
        int UpdateWhichPos(const char *pszColoumValue, char* const &pData, int  iPos);
        int DeleteWhichPos(int iPos,int iFlagPos, int iOffSet);

        int Lock();//�䳤�洢������
        int UnLock();//�䳤�洢������
        int GetStoragePos(const char *  pDataCell,int & iStorageFlag,long & lPos,long & lOffset);//��ȡ�洢pos
        bool HasValue(const char * pData,int iOffSet);//�жϴ洢��Ԫ�Ƿ���ֵ
    private:
        int GetFreeNode(int iMaxFreeNode, int& iNumFreeNode, int* pFreeNode, int & iFindPos,bool & bFound,int &iCurrentPos);//��ȡ���нڵ�
        int InsertWhichPosX(int iLen, const char* pszColumValue, int iMaxFreeNode, int& iNumFreeNode,
                                   int* pFreeNode, int &iCurrentPos, TvarcharNode* ptVarchar, char** pVarcharAddr);
        int DeleteWhichPosX(int iPos,int iFlagPos,int OffSet, int iLen, TvarcharNode* ptVarchar, char** pVarcharAddr,
                                   int &iCurrentPos, int& iMaxFreeNode, int* pFreeNode, int& iNumFreeNode);
       int  GetValue(int iShmNo, char* &pVarcharAddr, char* &pResultValue, int iLen, int iOffSet);
       int CreateShm(int len, int iPos, char* &pAddr);
      int GetDataSize(int iLen);
      int SetStoragePos(char *pDataCell,int iStorageFlag,long lPos,long lOffset);//����varchar �洢

    private:
        TMdbDSN *m_pTMdbDSN;
        TvarcharBlock *m_pVarcharBlock; //�䳤���ݴ洢��ַ
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
    int Init(const char* sDsn); //��ʼ��key
    int GetFree();
    int CreateVarChar();//����VARCHAR
    int CreateShm(TMdbVarchar *pVarChar,bool bAddToFile=true); //����ĳ����Ĺ����ڴ�
    int VarCharWhichStore(int iLen,int& iPos);
    int AddNode(TMdbVarchar* pVarChar, TMdbTSNode &nod);//�ѽڵ��������
    TMdbTSNode * GetNextNode(TMdbTSNode * pCurNode);//��ȡ��һ��node
    int InitPage(char* pAddr, int iPageSID, int iPageFID, TMdbVarchar* pVarChar);//��ʼ�������ҳ��
    char* GetAddrByPageID(TMdbVarchar* pVarChar,int iPageID);//����pageid��ȡpage��ַ
    char* GetAddrByPageID(TMdbVarchar* pVarChar,int iPageID,bool& isNoPage);//����pageid��ȡpage��ַ
    int Insert(char* pValue,int& iWhichPos,unsigned int& iRowId,char cStorage,bool bBlob=false); //����һ��varchar����
    int Update(char * pValue,int& iWhichPos,unsigned int& iRowId,char cStorage,bool bBlob=false); //����һ��varchar����
    int Delete(int& iWhichPos, unsigned int& iRowId); //ɾ��һ��varchar����
    int SetStorgePos(int iWhichPos, unsigned int iRowId,char* pAddr);
    int GetStoragePos(char* pAddr,int & iWhichPos,unsigned int & iRowId);//��ȡ�洢pos
    int  GetVarcharValue(char *pResultValue ,char* const pData, bool bBlob = false);
    int CreateOrAddFile();
    int GetFreePage(TMdbPage * & pFreePage);
    int GetValueSize(int iWhichPos);
    int RemovePage(TMdbPage * pPageToRemove,int & iHeadPageId);//�Ƴ�һ��page
    int AddPageToTop(TMdbPage * pPageToAdd,int & iHeadPageId);//��page��ӵ�ͷ
    int PageFreeToFull(TMdbPage* pCurPage);
    int PageFullToFree(TMdbPage* pCurPage);
    int SetPageDirtyFlag(int iPageID);//������ҳ��
    char* GetAddressRowId(TMdbRowID* pRowID);
    TMdbTSNode* GetNodeByPageId(int iPageID);
    bool NeedStorage();//�Ƿ���Ҫ�ļ��洢
    int AddToFile(TMdbVarchar *pVarChar,TMdbTSNode& node);//����������ڴ�ӳ�䵽�ļ���ȥ
    long long GetFileSize(char* sFile);
    void SetStorageFlag();
    void SetVarchar(TMdbVarchar* pVarChar);
private:
    TMdbVarchar    *m_pVarChar; //VARCHAR��ָ��
    TMdbShmDSN     *m_pShmDSN;
    TMdbDSN        *m_pDsn;
    TMdbPageCtrl   m_mdbPageCtrl;
    FILE * m_pVarcharFile;
    bool m_bStorageFlag; //�Ƿ��ļ��洢
};

//Varchar�ļ�
//}

#endif //__QUICK_MEMORY_DATABASE_VARCHAR_MANAGER_H__

