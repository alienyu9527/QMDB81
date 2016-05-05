/**
 * @file mdbStrings.h
 * @brief �ַ���������Լ�TStringObject��
 *
 * �ַ���������Լ�TStringObject��
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121214
 * @warning
 */

#ifndef _MDB_NTC_H_Strings_
#define _MDB_NTC_H_Strings_
#include <stdarg.h>
#include "Common/mdbCommons.h"
//namespace QuickMDB
//{
        class TMdbNtcStringBuffer;
        class TMdbNtcThreadSpinLock;
        /**
         * @brief �ַ���TString��Ķ���
         *
         *  �ַ���TString��Ķ���
         *
         */
        class TMdbNtcString
        {
            /** \example  example_TString.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
        protected:            
            /**
             * @brief string��һЩ������Ϣ
             * 
             */
            struct TStringData
            {
                TMdbNtcThreadSpinLock*      pSpinlock;///< ������
                unsigned int    uiAllocLength;///< ������ĳ���
                unsigned int    uiLength; ///< �ַ�������
                unsigned int    uiRefcnt;///<���ø��� 
                TStringData(unsigned int uiAllocLength = 0,
                    unsigned int uiLength = 0, bool bWithLock = true);
                ~TStringData();
                int AddRef();
                /**
                 * @brief ������ã��������󣬷������ù�0�����ͷ�buffer
                 * 
                 * @param bDelLock [in] �Ƿ��ͷ�buffer��ͬʱɾ����
                 * @return int
                 * @retval �˴ν��������ü������ӷ���ֵ�����ж��Ƿ����ɱ����ͷ�buffer��(���Ϊ0)
                 */
                unsigned int Release(bool bDelLock = true);
                inline char* GetBuffer()
                {
                    return (char *)(this + 1);
                }
            };
        public:
            TMdbNtcString();
            /**
             * @brief ���캯��,����ָ����ģʽ��ʽ����
             * 
             * @param iBufferSize [in] buffer�Ĵ�С
             * @param pszFormat [in] ��ʽ����
             */
            TMdbNtcString(int iBufferSize, const char* pszFormat = NULL, ...);
            /**
             * @brief ���캯������iRepeat���ַ�cSrc��ֵ����ǰ�ַ���
             * 
             * @param cSrc [in] �ַ�
             * @param iRepeat [in] �ظ�����
             */
            TMdbNtcString(char cSrc, int iRepeat = 1);
            /**
             * @brief ���캯������һ���ַ���������ǰ�ַ���
             * 
             * @param pszStr [in] ������ַ���     
             * @param iLen [in] ��pszStr�п������ַ�����,-1��ʾ��ĩβ
             */
            TMdbNtcString(const char* pszStr, int iLen = -1);
            /**
             * @brief �������캯��
             * 
             * @param oStr [in] ������ַ���
             */
            TMdbNtcString(const TMdbNtcString& oStr);

            /**
              * @brief ��������
              */
            ~TMdbNtcString();
            
            /**
             * @brief ��ֵ��������iRepeat���ַ�cSrc��ֵ����ǰ�ַ���
             * 
             * @param cSrc [in] �ַ�
             * @param iRepeat [in] �ظ�����
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& Assign(char cSrc, int iRepeat = 1);
            /**
             * @brief ��ֵ��������һ���ַ���������ǰ�ַ���
             * 
             * @param pszStr [in] ������ַ���
             * @param iLen [in] ��Ŀ,-1��ʾ��ĩβ
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& Assign(const char* pszStr, int iLen = -1);
            /**
             * @brief �������캯��
             * 
             * @param oStr [in] ������ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& Assign(const TMdbNtcString& oStr);
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param cSrc [in] �ַ�
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcString& operator = (char cSrc)
            {
                return Assign(cSrc, 1);
            }
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param pszSrc [in] ������ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcString& operator = (const char* pszSrc)
            {
                return Assign(pszSrc);
            }
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param oStr [in] ������ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcString& operator = (const TMdbNtcString& oStr)
            {
                return Assign(oStr);
            }            
            /**
             * @brief ����ַ���
             * 
             * @return TMdbNtcString
             * @retval ��������
             */
            TMdbNtcString& Clear();

            /**
             * @brief Ԥ������
             * @param uiReserveSize [in] Ԥ����С
             */
            char*  Reserve(MDB_UINT32 uiReserveSize);
            
            /**
             * @brief ����ַ���,���ͷ��ڴ�
             * 
             */
            void Release();
            /**
             * @brief ͨ�������[]���ָ��λ�õ��ַ�
             * 
             * @param uiIndex [in] �±�λ��
             * @return char
            */    
            inline char operator [](unsigned int uiIndex) const
            {
                if(m_pszBuffer == NULL) return '\0';
                else if( uiIndex > GetLength() - 1 ) return '\0';
                else return *(m_pszBuffer+uiIndex);
            }

            inline char& operator [](unsigned int uiIndex)
            {
                MDB_NTC_ZF_ASSERT(m_pszBuffer);
                MDB_NTC_ZF_ASSERT( uiIndex <= GetLength() - 1 );
                return *(m_pszBuffer+uiIndex);
            }
            /**
             * @brief ���ָ��λ�õ��ַ�
             * 
             * @param uiIndex [in] �±�λ��
             * @return char
             */
            inline char at(unsigned int uiIndex) const
            {
                if(m_pszBuffer == NULL) return '\0';
                else if( uiIndex > GetLength() - 1 ) return '\0';
                else return *(m_pszBuffer+uiIndex);
            }

            inline char& at(unsigned int uiIndex)
            {
                MDB_NTC_ZF_ASSERT(m_pszBuffer);
                MDB_NTC_ZF_ASSERT( uiIndex <= GetLength() - 1 );
                return *(m_pszBuffer+uiIndex);
            }

            /**
             * @brief ��ĩβ׷���ַ�
             * 
             * @param pszStr [in] ��Ҫ��ӵ��ַ�
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcString& operator += (char cStr)
            {
                return Append(cStr);
            }
            /**
             * @brief ��ĩβ׷���ַ���
             * 
             * @param pszStr [in] ��Ҫ��ӵ��ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcString& operator += (const char* pszStr)
            {
                return Append(pszStr);
            }
            /**
             * @brief ��ĩβ׷���ַ���
             * 
             * @param oStr [in] ��Ҫ��ӵ��ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcString& operator += (const TMdbNtcString& oStr)
            {
                return Append(oStr);
            }
            /**
             * @brief ��ĩβ׷���ַ���
             * 
             * @param cTarget [in] ��Ҫ׷�ӵ��ַ�
             * @param iRepeat [in] �ظ�����
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& Append(char cTarget, int iRepeat = 1);
            /**
             * @brief ��ĩβ׷���ַ���
             * 
             * @param pszStr [in] ��Ҫ��ӵ��ַ���
             * @param iLen [in] ��Ŀ,-1��ʾ��ĩβ
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& Append(const char* pszStr, int iLen = -1);
            /**
             * @brief ��ĩβ׷���ַ���
             * 
             * @param oStr [in] ��Ҫ��ӵ��ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& Append(const TMdbNtcString& oStr);
            /**
             * @brief �����ַ�
             * 
             * @param iIndex [in] �����λ�ã�-1��ʾĩβ
             * @param cTarget [in] Ҫ������ַ�
             * @param iRepeat [in] �ظ�����
             * @return int
             * @retval ���ַ����ڵ�λ��
             */
            int Insert(int iIndex, char cTarget, int iRepeat = 1);
            /**
             * @brief �����ַ���
             * 
             * @param iIndex [in] �����λ�ã�-1��ʾĩβ
             * @param pszTarget [in] Ҫ������ַ���
             * @param iLen [in] ������ַ���Ŀ,-1��ʾ��pszTargetĩβ
             * @return int
             * @retval ���ַ������ڵ�λ��
             */
            int Insert(int iIndex, const char* pszTarget, int iLen = -1);
            /**
             * @brief �����ַ���
             * 
             * @param iIndex [in] �����λ�ã�-1��ʾĩβ
             * @param oStr [in] ��Ҫ��ӵ��ַ���
             * @return int
             * @retval ���ַ������ڵ�λ��
             */
            int Insert(int iIndex, const TMdbNtcString& oStr);
            /**
             * @brief �Ƴ�ָ�����ַ�
             * 
             * @param cTarget [in] ��Ҫ�Ƴ����ַ�
             * @return int
             * @retval ɾ�����ַ�����
             */
            int Remove(char cTarget);
            /**
             * @brief ɾ��ָ����Ŀ���ַ�
             * 
             * @param iStart [in] ɾ������ʼλ��
             * @param iCount [in] ɾ�����ַ���,-1��ʾɾ����ĩβ
             * @return int
             * @retval ʵ��ɾ�����ַ�����
             */
            int Delete(int iStart, int iCount = 1);
            /**
             * @brief ����ָ����ģʽ��ʽ����
             * 
             * @param iBufferSize [in] buffer�Ĵ�С����������洢��ʽ�����Ļ����С
             * @param pszFormat [in] ��ʽ����
             */
            void Snprintf(int iBufferSize, const char* pszFormat, ...);
            /**
             * @brief ����ָ����ģʽ��ʽ����
             * 
             * @param iBufferSize [in] buffer�Ĵ�С����������洢��ʽ�����Ļ����С
             * @param pszFormat [in] ��ʽ����
             */
            void vSnprintf(int iBufferSize, const char* pszFormat, va_list ap);
            /**
             * @brief ��ȡ�Ӵ�
             * 
             * @param iStart [in] ��ʼλ��
             * @param iCount [in] ��Ҫȡ���Ӵ�����,-1��ʾ����β
             * @return TMdbNtcStringBuffer
             * @retval �Ӵ�
             */
            TMdbNtcStringBuffer Substr(int iStart, int iCount = -1) const;
            /**
             * @brief �ַ�����ȫ�Ƚϣ���strcmp����
             * 
             * @param pszTarget [in] Ŀ���ַ���
             * @param bCase [in] �Ƿ����ִ�Сд��Ĭ������
             * @return int
             * @retval =0 ���߳���������ַ�����ͬ
             * @retval >0 �������pszTarget
             * @retval <0 ����С��pszTarget
             */
            inline int Compare(const char* pszTarget, bool bCase = true) const
            {
                if(pszTarget == NULL) pszTarget = "";
                if(m_pszBuffer) return bCase?strcmp(m_pszBuffer, pszTarget):mdb_ntc_stricmp(m_pszBuffer, pszTarget);
                else if(*pszTarget == '\0') return 0;
                else return -1;
            }
            /**
             * @brief �ַ�����ȫ�Ƚ�
             * 
             * @param oStr [in] ��Ҫ�Ƚϵ��ַ���
             * @param bCase [in] �Ƿ����ִ�Сд��Ĭ������
             * @return int
             * @retval =0 ���߳���������ַ�����ͬ
             * @retval >0 �������oStr
             * @retval <0 ����С��oStr
             */
            inline int Compare(const TMdbNtcString& oStr, bool bCase = true) const
            {
                if(m_pszBuffer == oStr.m_pszBuffer) return 0;
                else return Compare(oStr.c_str(), bCase);
            }
            /**
             * @brief �Ƿ��ַ����Ƿ���ָ����Ϊǰ׺����Щ����strstr��������ǰ׺ƥ��
             * 
             * @param pszPrefix [in] ǰ׺
             * @param bCase     [in] �Ƿ����ִ�Сд��Ĭ������
             * @return const char*
             * @retval NULL   ��ʾ�����Դ�Ϊǰ׺
             * @retval ��NULL ��ʾƥ�䣬����ƫ��Prefix���ָ��
             */
            const char* StrPrefix(const char* pszPrefix, bool bCase = true) const;
            /**
             * @brief �Ƿ��ַ����Ƿ���ָ����Ϊ��׺
             * 
             * @param pszSuffix [in] ��׺
             * @param bCase     [in] �Ƿ����ִ�Сд��Ĭ������
             * @return const char*
             * @retval NULL   ��ʾ�����Դ�Ϊ��׺
             * @retval ��NULL ��ʾƥ�䣬���ؿ�ʼƥ���λ��
             */
            const char* StrSuffix(const char* pszSuffix, bool bCase = true) const;
            /**
             * @brief ת��Ϊint
             * 
             * @return int
             */
            int ToInt();
            /**
             * @brief ת��Ϊint64
             * 
             * @return MDB_INT64
             */
            MDB_INT64 ToInt64();
            /**
             * @brief ת��Ϊ˫����
             * 
             * @param iPrecision [in] ��ʾ�����Ƕ��٣�������λС������-1��ʾ��������С��
             * @param bRounding [in] ��ʾ�Ƿ��������룬Ĭ��Ϊtrue
             * @return double
             */
            double ToDouble(int iPrecision = -1, bool bRounding = true);
            /**
             * @brief ת��ΪСд��ĸ
             * 
             */
            TMdbNtcString& ToLower();
            /**
             * @brief ת��Ϊ��д��ĸ
             * 
             */
            TMdbNtcString& ToUpper();
            /**
             * @brief ȥ���ұ�ָ�����ַ�
             * 
             * @param cTarget [in] ָ�����ַ�
             */
            TMdbNtcString& TrimRight(char cTarget);
            /**
             * @brief ȥ�����ָ�����ַ�
             * 
             * @param cTarget [in] ָ�����ַ�
             */
            TMdbNtcString& TrimLeft(char cTarget);
            /**
             * @brief ȥ������ָ�����ַ�
             * 
             * @param cTarget [in] ָ�����ַ�
             */
            TMdbNtcString& Trim(char cTarget);
            /**
             * @brief ȥ���ұ�ָ����һЩ�ַ�(ֻҪ���������κ�һ���ַ�), Ĭ����ȥ���հ�
             * 
             * @param pszTarget [in] ָ�����ַ���
             */
            TMdbNtcString& TrimRight(const char* pszTarget = " \t");
            /**
             * @brief ȥ�����ָ����һЩ�ַ�(ֻҪ���������κ�һ���ַ�), Ĭ����ȥ���հ�
             * 
             * @param pszTarget [in] ָ�����ַ���
             */
            TMdbNtcString& TrimLeft(const char* pszTarget = " \t");
            /**
             * @brief ȥ������ָ����һЩ�ַ�(ֻҪ���������κ�һ���ַ�), Ĭ����ȥ���հ�
             * 
             * @param pszTarget [in] ָ�����ַ���
             */
            TMdbNtcString& Trim(const char* pszTarget = " \t");
            /**
             * @brief ȥ���ұ�ָ�����ַ������ַ�����Ϊһ���������Ƚ�
             * 
             * @param pszTarget [in] ָ�����ַ���
             */
            TMdbNtcString& TrimRightStr(const char* pszTarget);
            /**
             * @brief ȥ�����ָ�����ַ������ַ�����Ϊһ���������Ƚ�
             * 
             * @param pszTarget [in] ָ�����ַ���
             */
            TMdbNtcString& TrimLeftStr(const char* pszTarget);
            /**
             * @brief ȥ������ָ�����ַ������ַ�����Ϊһ���������Ƚ�
             * 
             * @param pszTarget [in] ָ�����ַ���
             */
            TMdbNtcString& TrimStr(const char* pszTarget);
            /**
             * @brief �滻�ַ�
             * 
             * @param cOld [in] ���ַ�
             * @param cNew [in] ���ַ�
             */
            TMdbNtcString& Replace(char cOld, char cNew);
            /**
             * @brief �滻�ַ���
             * 
             * @param pszOld [in] ���ַ���
             * @param pszNew [in] ���ַ���
             */
            TMdbNtcString& Replace(const char* pszOld, const char* pszNew);
            /**
             * @brief ��һ���֣��滻Ϊָ���ַ�
             *      
             * @param iStart [in] �滻��ʼλ��
             * @param iCount [in] ��Ŀ,-1��ʾ��ĩβ
             * @param cNew [in] ���ַ�
             * @return void
             */
            TMdbNtcString& Replace(int iStart, int iCount, char cNew);
            /**
             * @brief ��һ���֣��滻Ϊָ���ַ���
             *      
             * @param iStart [in] �滻��ʼλ��
             * @param iCount [in] ��Ŀ,-1��ʾ��ĩβ
             * @param pszNew [in] ���ַ���
             * @return void
             */
            TMdbNtcString& Replace(int iStart, int iCount, const char* pszNew);
            /**
             * @brief ����ָ���ַ����ֵ�λ��
             * 
             * @param cTarget [in] ָ���Ĳ����ַ�
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int Find(char cTarget, int iStart = 0) const;
            /**
             * @brief ����ָ���ַ������ֵ�λ��
             * 
             * @param pszTarget [in] ָ���Ĳ����ַ���
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int Find(const char* pszTarget, int iStart = 0) const;
            /**
             * @brief �������ָ���ַ����ֵ�λ��
             * 
             * @param cTarget [in] ָ���Ĳ����ַ�
             * @param iStart [in] ���ҵĿ�ʼλ�ã��������ʼλ��,-1��ʾ���
             * @return int
             * @retval �����ҵ���λ��
             */
            int ReverseFind(char cTarget, int iStart = -1) const;
            /**
             * @brief �������ָ���ַ������ֵ�λ��
             * 
             * @param pszTarget [in] ָ���Ĳ����ַ���
             * @param iStart [in] ���ҵĿ�ʼλ�ã��������ʼλ��,-1��ʾ���
             * @return int
             * @retval �����ҵ���λ��
             */
            int ReverseFind(const char* pszTarget, int iStart = -1) const;
            /**
             * @brief ���ҵ�һ����ָ���ַ���λ��
             * 
             * @param cTarget [in] ָ�����ַ�
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstOf(char cTarget, int iStart = 0);
            /**
             * @brief �����״γ��ֵ�ƥ��pszTarget�κ��ַ������ַ�����
             * 
             * @param pszTarget [in] ָ�����ַ���
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstOf(const char* pszTarget, int iStart = 0);
            /**
             * @brief �����״γ��ֵ��ǿɼ��ַ���λ�ã����ɴ�ӡ���ַ���λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstOfVisible(int iStart = 0);
            /**
             * @brief �����״γ��ֵ��ǿհ��ַ�(ascii��9, 10, 11, 12, 13, 32)��λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstOfSpace(int iStart = 0);
            /**
             * @brief �����״γ��ֵ��ǿո��ַ�(ascii��9, 32)��λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstOfBlank(int iStart = 0);
            /**
             * @brief �����״γ��ֵ������ֵ�λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstOfDigit(int iStart = 0);
            /**
             * @brief �����״γ��ֵ�����ĸ��λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstOfAlpha(int iStart = 0);
            /**
             * @brief �����״γ��ֵ�����ĸ�����ֵ�λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstOfAlphaDigit(int iStart = 0);
            /**
             * @brief ���ҵ�һ������ָ���ַ���λ��
             * 
             * @param cTarget [in] ָ�����ַ�
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstNotOf(char cTarget, int iStart = 0);
            /**
             * @brief �����״γ��ֵĲ�ƥ��pszTarget�κ��ַ������ַ�����
             * 
             * @param pszTarget [in] ָ�����ַ���
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstNotOf(const char* pszTarget, int iStart = 0);
            /**
             * @brief �����״γ��ֵĲ��ǿɼ��ַ���λ�ã������ɴ�ӡ���ַ���λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstNotOfVisible(int iStart = 0);
            /**
             * @brief �����״γ��ֵĲ��ǿհ��ַ�(ascii��9, 10, 11, 12, 13, 32)��λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstNotOfSpace(int iStart = 0);
            /**
             * @brief �����״γ��ֵĲ��ǿո��ַ�(ascii��9, 32)��λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstNotOfBlank(int iStart = 0);
            /**
             * @brief �����״γ��ֵĲ������ֵ�λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstNotOfDigit(int iStart = 0);
            /**
             * @brief �����״γ��ֵĲ�����ĸ��λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstNotOfAlpha(int iStart = 0);
            /**
             * @brief �����״γ��ֵĲ�����ĸ�����ֵ�λ��
             * 
             * @param iStart [in] ���ҵĿ�ʼλ��
             * @return int
             * @retval �����ҵ���λ��
             */
            int FindFirstNotOfAlphaDigit(int iStart = 0);
            /**
             * @brief �����ַ���bufferָ��[�����޸ı����UpdateLength����ʹ��]
             * 
             * @param iMinBufLength [in]Ҫ��õ���ͻ����С,
             *                                     ���iMinBufLength Ϊ-1��������С����
             * @return char*
             * @retval ���ػ�����ָ��
             */
            inline char* GetBuffer(int iMinBufLength = -1)
            {
                if (iMinBufLength > 0) Reserve((MDB_UINT32)iMinBufLength);
                return m_pszBuffer;
            }
            /**
             * @brief �����ַ���bufferָ��
             * 
             * @return const char*
             * @retval ���ػ�����ָ��
             */
            inline const char* c_str() const { return m_pszBuffer?(m_pszBuffer):""; }
            /**
             * @brief 
             *
             * @param iLength [in] ָ���ַ�������
             * @return unsigned int
             * @retval �����ַ�����ǰ����
             */
            unsigned int UpdateLength(int iLength = -1);
            /**
             * @brief ����string��������
             * 
             * @param oStr [in] ��Ҫ�������ݵ�string
             */
            void Swap(TMdbNtcString& oStr);
            
            /**
             * @brief �õ��ַ��������ô���
             * 
             * @return unsigned int
             * @retval �����ô���
             */
            inline unsigned int Refcnt() const
            {
                return m_pszBuffer?reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiRefcnt:0;
            }
            /**
             * @brief �õ�TString ����Ŀռ�
             * 
             * @return unsigned int
             * @retval ��TString ����Ŀռ�
             */
            inline unsigned int GetAllocLength() const
            {
                return m_pszBuffer?reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiAllocLength:0;
            }
            /**
             * @brief �õ��ַ����ĳ���
             * @return unsigned int
             * @retval �ַ����ĳ���
             */
            inline unsigned int GetLength() const
            {
                return m_pszBuffer?reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiLength:0;
            }
            /**
             * @brief �õ��ַ����ĳ���
             * @return unsigned int
             * @retval �ַ����ĳ���
             */
            inline unsigned int length() const
            {
                return GetLength();
            }
            /**
             * @brief �ж��ַ����Ƿ�Ϊ��,������Ϊ0
             * 
             * @return bool
             */
            inline bool IsEmpty() const
            {
                return m_pszBuffer?(reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiLength==0):true;
            }
        public://�����string����ʽ������йصĿ��ƺ��������
            /**
             * @brief ����ʽ�������
             * 
             * @param iValue [in] ����ֵ
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& operator << (MDB_INT16 iValue);
            TMdbNtcString& operator << (MDB_INT32 iValue);
            TMdbNtcString& operator << (MDB_INT64 iValue);
            TMdbNtcString& operator << (MDB_UINT16 iValue);
            TMdbNtcString& operator << (MDB_UINT32 iValue);
            TMdbNtcString& operator << (MDB_UINT64 iValue);
            TMdbNtcString& operator << (void* pAddress);
            /**
             * @brief ����ʽ�������
             * 
             * @param fValue [in] ��������ֵ
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& operator << (float fValue);
            /**
             * @brief ����ʽ�������
             * 
             * @param dValue [in] ˫������ֵ
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& operator << (double dValue);
            /**
             * @brief ����ʽ�������
             * 
             * @param pszValue [in] �ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& operator << (const char* pszValue);
            /**
             * @brief ����ʽ�������
             * 
             * @param cValue [in] �ַ�
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& operator << (char cValue);
            /**
             * @brief ����ʽ�������
             * 
             * @param sValue [in] �ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            TMdbNtcString& operator << (const TMdbNtcString& sValue);
            /**
             * @brief �������˫���Ȼ򸡵���ʱ��������С����λ��
             * 
             * @param n [in] ����С���㱣��λ��,-1��ʾԭʼ���
             * @return ��
             */
            inline void setdecimals(MDB_INT8 n = -1)
            {
                m_iDecimals = n;
            }
            /**
             * @brief ����ÿ������ķָ���
             * 
             * @param cDelimiter [in] ����ÿ������ķָ���
             * @return ��
             */
            inline void setdelimiter(char cDelimiter = '\0')
            {
                m_cDelimiter = cDelimiter;
            }
        protected:
            /**
             * @brief ��buffer����������ָ���Ĵ�С�Ŀռ�(Ԥ�����ܸ���Щ)
             * 
             * @param uiSize [in] Ҫ��buffer�������Ĵ�С
             * @return char*
             * @retval ִ��buffer��ָ��
             */
            char* GrowSize(MDB_UINT32 uiSize);
            /**
             * @brief �õ��ַ��������ô���
             * 
             * @return unsigned int
             * @retval �����ô���
             */
            // �õ��ַ��������ô���
            inline unsigned int& _Refcnt()
            {
                return reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiRefcnt;
            }
        protected:
            char*           m_pszBuffer;    ///< ִ���ַ����洢��
            bool            m_bLockFree;    ///< ��������Ƿ�Ϊ����String
            //��������ʽ���������
            MDB_INT8            m_iDecimals;///< С���㱣��λ��
            char            m_cDelimiter;///< ÿ������ķָ�����Ĭ����
            
        };
        /**
         * @brief �������������ͬʱ�����Ƹ�ʽ���н�
         * 
         */
        class TMdbNtcStringManip
        {
        public:
            TMdbNtcStringManip(void (*f)(TMdbNtcString&, MDB_INT8), MDB_INT8 t) : _fp(f), _tp(t) {}
            inline friend TMdbNtcString& operator<<(TMdbNtcString& s, const TMdbNtcStringManip& sm) { (*sm._fp)(s, sm._tp); return s; }
        private:       
            void (* _fp)(TMdbNtcString&, MDB_INT8);
            MDB_INT8 _tp;
        };
        /**
         * @brief �������˫���Ȼ򸡵���ʱ��������С����λ��
         * 
         * @param n [in] ����С���㱣��λ��,-1��ʾԭʼ���
         * @return ��
         */        
        inline void _mdb_ntc_setdecimals(TMdbNtcString& s, MDB_INT8 n = -1)
        {
            s.setdecimals(n);
        }
        inline TMdbNtcStringManip mdb_ntc_setdecimals(MDB_INT8 n = -1){return TMdbNtcStringManip(_mdb_ntc_setdecimals, n);}
        /**
         * @brief ����ÿ������ķָ���
         * 
         * @param cDelimiter [in] ����ÿ������ķָ���
         * @return ��
         */
        inline void _mdb_ntc_setdelimiter(TMdbNtcString& s, MDB_INT8 cDelimiter = '\0')
        {
            s.setdelimiter((char)cDelimiter);
        }
        inline TMdbNtcStringManip mdb_ntc_setdelimiter(char cDelimiter = '\0'){return TMdbNtcStringManip(_mdb_ntc_setdelimiter, (MDB_INT8)cDelimiter);}
        /**
         * @brief �������TString
         * 
         */
        class TMdbNtcStringBuffer:public TMdbNtcString
        {
        public:
            /**
              * @brief ���캯��
              *
              * ��Ĺ��캯��
              */
            TMdbNtcStringBuffer();
            /**
             * @brief ���캯��,����ָ����ģʽ��ʽ����
             * 
             * @param iBufferSize [in] buffer�Ĵ�С����������洢��ʽ�����Ļ����С
             * @param pszFormat [in] ��ʽ����
             */
            TMdbNtcStringBuffer(int iBufferSize, const char* pszFormat = NULL, ...);
            /**
             * @brief ���캯������iRepeat���ַ�cSrc��ֵ����ǰ�ַ���
             * 
             * @param cSrc [in] �ַ�
             * @param iRepeat [in] �ظ�����
             */
            TMdbNtcStringBuffer(char cSrc, int iRepeat = 1);
            /**
             * @brief ���캯������һ���ַ�����iStart��ʼ��iCount���ַ�������ǰ�ַ���
             * 
             * @param pszStr [in] ������ַ���
             * @param iLength [in] ����,-1��ʾ��ĩβ
             */
            TMdbNtcStringBuffer(const char* pszStr, int iLength = -1);
            /**
             * @brief �������캯��
             * 
             * @param oStr [in] ������ַ���
             */
            TMdbNtcStringBuffer(const TMdbNtcString& oStr);
            /**
             * @brief �������캯��
             * 
             * @param oStr [in] ������ַ���
             */
            TMdbNtcStringBuffer(const TMdbNtcStringBuffer& oStr);
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param cSrc [in] �ַ�
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcStringBuffer& operator = (char cSrc)
            {
                Assign(cSrc, 1);
                return *this;
            }
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param pszSrc [in] ������ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcStringBuffer& operator = (const char* pszSrc)
            {
                Assign(pszSrc);
                return *this;
            }
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param oStr [in] ������ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcStringBuffer& operator = (const TMdbNtcString& oStr)
            {
                Assign(oStr);
                return *this;
            }
        };
        /**
         * @brief �ṩ�޷���buffer
         * 
         */
        class TMdbNtcDataBuffer:protected TMdbNtcStringBuffer
        {
        public:
            /**
              * @brief ���캯��
              *
              * ��Ĺ��캯��
              */
            TMdbNtcDataBuffer();
            /**
             * @brief ���캯��,����ָ����ģʽ��ʽ����
             * 
             * @param iBufferSize [in] buffer�Ĵ�С����������洢��ʽ�����Ļ����С
             * @param pszFormat [in] ��ʽ����
             */
            TMdbNtcDataBuffer(int iBufferSize, const char* pszFormat = NULL, ...);
            /**
             * @brief ���캯������iRepeat���ַ�cSrc��ֵ����ǰ�ַ���
             * 
             * @param cSrc [in] �ַ�
             * @param iRepeat [in] �ظ�����
             */
            TMdbNtcDataBuffer(char cSrc, int iRepeat = 1);
            /**
             * @brief ���캯������һ���ַ�����iStart��ʼ��iCount���ַ�������ǰ�ַ���
             * 
             * @param pData [in] �����buffer
             * @param uiLength [in] ����
             */
            TMdbNtcDataBuffer(const unsigned char* pData, unsigned int uiLength);
            /**
             * @brief �������캯��
             * 
             * @param oStr [in] ������ַ���
             */
            TMdbNtcDataBuffer(const TMdbNtcString& oStr);
            /**
             * @brief �������캯��
             * 
             * @param oStr [in] ������ַ���
             */
            TMdbNtcDataBuffer(const TMdbNtcDataBuffer& oStr);
            /**
             * @brief �����ַ���bufferָ��[�����޸ı����UpdateLength����ʹ��]
             * 
             * @param iMinBufLength [in]Ҫ��õ���ͻ����С,
             *                                     ���iMinBufLength Ϊ-1��������С����
             * @return unsigned char*
             * @retval ���ػ�����ָ��
             */
            inline unsigned char* GetBuffer(int iMinBufLength = -1)
            {
                return (unsigned char*)TMdbNtcStringBuffer::GetBuffer(iMinBufLength);
            }
            /**
             * @brief ����buffer
             * 
             * @param pData [in] ����buffer
             * @param uiLength [in] buffer����
             * @return ��
             */
            inline void SetBuffer(const unsigned char *pData, unsigned int uiLength)
            {
                Clear();
                Append((const char*)pData, (int)uiLength);
            }
            using TMdbNtcStringBuffer::Clear;
            using TMdbNtcStringBuffer::at;        
            using TMdbNtcStringBuffer::c_str;
            using TMdbNtcStringBuffer::GetLength;
            using TMdbNtcStringBuffer::length;
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param cSrc [in] �ַ�
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcDataBuffer& operator = (char cSrc)
            {
                Assign(cSrc, 1);
                return *this;
            }
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param pszSrc [in] ������ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcDataBuffer& operator = (const char* pszSrc)
            {
                Assign(pszSrc);
                return *this;
            }
            /**
             * @brief ���ظ�ֵ�����
             * 
             * @param oStr [in] ������ַ���
             * @return TMdbNtcString
             * @retval ���ض�����
             */
            inline TMdbNtcDataBuffer& operator = (const TMdbNtcString& oStr)
            {
                Assign(oStr);
                return *this;
            }
        };
        inline bool operator==(const TMdbNtcString& s1, const TMdbNtcString& s2)
        {
            return s1.Compare(s2)==0;
        }

        inline bool operator==(const TMdbNtcString& s1, const char* s2)
        {
            return s1.Compare(s2)==0;
        }

        inline bool operator==(const char* s1, const TMdbNtcString& s2)
        {
            return s2.Compare(s1)==0;
        }

        inline bool operator!=(const TMdbNtcString& s1, const TMdbNtcString& s2)
        {
            return s1.Compare(s2)!=0;
        }

        inline bool operator!=(const TMdbNtcString& s1, const char* s2)
        {
            return s1.Compare(s2)!=0;
        }

        inline bool operator!=(const char* s1, const TMdbNtcString& s2)
        {
            return s2.Compare(s1)!=0;
        }

        inline bool operator<(const TMdbNtcString& s1, const TMdbNtcString& s2)
        {
            return s1.Compare(s2) < 0;
        }

        inline bool operator<(const TMdbNtcString& s1, const char* s2)
        {
            return s1.Compare(s2) < 0;
        }

        inline bool operator<(const char* s1, const TMdbNtcString& s2)
        {
            return s2.Compare(s1) > 0;
        }

        inline bool operator>(const TMdbNtcString& s1, const TMdbNtcString& s2)
        {
            return s1.Compare(s2) > 0;
        }

        inline bool operator>(const TMdbNtcString& s1, const char* s2)
        {
            return s1.Compare(s2) > 0;
        }

        inline bool operator>(const char* s1, const TMdbNtcString& s2)
        {
            return s2.Compare(s1) < 0;
        }

        inline bool operator<=(const TMdbNtcString& s1, const TMdbNtcString& s2)
        {
            return s1.Compare(s2) <= 0;
        }

        inline bool operator<=(const TMdbNtcString& s1, const char* s2)
        {
            return s1.Compare(s2) <= 0;
        }

        inline bool operator<=(const char* s1, const TMdbNtcString& s2)
        {
            return s2.Compare(s1) >= 0;
        }

        inline bool operator>=(const TMdbNtcString& s1, const TMdbNtcString& s2)
        {
            return s1.Compare(s2) >= 0;
        }

        inline bool operator>=(const TMdbNtcString& s1, const char* s2)
        {
            return s1.Compare(s2) >= 0;
        }

        inline bool operator>=(const char* s1, const TMdbNtcString& s2)
        {
            return s2.Compare(s1) <= 0;
        }

        TMdbNtcStringBuffer operator+(const TMdbNtcString& s1, char c);
        TMdbNtcStringBuffer operator+(const TMdbNtcString& s1, const char* s2);
        TMdbNtcStringBuffer operator+(const TMdbNtcString& s1, const TMdbNtcString& s2);
        TMdbNtcStringBuffer operator+(char c, const TMdbNtcString& s2);
        TMdbNtcStringBuffer operator+(const char* s1, const TMdbNtcString& s2);
//}
#endif
