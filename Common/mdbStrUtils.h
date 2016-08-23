/**
 * @file StrUtils.hxx
 * @brief 提供字符串检测和处理功能，如字符串格式判断，trm等操作
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
            //下面是静态方法
        public:            
            /**
             * @brief 判断源字符串是否与规则相匹配
             * 
             * @param pszStr [in] 源字符串
             * @param pszNameRule [in] 规则字符串，可以用通配符'*'和'?'，'*'表示多字通配，'?'表示单字通配
             * @param bMatchCase [in] 是否区分大小写，默认为区分大小写
             * @return int
             * @retval 0 成功
             */
            static bool MatchString(const char * pszStr, const char * pszNameRule, bool bMatchCase = true);
            /**
             * @brief 在pszSrc中查找字符串pszFind，pszFind中可以包含通配字符‘?’
             * 
             * @param pszSrc [in] 源字符串
             * @param pszFind [in] 需要查找的字符串，可以包含通配符'?'，'?'表示单字匹配
             * @param iStart [in] 查找的起始位置，默认为从0开始查找
             * @return int
             * @retval 找到则返回匹配位置，否则返回-1
             */
            static int FindString(const char* pszSrc, const char* pszFind, int iStart = 0);
            /**
             * @brief 转换为小写字母
             * 
             * @param pszStr [in] 要转换的字符串
             * @return char*
             * @retval 转换后的字符串
             */
            static char* ToLower(char* pszStr);
            /**
             * @brief 转换为大写字母
             * 
             * @param pszStr [in] 要转换的字符串
             * @return char*
             * @retval 转换后的字符串
             */
            static char* ToUpper(char* pszStr);
            /**
             * @brief 删除字符串两边的填充字符
             * 
             * @param pszSrc [in] 源字符串
             * @param cFill [in] 填充的字符
             * @return char*
             * @retval 被删除填充字符后的字符串
             */
            static char* Trim(char *pszSrc, char cFill);
            /**
             * @brief 删除字符串左边的填充字符
             * 
             * @param pszSrc [in] 源字符串
             * @param cFill [in] 填充的字符
             * @return char*
             * @retval 被删除填充字符后的字符串
             */
            static char* TrimLeft(char *pszSrc,char cFill);
            /**
             * @brief 删除字符串右边的填充字符
             * 
             * @param pszSrc [in] 源字符串
             * @param cFill [in] 填充的字符
             * @return char*
             * @retval 被删除填充字符后的字符串
             */
            static char* TrimRight(char *pszSrc,char cFill);
            /**
             * @brief 删除字符串两边的填充字符
             * 
             * @param pszSrc [in] 源字符串
             * @param pszFill [in] 填充的字符串(多个字符需要删除)，默认为空格和tab键
             * @param iFillLength [in] 填充的字符串长度,-1则使用strlen计算
             * @return char*
             * @retval 被删除填充字符后的字符串
             */
            static char* Trim(char *pszSrc, const char* pszFill = " \t", int iFillLength = -1);
            /**
             * @brief 删除字符串左边的填充字符
             * 
             * @param pszSrc [in] 源字符串
             * @param pszFill [in] 填充的字符串(多个字符需要删除)，默认为空格和tab键
             * @param iFillLength [in] 填充的字符串长度,-1则使用strlen计算
             * @return char*
             * @retval 被删除填充字符后的字符串
             */
            static char* TrimLeft(char *pszSrc, const char* pszFill = " \t", int iFillLength = -1);
            /**
             * @brief 删除字符串右边的填充字符
             * 
             * @param pszSrc [in] 源字符串
             * @param pszFill [in] 填充的字符串(多个字符需要删除)，默认为空格和tab键
             * @param iFillLength [in] 填充的字符串长度,-1则使用strlen计算
             * @return char*
             * @retval 被删除填充字符后的字符串
             */
            static char* TrimRight(char *pszSrc, const char* pszFill = " \t", int iFillLength = -1);
            /**
             * @brief 忽略大小写比较字符串大小
             * 
             * @param pszFirst [in] 源字符串
             * @param pszSecond [in] 目标字符串
             * @param uiLength [in] 比较的长度(0表示全部比较，非0表示比较的长度)
             * @return int
             * @retval =0  表示相等
             * @retval <0 表示源字符串小于目标字符串
             * @retval >0 表示源字符串大于目标字符串
             */
            inline static int StrNoCaseCmp(const char *pszFirst, const char *pszSecond, unsigned int uiLength=0)
            {
                if( 0 == uiLength )
                    return mdb_ntc_stricmp(pszFirst, pszSecond);
                else
                    return mdb_ntc_strnicmp(pszFirst, pszSecond, uiLength);                    
            }
            /**
             * @brief 是否字符串是否以指定串为前缀，有些类似strstr，但仅仅前缀匹配
             * 
             * @param pszSrc    [in] 源字符串
             * @param pszPrefix [in] 前缀
             * @param bCase     [in] 是否区分大小写，默认区分
             * @return const char*
             * @retval NULL   表示不是以此为前缀
             * @retval 非NULL 表示匹配，返回偏移Prefix后的指针
             */
            static const char* StrPrefix(const char *pszSrc, const char *pszPrefix, bool bCase = true);
            /**
             * @brief 是否字符串是否以指定串为后缀
             * 
             * @param pszSrc    [in] 源字符串
             * @param pszSuffix [in] 后缀
             * @param bCase     [in] 是否区分大小写，默认区分
             * @return const char*
             * @retval NULL   表示不是以此为后缀
             * @retval 非NULL 表示匹配，返回开始匹配的位置
             */
            static const char* StrSuffix(const char *pszSrc, const char* pszSuffix, bool bCase = true);
            /**
             * @brief 判断字符串是否为NULL
             * 
             * @param pszSrc [in] 字符串
             * @return bool
             * @retval 为true则pszSrc==NULL
             */
            inline static bool IsNull(const char* pszSrc)
            {
                return pszSrc == NULL;
            }
            /**
             * @brief 判断字符串是否为空
             * 
             * @param pszSrc [in] 字符串
             * @return bool
             * @retval true 为空
             */
            static bool IsEmpty(const char *pszSrc);
            /**
             * @brief 判断该字符串是否是 YYYYMMDDHHMMSS格式的日期时间型字符串
             * 
             * @param pszStr [in] 需要判断的字符串
             * @param iLength [in] 字符串的长度,-1表示以'\0'结尾
             * @return bool
             * @retval 是与否
             */
            static bool IsDateTime(const char* pszStr, int iLength = -1);
            /**
             * @brief 判断该字符串是否是 HHMMSS格式的日期时间型字符串
             * 
             * @param pszStr [in] 需要判断的字符串
             * @param iLength [in] 字符串的长度,-1表示以'\0'结尾
             * @return bool
             * @retval 是与否
             */
            static bool IsTime(const char * pszStr, int iLength = -1);
            /**
             * @brief 判断该字符串是否是 YYYYMMDD格式的日期时间型字符串
             * 
             * @param pszStr [in] 需要判断的字符串
             * @param iLength [in] 字符串的长度,-1表示以'\0'结尾
             * @return bool
             * @retval 是与否
             */
            static bool IsDate(const char * pszStr, int iLength = -1);
            /**
             * @brief 判断该字符串是否为ip地址
             * 
             * @param pszStr [in] 需要判断的字符串
             * @param iLength [in] 字符串的长度,-1表示以'\0'结尾
             * @return bool
             * @retval 是与否
             */
            static bool IsIPAddress(const char * pszStr, int iLength = -1);
            /**
             * @brief 判断该字符串是否全数字，支持+-号打头。
             * 
             * @param pszStr [in] 需要判断的字符串
             * @return bool
             * @retval 是与否
             */
            static bool IsDigital(const char * pszStr);
            /**
             * @brief 判断该字符串是否全字母。
             * 
             * @param pszStr [in] 需要判断的字符串
             * @return bool
             * @retval 是与否
             */
            static bool IsAlpha(const char * pszStr);
            /**
             * @brief 判断一个数字型的字符串是否超过了32位整数的范围
             * 
             * @param pszStr [in] 数字字符串
             * @param iFlag [in] 表示传入的数字字符串是正数还是负数
             * @return bool
             * @retval true 超出范围
             */
            static bool IsOutOfInt(const char * pszStr, int iSignFlag);
            /**
             * @brief 判断一个数字型的字符串是否超过了64位整数的范围
             * 
             * @param pszStr [in] 数字字符串
             * @param iFlag [in] 表示传入的数字字符串是正数还是负数
             * @return bool
             * @retval true 超出范围
             */
            static bool IsOutOfInt64(const char * pszStr, int iSignFlag);
            /**
             * @brief 将字符串转换为整数
             * 
             * @param pszSrc [in] 字符串
             * @return MDB_INT64
             * @retval 转换后的整数
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
             * @brief 将字符串转换为浮点数
             * 
             * @param pszSrc [in] 字符串
             * @return MDB_INT64
             * @retval 转换后的浮点数
             */
            inline static double StrToFloat(const char *pszSrc)
            {
                return atof(pszSrc);
            }
            /**
             * @brief 十六进制字符串转成十进制整数，支持大小写的16进制字符串
             * 
             * @param pszSrc [in] 字符串
             * @param iLength [in] 字符串长度,-1表示以'\0'结尾
             * @return MDB_INT64
             * @retval 转换好的整数
             */
            static MDB_INT64 HexToInt64(const char* pszSrc, int iLength = -1);
            /**
             * @brief 将字符串进行Hash，转换成一个比较散列的值
             * 
             * @param pszSrc [in] 字符串
             * @return MDB_INT64
             * @retval 转换后的hash值
             */
            static MDB_INT64 StrToHash(const char* pszSrc);
            /**
             * @brief 和strncpy功能相同，但是遇到endtag字符时会置'\0',并返回
             * 
             * @param pszDest [in] 目标字符串
             * @param pszSrc  [in] 源字符串
             * @param uiCopyLength [in] 拷贝长度
             * @param cEndTag [in] 字符串拷贝结束符，默认为'\0'字符
             * @return char*
             * @retval 返回pszDest
             */
            static char* StrCopy(char * pszDest, const char* pszSrc, MDB_UINT32 uiCopyLength, char cEndTag='\0');
            /**
             * @brief 过滤掉指定的字符
             * 
             * @param pszSrc [in] 源字符串
             * @param pszDest [in] 目标字符串
             * @param iMaxDestLen [in] 目标字符串的最大长度
             * @param cFilter [in] 指定过滤的字符
             * @return char*
             * @retval 返回pszDest
             */
            static char* FilterChar(const char *pszSrc, char *pszDest, int iMaxDestLen, char cFilter);
            /**
             * @brief 遇到回车符'\r'或者换行符'\n'截断，保留前面的字符串
             * 
             * @param pszLine [in] 字符串行
             * @return char*
             * @retval 去除后的字符串
             */
            static char* FormatChar(char *pszLine);
            /**
             * @brief 将一个字符串中的某个子串替换
             * 
             * @param pszOrgSrc [in] 源字符串
             * @param pszSubStr [in] 源字符串中需要替换的子串
             * @param pszReplaceStr [in] 替换成的字符串
             * @param pszOutStr [out] 返回结果
             * @param bSensitive [in] 是否区分大小写
             * @return char*
             * @retval 替换后的string
             */
            static char * Replace(const char * pszOrgStr, const char * pszSubStr, const char * pszReplaceStr,
                char *pszOutStr, bool bSensitive = true);
        public://下面为非静态函数
            TMdbNtcStrFunc();
            /**
             * @brief 将字符串中的环境变量替换，环境变量的形式为 $(环境变量名)
             * 使用了getenv在多线程下，不安全
             * 
             * @param pszSrc [in] 源字符串
             * @return const char*
             * @retval 已经被替换了环境变量的string
             */
            const char* ReplaceEnv(const char *pszSrc);
            /**
             * @brief 将一个字符串中的某个子串替换
             * 
             * @param pszOrgSrc [in] 源字符串
             * @param pszSubStr [in] 源字符串中需要替换的子串
             * @param pszReplaceStr [in] 替换成的字符串
             * @return const char*
             * @retval 替换后的string
             */
            const char* Replace(const char* pszOrgSrc, const char* pszSubStr, const char* pszReplaceStr);
            /**
             * @brief 从源串（第一个参数）中过滤字符串（第二个参数）中的每个字符
             * 
             * @param pszSrc [in]源字符串
             * @param pszSeparate [in] 过滤字符串
             * @return const char*
             * @retval 过滤掉分割字符的字符串
             */
            const char* FilterSeparate(const char *pszSrc,const char *pszSeparate);
            /**
             * @brief 将整型转换成字符串输出
             * 
             * @param iValue [in] 整数值
             * @return const char*
             * @retval 转换后的字符串
             */
            inline const char* IntToStr(MDB_INT64 iValue)
            {
                snprintf(m_szBuffer, sizeof(m_szBuffer), "%"MDB_NTC_ZS_FORMAT_INT64, iValue);
                m_szBuffer[sizeof(m_szBuffer)-1] = '\0';
                return m_szBuffer;
            }
            /**
             * @brief 将浮点数转换成字符串输出
             * 
             * @param dValue [in] 浮点数值
             * @param iDecimals [in] 设置小数点保留位数,-1表示原始输出
             * @return const char*
             * @retval 转换后的字符串
             */
            inline const char* FloatToStr(double dValue)
            {
                snprintf(m_szBuffer, sizeof(m_szBuffer), "%f", dValue);
                m_szBuffer[sizeof(m_szBuffer)-1] = '\0';
                return m_szBuffer;
            }
            const char* FloatToStr(double dValue, MDB_INT8 iDecimals);
            /**
             * @brief 将整数转换为16进制字符串
             * 
             * @param iValue [in] 整数值
             * @return const char*
             * @retval 转换后的字符串,字母为大写字母
             */
            const char* IntToHexStr(MDB_INT64 iValue);
        protected:
            TMdbNtcStringBuffer m_sRetStr;
        public:
            char m_szBuffer[32];///< 用于整数，浮点数快速转换用
        };
        extern TMdbNtcStrFunc g_oMdbNtcStrFunc;
        #define MdbNtcStrUtils g_oMdbNtcStrFunc
//}
#endif
