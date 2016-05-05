/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbObserve.h		
*@Description�� mdb�۲��
*@Author:			jin.shaohua
*@Date��	    2012.10
*@History:
******************************************************************************************/
#ifndef _MDB_OBSERVE_CTRL_H_
#define _MDB_OBSERVE_CTRL_H_
#include "Helper/mdbStruct.h"
#include "Helper/mdbCacheLog.h"
#include "Helper/mdbShmSTL.h"
#include "Helper/mdbDictionary.h"

//namespace QuickMDB{

#define MAX_OBSERVE_POINT 16  //�ݶ�16���۲��
    class TObserveBase;
    class TMdbSqlParser;
    class TMdbExecuteEngine;


    //�۲�����
    enum E_OBSERVE_TYPE
    {
    	OB_TABLE_EXEC  = 0, //�۲��ִ��SQL������:insert ,delete,update
    	OB_END
    };

    //�۲��
    class TObservePoint
    {
    public:
    	int    m_iType;//�۲�����
    	char m_sName[MAX_NAME_LEN];//�۲���
    	struct timeval m_tTerminal;//�۲����ʱ��
    	char m_sParam[1024];//�۲����
    };

    //�۲�����
    class TObserveMgr
    {
    public:
    	int InitObservePoint(TShmList<TObservePoint> & ObList);//��ʼ���۲����Ϣ
    	TObserveBase * GetObserveInst(int iObserveType);//��ȡ�۲⹦����
    	int ShowAllObservePoint(const char * sDsn);//��ʾ���й۲����Ϣ
    };
    //�۲��������
    class TObserveParam
    {
    public:
    	TObserveParam();
    	~TObserveParam();
    	
    };
    class TMdbShmDSN;
    //�۲����
    class TObserveBase
    {
    public:
    	TObserveBase();
    	virtual ~TObserveBase();
    	int Init(const char * sDsn,int iObserveType);//������ʼ��
    	int SetSQLParser(TMdbSqlParser * pSqlParser);//����SQL��������
    	int SetExecEngine(TMdbExecuteEngine * pExecEngine);//����ִ������
    	int GeneralParaseParam(const char * sParam);//ͨ�ý�������
    	int SetLogFileName(const char * sFileNameFormat);//��ȡ�ļ���
    	int SetLogFileSize(const char * sFileSizeFormat);//�����ļ���С
    	int SetFlushCycle(const char * sFlushCycleFormat);//��������ˢ�²���
    	virtual int ParseParam(const char * sParam) = 0;//��������
    	virtual int Record() = 0;//��¼�۲���Ϣ
    	int StopObServe();//ֹͣ�۲�
    protected:
    	int Log(const char * fmt, ...);//��¼
    	virtual bool bNeedToObserve(){return true;}//�Ƿ���Ҫobserve
    	inline bool bTerminalObserve();//�Ƿ���ֹ�۲�
    protected:

        TMdbShmDSN * m_pShmDsn;
        TMdbDSN * m_pDsn;

        TObservePoint * m_pObservePoint;//�۲����Ϣ
        char 	   m_sLogFile[MAX_PATH_NAME_LEN];//��־·��
        TMdbSqlParser * m_pSqlParser;
        int             m_iLogFileSizeM;//��־��С��λ:M
        TMdbExecuteEngine * m_pExecEngine;
        std::map<std::string, bool> m_bObTable;
        TCacheLog m_tCacheLog;//����ʽ���
        char *  m_pTempMem;
    };

    //�۲�������Ĳ�����¼
    class TObserveTableExec:public TObserveBase
    {
    public:
    	TObserveTableExec();
    	~TObserveTableExec();
    public:
    	int ParseParam(const char * sParam);//��������
    	int Record();//��¼
    protected:
    	bool bNeedToObserve();//�Ƿ���Ҫobserve
    	int 	ReParseParam();//���½�������
    private:
    	struct timeval m_tOldTerminal;//��¼�۲����ʱ��
    };

#define OB_TABLE_EXEC_DEF TObserveTableExec m_tObserveTableExec;
#define OB_TABLE_EXEC_INIT(_dsn,_sqlParser) m_tObserveTableExec.Init(_dsn,OB_TABLE_EXEC);m_tObserveTableExec.SetSQLParser(_sqlParser);
#define OB_TABLE_EXEC_RECORD(...) m_tObserveTableExec.Record();
//}

#endif

