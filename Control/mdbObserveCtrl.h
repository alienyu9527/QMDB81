/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbObserve.h		
*@Description： mdb观测点
*@Author:			jin.shaohua
*@Date：	    2012.10
*@History:
******************************************************************************************/
#ifndef _MDB_OBSERVE_CTRL_H_
#define _MDB_OBSERVE_CTRL_H_
#include "Helper/mdbStruct.h"
#include "Helper/mdbCacheLog.h"
#include "Helper/mdbShmSTL.h"
#include "Helper/mdbDictionary.h"

//namespace QuickMDB{

#define MAX_OBSERVE_POINT 16  //暂定16个观测点
    class TObserveBase;
    class TMdbSqlParser;
    class TMdbExecuteEngine;


    //观测类型
    enum E_OBSERVE_TYPE
    {
    	OB_TABLE_EXEC  = 0, //观测表执行SQL与数据:insert ,delete,update
    	OB_END
    };

    //观测点
    class TObservePoint
    {
    public:
    	int    m_iType;//观测类型
    	char m_sName[MAX_NAME_LEN];//观测名
    	struct timeval m_tTerminal;//观测结束时间
    	char m_sParam[1024];//观测参数
    };

    //观测点管理
    class TObserveMgr
    {
    public:
    	int InitObservePoint(TShmList<TObservePoint> & ObList);//初始化观测点信息
    	TObserveBase * GetObserveInst(int iObserveType);//获取观测功能类
    	int ShowAllObservePoint(const char * sDsn);//显示所有观测点信息
    };
    //观测参数配置
    class TObserveParam
    {
    public:
    	TObserveParam();
    	~TObserveParam();
    	
    };
    class TMdbShmDSN;
    //观测基类
    class TObserveBase
    {
    public:
    	TObserveBase();
    	virtual ~TObserveBase();
    	int Init(const char * sDsn,int iObserveType);//基础初始化
    	int SetSQLParser(TMdbSqlParser * pSqlParser);//设置SQL解析器，
    	int SetExecEngine(TMdbExecuteEngine * pExecEngine);//设置执行引擎
    	int GeneralParaseParam(const char * sParam);//通用解析参数
    	int SetLogFileName(const char * sFileNameFormat);//获取文件名
    	int SetLogFileSize(const char * sFileSizeFormat);//设置文件大小
    	int SetFlushCycle(const char * sFlushCycleFormat);//设置周期刷新参数
    	virtual int ParseParam(const char * sParam) = 0;//解析参数
    	virtual int Record() = 0;//记录观测信息
    	int StopObServe();//停止观测
    protected:
    	int Log(const char * fmt, ...);//记录
    	virtual bool bNeedToObserve(){return true;}//是否需要observe
    	inline bool bTerminalObserve();//是否终止观测
    protected:

        TMdbShmDSN * m_pShmDsn;
        TMdbDSN * m_pDsn;

        TObservePoint * m_pObservePoint;//观测点信息
        char 	   m_sLogFile[MAX_PATH_NAME_LEN];//日志路径
        TMdbSqlParser * m_pSqlParser;
        int             m_iLogFileSizeM;//日志大小单位:M
        TMdbExecuteEngine * m_pExecEngine;
        std::map<std::string, bool> m_bObTable;
        TCacheLog m_tCacheLog;//缓存式输出
        char *  m_pTempMem;
    };

    //观测对主键的操作记录
    class TObserveTableExec:public TObserveBase
    {
    public:
    	TObserveTableExec();
    	~TObserveTableExec();
    public:
    	int ParseParam(const char * sParam);//解析参数
    	int Record();//记录
    protected:
    	bool bNeedToObserve();//是否需要observe
    	int 	ReParseParam();//重新解析参数
    private:
    	struct timeval m_tOldTerminal;//记录观测结束时间
    };

#define OB_TABLE_EXEC_DEF TObserveTableExec m_tObserveTableExec;
#define OB_TABLE_EXEC_INIT(_dsn,_sqlParser) m_tObserveTableExec.Init(_dsn,OB_TABLE_EXEC);m_tObserveTableExec.SetSQLParser(_sqlParser);
#define OB_TABLE_EXEC_RECORD(...) m_tObserveTableExec.Record();
//}

#endif

