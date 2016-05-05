/**
 * @file mdbStrings.h
 * @brief 字符串相关类以及TStringObject类
 *
 * 字符串相关类以及TStringObject类
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
         * @brief 字符串TString类的定义
         *
         *  字符串TString类的定义
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
             * @brief string的一些共享信息
             * 
             */
            struct TStringData
            {
                TMdbNtcThreadSpinLock*      pSpinlock;///< 自旋锁
                unsigned int    uiAllocLength;///< 所申请的长度
                unsigned int    uiLength; ///< 字符串长度
                unsigned int    uiRefcnt;///<引用个数 
                TStringData(unsigned int uiAllocLength = 0,
                    unsigned int uiLength = 0, bool bWithLock = true);
                ~TStringData();
                int AddRef();
                /**
                 * @brief 解除引用，如果解除后，发现引用归0，则释放buffer
                 * 
                 * @param bDelLock [in] 是否释放buffer的同时删除锁
                 * @return int
                 * @retval 此次解除后的引用计数，从返回值可以判断是否是由本次释放buffer的(如果为0)
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
             * @brief 构造函数,按照指定的模式格式化串
             * 
             * @param iBufferSize [in] buffer的大小
             * @param pszFormat [in] 格式化串
             */
            TMdbNtcString(int iBufferSize, const char* pszFormat = NULL, ...);
            /**
             * @brief 构造函数，用iRepeat个字符cSrc赋值给当前字符串
             * 
             * @param cSrc [in] 字符
             * @param iRepeat [in] 重复次数
             */
            TMdbNtcString(char cSrc, int iRepeat = 1);
            /**
             * @brief 构造函数，将一个字符串赋给当前字符串
             * 
             * @param pszStr [in] 构造的字符串     
             * @param iLen [in] 从pszStr中拷贝的字符个数,-1表示到末尾
             */
            TMdbNtcString(const char* pszStr, int iLen = -1);
            /**
             * @brief 拷贝构造函数
             * 
             * @param oStr [in] 构造的字符串
             */
            TMdbNtcString(const TMdbNtcString& oStr);

            /**
              * @brief 析构函数
              */
            ~TMdbNtcString();
            
            /**
             * @brief 赋值函数，用iRepeat个字符cSrc赋值给当前字符串
             * 
             * @param cSrc [in] 字符
             * @param iRepeat [in] 重复次数
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& Assign(char cSrc, int iRepeat = 1);
            /**
             * @brief 赋值函数，将一个字符串赋给当前字符串
             * 
             * @param pszStr [in] 构造的字符串
             * @param iLen [in] 数目,-1表示到末尾
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& Assign(const char* pszStr, int iLen = -1);
            /**
             * @brief 拷贝构造函数
             * 
             * @param oStr [in] 构造的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& Assign(const TMdbNtcString& oStr);
            /**
             * @brief 重载赋值运算符
             * 
             * @param cSrc [in] 字符
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcString& operator = (char cSrc)
            {
                return Assign(cSrc, 1);
            }
            /**
             * @brief 重载赋值运算符
             * 
             * @param pszSrc [in] 构造的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcString& operator = (const char* pszSrc)
            {
                return Assign(pszSrc);
            }
            /**
             * @brief 重载赋值运算符
             * 
             * @param oStr [in] 构造的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcString& operator = (const TMdbNtcString& oStr)
            {
                return Assign(oStr);
            }            
            /**
             * @brief 清空字符串
             * 
             * @return TMdbNtcString
             * @retval 返回自身
             */
            TMdbNtcString& Clear();

            /**
             * @brief 预留函数
             * @param uiReserveSize [in] 预留大小
             */
            char*  Reserve(MDB_UINT32 uiReserveSize);
            
            /**
             * @brief 清空字符串,并释放内存
             * 
             */
            void Release();
            /**
             * @brief 通过运算符[]获得指定位置的字符
             * 
             * @param uiIndex [in] 下标位置
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
             * @brief 获得指定位置的字符
             * 
             * @param uiIndex [in] 下标位置
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
             * @brief 在末尾追加字符
             * 
             * @param pszStr [in] 需要添加的字符
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcString& operator += (char cStr)
            {
                return Append(cStr);
            }
            /**
             * @brief 在末尾追加字符串
             * 
             * @param pszStr [in] 需要添加的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcString& operator += (const char* pszStr)
            {
                return Append(pszStr);
            }
            /**
             * @brief 在末尾追加字符串
             * 
             * @param oStr [in] 需要添加的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcString& operator += (const TMdbNtcString& oStr)
            {
                return Append(oStr);
            }
            /**
             * @brief 在末尾追加字符串
             * 
             * @param cTarget [in] 需要追加的字符
             * @param iRepeat [in] 重复次数
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& Append(char cTarget, int iRepeat = 1);
            /**
             * @brief 在末尾追加字符串
             * 
             * @param pszStr [in] 需要添加的字符串
             * @param iLen [in] 数目,-1表示到末尾
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& Append(const char* pszStr, int iLen = -1);
            /**
             * @brief 在末尾追加字符串
             * 
             * @param oStr [in] 需要添加的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& Append(const TMdbNtcString& oStr);
            /**
             * @brief 插入字符
             * 
             * @param iIndex [in] 插入的位置，-1表示末尾
             * @param cTarget [in] 要插入的字符
             * @param iRepeat [in] 重复次数
             * @return int
             * @retval 新字符所在的位置
             */
            int Insert(int iIndex, char cTarget, int iRepeat = 1);
            /**
             * @brief 插入字符串
             * 
             * @param iIndex [in] 插入的位置，-1表示末尾
             * @param pszTarget [in] 要插入的字符串
             * @param iLen [in] 插入的字符数目,-1表示到pszTarget末尾
             * @return int
             * @retval 新字符串所在的位置
             */
            int Insert(int iIndex, const char* pszTarget, int iLen = -1);
            /**
             * @brief 插入字符串
             * 
             * @param iIndex [in] 插入的位置，-1表示末尾
             * @param oStr [in] 需要添加的字符串
             * @return int
             * @retval 新字符串所在的位置
             */
            int Insert(int iIndex, const TMdbNtcString& oStr);
            /**
             * @brief 移除指定的字符
             * 
             * @param cTarget [in] 需要移除的字符
             * @return int
             * @retval 删除的字符个数
             */
            int Remove(char cTarget);
            /**
             * @brief 删除指定数目的字符
             * 
             * @param iStart [in] 删除的起始位置
             * @param iCount [in] 删除的字符数,-1表示删除到末尾
             * @return int
             * @retval 实际删除的字符个数
             */
            int Delete(int iStart, int iCount = 1);
            /**
             * @brief 按照指定的模式格式化串
             * 
             * @param iBufferSize [in] buffer的大小，用于申请存储格式化串的缓存大小
             * @param pszFormat [in] 格式化串
             */
            void Snprintf(int iBufferSize, const char* pszFormat, ...);
            /**
             * @brief 按照指定的模式格式化串
             * 
             * @param iBufferSize [in] buffer的大小，用于申请存储格式化串的缓存大小
             * @param pszFormat [in] 格式化串
             */
            void vSnprintf(int iBufferSize, const char* pszFormat, va_list ap);
            /**
             * @brief 获取子串
             * 
             * @param iStart [in] 开始位置
             * @param iCount [in] 需要取的子串长度,-1表示到结尾
             * @return TMdbNtcStringBuffer
             * @retval 子串
             */
            TMdbNtcStringBuffer Substr(int iStart, int iCount = -1) const;
            /**
             * @brief 字符串完全比较，与strcmp类似
             * 
             * @param pszTarget [in] 目标字符串
             * @param bCase [in] 是否区分大小写，默认区分
             * @return int
             * @retval =0 两者长度相等且字符串相同
             * @retval >0 本身大于pszTarget
             * @retval <0 本身小于pszTarget
             */
            inline int Compare(const char* pszTarget, bool bCase = true) const
            {
                if(pszTarget == NULL) pszTarget = "";
                if(m_pszBuffer) return bCase?strcmp(m_pszBuffer, pszTarget):mdb_ntc_stricmp(m_pszBuffer, pszTarget);
                else if(*pszTarget == '\0') return 0;
                else return -1;
            }
            /**
             * @brief 字符串完全比较
             * 
             * @param oStr [in] 需要比较的字符串
             * @param bCase [in] 是否区分大小写，默认区分
             * @return int
             * @retval =0 两者长度相等且字符串相同
             * @retval >0 本身大于oStr
             * @retval <0 本身小于oStr
             */
            inline int Compare(const TMdbNtcString& oStr, bool bCase = true) const
            {
                if(m_pszBuffer == oStr.m_pszBuffer) return 0;
                else return Compare(oStr.c_str(), bCase);
            }
            /**
             * @brief 是否字符串是否以指定串为前缀，有些类似strstr，但仅仅前缀匹配
             * 
             * @param pszPrefix [in] 前缀
             * @param bCase     [in] 是否区分大小写，默认区分
             * @return const char*
             * @retval NULL   表示不是以此为前缀
             * @retval 非NULL 表示匹配，返回偏移Prefix后的指针
             */
            const char* StrPrefix(const char* pszPrefix, bool bCase = true) const;
            /**
             * @brief 是否字符串是否以指定串为后缀
             * 
             * @param pszSuffix [in] 后缀
             * @param bCase     [in] 是否区分大小写，默认区分
             * @return const char*
             * @retval NULL   表示不是以此为后缀
             * @retval 非NULL 表示匹配，返回开始匹配的位置
             */
            const char* StrSuffix(const char* pszSuffix, bool bCase = true) const;
            /**
             * @brief 转换为int
             * 
             * @return int
             */
            int ToInt();
            /**
             * @brief 转换为int64
             * 
             * @return MDB_INT64
             */
            MDB_INT64 ToInt64();
            /**
             * @brief 转换为双精度
             * 
             * @param iPrecision [in] 表示精度是多少（保留几位小数），-1表示保留所有小数
             * @param bRounding [in] 表示是否四舍五入，默认为true
             * @return double
             */
            double ToDouble(int iPrecision = -1, bool bRounding = true);
            /**
             * @brief 转换为小写字母
             * 
             */
            TMdbNtcString& ToLower();
            /**
             * @brief 转换为大写字母
             * 
             */
            TMdbNtcString& ToUpper();
            /**
             * @brief 去除右边指定的字符
             * 
             * @param cTarget [in] 指定的字符
             */
            TMdbNtcString& TrimRight(char cTarget);
            /**
             * @brief 去除左边指定的字符
             * 
             * @param cTarget [in] 指定的字符
             */
            TMdbNtcString& TrimLeft(char cTarget);
            /**
             * @brief 去除两边指定的字符
             * 
             * @param cTarget [in] 指定的字符
             */
            TMdbNtcString& Trim(char cTarget);
            /**
             * @brief 去除右边指定的一些字符(只要等于其中任何一个字符), 默认是去除空白
             * 
             * @param pszTarget [in] 指定的字符串
             */
            TMdbNtcString& TrimRight(const char* pszTarget = " \t");
            /**
             * @brief 去除左边指定的一些字符(只要等于其中任何一个字符), 默认是去除空白
             * 
             * @param pszTarget [in] 指定的字符串
             */
            TMdbNtcString& TrimLeft(const char* pszTarget = " \t");
            /**
             * @brief 去除两边指定的一些字符(只要等于其中任何一个字符), 默认是去除空白
             * 
             * @param pszTarget [in] 指定的字符串
             */
            TMdbNtcString& Trim(const char* pszTarget = " \t");
            /**
             * @brief 去除右边指定的字符串，字符串作为一个整体参与比较
             * 
             * @param pszTarget [in] 指定的字符串
             */
            TMdbNtcString& TrimRightStr(const char* pszTarget);
            /**
             * @brief 去除左边指定的字符串，字符串作为一个整体参与比较
             * 
             * @param pszTarget [in] 指定的字符串
             */
            TMdbNtcString& TrimLeftStr(const char* pszTarget);
            /**
             * @brief 去除两边指定的字符串，字符串作为一个整体参与比较
             * 
             * @param pszTarget [in] 指定的字符串
             */
            TMdbNtcString& TrimStr(const char* pszTarget);
            /**
             * @brief 替换字符
             * 
             * @param cOld [in] 旧字符
             * @param cNew [in] 新字符
             */
            TMdbNtcString& Replace(char cOld, char cNew);
            /**
             * @brief 替换字符串
             * 
             * @param pszOld [in] 旧字符串
             * @param pszNew [in] 新字符串
             */
            TMdbNtcString& Replace(const char* pszOld, const char* pszNew);
            /**
             * @brief 将一部分，替换为指定字符
             *      
             * @param iStart [in] 替换开始位置
             * @param iCount [in] 数目,-1表示到末尾
             * @param cNew [in] 新字符
             * @return void
             */
            TMdbNtcString& Replace(int iStart, int iCount, char cNew);
            /**
             * @brief 将一部分，替换为指定字符串
             *      
             * @param iStart [in] 替换开始位置
             * @param iCount [in] 数目,-1表示到末尾
             * @param pszNew [in] 新字符串
             * @return void
             */
            TMdbNtcString& Replace(int iStart, int iCount, const char* pszNew);
            /**
             * @brief 查找指定字符出现的位置
             * 
             * @param cTarget [in] 指定的查找字符
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int Find(char cTarget, int iStart = 0) const;
            /**
             * @brief 查找指定字符串出现的位置
             * 
             * @param pszTarget [in] 指定的查找字符串
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int Find(const char* pszTarget, int iStart = 0) const;
            /**
             * @brief 反向查找指定字符出现的位置
             * 
             * @param cTarget [in] 指定的查找字符
             * @param iStart [in] 查找的开始位置（相对于起始位置,-1表示最后）
             * @return int
             * @retval 返回找到的位置
             */
            int ReverseFind(char cTarget, int iStart = -1) const;
            /**
             * @brief 反向查找指定字符串出现的位置
             * 
             * @param pszTarget [in] 指定的查找字符串
             * @param iStart [in] 查找的开始位置（相对于起始位置,-1表示最后）
             * @return int
             * @retval 返回找到的位置
             */
            int ReverseFind(const char* pszTarget, int iStart = -1) const;
            /**
             * @brief 查找第一个是指定字符的位置
             * 
             * @param cTarget [in] 指定的字符
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstOf(char cTarget, int iStart = 0);
            /**
             * @brief 查找首次出现的匹配pszTarget任何字符的首字符索引
             * 
             * @param pszTarget [in] 指定的字符串
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstOf(const char* pszTarget, int iStart = 0);
            /**
             * @brief 查找首次出现的是可见字符的位置，即可打印的字符的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstOfVisible(int iStart = 0);
            /**
             * @brief 查找首次出现的是空白字符(ascii码9, 10, 11, 12, 13, 32)的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstOfSpace(int iStart = 0);
            /**
             * @brief 查找首次出现的是空格字符(ascii码9, 32)的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstOfBlank(int iStart = 0);
            /**
             * @brief 查找首次出现的是数字的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstOfDigit(int iStart = 0);
            /**
             * @brief 查找首次出现的是字母的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstOfAlpha(int iStart = 0);
            /**
             * @brief 查找首次出现的是字母和数字的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstOfAlphaDigit(int iStart = 0);
            /**
             * @brief 查找第一个不是指定字符的位置
             * 
             * @param cTarget [in] 指定的字符
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstNotOf(char cTarget, int iStart = 0);
            /**
             * @brief 查找首次出现的不匹配pszTarget任何字符的首字符索引
             * 
             * @param pszTarget [in] 指定的字符串
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstNotOf(const char* pszTarget, int iStart = 0);
            /**
             * @brief 查找首次出现的不是可见字符的位置，即不可打印的字符的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstNotOfVisible(int iStart = 0);
            /**
             * @brief 查找首次出现的不是空白字符(ascii码9, 10, 11, 12, 13, 32)的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstNotOfSpace(int iStart = 0);
            /**
             * @brief 查找首次出现的不是空格字符(ascii码9, 32)的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstNotOfBlank(int iStart = 0);
            /**
             * @brief 查找首次出现的不是数字的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstNotOfDigit(int iStart = 0);
            /**
             * @brief 查找首次出现的不是字母的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstNotOfAlpha(int iStart = 0);
            /**
             * @brief 查找首次出现的不是字母和数字的位置
             * 
             * @param iStart [in] 查找的开始位置
             * @return int
             * @retval 返回找到的位置
             */
            int FindFirstNotOfAlphaDigit(int iStart = 0);
            /**
             * @brief 返回字符串buffer指针[如有修改必须和UpdateLength配套使用]
             * 
             * @param iMinBufLength [in]要获得的最低缓存大小,
             *                                     如果iMinBufLength 为-1缓冲区大小不变
             * @return char*
             * @retval 返回缓冲区指针
             */
            inline char* GetBuffer(int iMinBufLength = -1)
            {
                if (iMinBufLength > 0) Reserve((MDB_UINT32)iMinBufLength);
                return m_pszBuffer;
            }
            /**
             * @brief 返回字符串buffer指针
             * 
             * @return const char*
             * @retval 返回缓冲区指针
             */
            inline const char* c_str() const { return m_pszBuffer?(m_pszBuffer):""; }
            /**
             * @brief 
             *
             * @param iLength [in] 指定字符串长度
             * @return unsigned int
             * @retval 返回字符串当前长度
             */
            unsigned int UpdateLength(int iLength = -1);
            /**
             * @brief 两个string交换数据
             * 
             * @param oStr [in] 需要交换数据的string
             */
            void Swap(TMdbNtcString& oStr);
            
            /**
             * @brief 得到字符串被引用次数
             * 
             * @return unsigned int
             * @retval 被引用次数
             */
            inline unsigned int Refcnt() const
            {
                return m_pszBuffer?reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiRefcnt:0;
            }
            /**
             * @brief 得到TString 申请的空间
             * 
             * @return unsigned int
             * @retval 被TString 申请的空间
             */
            inline unsigned int GetAllocLength() const
            {
                return m_pszBuffer?reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiAllocLength:0;
            }
            /**
             * @brief 得到字符串的长度
             * @return unsigned int
             * @retval 字符串的长度
             */
            inline unsigned int GetLength() const
            {
                return m_pszBuffer?reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiLength:0;
            }
            /**
             * @brief 得到字符串的长度
             * @return unsigned int
             * @retval 字符串的长度
             */
            inline unsigned int length() const
            {
                return GetLength();
            }
            /**
             * @brief 判断字符串是否为空,即长度为0
             * 
             * @return bool
             */
            inline bool IsEmpty() const
            {
                return m_pszBuffer?(reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiLength==0):true;
            }
        public://下面和string的流式化输出有关的控制和输出函数
            /**
             * @brief 流格式化运算符
             * 
             * @param iValue [in] 整型值
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& operator << (MDB_INT16 iValue);
            TMdbNtcString& operator << (MDB_INT32 iValue);
            TMdbNtcString& operator << (MDB_INT64 iValue);
            TMdbNtcString& operator << (MDB_UINT16 iValue);
            TMdbNtcString& operator << (MDB_UINT32 iValue);
            TMdbNtcString& operator << (MDB_UINT64 iValue);
            TMdbNtcString& operator << (void* pAddress);
            /**
             * @brief 流格式化运算符
             * 
             * @param fValue [in] 浮点数数值
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& operator << (float fValue);
            /**
             * @brief 流格式化运算符
             * 
             * @param dValue [in] 双精度数值
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& operator << (double dValue);
            /**
             * @brief 流格式化运算符
             * 
             * @param pszValue [in] 字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& operator << (const char* pszValue);
            /**
             * @brief 流格式化运算符
             * 
             * @param cValue [in] 字符
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& operator << (char cValue);
            /**
             * @brief 流格式化运算符
             * 
             * @param sValue [in] 字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            TMdbNtcString& operator << (const TMdbNtcString& sValue);
            /**
             * @brief 设置输出双精度或浮点数时，保留的小数点位数
             * 
             * @param n [in] 设置小数点保留位数,-1表示原始输出
             * @return 无
             */
            inline void setdecimals(MDB_INT8 n = -1)
            {
                m_iDecimals = n;
            }
            /**
             * @brief 设置每次输出的分隔符
             * 
             * @param cDelimiter [in] 设置每次输出的分隔符
             * @return 无
             */
            inline void setdelimiter(char cDelimiter = '\0')
            {
                m_cDelimiter = cDelimiter;
            }
        protected:
            /**
             * @brief 将buffer增长到容纳指定的大小的空间(预留可能更多些)
             * 
             * @param uiSize [in] 要将buffer增长到的大小
             * @return char*
             * @retval 执行buffer的指针
             */
            char* GrowSize(MDB_UINT32 uiSize);
            /**
             * @brief 得到字符串被引用次数
             * 
             * @return unsigned int
             * @retval 被引用次数
             */
            // 得到字符串被引用次数
            inline unsigned int& _Refcnt()
            {
                return reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiRefcnt;
            }
        protected:
            char*           m_pszBuffer;    ///< 执行字符串存储区
            bool            m_bLockFree;    ///< 本身对象是否为无锁String
            //下面是流式化输出属性
            MDB_INT8            m_iDecimals;///< 小数点保留位数
            char            m_cDelimiter;///< 每次输出的分隔符，默认无
            
        };
        /**
         * @brief 用于连续输出的同时，控制格式的中介
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
         * @brief 设置输出双精度或浮点数时，保留的小数点位数
         * 
         * @param n [in] 设置小数点保留位数,-1表示原始输出
         * @return 无
         */        
        inline void _mdb_ntc_setdecimals(TMdbNtcString& s, MDB_INT8 n = -1)
        {
            s.setdecimals(n);
        }
        inline TMdbNtcStringManip mdb_ntc_setdecimals(MDB_INT8 n = -1){return TMdbNtcStringManip(_mdb_ntc_setdecimals, n);}
        /**
         * @brief 设置每次输出的分隔符
         * 
         * @param cDelimiter [in] 设置每次输出的分隔符
         * @return 无
         */
        inline void _mdb_ntc_setdelimiter(TMdbNtcString& s, MDB_INT8 cDelimiter = '\0')
        {
            s.setdelimiter((char)cDelimiter);
        }
        inline TMdbNtcStringManip mdb_ntc_setdelimiter(char cDelimiter = '\0'){return TMdbNtcStringManip(_mdb_ntc_setdelimiter, (MDB_INT8)cDelimiter);}
        /**
         * @brief 无锁版的TString
         * 
         */
        class TMdbNtcStringBuffer:public TMdbNtcString
        {
        public:
            /**
              * @brief 构造函数
              *
              * 类的构造函数
              */
            TMdbNtcStringBuffer();
            /**
             * @brief 构造函数,按照指定的模式格式化串
             * 
             * @param iBufferSize [in] buffer的大小，用于申请存储格式化串的缓存大小
             * @param pszFormat [in] 格式化串
             */
            TMdbNtcStringBuffer(int iBufferSize, const char* pszFormat = NULL, ...);
            /**
             * @brief 构造函数，用iRepeat个字符cSrc赋值给当前字符串
             * 
             * @param cSrc [in] 字符
             * @param iRepeat [in] 重复次数
             */
            TMdbNtcStringBuffer(char cSrc, int iRepeat = 1);
            /**
             * @brief 构造函数，将一个字符串从iStart开始的iCount个字符赋给当前字符串
             * 
             * @param pszStr [in] 构造的字符串
             * @param iLength [in] 长度,-1表示到末尾
             */
            TMdbNtcStringBuffer(const char* pszStr, int iLength = -1);
            /**
             * @brief 拷贝构造函数
             * 
             * @param oStr [in] 构造的字符串
             */
            TMdbNtcStringBuffer(const TMdbNtcString& oStr);
            /**
             * @brief 拷贝构造函数
             * 
             * @param oStr [in] 构造的字符串
             */
            TMdbNtcStringBuffer(const TMdbNtcStringBuffer& oStr);
            /**
             * @brief 重载赋值运算符
             * 
             * @param cSrc [in] 字符
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcStringBuffer& operator = (char cSrc)
            {
                Assign(cSrc, 1);
                return *this;
            }
            /**
             * @brief 重载赋值运算符
             * 
             * @param pszSrc [in] 构造的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcStringBuffer& operator = (const char* pszSrc)
            {
                Assign(pszSrc);
                return *this;
            }
            /**
             * @brief 重载赋值运算符
             * 
             * @param oStr [in] 构造的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcStringBuffer& operator = (const TMdbNtcString& oStr)
            {
                Assign(oStr);
                return *this;
            }
        };
        /**
         * @brief 提供无符号buffer
         * 
         */
        class TMdbNtcDataBuffer:protected TMdbNtcStringBuffer
        {
        public:
            /**
              * @brief 构造函数
              *
              * 类的构造函数
              */
            TMdbNtcDataBuffer();
            /**
             * @brief 构造函数,按照指定的模式格式化串
             * 
             * @param iBufferSize [in] buffer的大小，用于申请存储格式化串的缓存大小
             * @param pszFormat [in] 格式化串
             */
            TMdbNtcDataBuffer(int iBufferSize, const char* pszFormat = NULL, ...);
            /**
             * @brief 构造函数，用iRepeat个字符cSrc赋值给当前字符串
             * 
             * @param cSrc [in] 字符
             * @param iRepeat [in] 重复次数
             */
            TMdbNtcDataBuffer(char cSrc, int iRepeat = 1);
            /**
             * @brief 构造函数，将一个字符串从iStart开始的iCount个字符赋给当前字符串
             * 
             * @param pData [in] 构造的buffer
             * @param uiLength [in] 长度
             */
            TMdbNtcDataBuffer(const unsigned char* pData, unsigned int uiLength);
            /**
             * @brief 拷贝构造函数
             * 
             * @param oStr [in] 构造的字符串
             */
            TMdbNtcDataBuffer(const TMdbNtcString& oStr);
            /**
             * @brief 拷贝构造函数
             * 
             * @param oStr [in] 构造的字符串
             */
            TMdbNtcDataBuffer(const TMdbNtcDataBuffer& oStr);
            /**
             * @brief 返回字符串buffer指针[如有修改必须和UpdateLength配套使用]
             * 
             * @param iMinBufLength [in]要获得的最低缓存大小,
             *                                     如果iMinBufLength 为-1缓冲区大小不变
             * @return unsigned char*
             * @retval 返回缓冲区指针
             */
            inline unsigned char* GetBuffer(int iMinBufLength = -1)
            {
                return (unsigned char*)TMdbNtcStringBuffer::GetBuffer(iMinBufLength);
            }
            /**
             * @brief 设置buffer
             * 
             * @param pData [in] 数据buffer
             * @param uiLength [in] buffer长度
             * @return 无
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
             * @brief 重载赋值运算符
             * 
             * @param cSrc [in] 字符
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcDataBuffer& operator = (char cSrc)
            {
                Assign(cSrc, 1);
                return *this;
            }
            /**
             * @brief 重载赋值运算符
             * 
             * @param pszSrc [in] 构造的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
             */
            inline TMdbNtcDataBuffer& operator = (const char* pszSrc)
            {
                Assign(pszSrc);
                return *this;
            }
            /**
             * @brief 重载赋值运算符
             * 
             * @param oStr [in] 构造的字符串
             * @return TMdbNtcString
             * @retval 返回对象本身
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
