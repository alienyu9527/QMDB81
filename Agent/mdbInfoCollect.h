/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbInfoCollect.h		
*@Description： mdb信息采集
*@Author:			jin.shaohua
*@Date：	    2013.2
*@History:
******************************************************************************************/
#ifndef _MDB_INFO_COLLECT_H_
#define _MDB_INFO_COLLECT_H_
#include "Helper/mdbJson.h"
#include "Control/mdbMgrShm.h"
#include <string>

//namespace QuickMDB{

    class TMdbInfoCollect
    {
    public:
        TMdbInfoCollect();
        ~TMdbInfoCollect();
        int Attach(const char * sDsn);//attach
        int GetInfo(std::string & sJsonStr,const char * sInfoType);//获取内存信息
    private:
        int SerializeMemInfo(MDB_JSON_WRITER_DEF &writer);//序列化内存信息
        int SerializeProcInfo(MDB_JSON_WRITER_DEF &writer);//序列化进行信息
        int SerializeLinkInfo(MDB_JSON_WRITER_DEF &writer);//序列化链接信息
        int SerializeTableInfo(MDB_JSON_WRITER_DEF &writer);//表信息
        int SerializeDsnInfo(MDB_JSON_WRITER_DEF &writer);//dsn信息
        int SerializeInfoFromDB(MDB_JSON_WRITER_DEF &writer,const char * sSQL,const char * sDataTip);//从数据库获取信息
        TMdbShmDSN * m_pShmDSN;
    };

//}
#endif
