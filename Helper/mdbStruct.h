/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbStruct.h		
*@Description�� mdb�Ļ���������ṹ
*@Author:		
*@Date��	    2014/02/11
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_STRUCT_H__
#define __MINI_DATABASE_STRUCT_H__

#include "Helper/TThreadLog.h"    //��־
#include <assert.h>
#include "Helper/mdbErr.h"  
//#include "BillingSDK.h"
#include "Helper/parser.h"
#include "Common/mdbProtocolWinntTcp.h"
#include "Common/mdbNtcEngine.h"
#include "Common/mdbNtcSplit.h"
#include "Common/mdbStrUtils.h"
#include "Common/mdbFileUtils.h"
#include "Common/mdbSysTimerManager.h"

//namespace QuickMDB
//{
//�汾��Ϣ
/*
�汾��Ϣ˵�������磺QuickMDB V1.2.14.20140109
V1.2.14����ָ�ں˰汾�ţ����ֻ�����ں˷����ش�����ʱ��Ż��޸ġ�����Ҫ�ؽ�qmdb��ͨ��������ʽ������
20140109�����ڰ汾�ţ������������д��������ͻ��޸�������ʶ��ǰqmdb���һ���޸���ʲôʱ��

QMDBֻά�����汾��ֻ�е����ֽ���bug���Ż����ֳ���֧�汾���޸ġ���ʱ���ֳ���֧�Ĵ���汾��ͻ��һ����path��(�����汾���ֳ���),��Ӧ�����ڰ汾Ҳ���޸ġ�
QuickMDB V1.2.14.20131226.patch
*/
#define MDB_VERSION  "QuickMDB V1.4.1.20160428"


/**
 * @brief ����·������
 * 
 */
#define MDB_SHM_BASE "QuickMDB_HOME"  //�����ڴ����

#define MDB_HOME_ENV "$(QuickMDB_HOME)"  //mdb_home��������

#define MAX_IP_LEN      16  //IP����󳤶�

#define MAX_DSN_COUNTS 10  //����DSN����

	//����״̬
	enum E_TRANSACTION_STATE
	{
		//TRANSACTION_OUT 	 = 1,	  //�����⣬��ʱ�������������״̬
		TRANS_IN  	 = 2,     //������
		TRANS_COMMIT 	 = 3,     //�ύ��
		TRANS_ROLLBACK = 4      //�ع���
	};

	//minidb��������Դ
#define SOURCE_APP  1
#define SOURCE_REP  2
#define SOURCE_PEER 3
#define SOURCE_PEER_REP 4

//���ֵ���󳤶�
#define MAX_NAME_LEN   256

//ʱ�����󳤶�
#define MAX_TIME_LEN      15

//ʱ�����󳤶�
#define DATE_TIME_SIZE      15

#define MIN_DISK_SPACE_KB   (32*1024ULL)   //32M

#define MAX_TABLE_COUNTS  (500)

#define MAX_FILE_NAME  (256)

#define MAX_PATH_NAME_LEN   512

#define MAX_SEND_BUF  (1024*32)
#define MAX_VARCHAR_SIZE  (128*1024*1024)
#define MAX_SHM_ID         100 //�������Ĺ����ڴ���
#define MAX_VARCHAR_SHM_ID 10000

//����������Key��ʵ�ʹ�������Key�����ݿ��ʵ�����
#define MANAGER_KEY       0x4765454

#define GET_MGR_KEY(_dsnValue)  MANAGER_KEY + (_dsnValue)
//�䳤���ݴ洢������Key,Ҳ�����ݿ�ʵ���й�
#define GET_VARCHAR_KEY(_dsnValue)  0x5765454 + 1000000*(_dsnValue)

// Ӱ�ӱ�ˢ�·���˹����ڴ���key
#define SHADOW_SERVER_KEY 0x7765454

// Ĭ�ϵ�·��ID����Ϊ������·��ʹ��
#define DEFALUT_ROUT_ID (-999)
#define DEFALUT_ROUT_ID_STRING "-999"


#ifdef WIN32
typedef HANDLE          SHAMEM_T;
#define INITVAl         NULL
#pragma warning(disable:4018)
#pragma warning(disable:4390)
typedef char*             SMKey;
#define SHMID_FMT        "%p"
#else
typedef int             SHAMEM_T;
typedef key_t		SMKey;
#define INITVAl          -1
#define SHMID_FMT   "%d"
#endif

//���·���������ö��ŷָ�
#define MAX_ROUTER_LIST_LEN 1024

#define MAX_SEQUENCE_COUNTS 255

//ϵͳ���������û���
#define MAX_USER_COUNT  64

//��������������
#define MAX_MUTEX_COUNTS  10000

//varchar������������
#define MAX_VARCHAR_MUTEX_COUNTS 10000

//�������ļ����С
#define INTERVAL_SIZE      1024

//db��״̬
#define DB_unused '0'    //δʹ��----���״̬�����ϲ�����
#define DB_running 'U'   //����ʹ��--������ͬ����
#define DB_loading 'L'   //������������--ָ���ڴ�Oracle��������
#define DB_repping 'R'   //����ͬ�����ݣ�ָ��������ʱ��ͬ������---��ʱ������д������ļ�
#define DB_stop 'S'      //ֹͣ---��ʱ���ݲ���ʹ��


//ͬ������:'0'-��ͬ��;'1'-��Oracleͬ��;'2'-�ͱ���ͬ��;'3'-��ͬ��
#define DSN_No_Rep  0x0000
#define DSN_Ora_Rep 0x0001
#define DSN_Rep         0x0002
#define DSN_Two_Rep 0x0003


//ϵͳ����������
#define MAX_PROCESS_COUNTS 450

#define MAX_COLUMN_COUNTS 64

#define MAX_PRIMARY_KEY  10

//����״̬
#define Link_use  'L'  //��������ʹ��
#define Link_down 'D'  //�����Ѿ�Down��

//���������
#define MAX_LINK_COUNTS  5500


//agent���˿���
#define MAX_AGENT_PORT_COUNTS 5


//���job����
#define MAX_JOB_COUNT 100

//���Ե������������3�����޷����ӣ�����Ϊ�Զ�û������
#define MAX_TRY_SECONDS 3

//һ��������������м�����
#define MAX_INDEX_COLUMN_COUNTS 10

//һ��������SQL��
#define MAX_SQL_COUNTS      255

//���������������
typedef void* LPVOID;


#define QueryHasProperty(E,P)     (((E)&(P))==(P))
#define QueryHasAnyProperty(E,P)  (((E)&(P))!=0)
#define QuerySetProperty(E,P)     (E)|=(P)
#define QueryClearProperty(E,P)   (E)&=~(P)


#define MAXTABLELEN 1024 //Ĭ�Ϲ�ϣ�������С        




class TMdbOtherAttr
{
public:
    void Clear()
   	{
        memset(sAttrName,0,sizeof(sAttrName));
        memset(sAttrValue,0,sizeof(sAttrValue));
   	}

    char sAttrName[MAX_NAME_LEN];
    char sAttrValue[1024];    
};




//���__FILE__,__LINE__����������
#define SAFESTRCPY(_dst,_diz,_src) SAFESTRCPY_IN(_dst,_diz,_src,__FILE__,__LINE__)
inline void SAFESTRCPY_IN(char* dst, int diz, const char* src,const char * sFile,int iLine)
{
    if(dst == 0 )
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM,"dst=NULL Exception,error postion:file=[%s],line=[%d].", sFile, iLine);
        return;
    }
    if(src == 0)
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM,"src=NULL.error postion:file=[%s],line=[%d].", sFile, iLine);
        strcpy(dst, "");
        return;
    }
    if((int)(strlen(src)+1) > (diz))
    {
        TADD_ERROR(ERR_APP_STRING_OUT_BOUND,"String Out of Bounds src=[%s] > diz=[%d],error postion:file=[%s],line=[%d].", src, (diz),sFile,iLine);
        strncpy(dst, src, (diz));
        (dst)[diz-1] = '\0';
    }
    else
    {
        strcpy(dst, src);
    }
}

   

    

#define MAX_PATH_FILE       255
#define DATE_TIME_SIZE      15   //����ʱ���ʽ��СYYYYMMDDHHMISS
#define MDB_CHAR_SIZE    (8)  //mdb��char���͵�size


    //minidb֧�ֵ�hash�������������¶��֣�
    #define HT_Unknown 0   //δ֪����
    #define HT_Int     1   //��ֵ����
    #define HT_Char    2   //�ַ�������
    #define HT_CMP     3   //��������

    //minidb�����ݲ���Ȩ��
    #define MDB_ADMIN  'A'
    #define MDB_WRITE  'W'
    #define MDB_READ   'R'

    //�����������ڴ��С
    #define MAX_MGR_SHM_SIZE   (1024*1024*1024)

    //minidb�ı�״̬
    #define Table_unused '0'    //δʹ��----���״̬�����ϲ�����
    #define Table_running '1'   //����ʹ��--������ͬ����
    #define Table_loading '2'   //������������--ָ���ڴ�Oracle��������
    #define Table_repping '3'   //����ͬ�����ݣ�ָ��������ʱ��ͬ������---��ʱ������д������ļ�
    #define Table_watingRep '4' //��ͬ��---��ʱ���ݲ�������������ͬ��
    #define Table_temp_watingRep '5' //��ʱ��ͬ������ͬ������ʱ�Ͽ�

    #define ROUTER_ID_COL_NAME "routing_id" //·��ID������

    
    //����״̬
    #define PSTOP     'S'  //ֹͣ
    #define PFREE     'F'  //����
    #define PBUSY     'B'  //æµ
    #define PKILL     'K'  //ɱ��
    #define PDUMP     'D'  //mdbAgent ץ��

    //Ĭ�϶˿�
    #define DEFAULT_PORT    6666

     //��ռ�״̬����0��-δ����;��1��-�Ѿ�����;��2��-���ڴ���;��3����������
    #define TableSpace_unused     '0' //��0��-δ����
    #define TableSpace_using      '1' //��1��-����ʹ��
    #define TableSpace_creating   '2' //��2��-���ڴ���
    #define TableSpace_destroying '3' //��3��-��������

    //�������������
    #define MAX_PRIMARY_KEY_CC  10

    //���ݿ������״̬
    #define DB_SELF_CREATE    0        //�Լ��������Զ�û������
    #define DB_NORMAL_LOAD    1        //�������أ��Զ���������
    
    //minidb֧�ֵ�SQL��������
		#define OP_Query  TK_SELECT  //��ѯ���� 1
		#define OP_Update TK_UPDATE   //��������2
		#define OP_Delete TK_DELETE  //ɾ������3
		#define OP_Insert TK_INSERT   //��������4
		#define OP_IstUpd 5    //���û����������룬�������5


    /*enum E_STORAGE_TYPE
    {
        STORAGE_ORACLE = 0,
        STORAGE_MYSQL = 1,
        STORAGE_FILE = 2,
        STORAGE_MDB = 3,
        STORAGE_UNKOWN
    };*/


    #define MDB_DS_TYPE_NO      'N' //������Դ��ֻ�����ڴ���
    #define MDB_DS_TYPE_FILE    'F' //�ļ�����Դ
    #define MDB_DS_TYPE_ORACLE  'O' //Oracle��Ϊ����Դ
    #define MDB_DS_TYPE_MYSQL  'M' //MySql��Ϊ����Դ


// MDB  ��� ͬ��������������
#define REP_DB2MDB "DB2MDB"
#define REP_MDB2DB  "MDB2DB"
#define REP_NoRep   "NoRep"

// ���ͬ�����ԣ�Ĭ��Ϊ0
enum E_TAB_REP_TYPE
{
    REP_FROM_DB = 0, //  ��oracle ͬ��
    REP_TO_DB = 1,  //  ��Oraͬ��
    REP_NO_REP = 2, // ��ͬ��
    REP_TO_DB_MDB =3 //��db mdbͬ��������1.2
    };

    enum E_LOAD_PRIORITY
    {
        LOAD_FILE_FIRST = 1,  // �ļ��洢����
        LOAD_DB_FIRST = 2 // ���ݿ�洢����
};

    #define SYS_TABLE_SPACE "SYS_TABLE_SPACE"

    // qmdbϵͳ��DBA_SEQUENCE����
    #define SYS_DBA_SEQUENCE "DBA_SEQUENCE"

    // ����ͬ����ʹ�ü�¼��ʽ�汾
    #define VERSION_DATA_SYNC 'S'
    
    // ���ݲ�����ʹ�ü�¼��ʽ�汾
    #define VERSION_DATA_CAPTURE 'C'

    // ����1.2 ��¼��ʽ�汾,old
    #define VERSION_DATA_12 'O'
//}

#endif //__MINI_DATABASE_STRUCT_H__


