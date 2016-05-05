/**
 * @file mdbBaseObject.h
 * @brief 对象基类的封装
 *
 * 对象基类的封装
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
         * @brief hash函数，根据字符串算出key
         * 
         * @param pszKey [in] hash key字符串
         * @return MDB_UINT32
         * @retval hash key
         */
        MDB_UINT32 MdbNtcHashFunc(const char* pszKey);

        class TMdbNtcBaseObject;
        /**
         * @brief 定义运行时类型
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
             * @brief 是否派生于某个类型
             * 
             * @param pRuntimeObject [in] 传入类似ZF_RUNTIME_OBJECT(TLongObjct)
             * @return bool
             * @retval true 是派生于指定的类型
             */
            bool IsDerivedFrom(const TMdbRuntimeObject* pRuntimeObject) const;
        protected:
            const char* m_pszClassName;///< 类型名称
            const TMdbRuntimeObject* m_pBaseRuntimeObject;///< 基类的运行时类型
            const unsigned int m_iObjectSize;
            TMdbNtcBaseObject* (*m_pfnCreateObject)();
        };
        /**
         * @brief 用于得到指定object，用于比RTTI更快速的类型判断
         * 和GetObjectType函数联合使用，可动态判断派生类对象是否为指定类对象（非RTTI机制）
         * 用法pLongObject->GetObjectType()==MDB_ZF_RUNTIME_OBJECT(TMdbNtcLongObject)
         * 或者pLongObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcLongObject))
         */
        #define MDB_ZF_RUNTIME_OBJECT(class_name) (&class_name::oRuntimeObject)
        #define MDB_ZF_DYNAMIC_CAST(class_name, x) ((((/*QuickMDB::*/TMdbNtcBaseObject*)x)->IsDerivedFrom(MDB_ZF_RUNTIME_OBJECT(class_name)))?static_cast<class_name *>(x):NULL)
        
        /*
         * @brief 用于提供类名和类大小的获取，实际对象受类中指针因素，占用内存会比实际类大小更大
         * 同时提供了类型判断的方法pLongObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcLongObject))
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
         * @brief 对于object声明的实现，放在cpp文件中
         * 
         */
        #define MDB_ZF_IMPLEMENT_OBJECT(class_name, base_class_name) const /*QuickMDB::*/TMdbRuntimeObject class_name::oRuntimeObject(#class_name, sizeof(class class_name), NULL, &base_class_name::oRuntimeObject);
        #define MDB_ZF_DECLARE_DYNCREATE_OBJECT(class_name) MDB_ZF_DECLARE_OBJECT(class_name)\
                                                        static /*QuickMDB::*/TMdbNtcBaseObject* CreateObject(){ return new class_name;}
        #define MDB_ZF_IMPLEMENT_DYNCREATE_OBJECT(class_name, base_class_name) const /*QuickMDB::*/TMdbRuntimeObject class_name::oRuntimeObject(#class_name, sizeof(class class_name), &class_name::CreateObject, &base_class_name::oRuntimeObject);

        /*
         * @brief 对象基类
         *
         * 对象基类封装
         */
        class TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcBaseObject);
        public:
            /**
             * @brief 构造函数
             *
             * 类的构造函数
             */
            TMdbNtcBaseObject();

            /**
             * @brief 析构函数
             *
             * 类的析构函数
             */
            virtual ~TMdbNtcBaseObject();

            /**
             * @brief 输出成员信息
             *
             * 输出成员信息
             *
             * @return TMdbNtcStringBuffer
             */
            virtual TMdbNtcStringBuffer ToString() const;
            /**
             * @brief 根据Object得到hash值
             * 
             * @return MDB_UINT64
             * @retval hash值
             */
            virtual MDB_UINT64 ToHash() const;
            /**
             * @brief 对象之间的比较
             * 注意此函数为const,重载时需注意
             * @param pObject [in] 需要与之比较的对象
             * @return MDB_INT64
             * @retval =0 相等
             * @retval >0 this->data > pObject->data
             * @retval <0 this->data < pObject->data
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const
            {
                return const_cast<TMdbNtcBaseObject*>(this)->Compare(pObject);
            }
            /**
             * @brief 对象之间的比较
             * 注意此函数为非const,重载时需注意
             * @param pObject [in] 需要与之比较的对象
             * @return int
             * @retval =0 相等
             * @retval >0 this->data > pObject->data
             * @retval <0 this->data < pObject->data
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject)
            {
                return static_cast<const TMdbNtcBaseObject*>(this)->Compare(pObject);
            }
            /**
             * @brief 拷贝pObject的值
             *
             *  拷贝pObject的值
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
            bool    m_bConstructResult;///构造结果
        };

        /*
          * @brief 表示记录的对象基类
          *
          * 记录对象类封装
          */
        class TMdbNtcRecordObject : public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcRecordObject);
        };
        /*
          * @brief 长整形对象类
          *
          * 长整形对象类封装
          */
        class TMdbNtcLongObject : public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcLongObject);
        public:
            /**
                * @brief 构造函数
                *
                * 类的构造函数
                */
            TMdbNtcLongObject();

            /**
                * @brief 构造函数
                *
                * 类的构造函数
                * @param lValue [in] 给value的值
                */
            TMdbNtcLongObject(MDB_INT64 lValue);

            /**
                * @brief 构造函数
                *
                * 类的构造函数
                *
                * @param right [in] 给value的值
                */
            TMdbNtcLongObject(const TMdbNtcLongObject& right);

            /**
                * @brief 析构函数
                *
                * 类的析构函数
                */
            virtual ~TMdbNtcLongObject();
            
            /**
             * @brief 输出成员信息
             *
             * 输出成员信息
             *
             * @return TMdbNtcStringBuffer
             */
            virtual TMdbNtcStringBuffer ToString() const;
            /**
             * @brief 根据Object得到hash值
             * 
             * @return MDB_UINT64
             * @retval hash值
             */
            virtual MDB_UINT64 ToHash() const
            {
                return (MDB_UINT64)m_lValue;
            }
            /**
                * @brief 设置对象值
                *
                * 设置对象值
                *
                * @return void
                */
            void SetValue(MDB_INT64 lValue);

            /**
                * @brief 获得对象的属性值
                *
                * 获得对象的属性值
                *
                * @return MDB_INT64
                * @retval LongObject的对象属性值
                */
            MDB_INT64 GetValue() const;
            /**
             * @brief 对象之间的比较
             *
             * @param pObject [in] 需要与之比较的对象
             * @return MDB_INT64
             * @retval =0 相等， >0 大于, <0 小于
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const;

            /**
                * @brief 拷贝对象
                *
                * 拷贝对象
                *
                * @param pObject [in] 来源对象
                */
            void Assign(const TMdbNtcBaseObject *pObject);
            
        private:
            MDB_INT64 m_lValue;  ////TLongObject的属性值

        };

        /*
          * @brief string对象类
          *
          * string对象类封装
          */
        class TMdbNtcStringObject : public TMdbNtcBaseObject, public TMdbNtcStringBuffer
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcStringObject);
        public:
            /**
              * @brief 构造函数
              *
              * 类的构造函数
              */
            TMdbNtcStringObject();
            /**
             * @brief 构造函数,按照指定的模式格式化串
             * 
             * @param iBufferSize [in] buffer的大小，用于申请存储格式化串的缓存大小
             * @param pszFormat [in] 格式化串
             */
            TMdbNtcStringObject(int iBufferSize, const char* pszFormat = NULL, ...);
            /**
             * @brief 构造函数，用iRepeat个字符cSrc赋值给当前字符串
             * 
             * @param cSrc [in] 字符
             * @param iRepeat [in] 重复次数
             */
            TMdbNtcStringObject(char cSrc, int iRepeat = 1);
            /**
             * @brief 构造函数，将一个字符串从iStart开始的iCount个字符赋给当前字符串
             * 
             * @param pszStr [in] 构造的字符串
             * @param iLength [in] 长度,-1表示到末尾
             */
            TMdbNtcStringObject(const char* pszStr, int iLength = -1);
            /**
             * @brief 拷贝构造函数
             * 
             * @param oStr [in] 构造的字符串
             */
            TMdbNtcStringObject(const TMdbNtcStringObject& oStr);
            /**
             * @brief 适合TString和TMdbNtcStringBuffer对象的拷贝构造函数
             * 
             * @param oStr [in] 构造的字符串
             */
            TMdbNtcStringObject(const TMdbNtcString& oStr);
            /**
             * @brief 重载赋值运算符
             * 
             * @param cSrc [in] 字符
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcStringObject& operator = (char cSrc)
            {
                TMdbNtcStringBuffer::Assign(cSrc, 1);
                return *this;
            }
            /**
             * @brief 重载赋值运算符
             * 
             * @param pszSrc [in] 构造的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcStringObject& operator = (const char* pszSrc)
            {
                TMdbNtcStringBuffer::Assign(pszSrc);
                return *this;
            }
            /**
             * @brief 重载赋值运算符
             * 
             * @param oStr [in] 构造的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcStringObject& operator = (const TMdbNtcString& oStr)
            {
                TMdbNtcStringBuffer::Assign(oStr);
                return *this;
            }
            /**
             * @brief 输出成员信息
             *
             * 输出成员信息
             *
             * @return TMdbNtcString
             */
            virtual TMdbNtcStringBuffer ToString() const
            {
                return *this;
            }
            /**
             * @brief 根据Object得到hash值
             * 
             * @return MDB_UINT64
             * @retval hash值
             */
            virtual MDB_UINT64 ToHash() const
            {
                return MdbNtcHashFunc(m_pszBuffer?m_pszBuffer:"");
            }
            using TMdbNtcStringBuffer::Compare;
            using TMdbNtcStringBuffer::Assign;
            /**
             * @brief 对象之间的比较
             *
             * @param pObject [in] 需要与之比较的对象
             * @return MDB_INT64
             * @retval =0 相等， >0 大于, <0 小于
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const;
        };
//    }
//}
#endif
