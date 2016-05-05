/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbInfoCollect.h		
*@Description�� mdb��Ϣ�ɼ�
*@Author:			jin.shaohua
*@Date��	    2013.2
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
        int GetInfo(std::string & sJsonStr,const char * sInfoType);//��ȡ�ڴ���Ϣ
    private:
        int SerializeMemInfo(MDB_JSON_WRITER_DEF &writer);//���л��ڴ���Ϣ
        int SerializeProcInfo(MDB_JSON_WRITER_DEF &writer);//���л�������Ϣ
        int SerializeLinkInfo(MDB_JSON_WRITER_DEF &writer);//���л�������Ϣ
        int SerializeTableInfo(MDB_JSON_WRITER_DEF &writer);//����Ϣ
        int SerializeDsnInfo(MDB_JSON_WRITER_DEF &writer);//dsn��Ϣ
        int SerializeInfoFromDB(MDB_JSON_WRITER_DEF &writer,const char * sSQL,const char * sDataTip);//�����ݿ��ȡ��Ϣ
        TMdbShmDSN * m_pShmDSN;
    };

//}
#endif
