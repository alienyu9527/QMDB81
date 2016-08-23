/**
 * @file StrUtils.hxx
 * @brief �ṩ�ַ������ʹ����ܣ����ַ�����ʽ�жϣ�trm�Ȳ���
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121214
 * @warning
 */
#ifndef _MDB_H_StrUtils_
#define _MDB_H_StrUtils_
#include "Common/mdbCommons.h"
#include "Common/mdbStrings.h"
//namespace QuickMDB
//{
        class TMdbNtcStrFunc
        {
            /** \example  example_TStrFunc.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
            //�����Ǿ�̬����
        public:            
            /**
             * @brief �ж�Դ�ַ����Ƿ��������ƥ��
             * 
             * @param pszStr [in] Դ�ַ���
             * @param pszNameRule [in] �����ַ�����������ͨ���'*'��'?'��'*'��ʾ����ͨ�䣬'?'��ʾ����ͨ��
             * @param bMatchCase [in] �Ƿ����ִ�Сд��Ĭ��Ϊ���ִ�Сд
             * @return int
             * @retval 0 �ɹ�
             */
            static bool MatchString(const char * pszStr, const char * pszNameRule, bool bMatchCase = true);
            /**
             * @brief ��pszSrc�в����ַ���pszFind��pszFind�п��԰���ͨ���ַ���?��
             * 
             * @param pszSrc [in] Դ�ַ���
             * @param pszFind [in] ��Ҫ���ҵ��ַ��������԰���ͨ���'?'��'?'��ʾ����ƥ��
             * @param iStart [in] ���ҵ���ʼλ�ã�Ĭ��Ϊ��0��ʼ����
             * @return int
             * @retval �ҵ��򷵻�ƥ��λ�ã����򷵻�-1
             */
            static int FindString(const char* pszSrc, const char* pszFind, int iStart = 0);
            /**
             * @brief ת��ΪСд��ĸ
             * 
             * @param pszStr [in] Ҫת�����ַ���
             * @return char*
             * @retval ת������ַ���
             */
            static char* ToLower(char* pszStr);
            /**
             * @brief ת��Ϊ��д��ĸ
             * 
             * @param pszStr [in] Ҫת�����ַ���
             * @return char*
             * @retval ת������ַ���
             */
            static char* ToUpper(char* pszStr);
            /**
             * @brief ɾ���ַ������ߵ�����ַ�
             * 
             * @param pszSrc [in] Դ�ַ���
             * @param cFill [in] �����ַ�
             * @return char*
             * @retval ��ɾ������ַ�����ַ���
             */
            static char* Trim(char *pszSrc, char cFill);
            /**
             * @brief ɾ���ַ�����ߵ�����ַ�
             * 
             * @param pszSrc [in] Դ�ַ���
             * @param cFill [in] �����ַ�
             * @return char*
             * @retval ��ɾ������ַ�����ַ���
             */
            static char* TrimLeft(char *pszSrc,char cFill);
            /**
             * @brief ɾ���ַ����ұߵ�����ַ�
             * 
             * @param pszSrc [in] Դ�ַ���
             * @param cFill [in] �����ַ�
             * @return char*
             * @retval ��ɾ������ַ�����ַ���
             */
            static char* TrimRight(char *pszSrc,char cFill);
            /**
             * @brief ɾ���ַ������ߵ�����ַ�
             * 
             * @param pszSrc [in] Դ�ַ���
             * @param pszFill [in] �����ַ���(����ַ���Ҫɾ��)��Ĭ��Ϊ�ո��tab��
             * @param iFillLength [in] �����ַ�������,-1��ʹ��strlen����
             * @return char*
             * @retval ��ɾ������ַ�����ַ���
             */
            static char* Trim(char *pszSrc, const char* pszFill = " \t", int iFillLength = -1);
            /**
             * @brief ɾ���ַ�����ߵ�����ַ�
             * 
             * @param pszSrc [in] Դ�ַ���
             * @param pszFill [in] �����ַ���(����ַ���Ҫɾ��)��Ĭ��Ϊ�ո��tab��
             * @param iFillLength [in] �����ַ�������,-1��ʹ��strlen����
             * @return char*
             * @retval ��ɾ������ַ�����ַ���
             */
            static char* TrimLeft(char *pszSrc, const char* pszFill = " \t", int iFillLength = -1);
            /**
             * @brief ɾ���ַ����ұߵ�����ַ�
             * 
             * @param pszSrc [in] Դ�ַ���
             * @param pszFill [in] �����ַ���(����ַ���Ҫɾ��)��Ĭ��Ϊ�ո��tab��
             * @param iFillLength [in] �����ַ�������,-1��ʹ��strlen����
             * @return char*
             * @retval ��ɾ������ַ�����ַ���
             */
            static char* TrimRight(char *pszSrc, const char* pszFill = " \t", int iFillLength = -1);
            /**
             * @brief ���Դ�Сд�Ƚ��ַ�����С
             * 
             * @param pszFirst [in] Դ�ַ���
             * @param pszSecond [in] Ŀ���ַ���
             * @param uiLength [in] �Ƚϵĳ���(0��ʾȫ���Ƚϣ���0��ʾ�Ƚϵĳ���)
             * @return int
             * @retval =0  ��ʾ���
             * @retval <0 ��ʾԴ�ַ���С��Ŀ���ַ���
             * @retval >0 ��ʾԴ�ַ�������Ŀ���ַ���
             */
            inline static int StrNoCaseCmp(const char *pszFirst, const char *pszSecond, unsigned int uiLength=0)
            {
                if( 0 == uiLength )
                    return mdb_ntc_stricmp(pszFirst, pszSecond);
                else
                    return mdb_ntc_strnicmp(pszFirst, pszSecond, uiLength);                    
            }
            /**
             * @brief �Ƿ��ַ����Ƿ���ָ����Ϊǰ׺����Щ����strstr��������ǰ׺ƥ��
             * 
             * @param pszSrc    [in] Դ�ַ���
             * @param pszPrefix [in] ǰ׺
             * @param bCase     [in] �Ƿ����ִ�Сд��Ĭ������
             * @return const char*
             * @retval NULL   ��ʾ�����Դ�Ϊǰ׺
             * @retval ��NULL ��ʾƥ�䣬����ƫ��Prefix���ָ��
             */
            static const char* StrPrefix(const char *pszSrc, const char *pszPrefix, bool bCase = true);
            /**
             * @brief �Ƿ��ַ����Ƿ���ָ����Ϊ��׺
             * 
             * @param pszSrc    [in] Դ�ַ���
             * @param pszSuffix [in] ��׺
             * @param bCase     [in] �Ƿ����ִ�Сд��Ĭ������
             * @return const char*
             * @retval NULL   ��ʾ�����Դ�Ϊ��׺
             * @retval ��NULL ��ʾƥ�䣬���ؿ�ʼƥ���λ��
             */
            static const char* StrSuffix(const char *pszSrc, const char* pszSuffix, bool bCase = true);
            /**
             * @brief �ж��ַ����Ƿ�ΪNULL
             * 
             * @param pszSrc [in] �ַ���
             * @return bool
             * @retval Ϊtrue��pszSrc==NULL
             */
            inline static bool IsNull(const char* pszSrc)
            {
                return pszSrc == NULL;
            }
            /**
             * @brief �ж��ַ����Ƿ�Ϊ��
             * 
             * @param pszSrc [in] �ַ���
             * @return bool
             * @retval true Ϊ��
             */
            static bool IsEmpty(const char *pszSrc);
            /**
             * @brief �жϸ��ַ����Ƿ��� YYYYMMDDHHMMSS��ʽ������ʱ�����ַ���
             * 
             * @param pszStr [in] ��Ҫ�жϵ��ַ���
             * @param iLength [in] �ַ����ĳ���,-1��ʾ��'\0'��β
             * @return bool
             * @retval �����
             */
            static bool IsDateTime(const char* pszStr, int iLength = -1);
            /**
             * @brief �жϸ��ַ����Ƿ��� HHMMSS��ʽ������ʱ�����ַ���
             * 
             * @param pszStr [in] ��Ҫ�жϵ��ַ���
             * @param iLength [in] �ַ����ĳ���,-1��ʾ��'\0'��β
             * @return bool
             * @retval �����
             */
            static bool IsTime(const char * pszStr, int iLength = -1);
            /**
             * @brief �жϸ��ַ����Ƿ��� YYYYMMDD��ʽ������ʱ�����ַ���
             * 
             * @param pszStr [in] ��Ҫ�жϵ��ַ���
             * @param iLength [in] �ַ����ĳ���,-1��ʾ��'\0'��β
             * @return bool
             * @retval �����
             */
            static bool IsDate(const char * pszStr, int iLength = -1);
            /**
             * @brief �жϸ��ַ����Ƿ�Ϊip��ַ
             * 
             * @param pszStr [in] ��Ҫ�жϵ��ַ���
             * @param iLength [in] �ַ����ĳ���,-1��ʾ��'\0'��β
             * @return bool
             * @retval �����
             */
            static bool IsIPAddress(const char * pszStr, int iLength = -1);
            /**
             * @brief �жϸ��ַ����Ƿ�ȫ���֣�֧��+-�Ŵ�ͷ��
             * 
             * @param pszStr [in] ��Ҫ�жϵ��ַ���
             * @return bool
             * @retval �����
             */
            static bool IsDigital(const char * pszStr);
            /**
             * @brief �жϸ��ַ����Ƿ�ȫ��ĸ��
             * 
             * @param pszStr [in] ��Ҫ�жϵ��ַ���
             * @return bool
             * @retval �����
             */
            static bool IsAlpha(const char * pszStr);
            /**
             * @brief �ж�һ�������͵��ַ����Ƿ񳬹���32λ�����ķ�Χ
             * 
             * @param pszStr [in] �����ַ���
             * @param iFlag [in] ��ʾ����������ַ������������Ǹ���
             * @return bool
             * @retval true ������Χ
             */
            static bool IsOutOfInt(const char * pszStr, int iSignFlag);
            /**
             * @brief �ж�һ�������͵��ַ����Ƿ񳬹���64λ�����ķ�Χ
             * 
             * @param pszStr [in] �����ַ���
             * @param iFlag [in] ��ʾ����������ַ������������Ǹ���
             * @return bool
             * @retval true ������Χ
             */
            static bool IsOutOfInt64(const char * pszStr, int iSignFlag);
            /**
             * @brief ���ַ���ת��Ϊ����
             * 
             * @param pszSrc [in] �ַ���
             * @return MDB_INT64
             * @retval ת���������
             */
            inline static MDB_INT64 StrToInt(const char *pszSrc)
            {
            #ifdef OS_WINDOWS
                return pszSrc?_atoi64(pszSrc):0;
            #else
                return pszSrc?atoll(pszSrc):0;
            #endif
            }
            /**
             * @brief ���ַ���ת��Ϊ������
             * 
             * @param pszSrc [in] �ַ���
             * @return MDB_INT64
             * @retval ת����ĸ�����
             */
            inline static double StrToFloat(const char *pszSrc)
            {
                return atof(pszSrc);
            }
            /**
             * @brief ʮ�������ַ���ת��ʮ����������֧�ִ�Сд��16�����ַ���
             * 
             * @param pszSrc [in] �ַ���
             * @param iLength [in] �ַ�������,-1��ʾ��'\0'��β
             * @return MDB_INT64
             * @retval ת���õ�����
             */
            static MDB_INT64 HexToInt64(const char* pszSrc, int iLength = -1);
            /**
             * @brief ���ַ�������Hash��ת����һ���Ƚ�ɢ�е�ֵ
             * 
             * @param pszSrc [in] �ַ���
             * @return MDB_INT64
             * @retval ת�����hashֵ
             */
            static MDB_INT64 StrToHash(const char* pszSrc);
            /**
             * @brief ��strncpy������ͬ����������endtag�ַ�ʱ����'\0',������
             * 
             * @param pszDest [in] Ŀ���ַ���
             * @param pszSrc  [in] Դ�ַ���
             * @param uiCopyLength [in] ��������
             * @param cEndTag [in] �ַ���������������Ĭ��Ϊ'\0'�ַ�
             * @return char*
             * @retval ����pszDest
             */
            static char* StrCopy(char * pszDest, const char* pszSrc, MDB_UINT32 uiCopyLength, char cEndTag='\0');
            /**
             * @brief ���˵�ָ�����ַ�
             * 
             * @param pszSrc [in] Դ�ַ���
             * @param pszDest [in] Ŀ���ַ���
             * @param iMaxDestLen [in] Ŀ���ַ�������󳤶�
             * @param cFilter [in] ָ�����˵��ַ�
             * @return char*
             * @retval ����pszDest
             */
            static char* FilterChar(const char *pszSrc, char *pszDest, int iMaxDestLen, char cFilter);
            /**
             * @brief �����س���'\r'���߻��з�'\n'�ضϣ�����ǰ����ַ���
             * 
             * @param pszLine [in] �ַ�����
             * @return char*
             * @retval ȥ������ַ���
             */
            static char* FormatChar(char *pszLine);
            /**
             * @brief ��һ���ַ����е�ĳ���Ӵ��滻
             * 
             * @param pszOrgSrc [in] Դ�ַ���
             * @param pszSubStr [in] Դ�ַ�������Ҫ�滻���Ӵ�
             * @param pszReplaceStr [in] �滻�ɵ��ַ���
             * @param pszOutStr [out] ���ؽ��
             * @param bSensitive [in] �Ƿ����ִ�Сд
             * @return char*
             * @retval �滻���string
             */
            static char * Replace(const char * pszOrgStr, const char * pszSubStr, const char * pszReplaceStr,
                char *pszOutStr, bool bSensitive = true);
        public://����Ϊ�Ǿ�̬����
            TMdbNtcStrFunc();
            /**
             * @brief ���ַ����еĻ��������滻��������������ʽΪ $(����������)
             * ʹ����getenv�ڶ��߳��£�����ȫ
             * 
             * @param pszSrc [in] Դ�ַ���
             * @return const char*
             * @retval �Ѿ����滻�˻���������string
             */
            const char* ReplaceEnv(const char *pszSrc);
            /**
             * @brief ��һ���ַ����е�ĳ���Ӵ��滻
             * 
             * @param pszOrgSrc [in] Դ�ַ���
             * @param pszSubStr [in] Դ�ַ�������Ҫ�滻���Ӵ�
             * @param pszReplaceStr [in] �滻�ɵ��ַ���
             * @return const char*
             * @retval �滻���string
             */
            const char* Replace(const char* pszOrgSrc, const char* pszSubStr, const char* pszReplaceStr);
            /**
             * @brief ��Դ������һ���������й����ַ������ڶ����������е�ÿ���ַ�
             * 
             * @param pszSrc [in]Դ�ַ���
             * @param pszSeparate [in] �����ַ���
             * @return const char*
             * @retval ���˵��ָ��ַ����ַ���
             */
            const char* FilterSeparate(const char *pszSrc,const char *pszSeparate);
            /**
             * @brief ������ת�����ַ������
             * 
             * @param iValue [in] ����ֵ
             * @return const char*
             * @retval ת������ַ���
             */
            inline const char* IntToStr(MDB_INT64 iValue)
            {
                snprintf(m_szBuffer, sizeof(m_szBuffer), "%"MDB_NTC_ZS_FORMAT_INT64, iValue);
                m_szBuffer[sizeof(m_szBuffer)-1] = '\0';
                return m_szBuffer;
            }
            /**
             * @brief ��������ת�����ַ������
             * 
             * @param dValue [in] ������ֵ
             * @param iDecimals [in] ����С���㱣��λ��,-1��ʾԭʼ���
             * @return const char*
             * @retval ת������ַ���
             */
            inline const char* FloatToStr(double dValue)
            {
                snprintf(m_szBuffer, sizeof(m_szBuffer), "%f", dValue);
                m_szBuffer[sizeof(m_szBuffer)-1] = '\0';
                return m_szBuffer;
            }
            const char* FloatToStr(double dValue, MDB_INT8 iDecimals);
            /**
             * @brief ������ת��Ϊ16�����ַ���
             * 
             * @param iValue [in] ����ֵ
             * @return const char*
             * @retval ת������ַ���,��ĸΪ��д��ĸ
             */
            const char* IntToHexStr(MDB_INT64 iValue);
        protected:
            TMdbNtcStringBuffer m_sRetStr;
        public:
            char m_szBuffer[32];///< ��������������������ת����
        };
        extern TMdbNtcStrFunc g_oMdbNtcStrFunc;
        #define MdbNtcStrUtils g_oMdbNtcStrFunc
//}
#endif
