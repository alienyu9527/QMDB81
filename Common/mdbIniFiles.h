/**
 * @file IniFiles.hxx
 * @brief 与ini文件解析相关的封装
 *
 * 与ini文件解析相关的封装
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121214
 * @warning
 */
#ifndef _MDB_H_IniFiles_
#define _MDB_H_IniFiles_
#include "Common/mdbCommons.h"
#include "Common/mdbDataStructs.h"
//namespace QuickMDB
//{
        /**
         * @brief 配置文件类, 兼容之前的接口
         * 
         * 每次读取都是从文件头开始查找，适合只读取少量的键信息
         * 
         */
        class TMdbNtcReadIni:public TMdbNtcBaseObject
        {
            /** \example  example_TReadIni.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
             MDB_ZF_DECLARE_OBJECT(TMdbNtcReadIni);
        public:
            TMdbNtcReadIni();
            ~TMdbNtcReadIni();
            /**
             * @brief 打开文件
             * 
             * @param pszFilePath [in] ini文件路径
             * @param cCommentSep [in]注释符
             */
            bool OpenFile(const char* pszFilePath, char cCommentSep = '#');
            /**
             * @brief 关闭文件
             *      
             */
            void CloseFile();
            /**
             * @brief 读取整形配置项，如果取值为空，则取默认值
             * 
             * @param pszSection [in] 段名
             * @param pszKey [in] 键名
             * @param iDefVal [in] 默认值
             * @return int
             * @retval 键值
             */
            int ReadInteger(const char *pszSection, const char *pszKey, int iDefVal = 0);
            /**
             * @brief 读取整形配置项
             * 
             * @param pszSection [in] 段名
             * @param pszKey [in] 键名
             * @param pszDefVal [in] 默认值
             * @return TMdbNtcStringBuffer
             * @retval 键值
             */
            TMdbNtcStringBuffer ReadString(const char *pszSection, const char *pszKey, const char *pszDefVal = "");
            /**
             * @brief 判断section是否存在
             * 
             * @param pszSection [in] 段名
             * @return bool
             * @retval true 存在
             */
            bool SectionCheck(const char *pszSection);
            /**
             * @brief 判断键名是否存在
             * 
             * @param pszSection [in] 段名
             * @param pszKey [in] 键名
             * @return bool
             * @retval true 存在
             */
            bool KeyCheck(const char *pszSection, const char *pszKey);
			/** @brief 从文件读入的密码解密
			 */
			char *Decrypt(char * password);
        private:
            FILE* m_fp;///< 文件指针
            char  m_cCommentSep; ///注释符
        };

        /**
         * @brief 配置文件类,一次性解析，这样对于读取多个键信息，速度快
         * 
         */
        class TMdbNtcIniParser:public TMdbNtcBaseObject
        {
            /** \example  example_TIniParser.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
             MDB_ZF_DECLARE_OBJECT(TMdbNtcIniParser);
        private:
            class TSectionInfo:public TMdbNtcBaseObject
            {
            public:
                TMdbNtcStringBuffer sSectionName;///< 段名
                TMdbNtcBaseList lsKeyInfo;///< 键信息链表
                TSectionInfo()
                {
                    lsKeyInfo.SetAutoRelease(true);
                }
                virtual TMdbNtcStringBuffer ToString( ) const;
                virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const;
                //~TSectionInfo();
            };
            class TKeyInfo:public TMdbNtcBaseObject
            {
            public:
                TMdbNtcStringBuffer sKeyName;///< 键名
                TMdbNtcStringBuffer sKeyValue;///< 键值
                TMdbNtcStringBuffer sComment;///< 注释信息
                virtual TMdbNtcStringBuffer ToString( ) const;
                virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const;
            };
        public:
            TMdbNtcIniParser();
            ~TMdbNtcIniParser();
            /**
             * @brief 解析一个ini配置文件
             * 
             * @param pszFilePath [in] 文件路径
             * @param cCommentChar [in] 注释分隔符,此符号后表示注释信息
             * @return int
             * @retval 0 成功
             */
            bool LoadFromFile(const char* pszFilePath, char cCommentChar = '#');
            /**
             * @brief 重新加载配置文件
             * 
             * @return int
             * @retval 0 成功
             */
            bool Reload();
            /**
             * @brief 删除节点信息
             * 
             * @return void
             */
            void Relese();
            /**
             * @brief 保存一个ini配置文件
             * 
             * @param pszFilePath [in] 文件路径
             * @param cCommentChar [in] 注释分隔符,此符号后表示注释信息
             * @return bool
             * @retval true 成功
             */
            bool SaveToFile(const char* pszFilePath, char cCommentChar = '#');
            /**
             * @brief 读取整形配置项，如果取值为空，则取默认值
             * 
             * @param pszSection [in] 段名
             * @param pszKey [in] 键名
             * @param iDefVal [in] 默认值
             * @return int
             * @retval 键值
             */
            int ReadInteger(const char *pszSection, const char *pszKey, int iDefVal = 0);
            /**
             * @brief 读取字符串配置项
             * 
             * @param pszSection [in] 段名
             * @param pszKey [in] 键名
             * @param pszDefVal [in] 默认值
             * @return TMdbNtcStringBuffer
             * @retval 键值
             */
            TMdbNtcStringBuffer ReadString(const char *pszSection, const char *pszKey, const char *pszDefVal = "");
            /**
             * @brief 写入整形配置项，如果取值为空，则取默认值
             * 
             * @param pszSection [in] 段名
             * @param pszKey [in] 键名
             * @param iVal [in] 键值
             */
            void WriteInteger(const char *pszSection, const char *pszKey, int iVal);
            /**
             * @brief 写入字符串配置项
             * 
             * @param pszSection [in] 段名
             * @param pszKey [in] 键名
             * @param pszVal [in] 键值
             */
            void WriteString(const char *pszSection, const char *pszKey, const char *pszVal);
            /**
             * @brief 判断section是否存在
             * 
             * @param pszSection [in] 段名
             * @return bool
             * @retval true 存在
             */
            bool SectionCheck(const char *pszSection);
            /**
             * @brief 判断键名是否存在
             * 
             * @param pszSection [in] 段名
             * @param pszKey [in] 键名
             * @return bool
             * @retval true 存在
             */
            bool KeyCheck(const char *pszSection, const char *pszKey);
            /**
             * @brief 移除section
             * 
             * @param pszSection [in] 段名
             */
            void RemoveSection(const char *pszSection);
            /**
             * @brief 移除键
             * 
             * @param pszSection [in] 段名
             * @param pszKey [in] 键名
             */
            void RemoveKey(const char *pszSection, const char *pszKey);
        protected:
            /**
             * @brief根据key值查找对应的迭代器
             * 
             * @param list [in] 在这个list上查找key值
             * @param pszKey [in] 键名
             * @return TMdbNtcBaseList::iterator
             */
            TMdbNtcBaseList::iterator GetKeyIterator(TMdbNtcBaseList &list, const char *pszKey);

            /**
             * @brief 根据key值查找对应的迭代器
             * 
             * @param pszSection [in] 段名
             * @return TMdbNtcBaseList::iterator
             */
            TMdbNtcBaseList::iterator GetSectionIterator(const char *pszSection);
        protected:
            TMdbNtcStringBuffer m_sFilePath;///< 加载的文件路径            
            TMdbNtcBaseList m_lsSectionInfo;///< 由section组成的链表，按照文件顺序
            char m_cCommentSep;
        };
//}
#endif
