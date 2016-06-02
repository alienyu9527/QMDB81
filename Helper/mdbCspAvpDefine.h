#ifndef _MDB_CSP_AVP_DEFINE_H_
#define _MDB_CSP_AVP_DEFINE_H_
//CSP协议中所使用的AVP code 的定义

//namespace QuickMDB{

//应答码
#define AVP_ANSWER_CODE  	2
//应答消息
#define AVP_ANSWER_MSG      3

//如果要换个端口号重连，消息码
#define AVP_ANSWER_PORT     4

//登陆请求
#define AVP_USER_NAME 		263
#define AVP_USER_PWD  		264
#define AVP_PROCESS_NAME  	266
#define AVP_PROCESS_ID	  	267
#define AVP_THREAD_ID  		268
#define AVP_LOW_PRIORITY    269
#define AVP_ORIGIN_REALM    270
#define AVP_OS_USER_NAME    271
//第几次连接请求
#define AVP_CON_NUM         272
#define AVP_TERMINAL_NAME   400

//剔除链接请求
#define AVP_SESSION_ID      20337
//剔除链接响应
//.....
//SQL执行请求
#define AVP_SQL_LABEL       20501
#define AVP_SQL_TYPE        20502
#define AVP_SQL_STATEMENT   20503
#define AVP_SQL_OPER		20504
#define AVP_SQL_FLAG        20521
//SQL执行响应
#define AVP_AFFECTED_ROW    20516
#define AVP_SELECT_HAVE_NEXT 20505
#define AVP_ROW_GROUP      	20508
#define AVP_COLUMN_GROUP	20506
#define AVP_COLUMN_NAME	20507
#define AVP_COLUMN_VALUE	20510
//动态SQL执行参数请求
#define AVP_PARAM_GROUP     20513
#define AVP_PARAM_NAME		20515
#define AVP_PARAM_VALUE   	20514
#define AVP_BATCH_GROUP   	20517
#define AVP_PARAM_STR_GROUP 20518//批处理区分

//SQL事务请求
#define AVP_COMMAND_NAME    20701
//SQL事务应答
//---
//调整日志级别请求
#define AVP_LOG_LEVEL		20801
#define AVP_PROCESS_ID		267
#define AVP_PROCESS_NAME	266
//调整日志级别响应
//--
//SQL查询NEXT请求
#define AVP_FETCH_ROWS		20530
//SQL 查询NEXT响应
//---
//创建表请求
#define AVP_TABLE_ID		30301
#define AVP_TABLE_NAME		30302
#define AVP_SPACE_ID		30303
#define AVP_RECORD_COUNTS	30304
#define AVP_EXPAND_RECORD	30305
#define AVP_IS_READ_LOCK	30306
#define AVP_IS_WRITE_LOCK	30307
#define AVP_IS_ROLLBACK		30308
#define AVP_IS_LOADDATA     30309 //Is-AfterCreate-LoadData
#define AVP_FILTER_SQL		30311
//#define AVP_COLUMN_GROUP	20506
//#define AVP_NEW_COLUMN_GROUP 30312 //20506已定义过
#define AVP_COLUMN_TYPE     20511
#define AVP_COLUMN_LENGTH 	20509
#define AVP_COLUMN_POS		30313
#define AVP_COLUMN_REP_TYPE	30314
#define AVP_INDEX_GROUP		30315
#define AVP_INDEX_NAME		30316
#define AVP_INDEX_PRIORITY	30317
#define AVP_PK_GROUP		30318
//创建表响应
//..
//删除表请求
//--
//删除表响应
//--
//创建用户请求
//--
//创建用户响应
//--
//删除用户请求
//--
//删除用户响应
//--
//获取序列请求
#define AVP_SEQUENCE_NAME	277
//获取序列响应
#define AVP_SEQUENCE_VALUE	278

//}
#endif
