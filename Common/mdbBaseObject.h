/**
 * @file mdbBaseObject.h
 * @brief �������ķ�װ
 *
 * �������ķ�װ
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121225
 * @warning
 */
#ifndef _MDB_H_BaseObject_
#define _MDB_H_BaseObject_
#include "Common/mdbCommons.h"
#include "Common/mdbStrings.h"
//namespace QuickMDB
//{
//    namespace BillingSDK
//    {
        typedef MDB_UINT32 (*mdb_ntc_hash_func)(const char* pszKey);
        
        /**
         * @brief hash�����������ַ������key
         * 
         * @param pszKey [in] hash key�ַ���
         * @return MDB_UINT32
         * @retval hash key
         */
        MDB_UINT32 MdbNtcHashFunc(const char* pszKey);

        class TMdbNtcBaseObject;
        /**
         * @brief ��������ʱ����
         * 
         */
        class TMdbRuntimeObject
        {
        public:
            TMdbRuntimeObject(const char* pszClassName, unsigned  int iObjectSize,
                TMdbNtcBaseObject* (*pfnCreateObject)() = NULL,
                const TMdbRuntimeObject* pBaseRuntimeObject = NULL)
                :m_pszClassName(pszClassName), m_pBaseRuntimeObject(pBaseRuntimeObject),
                m_iObjectSize(iObjectSize), m_pfnCreateObject(pfnCreateObject)
            {
                MDB_NTC_ZF_ASSERT(!(m_pBaseRuntimeObject && m_pfnCreateObject
                    && m_pfnCreateObject == m_pBaseRuntimeObject->m_pfnCreateObject));
            }
            const char* GetObjectName() const {return m_pszClassName;}
            unsigned int GetObjectSize()const {return m_iObjectSize;}
            const char* GetParentObjectName() const;
            TMdbNtcBaseObject* CreateObject() const{return m_pfnCreateObject?m_pfnCreateObject():NULL;}
            const TMdbRuntimeObject * GetParentRuntimeObject() const {return m_pBaseRuntimeObject;}
            /**
             * @brief �Ƿ�������ĳ������
             * 
             * @param pRuntimeObject [in] ��������ZF_RUNTIME_OBJECT(TLongObjct)
             * @return bool
             * @retval true ��������ָ��������
             */
            bool IsDerivedFrom(const TMdbRuntimeObject* pRuntimeObject) const;
        protected:
            const char* m_pszClassName;///< ��������
            const TMdbRuntimeObject* m_pBaseRuntimeObject;///< ���������ʱ����
            const unsigned int m_iObjectSize;
            TMdbNtcBaseObject* (*m_pfnCreateObject)();
        };
        /**
         * @brief ���ڵõ�ָ��object�����ڱ�RTTI�����ٵ������ж�
         * ��GetObjectType��������ʹ�ã��ɶ�̬�ж�����������Ƿ�Ϊָ������󣨷�RTTI���ƣ�
         * �÷�pLongObject->GetObjectType()==MDB_ZF_RUNTIME_OBJECT(TMdbNtcLongObject)
         * ����pLongObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcLongObject))
         */
        #define MDB_ZF_RUNTIME_OBJECT(class_name) (&class_name::oRuntimeObject)
        #define MDB_ZF_DYNAMIC_CAST(class_name, x) ((((/*QuickMDB::*/TMdbNtcBaseObject*)x)->IsDerivedFrom(MDB_ZF_RUNTIME_OBJECT(class_name)))?static_cast<class_name *>(x):NULL)
        
        /*
         * @brief �����ṩ���������С�Ļ�ȡ��ʵ�ʶ���������ָ�����أ�ռ���ڴ���ʵ�����С����
         * ͬʱ�ṩ�������жϵķ���pLongObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcLongObject))
        */
        #define MDB_ZF_DECLARE_OBJECT(class_name) \
        public: \
            static const TMdbRuntimeObject oRuntimeObject;\
            virtual const char* GetObjectName() const{return #class_name;}\
            virtual const unsigned int GetObjectSize() const {return sizeof(class_name);}\
            virtual const TMdbRuntimeObject* GetRuntimeObject() const{ return &oRuntimeObject;}\
            bool IsKindOf(const TMdbRuntimeObject* pRuntimeObject) const{ return this?pRuntimeObject == GetRuntimeObject():false;}\
            bool IsDerivedFrom(const TMdbRuntimeObject* pRuntimeObject) const{ return this?GetRuntimeObject()->IsDerivedFrom(pRuntimeObject):false;}
        /**
         * @brief ����object������ʵ�֣�����cpp�ļ���
         * 
         */
        #define MDB_ZF_IMPLEMENT_OBJECT(class_name, base_class_name) const /*QuickMDB::*/TMdbRuntimeObject class_name::oRuntimeObject(#class_name, sizeof(class class_name), NULL, &base_class_name::oRuntimeObject);
        #define MDB_ZF_DECLARE_DYNCREATE_OBJECT(class_name) MDB_ZF_DECLARE_OBJECT(class_name)\
                                                        static /*QuickMDB::*/TMdbNtcBaseObject* CreateObject(){ return new class_name;}
        #define MDB_ZF_IMPLEMENT_DYNCREATE_OBJECT(class_name, base_class_name) const /*QuickMDB::*/TMdbRuntimeObject class_name::oRuntimeObject(#class_name, sizeof(class class_name), &class_name::CreateObject, &base_class_name::oRuntimeObject);

        /*
         * @brief �������
         *
         * ��������װ
         */
        class TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcBaseObject);
        public:
            /**
             * @brief ���캯��
             *
             * ��Ĺ��캯��
             */
            TMdbNtcBaseObject();

            /**
             * @brief ��������
             *
             * �����������
             */
            virtual ~TMdbNtcBaseObject();

            /**
             * @brief �����Ա��Ϣ
             *
             * �����Ա��Ϣ
             *
             * @return TMdbNtcStringBuffer
             */
            virtual TMdbNtcStringBuffer ToString() const;
            /**
             * @brief ����Object�õ�hashֵ
             * 
             * @return MDB_UINT64
             * @retval hashֵ
             */
            virtual MDB_UINT64 ToHash() const;
            /**
             * @brief ����֮��ıȽ�
             * ע��˺���Ϊconst,����ʱ��ע��
             * @param pObject [in] ��Ҫ��֮�ȽϵĶ���
             * @return MDB_INT64
             * @retval =0 ���
             * @retval >0 this->data > pObject->data
             * @retval <0 this->data < pObject->data
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const
            {
                return const_cast<TMdbNtcBaseObject*>(this)->Compare(pObject);
            }
            /**
             * @brief ����֮��ıȽ�
             * ע��˺���Ϊ��const,����ʱ��ע��
             * @param pObject [in] ��Ҫ��֮�ȽϵĶ���
             * @return int
             * @retval =0 ���
             * @retval >0 this->data > pObject->data
             * @retval <0 this->data < pObject->data
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject)
            {
                return static_cast<const TMdbNtcBaseObject*>(this)->Compare(pObject);
            }
            /**
             * @brief ����pObject��ֵ
             *
             *  ����pObject��ֵ
             *
             * @return void
             */
            virtual void Assign(const TMdbNtcBaseObject *pObject);
            inline bool GetConstructResult(){return m_bConstructResult;}
            inline void  SetConstructResult(bool bConstructResult)
            {
                m_bConstructResult = bConstructResult;
            }
            
        protected:
            bool    m_bConstructResult;///������
        };

        /*
          * @brief ��ʾ��¼�Ķ������
          *
          * ��¼�������װ
          */
        class TMdbNtcRecordObject : public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcRecordObject);
        };
        /*
          * @brief �����ζ�����
          *
          * �����ζ������װ
          */
        class TMdbNtcLongObject : public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcLongObject);
        public:
            /**
                * @brief ���캯��
                *
                * ��Ĺ��캯��
                */
            TMdbNtcLongObject();

            /**
                * @brief ���캯��
                *
                * ��Ĺ��캯��
                * @param lValue [in] ��value��ֵ
                */
            TMdbNtcLongObject(MDB_INT64 lValue);

            /**
                * @brief ���캯��
                *
                * ��Ĺ��캯��
                *
                * @param right [in] ��value��ֵ
                */
            TMdbNtcLongObject(const TMdbNtcLongObject& right);

            /**
                * @brief ��������
                *
                * �����������
                */
            virtual ~TMdbNtcLongObject();
            
            /**
             * @brief �����Ա��Ϣ
             *
             * �����Ա��Ϣ
             *
             * @return TMdbNtcStringBuffer
             */
            virtual TMdbNtcStringBuffer ToString() const;
            /**
             * @brief ����Object�õ�hashֵ
             * 
             * @return MDB_UINT64
             * @retval hashֵ
             */
            virtual MDB_UINT64 ToHash() const
            {
                return (MDB_UINT64)m_lValue;
            }
            /**
                * @brief ���ö���ֵ
                *
                * ���ö���ֵ
                *
                * @return void
                */
            void SetValue(MDB_INT64 lValue);

            /**
                * @brief ��ö��������ֵ
                *
                * ��ö��������ֵ
                *
                * @return MDB_INT64
                * @retval LongObject�Ķ�������ֵ
                */
            MDB_INT64 GetValue() const;
            /**
             * @brief ����֮��ıȽ�
             *
             * @param pObject [in] ��Ҫ��֮�ȽϵĶ���
             * @return MDB_INT64
             * @retval =0 ��ȣ� >0 ����, <0 С��
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const;

            /**
                * @brief ��������
                *
                * ��������
                *
                * @param pObject [in] ��Դ����
                */
            void Assign(const TMdbNtcBaseObject *pObject);
            
        private:
            MDB_INT64 m_lValue;  ////TLongObject������ֵ

        };

        /*
          * @brief string������
          *
          * string�������װ
          */
        class TMdbNtcStringObject : public TMdbNtcBaseObject, public TMdbNtcStringBuffer
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcStringObject);
        public:
            /**
              * @brief ���캯��
              *
              * ��Ĺ��캯��
              */
            TMdbNtcStringObject();
            /**
             * @brief ���캯��,����ָ����ģʽ��ʽ����
             * 
             * @param iBufferSize [in] buffer�Ĵ�С����������洢��ʽ�����Ļ����С
             * @param pszFormat [in] ��ʽ����
             */
            TMdbNtcStringObject(int iBufferSize, const char* pszFormat = NULL, ...);
            /**
             * @brief ���캯������iRepeat���ַ�cSrc��ֵ����ǰ�ַ���
             * 
             * @param cSrc [in] �ַ�
             * @param iRepeat [in] �ظ�����
             */
            TMdbNtcStringObject(char cSrc, int iRepeat = 1);
            /**
             * @brief ���캯������һ���ַ�����iStart��ʼ��iCount���ַ�������ǰ�ַ���
             * 
             * @param pszStr [in] ������ַ���
             * @param iLength [in] ����,-1��ʾ��ĩβ
             */
            TMdbNtcStringObject(const char* pszStr, int iLength = -1);
            /**
             * @brief �������캯��
             * 
             * @param oStr [in] ������ַ���
             */
            TMdbNtcStringObject(const TMdbNtcStringObject& oStr);
            /**
             * @brief �ʺ�TString��TMdbNtcStringBuffer����Ŀ������캯��
             * 
             * @param oStr [in] ������ַ���
             */
            TMdbNtcStringObject(const TMdbNtcString& oStr);
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param cSrc [in] �ַ�
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcStringObject& operator = (char cSrc)
            {
                TMdbNtcStringBuffer::Assign(cSrc, 1);
                return *this;
            }
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param pszSrc [in] ������ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcStringObject& operator = (const char* pszSrc)
            {
                TMdbNtcStringBuffer::Assign(pszSrc);
                return *this;
            }
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param oStr [in] ������ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcStringObject& operator = (const TMdbNtcString& oStr)
            {
                TMdbNtcStringBuffer::Assign(oStr);
                return *this;
            }
            /**
             * @brief �����Ա��Ϣ
             *
             * �����Ա��Ϣ
             *
             * @return TMdbNtcString
             */
            virtual TMdbNtcStringBuffer ToString() const
            {
                return *this;
            }
            /**
             * @brief ����Object�õ�hashֵ
             * 
             * @return MDB_UINT64
             * @retval hashֵ
             */
            virtual MDB_UINT64 ToHash() const
            {
                return MdbNtcHashFunc(m_pszBuffer?m_pszBuffer:"");
            }
            using TMdbNtcStringBuffer::Compare;
            using TMdbNtcStringBuffer::Assign;
            /**
             * @brief ����֮��ıȽ�
             *
             * @param pObject [in] ��Ҫ��֮�ȽϵĶ���
             * @return MDB_INT64
             * @retval =0 ��ȣ� >0 ����, <0 С��
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const;
        };
//    }
//}
#endif
