#include "Common/mdbBaseObject.h"
#include "Common/mdbDataStructs.h"
//namespace ZSmart
//
//{
//namespace QuickMDB
//{
        //根据字符串算出key
        MDB_UINT32 MdbNtcHashFunc(const char* pszKey)
        {
            const unsigned char* pKey = reinterpret_cast<const unsigned char*>(pszKey);
            MDB_UINT32 hval = 0;
            MDB_UINT32 g; 
            while (*pKey != 0) 
            { 
                hval = (hval << 4) + (MDB_UINT32)(*pKey++); 
                if ((g = (hval & 0xF0000000))) 
                { 
                    hval = hval ^ (g >> 24);
                    hval = hval ^ g; 
                } 
            } 
            return hval;
        }

        bool TMdbRuntimeObject::IsDerivedFrom(const TMdbRuntimeObject* pRuntimeObject) const
        {
            if(this == NULL) return false;
            const TMdbRuntimeObject* pBaseRuntimeObject = this;
            do
            {
                if(pBaseRuntimeObject == pRuntimeObject)
                {
                    return true;
                }
                else
                {
                    pBaseRuntimeObject = pBaseRuntimeObject->m_pBaseRuntimeObject;
                }
            }while(pBaseRuntimeObject);
            return false;
        }

        const char* TMdbRuntimeObject::GetParentObjectName() const
        {
            if(this && m_pBaseRuntimeObject)
            {
                return m_pBaseRuntimeObject->m_pszClassName;
            }
            else
            {
                return "";
            }
        }
        const TMdbRuntimeObject TMdbNtcBaseObject::oRuntimeObject("TMdbNtcBaseObject",sizeof(TMdbNtcBaseObject));
        //MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcBaseObject);
        TMdbNtcBaseObject::TMdbNtcBaseObject()
        {
            m_bConstructResult = true;
        }

        TMdbNtcBaseObject::~TMdbNtcBaseObject()
        {
        }

        //获得对象名
        TMdbNtcStringBuffer TMdbNtcBaseObject::ToString() const
        {    
            return "";
        }

        MDB_UINT64 TMdbNtcBaseObject::ToHash() const
        {    
            return 0;
        }

        //拷贝对象
        void TMdbNtcBaseObject::Assign(const TMdbNtcBaseObject *pObject)
        {
        }
        
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcRecordObject, TMdbNtcBaseObject);

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcLongObject, TMdbNtcBaseObject);
        //解开：const TMdbRuntimeObject TMdbNtcLongObject::oObjTLongObject("TMdbNtcLongObject", &TMdbNtcBaseObject::oObjTMdbNtcBaseObject);
        TMdbNtcLongObject::TMdbNtcLongObject()
        {
            m_lValue = 0;
        }

        TMdbNtcLongObject::TMdbNtcLongObject(MDB_INT64 lValue)
        {
            m_lValue = lValue;
        }

        TMdbNtcLongObject::TMdbNtcLongObject(const TMdbNtcLongObject& right)
        {
            Assign((TMdbNtcBaseObject*)&right);
        }

        TMdbNtcLongObject::~TMdbNtcLongObject()
        {
        }

        TMdbNtcStringBuffer TMdbNtcLongObject::ToString( ) const
        {
            return TMdbNtcStringBuffer(32, "%"MDB_NTC_ZS_FORMAT_INT64, m_lValue);
        }

        //给LongObject的value赋值
        void TMdbNtcLongObject::SetValue(MDB_INT64 lValue)
        {
            m_lValue = lValue;
        }

        //获得LongObject的value值
        MDB_INT64 TMdbNtcLongObject::GetValue() const
        {
            return m_lValue;
        }

        //拷贝对象
        void TMdbNtcLongObject::Assign(const TMdbNtcBaseObject *pObject)
        {
            if ((NULL != pObject) && (this != pObject))
            {
                m_lValue = ((TMdbNtcLongObject*)pObject)->m_lValue;
            }

        }

        MDB_INT64 TMdbNtcLongObject::Compare(const TMdbNtcBaseObject *pObject) const
        {
            if(pObject == NULL) return 1;
            else return m_lValue-((const TMdbNtcLongObject* )pObject)->m_lValue;
        }
        
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcStringObject, TMdbNtcBaseObject);
        TMdbNtcStringObject::TMdbNtcStringObject()
        {
        }

        TMdbNtcStringObject::TMdbNtcStringObject(char cSrc, int iRepeat /* = 1 */)
            :TMdbNtcStringBuffer(cSrc, iRepeat)
        {
        }

        TMdbNtcStringObject::TMdbNtcStringObject(const char* pszStr, int iLength /* = -1 */)
            :TMdbNtcStringBuffer(pszStr, iLength)
        {
        }

        TMdbNtcStringObject::TMdbNtcStringObject(int iBufferSize, const char* pszFormat /* = NULL */, ...)
        {
            va_list ap;
            va_start (ap, pszFormat);
            TMdbNtcStringBuffer::vSnprintf(iBufferSize, pszFormat, ap);
            va_end (ap);
        }

        TMdbNtcStringObject::TMdbNtcStringObject(const TMdbNtcStringObject& oStr)
            :TMdbNtcStringBuffer(oStr)
        {
        }

        TMdbNtcStringObject::TMdbNtcStringObject(const TMdbNtcString& oStr)
            :TMdbNtcStringBuffer(oStr)
        {
        }

        MDB_INT64 TMdbNtcStringObject::Compare(const TMdbNtcBaseObject *pObject) const
        {
            if(pObject == NULL) return 1;
            else return TMdbNtcStringBuffer::Compare(*(const TMdbNtcStringObject* )pObject);
        }
//    }
//}
