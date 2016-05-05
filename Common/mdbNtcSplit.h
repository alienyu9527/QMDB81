/**
 * @file Split.hxx
 * @brief 字符串分割类的封装
 *
 * 字符串分割类的封装
 *
 * @author 技术框架小组
 * @version 1.0
 * @date 20121214
 * @warning
 */

#ifndef _MDB_NTC_H_Split_
#define _MDB_NTC_H_Split_
#include "Common/mdbCommons.h"
#include "Common/mdbBaseObject.h"
#include "Common/mdbDataStructs.h"
//namespace QuickMDB
//{
        /**
         * @brief 字符串分割类的封装
         *
         *  字符串分割类的封装
         *
         */
        class TMdbNtcSplit : public TMdbNtcBaseObject
        {
            /** \example  example_TSplit.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcSplit);

        public:
            /**
              * @brief 构造函数
              *
              */
            TMdbNtcSplit();

            /**
              * @brief 析构函数
              *
              */
            virtual ~TMdbNtcSplit();

            class TSplitField
            {
            public:
                TSplitField()
                {
                    m_uiFieldSize = 0;
                    m_ppFieldValue = NULL;
                    m_piFieldValueLength = NULL;
                    m_uiFieldCapacity = 256;
                }
                ~TSplitField()
                {
                    m_uiFieldSize = 0;
                    if( NULL !=  m_ppFieldValue )
                    {
                        delete [] m_ppFieldValue;
                        m_ppFieldValue = NULL;
                    }
                    if( NULL != m_piFieldValueLength )
                    {
                    	delete [] m_piFieldValueLength;
                    	m_piFieldValueLength = NULL;
                    }
                    m_uiFieldCapacity = 0;
                }
            public:
                unsigned int m_uiFieldSize;     ///分割后的字段数
                char **m_ppFieldValue;          ///分割后的字段值
                int *m_piFieldValueLength;		///分割后的字段值的长度
                unsigned int m_uiFieldCapacity;	///m_ppFieldValue的容量空间
            };

        public:
            /**
              * @brief 分割字符串
              * @param sSplitString [in] 待分割的字符串
              * @param cSplitChar [in] 分割符
              * @param bSkipEmptyField [in] 是否跳过空的子字段
              * @retval  bool true成功/false失败
              */
			bool SplitString(const char *sSplitString, const char cSplitChar, bool bSkipEmptyField = false);

            /**
              * @brief 分割字符串
              * @param sSplitString [in] 待分割的字符串
              * @param sSplitSubString [in] 分割字符串
              * @param bSkipEmptyField [in] 是否跳过空的子字段
              * @retval  bool true成功/false失败
              */
			bool SplitString(const char *sSplitString, const char *sSplitSubString, bool bSkipEmptyField = false);

			/**
			  *
			  * @brief 分割数据流
			  * @param sSplitString [in] 待分割的数据流
			  * @param iLength [in] 待分割的长度
			  * @param cSplitChar [in] 分割符
			  * @param bSkipEmptyField [in] 是否跳过空的子字段
			  * @param bReplaceSplitChar [in] 是否把分隔符替换成'\0'
			  * @retval  bool true成功/false 失败
			  */
		    bool SplitData(const char *sSplitString, int iLength, const char cSplitChar, bool bSkipEmptyField = false, bool bReplaceSplitChar = true);

            /**
              * @brief 得到分割后的字段数
              * @retval  unsigned int  分割后的字段数
              */
            unsigned int GetFieldCount();

            /**
              * @brief 根据索引号得到字段值
              * @param  iIndex字段索引
              * @retval const char * 字段索引对应的字段值
              */
            const char * Field(MDB_UINT32 iIndex);

            /**
              * @brief 根据数组下标得到字段值
              * @param iIndex [in] 数组下标
              * @retval const char *  数组下标对应的字段值
              */
            const char * operator [](MDB_UINT32 iIndex);

            /**
              * @brief 根据数组下标得到字段值的长度（主要提供给数据流使用）
              * @param iIndex [in] 数组下标
              * @retval int 数组下标对应的字段值的长度
              */
			int GetFieldValueLength(MDB_UINT32 iIndex);

        private:
            /**
             * @brief 增长空间函数
             * @param iLength [in] 申请空间的大小
             * @retval  bool true成功/false失败
             */
            bool GrowBuffer( int iLength );

            /**
             * @brief 为m_ppFieldValue 增长空间函数
             * @retval  bool true成功/false失败
             */
            bool GrowSplitField( );

        private:
            char *m_pBuffer;                ///存放待分割的字符串
            unsigned int m_uiBufferLength;  ///m_pBuffer的长度
            char m_cSplitChar;              ///分割符
            TSplitField m_oTSplitField;     ///分割字段对象
        };

//        /**
//         * @brief 字符串分割类
//         * 分割时，并不分配每块内存，而是只记录位置，获取指定序号分割字段时，再SubStr得到
//         */
//        class TQuickSplit
//        {
//        public:
//            class TFieldPos
//            {
//            public:
//                int iStart;
//                int iEnd;
//            };
//            /**
//              * @brief 构造函数
//              *
//              */
//            TQuickSplit();
//            /**
//              * @brief 析构函数
//              *
//              */
//            ~TQuickSplit();
//            /**
//             * @brief 分隔字符串
//             *
//             * @param pszBuffer [in] 要分割的字符串
//             * @param pszBuffer [in] 字符串长度，-1表示字符串是以'\0'结尾
//             * @param cDelimiter [in] 分隔符
//             * @param bSkipEmptyField [in] 是否跳过空的子字段
//             * @return int
//             * @retval 字段数
//             */
//            int Split(const char* pszBuffer, int iLength, char cDelimiter, bool bSkipEmptyField = false);
//            /**
//             * @brief 获得分割得到的字段数
//             *
//             * @return int
//             */
//            inline int GetFieldCount()
//            {
//                return m_iFieldCount;
//            }
//            /**
//             * @brief 获得指定的字段
//             *
//             * @param uiIndex [in] 第几个字段
//             * @return TMdbNtcStringBuffer
//             * @retval 字段的取值
//             */
//            TMdbNtcStringBuffer Field(unsigned int uiIndex);
//            /**
//              * @brief 根据数组下标得到字段值
//              * @param iIndex [in] 数组下标
//              * @retval TMdbNtcStringBuffer  数组下表对应的字段值
//              */
//            inline TMdbNtcStringBuffer operator[](unsigned int uiIndex)
//            {
//                return Field(uiIndex);
//            }
//            /**
//             * @brief 得到字段的位置信息
//             *
//             * @param uiIndex [in] 第几个字段
//             * @return TFieldPos*
//             * @retval NULL 表示没有找到此字段
//             */
//            TFieldPos* FieldPos(unsigned int uiIndex);
//        protected:
//            TFieldPos*      m_pFieldPosArray;
//            int             m_iFieldArraySize;
//            int             m_iFieldCount;
//            const char*     m_pszBuffer;       ///缓冲区指针
//            char            m_cDelimiter;      ///分隔符
//        };
//}
#endif
