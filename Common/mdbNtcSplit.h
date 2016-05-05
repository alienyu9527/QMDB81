/**
 * @file Split.hxx
 * @brief �ַ����ָ���ķ�װ
 *
 * �ַ����ָ���ķ�װ
 *
 * @author �������С��
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
         * @brief �ַ����ָ���ķ�װ
         *
         *  �ַ����ָ���ķ�װ
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
              * @brief ���캯��
              *
              */
            TMdbNtcSplit();

            /**
              * @brief ��������
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
                unsigned int m_uiFieldSize;     ///�ָ����ֶ���
                char **m_ppFieldValue;          ///�ָ����ֶ�ֵ
                int *m_piFieldValueLength;		///�ָ����ֶ�ֵ�ĳ���
                unsigned int m_uiFieldCapacity;	///m_ppFieldValue�������ռ�
            };

        public:
            /**
              * @brief �ָ��ַ���
              * @param sSplitString [in] ���ָ���ַ���
              * @param cSplitChar [in] �ָ��
              * @param bSkipEmptyField [in] �Ƿ������յ����ֶ�
              * @retval  bool true�ɹ�/falseʧ��
              */
			bool SplitString(const char *sSplitString, const char cSplitChar, bool bSkipEmptyField = false);

            /**
              * @brief �ָ��ַ���
              * @param sSplitString [in] ���ָ���ַ���
              * @param sSplitSubString [in] �ָ��ַ���
              * @param bSkipEmptyField [in] �Ƿ������յ����ֶ�
              * @retval  bool true�ɹ�/falseʧ��
              */
			bool SplitString(const char *sSplitString, const char *sSplitSubString, bool bSkipEmptyField = false);

			/**
			  *
			  * @brief �ָ�������
			  * @param sSplitString [in] ���ָ��������
			  * @param iLength [in] ���ָ�ĳ���
			  * @param cSplitChar [in] �ָ��
			  * @param bSkipEmptyField [in] �Ƿ������յ����ֶ�
			  * @param bReplaceSplitChar [in] �Ƿ�ѷָ����滻��'\0'
			  * @retval  bool true�ɹ�/false ʧ��
			  */
		    bool SplitData(const char *sSplitString, int iLength, const char cSplitChar, bool bSkipEmptyField = false, bool bReplaceSplitChar = true);

            /**
              * @brief �õ��ָ����ֶ���
              * @retval  unsigned int  �ָ����ֶ���
              */
            unsigned int GetFieldCount();

            /**
              * @brief ���������ŵõ��ֶ�ֵ
              * @param  iIndex�ֶ�����
              * @retval const char * �ֶ�������Ӧ���ֶ�ֵ
              */
            const char * Field(MDB_UINT32 iIndex);

            /**
              * @brief ���������±�õ��ֶ�ֵ
              * @param iIndex [in] �����±�
              * @retval const char *  �����±��Ӧ���ֶ�ֵ
              */
            const char * operator [](MDB_UINT32 iIndex);

            /**
              * @brief ���������±�õ��ֶ�ֵ�ĳ��ȣ���Ҫ�ṩ��������ʹ�ã�
              * @param iIndex [in] �����±�
              * @retval int �����±��Ӧ���ֶ�ֵ�ĳ���
              */
			int GetFieldValueLength(MDB_UINT32 iIndex);

        private:
            /**
             * @brief �����ռ亯��
             * @param iLength [in] ����ռ�Ĵ�С
             * @retval  bool true�ɹ�/falseʧ��
             */
            bool GrowBuffer( int iLength );

            /**
             * @brief Ϊm_ppFieldValue �����ռ亯��
             * @retval  bool true�ɹ�/falseʧ��
             */
            bool GrowSplitField( );

        private:
            char *m_pBuffer;                ///��Ŵ��ָ���ַ���
            unsigned int m_uiBufferLength;  ///m_pBuffer�ĳ���
            char m_cSplitChar;              ///�ָ��
            TSplitField m_oTSplitField;     ///�ָ��ֶζ���
        };

//        /**
//         * @brief �ַ����ָ���
//         * �ָ�ʱ����������ÿ���ڴ棬����ֻ��¼λ�ã���ȡָ����ŷָ��ֶ�ʱ����SubStr�õ�
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
//              * @brief ���캯��
//              *
//              */
//            TQuickSplit();
//            /**
//              * @brief ��������
//              *
//              */
//            ~TQuickSplit();
//            /**
//             * @brief �ָ��ַ���
//             *
//             * @param pszBuffer [in] Ҫ�ָ���ַ���
//             * @param pszBuffer [in] �ַ������ȣ�-1��ʾ�ַ�������'\0'��β
//             * @param cDelimiter [in] �ָ���
//             * @param bSkipEmptyField [in] �Ƿ������յ����ֶ�
//             * @return int
//             * @retval �ֶ���
//             */
//            int Split(const char* pszBuffer, int iLength, char cDelimiter, bool bSkipEmptyField = false);
//            /**
//             * @brief ��÷ָ�õ����ֶ���
//             *
//             * @return int
//             */
//            inline int GetFieldCount()
//            {
//                return m_iFieldCount;
//            }
//            /**
//             * @brief ���ָ�����ֶ�
//             *
//             * @param uiIndex [in] �ڼ����ֶ�
//             * @return TMdbNtcStringBuffer
//             * @retval �ֶε�ȡֵ
//             */
//            TMdbNtcStringBuffer Field(unsigned int uiIndex);
//            /**
//              * @brief ���������±�õ��ֶ�ֵ
//              * @param iIndex [in] �����±�
//              * @retval TMdbNtcStringBuffer  �����±��Ӧ���ֶ�ֵ
//              */
//            inline TMdbNtcStringBuffer operator[](unsigned int uiIndex)
//            {
//                return Field(uiIndex);
//            }
//            /**
//             * @brief �õ��ֶε�λ����Ϣ
//             *
//             * @param uiIndex [in] �ڼ����ֶ�
//             * @return TFieldPos*
//             * @retval NULL ��ʾû���ҵ����ֶ�
//             */
//            TFieldPos* FieldPos(unsigned int uiIndex);
//        protected:
//            TFieldPos*      m_pFieldPosArray;
//            int             m_iFieldArraySize;
//            int             m_iFieldCount;
//            const char*     m_pszBuffer;       ///������ָ��
//            char            m_cDelimiter;      ///�ָ���
//        };
//}
#endif
