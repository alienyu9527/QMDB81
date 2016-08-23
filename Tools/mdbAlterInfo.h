/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbAterInfo.h       
*@Description： 负责修改内存数据库系统及表信息
*@Author:       zhang.lin
*@Date：        2012年02月23日
*@History:
******************************************************************************************/
#ifndef __QUICK_MDB_ALTER_INFO_H__
#define __QUICK_MDB_ALTER_INFO_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{


#define SET_PROPERTY_INT_VALUE(PROPERTY,VARIABLE,VALUE)\
    if(VALUE <0 )\
    {\
        TADD_ERROR(-1,"invalid Integer Value[%s].",VALUE);\
        return -1;\
    }\
    else\
    {\
       VARIABLE =  VALUE;\
       TADD_NORMAL("after change: %s = [%d] in shared mem.",PROPERTY,VALUE);\
    }

#define SET_PROPERTY_BOOL_VALUE(PROPERTY,VARIABLE,VALUE)\
    if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "y") != 0 \
                && TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "n") != 0\
                && TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "true") != 0\
                && TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "false") != 0)\
    {\
        TADD_ERROR(-1,"invalid bool Value[%s]. valied value:[Y N TRUE FALSE]",VALUE);\
        return -1;\
    }\
    else\
    {\
        if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "y") == 0 \
            || TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "true") == 0)\
        {\
            VARIABLE = true;\
        }\
        else\
        {\
            VARIABLE = false;\
            TADD_NORMAL("after change: %s = [%s] in shared mem.",PROPERTY,VARIABLE?"true":"false");\
        }\
    }

#define BOOL_TO_STRING(VALUE)  ((VALUE)?"true":"false")

    class TMdbAlterInfo
    {
    public:
        TMdbAlterInfo();
        ~TMdbAlterInfo();

    	/******************************************************************************
    	* 函数名称  :  LinkDsn()
    	* 函数描述  :  链接某个DSN，但是不在管理区注册任何信息    
    	* 输入      :  pszDSN, 锁管理区所属的DSN 
    	* 输出      :  无
    	* 返回值    :  成功返回true, 失败返回false
    	* 作者      :  zhang.lin
    	*******************************************************************************/
    	bool LinkDsn(const char* pszDSN);

    	/******************************************************************************
    	* 函数名称  :  AlterSys()
    	* 函数描述  :  根据参数名，修改系统参数值
    	* 输入      :  pszName, pValue
    	* 输出      :  无
    	* 返回值    :  成功返回true, 失败返回false
    	* 作者      :  zhang.lin
    	*******************************************************************************/
    	int AlterSys(char* pszName,char* pValue);

    	/******************************************************************************
    	* 函数名称  :  AlterTable()
    	* 函数描述  :  根据参数名，修改参数值
    	* 输入      :  pszTable,pszProperty, pszValue
    	* 输出      :  无
    	* 返回值    :  成功返回true, 失败返回false
    	* 作者      :  zhang.lin
    	*******************************************************************************/
    	int AlterTable(char* pszTable,char* pszProperty,char* pszValue);

    	/******************************************************************************
    	* 函数名称  :  AlterColLen()
    	* 函数描述  :  根据表名&列名，设置长度
    	* 输入      :  pszTable,pszColumn, pszLen
    	* 输出      :  无
    	* 返回值    :  成功返回true, 失败返回false
    	* 作者      :  zhang.lin
    	*******************************************************************************/
    	bool AlterColLen(char* pszTable,char* pszColumn,char* pszLen);
    	int  SetProccessIsMonitor(const int iPid,const bool bMonitor);//设置进程是否备监控
    	int  SetDumpPackage(char cState);//设置CS 模式是否抓取数据包
        
    private:
    	/******************************************************************************
    	* 函数名称  :  SetTableValue()
    	* 函数描述  :  根据属性名，设置属性值
    	* 输入      :  pszProperty,pszValue
    	* 输出      :  无
    	* 返回值    :  无 
    	* 作者      :  zhang.lin
    	*******************************************************************************/
    	int SetTableValue(char* pszProperty,char* pszValue);


    	/******************************************************************************
    	* 函数名称  :  SetSysValue()
    	* 函数描述  :  根据属性名，设置属性值
    	* 输入      :  pszProperty,pszValue
    	* 输出      :  无
    	* 返回值    :  无 
    	* 作者      :  zhang.lin
    	*******************************************************************************/
    	int SetSysValue(char* pszProperty,char* pszValue);


    	/******************************************************************************
    	* 函数名称  :  SetSysAttr()
    	* 函数描述  : 添加属性，
    	* 输入      :  bSys,true:系统属性,false:表属性
    	* 输出      :  无
    	* 返回值    :  无 
    	* 作者      :  wang.liebao
    	*******************************************************************************/
        void SetSysAttr(bool bSys);//

    private:
        TMdbConfig *m_pConfig;
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn;     
        TMdbTable  *m_pMdbTable;
        TMdbTableSpace *m_pMdbTableSpace;
    	TMdbColumn *m_pColumn;
        char m_sAttrName[MAX_NAME_LEN];//DDL 语法对应的属性名字，如 log_level
        std::map<string,string> m_mapAttrName;//内存中属性与DDL配置属性的对应关系
    };

//}


#endif //__QUICK_MDB_ALTER_INFO_H__



