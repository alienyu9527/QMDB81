/****************************************************************************************
*@Copyrights  2010，中兴软创（南京）计算机有限公司 开发部 CCB项目组
*@                   All rights reserved.
*@Name：	      DCCParser.h		
*@Description： CSP协议编码解码		
*@Author:		    li.shugang.jiang.mingjun
*@Date：	      2010年10月21日
*@History:
******************************************************************************************/
#ifndef __ZTE_QUICK_MEMORY_DATABASE_CSP_PARSER__H__
#define __ZTE_QUICK_MEMORY_DATABASE_CSP_PARSER__H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include "Helper/mdbCspAvpDefine.h"

#ifdef WIN32
#pragma warning(disable:4244)
#endif

using namespace std;

//namespace QuickMDB{
    

//网络字节顺序采用big endian排序方式。
//big-endian : 地址低位存储值的高位, 地址高位存储值的低位
//例子：在内存中双字0x01020304的存储方式:
//  LE 04 03 02 01
//  BE 01 02 03 04
#define HEAD_CSP_KEY_VALUE "QUICKMDB"

#define MAX_AVP_NAME_LEN  64  //AVP名字的最大长度
#define MAX_AVP_VALUE_LEN MAX_BLOB_LEN//AVP值的最大长度
#define CSP_HEAD_VERSION  0x01
#define MAX_AVP_TIME_LEN  6
#define MAX_CSP_LEN            65535

//应用场景定义
#define CSP_APP_ERROR        	100
#define CSP_APP_LOGON      		200
#define CSP_APP_KICK_LINK  		220
#define CSP_APP_SEND_SQL   		230
#define CSP_APP_SEND_PARAM 		240
#define CSP_APP_ACTION     		250
#define CSP_APP_ADJUST_LOG 		260
#define CSP_APP_NEXT       		270
#define CSP_APP_C_TABLE    		280
#define CSP_APP_C_USER     		290
#define CSP_APP_D_TABLE    		300
#define CSP_APP_D_USER     		310
#define CSP_APP_GET_SEQUENCE    330
#define CSP_APP_SEND_PARAM_ARRAY 340  //批量绑定变量
#define CSP_APP_QMDB_INFO 350 //获取qmdb信息
#define SIZE_MSG_AVP_HEAD 43

static char  CSP_AVP_STRING[]=
"##CSP_APP_SEND_SQL\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE \n"
"<Sql-Label>                                  1                    20501       M           Unsigned32\n"
"{Sql-Type}                                   1                    20502       M           Integer32\n"
"{Sql-Statements}                             1                    20503       M           OctetString\n"
"{Sql-Oper}                                   1                    20504       M           Integer32\n"
"{Sql-Flag}                           1                    		20521       C           Integer32\n"
"##CSP_APP_SEND_SQL_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE \n"
"<Sql-Label>                                  1                    20501       M           Unsigned32\n"
"{Answer-Msg}                                 1                    3           M           OctetString\n"
"{Answer-Code}                                1                    2           M           Integer32\n"
"{Affected-Record-Number}                     1                    20516       M           Integer32\n"
"{Select-Have-Next}                           1                    20505       M           Integer32\n"
"*{Row-Group}                                 1                    20508       M           Grouped\n"
"*{Coloum-Group}                          2                    20506       M           Grouped\n"
"{Coloum-Name}                        3                    20507       M           OctetString	\n"
"{Coloum-Value}                       3                    20510       M           OctetString	\n"
"\n"
"##CSP_APP_LOGON\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE    \n"
"{User-Name}                                    1                    263       M           OctetString      \n"
"{User-Password}                                1                    264       M           OctetString\n"
"{Process-Name}                                 1                    266       M           OctetString\n"
"{Process-Id}                                   1                    267       M           Unsigned32\n"
"{Thread-Id}                                    1                    268       M           Unsigned32\n"
"{Low-Priority}                                 1                    269       C           Unsigned32\n"
"{OS-User-Name}                                 1                    271       M           OctetString\n"
"{CON-NUM}                                      1                    272       M           Integer32\n"
"{Terminal-Name}                                1                    400       M           OctetString\n"
"{Origin-Realm}                                 1                    270       M           OctetString\n"
"\n"
"##CSP_APP_LOGON_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"{Answer-Port}                                 1                    4       M               Integer32\n"                          
"{Answer-Code}                                 1                    2       M               Integer32\n"
"[Answer-Msg]                                  1                    3       C               OctetString	\n"
"\n"
"##CSP_APP_KICK_LINK\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE  \n"
"*{Session-Id}                                 1                    20337       M           Unsigned32\n"
"\n"
"##CSP_APP_KICK_LINK_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"{Answer-Code}                                 1                    2       M           Integer32\n"
"[Answer-Msg]                                  1                    3       C           OctetString\n"
"\n"
"##CSP_APP_SEND_PARAM\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"<Sql-Label>                                  1                    20501       M           Unsigned32\n"
"*{Param-Group}                               1                    20513       M           Grouped\n"
"{Param-Name}                             2                    20515       M           OctetString\n"
"{Param-Value}                            2                    20514       M           OctetString\n"
"\n"
"##CSP_APP_SEND_PARAM_ARRAY\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"<Sql-Label>                                  1                    20501       M           Unsigned32\n"
"*{Batch-Group}                             1                  20517      M           Grouped\n"
"*{Param-Group}                     2                   20518       M           Grouped\n"
//"{Param-Name}                3                    20515       M           OctetString\n"
//"{Param-Value}                 2                          20514       M           OctetString\n"
"\n"
"##CSP_APP_ACTION\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE  \n"
"{Command-Name}                                     1               20701       M          OctetString\n"
"\n"
"##CSP_APP_ACTION_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE  \n"
"{Answer-Msg}                                 1                    3           M           OctetString\n"
"{Answer-Code}                                1                    2           M           Integer32\n"
"\n"
"##CSP_APP_ADJUST_LOG\n"
"AVP_NAME                                   AVP-LEVLE         AVP_CODE  M-FLAG        DATA_TYPE\n"
"{Log-Level}                                 1                 20801       M          Unsigned32\n"
"[Process-Id]                                1                 267         C          Unsigned32\n"
"[Process-Name]                              1                 266         C          OctetString\n"
"\n"
"##CSP_APP_ADJUST_LOG_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"{Answer-Msg}                                 1                    3           M           OctetString\n"
"{Answer-Code}                                1                    2           M           Integer32\n"
"\n"
"##CSP_APP_ERROR\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"{Answer-Msg}                                 1                    3           M           OctetString\n"
"{Answer-Code}                                1                    2           M           Integer32\n"
"\n"
"##CSP_APP_NEXT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"<Sql-Label>                                     1                  20501       M          Unsigned32\n"
"[Fetch-Rows-Num]                                1                  20530       M          Unsigned32\n"
"\n"
"##CSP_APP_NEXT_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE     M-FLAG      DATA_TYPE\n"
"*{Sql-Group}                                     1                  20599       M           Grouped\n"
"<Sql-Label>                                  2                  20501       M           Unsigned32\n"
"{Sql-Oper}                                   2                  20504       M           OctetString\n"
"{Answer-Msg}                                 2                  3           M           OctetString\n"
"{Answer-Code}                                2                  2           M           Integer32\n"
"{Affected-Record-Number}                     2                  20516       M           Integer64\n"
"{Select-Have-Next}                           2                  20505       M           Integer32\n"
"*{Sql-Result}                                2                  20506       M           Grouped\n"
"{Coloum-Name}                            3                  20507       M           OctetString\n"
"\n"
"##CSP_APP_C_TABLE\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE    M-FLAG        DATA_TYPE\n"
"{Table-Id}                                     1                   	30301       M          Unsigned32\n"
"{Table-Name}                                   1                   	30302       M          OctetString\n"
"{Table-Space-Id}                               1                   	30303       M          Unsigned32\n"
"{Record-Counts}                                1                   	30304       C          Unsigned32\n"
"{Expand-Record}                                1                   	30305       C          Unsigned32\n"
"{Is-Read-Lock}                                 1                   	30306       C          OctetString\n"
"{Is-Write-Lock}                                1                   	30307       C          OctetString\n"
"{Is-Rollback}                                  1                 	30308	    C          OctetString\n"
"{Is-AfterCreate-LoadData}                      1                 	30309       M          OctetString\n"
"[Filter-Sql]                                   1                 	30311       C          OctetString\n"
"*{Coloum-Group}                                1                 	30312       C          Grouped\n"
"{Coloum-Name}                              2                 	20507       C          OctetString\n"
"{Coloum-Type}                              2                 	20508       C          OctetString\n"
"{Coloum-Length}                            2                 	20509       C          Unsigned32\n"
"{Column-Pos}                               2                 	30313       C          Unsigned32\n"
"{Coloum-Rep-Type}                          2                 	30314       C          OctetString\n"
"*{Index-Group}	                               1                 	30315       C          Grouped\n"
"{Index-Name}                               2                 	30316       C          OctetString\n"
"{Column-Pos}                               2                 	30313       C          OctetString\n"
"{Priority}                                 2                 	30317       C          Unsigned32\n"
"*{Primary-Key-Group}	                       1                 	30318       C          Grouped	\n"
"{Column-Pos}                               2                 	30313       C          Unsigned32\n"
"\n"
"##CSP_APP_C_TABLE_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE    \n"
"{Answer-Msg}                                 1                    3           M           OctetString\n"
"{Answer-Code}                                1                    2           M           Integer32\n"
"\n"
"##CSP_APP_C_USER\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE    \n"
"{User-Name}                                      1                263       M          OctetString\n"
"{User-Password}                                  1                264       M          OctetString\n"
"{User-Permissions}                               1                299       M          OctetString\n"
"\n"
"##CSP_APP_C_USER_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n" 
"{Answer-Msg}                                 1                    3           M           OctetString\n"
"{Answer-Code}                                1                    2           M           Integer32\n"
"\n"
"##CSP_APP_D_TABLE\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"{Table-Id}                                     1                 30301       M          Unsigned32\n"
"\n"
"##CSP_APP_D_TABLE_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"{Answer-Msg}                                 1                    3           M           OctetString\n"
"{Answer-Code}                                1                    2           M           Integer32\n"
"\n"
"##CSP_APP_D_USER\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"{User-Name}                                      1                 263       M          OctetString\n"
"\n"
"##CSP_APP_D_USER_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE\n"
"{Answer-Msg}                                 1                    3           M           OctetString\n"
"{Answer-Code}                                1                    2           M           Integer32\n"
"\n"
"##CSP_APP_GET_SEQUENCE\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE \n"
"{Sequence-Name}                              1                     277       M            OctetString\n"
"\n"
"##CSP_APP_GET_SEQUENCE_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE  \n"
"{Sequence-Value}                             1                    278         M           Integer64\n"
"{Answer-Msg}                                 1                    3           M           OctetString\n"
"{Answer-Code}                                1                    2           M           Integer32\n"
"\n"
"##CSP_APP_QMDB_INFO\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE    \n"
"{Command-Name}                                     1               20701       M          OctetString	\n"
"\n"
"##CSP_APP_QMDB_INFO_RESULT\n"
"AVP_NAME                                     AVP-LEVLE            AVP_CODE  M-FLAG        DATA_TYPE   \n"
"{Answer-Msg}                                 1                          3           M           OctetString\n"
"{Answer-Code}                                1                         2           M           Integer32\n";
//csp应用类型定义
struct ST_CSP_APP_TYPE
{
	int    iAppType;
	const char * sRequestName;
	const  char * sAnswerNem;
};
const static ST_CSP_APP_TYPE g_cspAppType[] = {
	{CSP_APP_ERROR,        "##CSP_APP_ERROR",     "##CSP_APP_ERROR"},
	{CSP_APP_LOGON,        "##CSP_APP_LOGON",     "##CSP_APP_LOGON_RESULT"},
	{CSP_APP_KICK_LINK,    "##CSP_APP_KICK_LINK", "##CSP_APP_KICK_LINK_RESULT"},
	{CSP_APP_SEND_SQL,     "##CSP_APP_SEND_SQL",  "##CSP_APP_SEND_SQL_RESULT"},
	{CSP_APP_SEND_PARAM,   "##CSP_APP_SEND_PARAM","##CSP_APP_SEND_PARAM"},
	{CSP_APP_SEND_PARAM_ARRAY,   "##CSP_APP_SEND_PARAM_ARRAY","##CSP_APP_SEND_PARAM_ARRAY"},
	{CSP_APP_ACTION,       "##CSP_APP_ACTION",    "##CSP_APP_ACTION_RESULT"},
	{CSP_APP_ADJUST_LOG,   "##CSP_APP_ADJUST_LOG","##CSP_APP_ADJUST_LOG_RESULT"},
	{CSP_APP_NEXT,         "##CSP_APP_NEXT",      "##CSP_APP_NEXT_RESULT"},
	{CSP_APP_C_TABLE,	   "##CSP_APP_C_TABLE",   "##CSP_APP_C_TABLE_RESULT"},
	{CSP_APP_C_USER,	   "##CSP_APP_C_USER",    "##CSP_APP_C_USER_RESULT"},
	{CSP_APP_D_TABLE,      "##CSP_APP_D_TABLE",   "##CSP_APP_D_TABLE_RESULT"},
	{CSP_APP_D_USER,       "##CSP_APP_D_USER",    "##CSP_APP_D_USER_RESULT"},
	{CSP_APP_GET_SEQUENCE,"##CSP_APP_GET_SEQUENCE","##CSP_APP_GET_SEQUENCE_RESULT"},
    {CSP_APP_QMDB_INFO,"##CSP_APP_QMDB_INFO","##CSP_APP_QMDB_INFO_RESULT"}

};



//数据类型定义
#define CSP_TYPE_UNKOWN 0
#define CSP_TYPE_STRING 1
#define CSP_TYPE_INT32  2
#define CSP_TYPE_UINT32 3
#define CSP_TYPE_INT64  4
#define CSP_TYPE_UINT64 5
#define CSP_TYPE_GROUP  6
#define CSP_TYPE_TIME   7




#define NUM1 16777216
#define NUM2 65536


//CSP协议解析异常
class TMdbCSPException
{
public:
    virtual ~TMdbCSPException() {}
    TMdbCSPException(){}
    TMdbCSPException(const int lErrCode, const char* pszFormat, ...);
    char *GetErrMsg() const {return((char*)m_sErrMsg);}
    int GetErrCode() {return m_lErrCode;}
protected:    
    char m_sErrMsg[1024];        //错误信息
    int m_lErrCode;              //错误号
};




//本地字节序<===>网络字节序
class TMdbAvpConvert
{
public:
	inline static int CharToInt(char cValue)
    {
        return cValue<0? cValue+256: cValue;
    }
    inline static void NetToInt32(unsigned char* pszAddr, int &iValue)
    {
    	iValue = CharToInt(pszAddr[0])*NUM1 + CharToInt(pszAddr[1])*NUM2 + CharToInt(pszAddr[2])*256 + CharToInt(pszAddr[3]);
    }    
    inline static void NetToInt64(unsigned char* pszAddr, long long &iValue)
    {
    	iValue = CharToInt(pszAddr[0])*256*256*256*256*256*256*256 + CharToInt(pszAddr[1])*256*256*256*256*256*256 + 
    	         CharToInt(pszAddr[2])*256*256*256*256*256 + CharToInt(pszAddr[3])*256*256*256*256 +
                 CharToInt(pszAddr[4])*16777216 + CharToInt(pszAddr[5])*65536 + 
                 CharToInt(pszAddr[6])*256 + CharToInt(pszAddr[7]);
    }    
    inline static void NetToUInt32(unsigned char* pszAddr, unsigned int &iValue)
    {
    	iValue = CharToInt(pszAddr[0])*16777216 + CharToInt(pszAddr[1])*65536 + 
    	         CharToInt(pszAddr[2])*256 + CharToInt(pszAddr[3]);
    }    
    inline static void NetToUInt64(unsigned char* pszAddr, unsigned long long &iValue)
    {
    	iValue = CharToInt(pszAddr[0])*256*256*256*256*256*256*256 + CharToInt(pszAddr[1])*256*256*256*256*256*256 + 
    	         CharToInt(pszAddr[2])*256*256*256*256*256 + CharToInt(pszAddr[3])*256*256*256*256 +
                 CharToInt(pszAddr[4])*256*256*256 + CharToInt(pszAddr[5])*256*256 + 
                 CharToInt(pszAddr[6])*256 + CharToInt(pszAddr[7]);
    }  
    inline static void Int32ToNet(unsigned char* pszAddr, int iValue)
    {
    	pszAddr[0] = (iValue>>24) & 0xff;
    	pszAddr[1] = (iValue>>16) & 0xff;
    	pszAddr[2] = (iValue>>8) & 0xff;
    	pszAddr[3] = iValue & 0xff;    
    }  
    inline static void Int64ToNet(unsigned char* pszAddr, long long iValue)
    {
    	pszAddr[0] = (iValue>>56) & 0xff;
    	pszAddr[1] = (iValue>>48) & 0xff;
   	 	pszAddr[2] = (iValue>>40) & 0xff;
    	pszAddr[3] = (iValue>>32) & 0xff;
    	pszAddr[4] = (iValue>>24) & 0xff;
    	pszAddr[5] = (iValue>>16) & 0xff;
    	pszAddr[6] = (iValue>>8)  & 0xff;
    	pszAddr[7] = iValue & 0xff;    
    } 
    inline static void UInt32ToNet(unsigned char* pszAddr, unsigned int iValue)
    {
    	pszAddr[0] = (iValue>>24) & 0xff;
    	pszAddr[1] = (iValue>>16) & 0xff;
    	pszAddr[2] = (iValue>>8)  & 0xff;
    	pszAddr[3] = iValue & 0xff;    
    }    
    inline static void UInt64ToNet(unsigned char* pszAddr, unsigned long long iValue)
    {
    	pszAddr[0] = (iValue>>56) & 0xff;
    	pszAddr[1] = (iValue>>48) & 0xff;
    	pszAddr[2] = (iValue>>40) & 0xff;
    	pszAddr[3] = (iValue>>32) & 0xff;
    	pszAddr[4] = (iValue>>24) & 0xff;
    	pszAddr[5] = (iValue>>16) & 0xff;
    	pszAddr[6] = (iValue>>8)  & 0xff;
    	pszAddr[7] = iValue & 0xff;        
    }   
};

//avp head 解析
class TMdbAvpHead
{
public:
    TMdbAvpHead();
    void CnvtToBin(unsigned char* pszMsg,int SessionId); 
    void BinToCnvt(unsigned char* pszMsg); 
    void CnvtToBinPlus(unsigned char* pszMsg,unsigned int SessionId); 
    void BinToCnvtPlus(unsigned char* pszMsg); 
    
    void Clear();
    void SetVersion(int iVer){iVersion = iVer;};
    //void SetBigEndian(int iBig){iIsBigEndian = iBig;};
    void SetCmdCode(int ComId);
    unsigned int GetSequence();
    void SetSequence(unsigned int iseq);
    void UpdateAvpFlag(){iCurrentAvpFlag ++;if(0== iCurrentAvpFlag)iCurrentAvpFlag = 1;}//更新avp flag
    char sHeadName[7]; //<QMDB>
    char sKeyValue[17]; //key 数字字符串加密表示 ,ZSMART_QMDB
    int iVersion;
    //int iIsBigEndian;
    unsigned int iLen;
    unsigned short int iCmdCode;
    unsigned int iSessionId;
    unsigned int iAnsCodePos;
    char sSendTime[7]; //表示时分秒
    unsigned int isequence; //序列
    unsigned int iCurrentAvpFlag;//当前avp的标识
    void Print();//打印消息包头信息
};

class TMdbCspParser;

//一个AVP项
class TMdbAvpItem
{
public:
    TMdbAvpItem();
    ~TMdbAvpItem();
    void Clear();
    int Serialize(unsigned char* pszOut,TMdbCspParser * pParser) throw (TMdbCSPException);//将该项序列化输出
    int SetAvp(unsigned char* pszDCC, int iLen,TMdbCspParser * pParser) throw (TMdbCSPException);//设置节点信息
   // int GetTotalLen();//获取本节点以及子节点的长度和
  //  int FinishFill();//完成填充
public:    
    unsigned int  iUpdateFlag;//更新标识，用来对比是否是老数据
    
    char sName[MAX_AVP_NAME_LEN];   //AVP名称
    int  iCode;            //AVP-Code
    int  iLen;             //AVP长度
    int  iType;                     //数据类型
    bool bIsExist;                  //是否存在
    bool bIsMul;                    //是否允许有多个
    bool bIsM;                      //是否必须存在
    int  iLevel;                    //所处层次，依次为1.2.3.....   
    
    char* pszValue;                 //AVP的值
    int                iValue;
    unsigned int       uiValue;
    long long          llValue;
    unsigned long long ullValue;
    bool m_bNULLValue;//NULL值
    
    TMdbAvpItem*  pFatherItem;  //父亲节点
    TMdbAvpItem*  pChildItem;   //子节点
    TMdbAvpItem*  pNextItem;    //兄弟节点
};

class TMdbAvpHelper;
//csp 协议解析类
class TMdbCspParser
{
	public:
		TMdbCspParser();
		~TMdbCspParser();
		/******************************************************************************
		* 函数名称	:  Init
		* 输入		:  iType - 协议包类型  bRequest- true(request) false(answer) 
		* 输出		:  
		* 返回值	:  成功返回0，失败返回错误码
		* 作者		:  jin.shaohua
		*******************************************************************************/
		int Init(int iType,bool bRequest);//初始化包结构
		void Print();//打印
		void SetVersion(int iVer){m_tHead.SetVersion(iVer);};
        //void SetBigEndian(int iBig){m_tHead.SetBigEndian(iBig);};
		int Serialize(unsigned char* pszDCC,int SessionId,int iSequence) throw (TMdbCSPException);    //序列化成二进制串
		int DeSerialize(unsigned char* pszDCC,int iLen) throw (TMdbCSPException);//解析二进制串
		//设置item的值
		int SetItemValue(TMdbAvpItem* pStartItem,int iCode,const char * sValue) throw (TMdbCSPException);
		int SetItemValue(TMdbAvpItem* pStartItem,int iCode,unsigned int iValue) throw (TMdbCSPException);
		int SetItemValue(TMdbAvpItem* pStartItem,int iCode,int iValue) throw (TMdbCSPException);
		int SetItemValue(TMdbAvpItem* pStartItem,int iCode,unsigned long long iValue) throw (TMdbCSPException);
		int SetItemValue(TMdbAvpItem* pStartItem,int iCode,long long iValue) throw (TMdbCSPException);
        int SetItemValueNULL(TMdbAvpItem* pStartItem,int iCode) throw (TMdbCSPException);//设置NULLvalue
        //批处理特殊处理
        int SetItemValueForParamGroup(TMdbAvpItem* pStartItem,int iCode,const char * sValue) throw (TMdbCSPException);
		TMdbAvpItem* m_pRootAvpItem;
		TMdbAvpHead  m_tHead;
		int Clear();//清理数据
		int GetTotalLen(){return m_iTotalLen;}//获取总长度
		int GetIncreaseLen(){int iTemp = m_iIncreaseLen;m_iIncreaseLen =0;return iTemp;}//获取最近一次增长的长度
		int GetItemLen(TMdbAvpItem* pStartItem);//获取总长度
		void FinishFillGroup(TMdbAvpItem* pStartItem,int iGroupLen);
		inline void GetAvpItemCodeLen(unsigned char* pszDCC,int & iCode,int & iLen)
		{
			iCode = pszDCC[0]*256 + pszDCC[1];
			iLen  = pszDCC[2]*256 + pszDCC[3];
		}
        TMdbAvpItem *   GetFreeAvpItem(TMdbAvpItem * pStartItem, int iAvpCode);//获取一个空闲avp item	
        char * 		    GetStringValue(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
		unsigned int    GetUINT32Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
		int 			GetINT32Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
		long long 	    GetINT64Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
		unsigned long long GetUINT64Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
        bool               IsNullValue(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);//判断是否是null value
		TMdbAvpItem * FindExistAvpItem(TMdbAvpItem * pStartItem, int iAvpCode);//寻找存在的avp item
		int           GetExistGroupItem(TMdbAvpItem * pStartItem,int iAvpCode,std::vector<TMdbAvpItem * > &vGroupAvpItem);//寻找存在avpgroup
		bool IsAvpFree(TMdbAvpItem* pItem){return pItem->iUpdateFlag != m_tHead.iCurrentAvpFlag;}
        TMdbAvpItem * GetExistGroupItemForParam(TMdbAvpItem * pStartItem,int iAvpCode);
        
        //批处理特殊处理
        char * GetStringValueForParamGroup(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
	protected:
		int ParserConfig(const char *pszIdemt);//解析配置文件
		TMdbAvpItem* ParserLine(const char* pszLineText);//把当前文字解析为AVP项
		int GetItemName(const char* pszLineText,int & iPos,TMdbAvpItem* pItem);
        int GetItemCode(const char* pszLineText,int & iPos,TMdbAvpItem* pItem);
		int AddItem(TMdbAvpItem* &pCurItem,TMdbAvpItem* pItem);
	protected:
		TMdbAvpItem*    CopyItem(TMdbAvpItem* pItem);//copy item
	    int m_iTotalLen;//总长度
	    int m_iIncreaseLen;//增长的长度

};

//csp协议包分析帮助类
class TMdbAvpHelper
{
	public:
		// 获取avp项中的code和len
		static inline void GetAvpItemCodeLen(unsigned char* pszDCC,int & iCode,int & iLen)
		{
			iCode = pszDCC[0]*256 + pszDCC[1];
			iLen  = pszDCC[2]*256 + pszDCC[3];
		}
		static void PrintTotalAvp(int iOffset,TMdbAvpItem * pRootAvp);//打印所有avp
		static void PrintAvpItem(int iOffset,TMdbAvpItem * pOneAvp);//打印avp
		//打印偏移量
		static inline void PrintOffset(int iOffset)
		{
			for (int i = 0;i<iOffset;i++)
			{
				printf("\t");
			}
		}
		static const char * TypeToStr(int iType);//avp数据类型int  转str
		static int    		StrToType(const char * sTypeStr);//avp数据类型str  转int
		
		static const char * GetAvpValue(TMdbAvpItem * pAvpItem,char * sValue);//获取avp 的值
		#if 0
		static TMdbAvpItem* CopyItem(TMdbAvpItem* pItem);//copy item
		static TMdbAvpItem * GetFreeAvpItem(TMdbAvpItem * pStartItem, int iAvpCode);//获取一个空闲avp item		
		static char * 		GetStringValue(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
		static unsigned int GetUINT32Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
		static int 			GetINT32Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
		static long long 	GetINT64Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
		static unsigned long long GetUINT64Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);
              static bool IsNullValue(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException);//判断是否是null value
		static TMdbAvpItem * FindExistAvpItem(TMdbAvpItem * pStartItem, int iAvpCode);//寻找存在的avp item
		static int GetExistGroupItem(TMdbAvpItem * pStartItem,int iAvpCode,std::vector<TMdbAvpItem * > &vGroupAvpItem);//寻找存在avpgroup
            #endif
    private:
            static const char *  m_cspTypeStr[] ;

};	


    inline void FastMemcpy (void* dest, void* src, size_t len)
    {
        if(!(len > 0))
        {
            return;
        }
        if(dest == src)
        {
            return;
        }

        register size_t n = len / 8;    /* count > 0 assumed */
        size_t c = len % 8;
        unsigned char* from = (unsigned char*)src;
        unsigned char* to = (unsigned char*)dest;

        for(size_t i=0; i<c; ++i)
        {
            *to++ = *from++;
        }

        for(size_t i = 0; i < n; ++i)
        {
            *to++ = *from++;
            *to++ = *from++;
            *to++ = *from++;
            *to++ = *from++;
            *to++ = *from++;
            *to++ = *from++;
            *to++ = *from++;
            *to++ = *from++;
        }

        *to = '\0';

    }

//}
#endif //__ZTE_QUICK_MEMORY_DATABASE_CSP_PARSER__H__

