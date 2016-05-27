/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbStruct.h		
*@Description： mdb的基本定义与结构
*@Author:		
*@Date：	    2014/02/11
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_STRUCT_H__
#define __MINI_DATABASE_STRUCT_H__

#include "Helper/TThreadLog.h"    //日志
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
//版本信息
/*
版本信息说明，比如：QuickMDB V1.2.14.20140109
V1.2.14：是指内核版本号，这个只有在内核发生重大变更的时候才会修改。（需要重建qmdb，通过主备方式升级）
20140109：日期版本号，这个如果当天有代码变更，就会修改他，标识当前qmdb最近一次修改是什么时候。

QMDB只维护主版本，只有当出现紧急bug，才会在现场分支版本上修改。这时候，现场分支的代码版本里就会多一个“path”(和主版本区分出来),相应的日期版本也会修改。
QuickMDB V1.2.14.20131226.patch
*/
#define MDB_VERSION  "QuickMDB V1.4.1.20160428"


/**
 * @brief 各种路径定义
 * 
 */
#define MDB_SHM_BASE "QuickMDB_HOME"  //共享内存基点

#define MDB_HOME_ENV "$(QuickMDB_HOME)"  //mdb_home环境变量

#define MAX_IP_LEN      16  //IP的最大长度

#define MAX_DSN_COUNTS 10  //最多的DSN数量

	//事务状态
	enum E_TRANSACTION_STATE
	{
		//TRANSACTION_OUT 	 = 1,	  //事务外，暂时不启用无事务的状态
		TRANS_IN  	 = 2,     //事务中
		TRANS_COMMIT 	 = 3,     //提交中
		TRANS_ROLLBACK = 4      //回滚中
	};

	//minidb的数据来源
#define SOURCE_APP  1
#define SOURCE_REP  2
#define SOURCE_PEER 3
#define SOURCE_PEER_REP 4

//名字的最大长度
#define MAX_NAME_LEN   256

//时间的最大长度
#define MAX_TIME_LEN      15

//时间的最大长度
#define DATE_TIME_SIZE      15

#define MIN_DISK_SPACE_KB   (32*1024ULL)   //32M

#define MAX_TABLE_COUNTS  (500)

#define MAX_FILE_NAME  (256)

#define MAX_PATH_NAME_LEN   512

#define MAX_SEND_BUF  (1024*32)
#define MAX_VARCHAR_SIZE  (128*1024*1024)
#define MAX_SHM_ID         100 //最多申请的共享内存数
#define MAX_VARCHAR_SHM_ID 10000

//管理区的主Key，实际管理区的Key和数据库的实例相关
#define MANAGER_KEY       0x4765454

#define GET_MGR_KEY(_dsnValue)  MANAGER_KEY + (_dsnValue)
//变长数据存储区的主Key,也和数据库实例有关
#define GET_VARCHAR_KEY(_dsnValue)  0x5765454 + 1000000*(_dsnValue)

// 影子表刷新服务端共享内存主key
#define SHADOW_SERVER_KEY 0x7765454

// 默认的路由ID，作为不区分路由使用
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

//最大路由链表长度用逗号分隔
#define MAX_ROUTER_LIST_LEN 1024

#define MAX_SEQUENCE_COUNTS 255

//系统定义的最大用户数
#define MAX_USER_COUNT  64

//互斥锁的最大个数
#define MAX_MUTEX_COUNTS  10000

//varchar互斥锁最大个数
#define MAX_VARCHAR_MUTEX_COUNTS 10000

//管理区的间隔大小
#define INTERVAL_SIZE      1024

//db的状态
#define DB_unused '0'    //未使用----这个状态基本上不存在
#define DB_running 'U'   //正在使用--正常的同步中
#define DB_loading 'L'   //正在上载数据--指正在从Oracle上载数据
#define DB_repping 'R'   //正在同步数据，指程序启动时的同步数据---此时的数据写入落地文件
#define DB_stop 'S'      //停止---此时数据不能使用


//同步属性:'0'-不同步;'1'-和Oracle同步;'2'-和备机同步;'3'-都同步
#define DSN_No_Rep  0x0000
#define DSN_Ora_Rep 0x0001
#define DSN_Rep         0x0002
#define DSN_Two_Rep 0x0003


//系统的最大进程数
#define MAX_PROCESS_COUNTS 450

#define MAX_COLUMN_COUNTS 64

#define MAX_PRIMARY_KEY  10

//连接状态
#define Link_use  'L'  //连接正在使用
#define Link_down 'D'  //连接已经Down掉

//最大链接数
#define MAX_LINK_COUNTS  5500


//agent最大端口数
#define MAX_AGENT_PORT_COUNTS 5


//最大job个数
#define MAX_JOB_COUNT 100

//尝试的秒数，如果在3秒内无法连接，就认为对端没有启动
#define MAX_TRY_SECONDS 3

//一个复合索引最多有几个列
#define MAX_INDEX_COLUMN_COUNTS 10

//一个表最大的SQL数
#define MAX_SQL_COUNTS      255

//定义基本数据类型
typedef void* LPVOID;


#define QueryHasProperty(E,P)     (((E)&(P))==(P))
#define QueryHasAnyProperty(E,P)  (((E)&(P))!=0)
#define QuerySetProperty(E,P)     (E)|=(P)
#define QueryClearProperty(E,P)   (E)&=~(P)


#define MAXTABLELEN 1024 //默认哈希索引表大小        




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




//添加__FILE__,__LINE__方便错误查找
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
#define DATE_TIME_SIZE      15   //日期时间格式大小YYYYMMDDHHMISS
#define MDB_CHAR_SIZE    (8)  //mdb中char类型的size


    //minidb支持的hash索引类型有如下二种：
    #define HT_Unknown 0   //未知类型
    #define HT_Int     1   //数值类型
    #define HT_Char    2   //字符串类型
    #define HT_CMP     3   //复合类型

    //minidb的数据操作权限
    #define MDB_ADMIN  'A'
    #define MDB_WRITE  'W'
    #define MDB_READ   'R'

    //管理区共享内存大小
    #define MAX_MGR_SHM_SIZE   (1024*1024*1024)

    //minidb的表状态
    #define Table_unused '0'    //未使用----这个状态基本上不存在
    #define Table_running '1'   //正在使用--正常的同步中
    #define Table_loading '2'   //正在上载数据--指正在从Oracle上载数据
    #define Table_repping '3'   //正在同步数据，指程序启动时的同步数据---此时的数据写入落地文件
    #define Table_watingRep '4' //待同步---此时数据并不做主备机的同步
    #define Table_temp_watingRep '5' //临时待同步，如同链接临时断开

    #define ROUTER_ID_COL_NAME "routing_id" //路由ID的列名

    
    //进程状态
    #define PSTOP     'S'  //停止
    #define PFREE     'F'  //空闲
    #define PBUSY     'B'  //忙碌
    #define PKILL     'K'  //杀死
    #define PDUMP     'D'  //mdbAgent 抓包

    //默认端口
    #define DEFAULT_PORT    6666

     //表空间状态：’0’-未创建;’1’-已经创建;’2’-正在创建;’3’正在销毁
    #define TableSpace_unused     '0' //’0’-未创建
    #define TableSpace_using      '1' //’1’-正在使用
    #define TableSpace_creating   '2' //’2’-正在创建
    #define TableSpace_destroying '3' //’3’-正在销毁

    //主键的最大列数
    #define MAX_PRIMARY_KEY_CC  10

    //数据库的启动状态
    #define DB_SELF_CREATE    0        //自己创建，对端没有启动
    #define DB_NORMAL_LOAD    1        //正常上载，对端正在运行
    
    //minidb支持的SQL操作类型
		#define OP_Query  TK_SELECT  //查询数据 1
		#define OP_Update TK_UPDATE   //更新数据2
		#define OP_Delete TK_DELETE  //删除数据3
		#define OP_Insert TK_INSERT   //插入数据4
		#define OP_IstUpd 5    //如果没有数据则插入，否则更新5


    /*enum E_STORAGE_TYPE
    {
        STORAGE_ORACLE = 0,
        STORAGE_MYSQL = 1,
        STORAGE_FILE = 2,
        STORAGE_MDB = 3,
        STORAGE_UNKOWN
    };*/


    #define MDB_DS_TYPE_NO      'N' //无数据源，只存在内存中
    #define MDB_DS_TYPE_FILE    'F' //文件数据源
    #define MDB_DS_TYPE_ORACLE  'O' //Oracle作为数据源
    #define MDB_DS_TYPE_MYSQL  'M' //MySql作为数据源


// MDB  表的 同步属性配置类型
#define REP_DB2MDB "DB2MDB"
#define REP_MDB2DB  "MDB2DB"
#define REP_NoRep   "NoRep"

// 表的同步属性，默认为0
enum E_TAB_REP_TYPE
{
    REP_FROM_DB = 0, //  从oracle 同步
    REP_TO_DB = 1,  //  向Ora同步
    REP_NO_REP = 2, // 不同步
    REP_TO_DB_MDB =3 //向db mdb同步，兼容1.2
    };

    enum E_LOAD_PRIORITY
    {
        LOAD_FILE_FIRST = 1,  // 文件存储优先
        LOAD_DB_FIRST = 2 // 数据库存储优先
};

    #define SYS_TABLE_SPACE "SYS_TABLE_SPACE"

    // qmdb系统表DBA_SEQUENCE表名
    #define SYS_DBA_SEQUENCE "DBA_SEQUENCE"

    // 数据同步所使用记录格式版本
    #define VERSION_DATA_SYNC 'S'
    
    // 数据捕获所使用记录格式版本
    #define VERSION_DATA_CAPTURE 'C'

    // 兼容1.2 记录格式版本,old
    #define VERSION_DATA_12 'O'
//}

#endif //__MINI_DATABASE_STRUCT_H__


