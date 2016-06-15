#ifndef __MDB_FLUSH_H__
#define __MDB_FLUSH_H__
#include "Helper/SqlParserStruct.h"
#include "Helper/mdbConfig.h"
#include <vector>
#include "Helper/mdbQueue.h"
#include "Control/mdbRowCtrl.h"

//namespace QuickMDB{

#define FLUSH_ORA     0x01  //��oracleˢ��
#define FLUSH_SHARD_BACKUP 0x02 // ��Ƭ����ͬ��
#define FLUSH_REDO    0x4  //ˢ��redo��־
#define FLUSH_CAPTURE    0x8  //ˢ��·�ɲ�������

    //����flushtype
#define FlushTypeHasProperty(V,P)     (((V)&(P))==(P))
#define FlushTypeHasAnyProperty(V,P)  (((V)&(P))!=0)
#define FlushTypeSetProperty(V,P)     (V)|=(P)
#define FlushTypeClearProperty(V,P)   (V)V&=~(P)

    class TMdbSqlParser;
    //column ֵ
    struct ST_COLUMN_VALUE
    {
        int iColPos;//��λ��
        char sColName[MAX_NAME_LEN];
        char * pValue;//ֵ��ַ
        int iValueSize;//���ݳ���
        bool bNull; //�Ƿ�Ϊ��
    };

    class TCOLUMN_POS
    {
    public:
        TCOLUMN_POS();
        ~TCOLUMN_POS();
        bool IsSame(int iSqlType,std::vector<ST_COLUMN_VALUE> & vColList,std::vector<ST_COLUMN_VALUE> & vWhereList);
        int SetPos(int iSqlType,std::vector<ST_COLUMN_VALUE> & vColList,std::vector<ST_COLUMN_VALUE> & vWhereList);
        void Clear();
    public:
        int m_pColumnPos[50];
        int m_pWherePos[10];
        int m_iColumnCount;
        int m_iWhereCount;
        int m_iSqlType;
        char m_sSQL[MAX_SQL_LEN];
    };
    class TTABLE_SQL_BUF
    {
    public:
        TTABLE_SQL_BUF();
        ~TTABLE_SQL_BUF();
    public:
        TCOLUMN_POS * GetFreeColumnPos();//��ȡһ�����е�columnPos;
        TCOLUMN_POS * m_pColmunPos[100];
    };

    inline int PosToStr(int iPos,char * sPos)
    {
        int iRet = 0;
        if(iPos<0 || iPos > 99){CHECK_RET(ERR_APP_INVALID_PARAM,"iPos must in [0,99].");}
        switch(iPos)
        {
        case 0:
            memcpy(sPos,"00",2);
            break;
        case 1:
            memcpy(sPos,"01",2);
            break;
        case 2:
            memcpy(sPos,"02",2);
            break;
        case 3:
            memcpy(sPos,"03",2);
            break;
        case 4:
            memcpy(sPos,"04",2);
            break;
        case 5:
            memcpy(sPos,"05",2);
            break;
        case 6:
            memcpy(sPos,"06",2);
            break;
        case 7:
            memcpy(sPos,"07",2);
            break;
        case 8:
            memcpy(sPos,"08",2);
            break;
        case 9:
            memcpy(sPos,"09",2);
            break;
        case 10:
            memcpy(sPos,"10",2);
            break;
        case 11:
            memcpy(sPos,"11",2);
            break;
        case 12:
            memcpy(sPos,"12",2);
            break;
        case 13:
            memcpy(sPos,"13",2);
            break;
        case 14:
            memcpy(sPos,"14",2);
            break;
        case 15:
            memcpy(sPos,"15",2);
            break;
        case 16:
            memcpy(sPos,"16",2);
            break;
        case 17:
            memcpy(sPos,"17",2);
            break;
        case 18:
            memcpy(sPos,"18",2);
            break;
        case 19:
            memcpy(sPos,"19",2);
            break;
        case 20:
            memcpy(sPos,"20",2);
            break;
        default:
            sPos[0] = iPos/10 + '0';
            sPos[1] = iPos%10 + '0';
            break;
        }
        return iRet;
    }

    //mdb�������
    class TMdbFlush
    {
    public:
        TMdbFlush();
        ~TMdbFlush();
        int Init(TMdbSqlParser * pMdbSqlParser,MDB_INT32 iFlags);
        int InsertIntoQueue(MDB_INT64 iLsn,long long iTimeStamp); //����������д�뻺�����
        int InsertIntoCapture(MDB_INT64 iLsn,long long iTimeStamp);
        bool bNeedToFlush();//�Ƿ���Ҫflush
        
    private:
        int FlushToBuf(TMdbQueue * pMemQueue,char * const sTemp,int iLen);
        //int GetSourceId();

        int FillRepData(char * sTemp,long long iPageLSN,long long iTimeStamp, char cVersion,int & iLen);
        int SetRepColmData(ST_MEM_VALUE_LIST & stMemValueList);

    private:

        TMdbSqlParser * m_pMdbSqlParser;
        TMdbConfig  * m_pConfig;
        TMdbMemQueue* m_pQueue;
        TMdbDSN     * m_pDsn;
        TMdbTable   *     m_pTable;
        int           m_iFlushType;//ˢ������
        TMdbQueue    m_QueueCtrl;
        //char          m_sDataBuf[MAX_VALUE_LEN];//��ʱ���ݻ���
        char* m_psDataBuff;
        TMdbRowCtrl m_tRowCtrl;//�����ݹ���
        TMdbMemQueue* m_pShardBackQueue;

        int m_iColmCount;
        int m_iNamePos ;
        int m_iColumLenPos;
        int m_iColumValuePos;
        char* m_psNameBuff;
		char* m_psColmLenBuff;
		char* m_psColmValueBuff;
		
    };

	//����ģʽ��ʹ�ã�����ͬ������
	class TMdbFlushTrans
    {
    public:
        TMdbFlushTrans();
        ~TMdbFlushTrans();
		int Init(TMdbShmDSN * pShmDSN,TMdbTable*  pTable, int iSqlType,char* pDataAddr);
		int InsertBufIntoQueue();
		int InsertBufIntoCapture();		
		int MakeBuf(MDB_INT64 iLsn,long long iTimeStamp);
		
		
    private:
		bool bNeedToFlush();//�Ƿ���Ҫflush
		bool IsCurRowNeedCapture();
        int FlushToBuf(TMdbQueue * pMemQueue,char * const sTemp,int iLen);
		long long GetRoutingID();
		int FillRepData(char * sTemp,long long iPageLSN, long long iTimeStamp,int & iLen);
		int SetRepColmDataAll();
		int SetRepColmDataPK();		
		void SetBufVersion(char cVersion);

    private:
        TMdbMemQueue* m_pQueue;
		TMdbConfig* m_pConfig;
        TMdbDSN     * m_pDsn;
        TMdbTable   *     m_pTable;
        int           m_iFlushType;//ˢ������
        int 		m_iSqlType;
		int     m_iBufLen;
		long long  m_llRoutingID;
        TMdbQueue    m_QueueCtrl;
        char* m_psDataBuff;
        TMdbRowCtrl m_tRowCtrl;//�����ݹ���
        TMdbMemQueue* m_pShardBackQueue;
		char*  m_pDataAddr;

        int m_iColmCount;
        int m_iNamePos ;
        int m_iColumLenPos;
        int m_iColumValuePos;
        char* m_psNameBuff;
		char* m_psColmLenBuff;
		char* m_psColmValueBuff;
		
    };

    //config shm��װ��
    class TConfigShmWrap
    {
    public:
        TConfigShmWrap();
        ~TConfigShmWrap();
        int Init(TMdbConfig * pConfig,TMdbShmDSN * pShmDSN);//��ʼ��
        TMdbTable * GetTableByName(const char* psTableName);//����ID��ȡtable
    private:
        TMdbShmDSN * m_pShmDSN;
        TMdbConfig * m_pConfig;//����
    };
//}

#endif

