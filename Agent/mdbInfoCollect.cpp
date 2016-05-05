/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    TMdbInfoCollect.cpp		
*@Description�� mdb��Ϣ�ɼ�
*@Author:			jin.shaohua
*@Date��	    2013.2
*@History:
******************************************************************************************/
#include "Agent/mdbInfoCollect.h"
#include "Control/mdbProcCtrl.h"
#include "Interface/mdbQuery.h"

//namespace QuickMDB{

#define _MDB_TRY_CATCH_BEGIN_ try{

#define _MDB_TRY_CATCH_END_ }\
    catch(TMdbException& e)\
    {\
        TADD_ERROR(e.GetErrCode(),"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());\
        iRet = e.GetErrCode();\
    }\
    catch(...)\
    {\
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");\
        iRet = ERROR_UNKNOWN;\
    }


    TMdbInfoCollect::TMdbInfoCollect():
    m_pShmDSN(NULL)
    {
        
    }
    TMdbInfoCollect::~TMdbInfoCollect()
    {
        
    }
    /******************************************************************************
    * ��������	:  Attach   
    * ��������	: ���ӹ����ڴ�
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfoCollect::Attach(const char * sDsn)
    {
        int iRet = 0;
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDSN);
        return iRet;
    }
    /******************************************************************************
    * ��������	:  GetMemInfo
    * ��������	:  ��ȡ�ڴ���Ϣ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfoCollect::GetInfo(std::string & sJsonStr,const char * sInfoType)
    {
        int iRet = 0;
        INIT_MDB_JSON_WRITER(writer,sJsonStr);
        writer.StartObject();
        if(TMdbNtcStrFunc::StrNoCaseCmp(sInfoType,"mem_info") == 0)
        {
            CHECK_RET(SerializeMemInfo(writer),"SerializeMemInfo failed.");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sInfoType,"proc_info") == 0)
        {
            CHECK_RET(SerializeProcInfo(writer),"SerializeMemInfo failed.");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sInfoType,"link_info") == 0)
        {
            CHECK_RET(SerializeLinkInfo(writer),"SerializeMemInfo failed.");
        }
        else if (TMdbNtcStrFunc::StrNoCaseCmp(sInfoType,"dsn_info") == 0)
        {
            CHECK_RET(SerializeDsnInfo(writer),"SerializeDsnInfo failed.");
        }
        writer.EndObject();
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SerializeMemInfo
    * ��������	:  ���л��ڴ���Ϣ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfoCollect::SerializeMemInfo(MDB_JSON_WRITER_DEF &writer)
    {
        int iRet = 0;
        CHECK_RET(SerializeInfoFromDB(writer,"select * from dba_resource ;","mem_info"),"SerializeInfoFromDB failed");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  SerializeProcInfo
    * ��������	:  ���л�������Ϣ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfoCollect::SerializeProcInfo(MDB_JSON_WRITER_DEF &writer)
    {
        int iRet = 0;
        writer.String("proc_info");
        writer.StartObject();
        writer.String("data");
        TMdbProcCtrl tProcCtrl;
        tProcCtrl.Init(m_pShmDSN->GetInfo()->sName);
        tProcCtrl.Serialize(writer);
        writer.EndObject();
        return iRet;
    }
    /******************************************************************************
    * ��������	:  SerializeLinkInfo
    * ��������	:  ���л�������Ϣ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfoCollect::SerializeLinkInfo(MDB_JSON_WRITER_DEF &writer)
    {
         int iRet = 0;
         CHECK_RET(SerializeInfoFromDB(writer,"select * from dba_session ;","link_info"),"SerializeInfoFromDB failed");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  SerializeTableInfo
    * ��������	:  ���л�����Ϣ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfoCollect::SerializeTableInfo(MDB_JSON_WRITER_DEF &writer)
    {
        int iRet = 0;
        return iRet;
    }
    /******************************************************************************
    * ��������	:  SerializeDsnInfo
    * ��������	:  ���л�dsn��Ϣ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfoCollect::SerializeDsnInfo(MDB_JSON_WRITER_DEF &writer)
    {
        int iRet = 0;
        writer.String("dsn_info");
        writer.StartObject();
        writer.String("dsn_name");
        writer.String(m_pShmDSN->GetInfo()->sName);
        writer.String("version");
        writer.String(MDB_VERSION);
        writer.EndObject();
        return iRet;
    }
    /******************************************************************************
    * ��������	:  SerializeInfoFromDB
    * ��������	: �����ݿ��ȡ��Ϣ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfoCollect::SerializeInfoFromDB(MDB_JSON_WRITER_DEF &writer,const char * sSQL,const char * sDataTip)
    {
         int iRet = 0;
        _MDB_TRY_CATCH_BEGIN_
        TMdbDatabase tDB;
        tDB.ConnectAsMgr(m_pShmDSN->GetInfo()->sName);//�������ݿ�
        TMdbQuery * pQuery = tDB.CreateDBQuery();
        pQuery->SetSQL(sSQL);
        pQuery->Open();
        writer.String(sDataTip);
        writer.StartObject();
        writer.String("data");
        //��ȡ����
        writer.StartArray();
        int i = 0;
        while(pQuery->Next())
        {
            writer.StartArray();
            for(i = 0; i < pQuery->FieldCount();++i)
            {
                writer.String(pQuery->Field(i).AsString());
            }
            writer.EndArray();
        }
        writer.EndArray();
        //��ȡ����
        writer.String("FiledName");

        writer.StartArray();
        for(i = 0;i < pQuery->FieldCount();++i)
        {
           writer.String(pQuery->Field(i).GetName());
        }
        writer.EndArray();
        writer.EndObject();
        _MDB_TRY_CATCH_END_
        return iRet;
    }

//}


