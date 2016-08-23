/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbCtrl.h
*@Description�� �ڴ����ݿ�Ŀ��Ƴ���
*@Author:		li.shugang
*@Date��	    2008��11��25��
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_CTROL_H__
#define __MINI_DATABASE_CTROL_H__

#include "Helper/mdbConfig.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbProcCtrl.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Replication/mdbRepCtrl.h"
#include <vector>

//namespace QuickMDB{

class TMdbCtrl
{
public:
    TMdbCtrl();
    ~TMdbCtrl();
public:
    int Init(const char* pszDsn);     //��ʼ��
    int Create();   //����ĳ���ڴ����ݿ�
    int Destroy(const char* pszDSN, bool bForceFlag = false);  //����ĳ���ڴ����ݿ�
    int Start();   //����ĳ���ڴ����ݿ�
    int Stop(const char*pszDSN);    //ֹͣĳ���ڴ����ݿ�
    bool IsMdbCreate();//�Ƿ���mdb���̴���
    bool IsStop(); //���ݿ��Ƿ���Ҫֹͣ
    void SetLoadFromDisk(bool bFlag){m_bLoadFromDisk = bFlag;}//�趨�Ӵ��̼���
private:
    int LockFile();  //��ס������ʶ�ļ�:-1-�ļ�������;0-����ʧ��;1-�����ɹ�
    int UnLockFile();
    int GetStartMethod();//�����ȷ��������ʽ
    int CreateSysMem(); //���������ڴ��
    int CreateTableSpace();//������ռ�
    int CreateTable();//�����ڴ��
    int CreateVarChar();//����varchar������
    int LoadData();//��������
    int LoadFromOra();  //��Oracle����ȫ������
    int LoadFromStandbyHost();//�ӱ�����������
    int LoadFromShardBackupHost(); // ��Ƭ��������
    int LoadSysData();   //��ʼ��ϵͳ��
    int LoadDBAUser(TMdbDatabase* pMdb);//����dba_user
    int LoadDBADual(TMdbDatabase* pMdb);//����dual��
    int LoadSequence();
    bool CheckVersion(TMdbDSN *pTMdbDSN);//У���ں˰汾�ͷ����汾����ȷ���Ƿ�Ҫ���´��������ڴ�
    int CheckSystem();//У��������
    int GetCSMethod(int iAgentPort);
    int GetProcToStart(std::vector<std::string > & vProcToStart);//��ȡҪ�����Ľ�����Ϣ
    bool NeedRemoveFile();
    int LoadFromDisk();//�Ӵ��̼���
	int CreateSBBufShm();

    /******************************************************************************
    * ��������	:  CheckTablespace
    * ��������	:  ��������ļ����ڴ��еı�ռ��Ƿ�һ��
    * ����		:  
    * ���		:  
    * ����ֵ	:  0- �ɹ�; ��0- ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int CheckTablespace(TMdbShmDSN* pMdbShmDSN);

    /******************************************************************************
    * ��������	:  CheckTables
    * ��������	:  ��������ļ����ڴ��еı��Ƿ�һ��
    * ����		:  
    * ���		:  
    * ����ֵ	:  0- �ɹ�; ��0- ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int CheckTables(TMdbShmDSN* pMdbShmDSN);
    /******************************************************************************
    * ��������	:  LockFile()
    * ��������	:  ��ס�ļ�pszFile    
    * ����		:  pszFile������ס���ļ���
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0��ʧ�ܷ�������ֵ
    * ����		:  li.shugang
    *******************************************************************************/
    int LockFile(const char* pszFile);	

    /******************************************************************************
    * ��������	:  UnLockFile()
    * ��������	:  �ͷ�������pszFile�ϵ���(��֧��win32ϵͳ)
    * ����		:  pszFile�����ͷ������ļ���
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0��ʧ�ܷ�������ֵ
    * ����		:  li.shugang
    *******************************************************************************/
    int UnLockFile(const char* pszFile);
	int CheckSysConfig(TMdbCfgDSN* pCfgDSN,TMdbShmDSN* pMdbShmDSN);//DSN����У��
    /******************************************************************************
    * ��������	:  ReportState()
    * ��������	:  ����Ƿ���Ҫ�ϱ�״̬�������Ҫ����mdbServer�ϱ�״̬
    * ����		:  
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0��ʧ�ܷ�������ֵ
    * ����		:  Jiang.lili
    *******************************************************************************/
    int ReportState(EMdbRepState eState);
private:
    TMdbConfig * m_pConfig;
    char m_sDsn[MAX_NAME_LEN];  //Ҫ���Ƶ��ڴ����ݿ��DSN����
    char m_sLockFile[MAX_FILE_NAME]; //��Ҫ��ס���ļ���
    TMdbProcCtrl m_tProcCtrl;
    char sPeerIP[MAX_IP_LEN];
    int iPeerPort;
    bool m_bLoadFromDisk;//�Ƿ�Ӵ��̼���
    TMdbShardBuckupCfgCtrl m_tSBCfgCtrl;
};

//}
#endif //__MINI_DATABASE_CTROL_H__


