/**
 * @file FileUtils.hxx
 * @brief 文件和目录操作，目录扫描，文件锁相关的类
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121214
 * @warning
 */
#ifndef _MDB_H_FileUtils_
#define _MDB_H_FileUtils_
#include "Common/mdbCommons.h"
#include "Common/mdbDataStructs.h"
//namespace QuickMDB
//{
        /**
         * @brief 文件/目录路径操作类
         * 
         */
        class TMdbNtcPathOper
        {
            /** \example  example_TPathOper.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
        public:
            /**
             * @brief 判断是否两个文件或目录是否处于相同文件系统
             * 
             * @param pszPath1 [in] 路径1
             * @param pszPath2 [in] 路径2
             * @return bool
             * @retval true 同一文件系统
             */
            static bool IsSameFileSystem(const char *pszPath1, const char *pszPath2);
            /**
             * @brief 获得文件创建时间
             * 
             * @param pszFilePath [in] 文件路径
             * @return MDB_INT64
             * @retval -1 获取失败
             */
            static MDB_INT64 GetCreateTime(const char *pszFilePath);
            /**
             * @brief 获得文件修改时间
             * 
             * @param pszFilePath [in] 文件路径
             * @return MDB_INT64
             * @retval -1 获取失败
             */
            static MDB_INT64 GetModifyTime(const char *pszFilePath);
            /**
             * @brief 获得文件最后访问时间
             * 
             * @param pszFilePath [in] 文件路径
             * @return MDB_INT64
             * @retval -1 获取失败
             */
            static MDB_INT64 GetAccessTime(const char *pszFilePath);
            /**
             * @brief 判断一个路径是否存在
             * 
             * @param pszPath [in] 路径
             * @return bool
             * @retval true 存在,false 不存在
             */    
            static bool IsExist(const char *pszPath);
            /**
             * @brief 判断文件是否存在
             * 
             * @param pszFilePath [in] 文件路径
             * @return bool
             * @retval true 存在（路径存在且是文件）
             */    
            static bool IsFileExist(const char *pszFilePath);
            /**
             * @brief 判断目录是否存在
             * 
             * @param pszDirPath [in] 目录路径
             * @return bool
             * @retval true 存在（路径存在且是目录）
             */    
            static bool IsDirExist(const char *pszDirPath);
            /**
             * @brief 检查指定目录下是否存在某一文件(win32下不区分大小写)
             * 
             * @param pszDirPath [in] 目录路径
             * @param pszFileName [in] 文件名称,win32下不区分大小写
             * @param bSearchSub [in] 是否搜索子目录
             * @return TMdbNtcStringBuffer
             * @retval 如果不为空，则表示找到的文件全路径
             */
            static TMdbNtcStringBuffer SearchFile(const char* pszDirPath, const char* pszFileName, bool bSearchSub = false);
            /**
             * @brief 检查指定目录下是否存在某一子目录(win32下不区分大小写)
             * 
             * @param pszDirPath [in] 当前目录路径
             * @param pszSubDirName [in] 子目录名称,win32下不区分大小写
             * @param bSearchSub [in] 是否搜索子目录
             * @return TMdbNtcStringBuffer
             * @retval 如果不为空，则表示找到的子目录全路径
             */
            static TMdbNtcStringBuffer SearchDir(const char* pszDirPath, const char* pszSubDirName, bool bSearchSub = false);
            /**
             * @brief 获得当前所在的目录
             * 
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetCurrentPath();
            /**
             * @brief 根据指定路径获得父级路径，根据'/'或'\\'分隔
             * 
             * @param pszPath [in] 路径
             * @param iLength [in] 路径长度，-1表示以'\0'结尾自动计算
             * @return TMdbNtcStringBuffer
             * @retval 父级路径
             */
            static TMdbNtcStringBuffer GetParentPath(const char* pszPath, int iLength  = -1 );
            /**
             * @brief 根据指定路径获得父级路径，根据'/'或'\\'分隔
             * 
             * @param sPath [in] 原路径
             * @return TMdbNtcStringBuffer
             * @retval 父级路径
             */
            static TMdbNtcStringBuffer GetParentPath(TMdbNtcStringBuffer sPath)
            {
                return GetParentPath(sPath.c_str(), (int)sPath.GetLength());
            }
        };

        /**
         * @brief 文件操作接口类
         * 
         * 此设计解决了之前操作出错后，无法获得具体出错信息的问题
         */
        class TMdbNtcFileOper:public TMdbNtcPathOper
        {
            /** \example  example_TFileOper.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
        public:
            /**
             * @brief 判断一个文件是否存在
             * 
             * @param pszPath [in] 文件路径
             * @return bool
             * @retval true 存在,false 不存在
             */    
            inline static bool IsExist(const char *pszPath)
            {
                return IsFileExist(pszPath);
            }
            /**
             * @brief 创建文件，如果文件不存在，则会创建大小为0的文件
             * 
             * @param pszFilePath [in] 目录路径             
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool MakeFile(const char *pszFilePath);
            /**
             * @brief 重命名文件，移动文件
             * 
             * @param pszSrcFilePath [in] 原路径
             * @param pszDestFilePath [in] 目标路径
             * @param bSameFileSystem [in] 是否为文件系统，如果不确定可以通过IsSameFileSystem来判断
             * @param uiFileSize [in] 源文件多大时才改名(0不管源文件多大都改名，非0如果源文件大小没达到uiFileSize此次Rename调用不改名)
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool Rename(const char * pszSrcFilePath, const char * pszDestFilePath, bool bSameFileSystem = true, MDB_UINT64 uiFileSize = 0);
            /**
             * @brief 删除文件
             * 
             * @param pszFilePath [in] 文件路径
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool  Remove(const char *pszFilePath);
            /**
             * @brief 拷贝文件
             * 
             * @param pszSrcFilePath [in] 原路径
             * @param pszDestFilePath [in] 目标路径
             * @param bFailIfExists [in] 如果存在是否失败，默认是存在则覆盖
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool Copy(const char * pszSrcFilePath, const char * pszDestFilePath, bool bFailIfExists = false);
            /**
             * @brief 获得文件的内容
             * 
             * @param pszFilePath [in] 文件路径
             * @param sContent    [out] 获得到的文件内容
             * @return bool
             * @retval true 成功
             */
            static bool ReadContent(const char *pszFilePath, TMdbNtcStringBuffer& sContent);
            /**
             * @brief 往文件中以覆盖方式写入内容
             * 
             * @param pszFilePath [in] 文件路径
             * @param pszContent  [in] 需要写入的内容
             * @param iLength  [in] 需要写入的长度，-1表示内容以'\0'结尾
             * @return bool
             * @retval true 成功
             */
            static bool WriteContent(const char *pszFilePath, const void* pszContent, int iLength = -1);
            /**
             * @brief 往文件中以追加方式写入内容
             * 
             * @param pszFilePath [in] 文件路径
             * @param pszContent  [in] 需要写入的内容
             * @param iLength  [in] 需要写入的长度，-1表示内容以'\0'结尾
             * @return bool
             * @retval true 成功
             */
            static bool AppendContent(const char *pszFilePath, const void* pszContent, int iLength = -1);
            /**
             * @brief 获取文件后缀名
             * 
             * @param pszFilePath [in] 文件路径
             * @return const char*
             * @retval NULL 没有后缀名
             */
            static const char* GetFileExt(const char * pszFilePath);
            /**
             * @brief 从路径中取出文件名
             * 
             * @param pszFilePath [in] 文件路径
             * @return const char*
             */
            static const char* GetFileName(const char * pszFilePath);
            /**
             * @brief 得到不含路径的文件名，经过Trim去掉左右无效字符
             * 
             * @param pszFilePath [in] 文件路径
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetPureFileName(const char * pszFilePath);
            /**
             * @brief 获取文件大小
             * 
             * @param pszFilePath [in] 文件路径
             * @param iSize [out] 文件大小
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool GetFileSize(const char *pszFilePath, MDB_UINT64& uiSize);
        #ifndef OS_WINDOWS
            /**
             * @brief UNIX 平台下替换ftok根据路径和ID获取IPC唯一标识
             * 
             * @param pszFilePath [in] 文件路径
             * @return key_t
             */
            static key_t Ftok(const char *pszFilePath);
        #endif
        };

        /**
         * @brief 目录操作接口类
         * 
         */
        class TMdbNtcDirOper:public TMdbNtcPathOper
        {
            /** \example  example_TDirOper.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
        public:
            /**
             * @brief 判断一个目录是否存在
             * 
             * @param pszPath [in] 目录路径
             * @return bool
             * @retval true 存在,false 不存在
             */    
            inline static bool IsExist(const char *pszPath)
            {
                return IsDirExist(pszPath);
            }
            /**
             * @brief 创建新目录
             * 
             * @param pszDirPath [in] 目录路径             
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool MakeDir(const char *pszDirPath);
            /**
             * @brief 创建完整的新目录，相当于linux/unix下的make -p,且支持跨平台
             * 
             * @param pszDirPath [in] 目录路径
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool MakeFullDir(const char *pszDirPath);
            /**
             * @brief 重命名文件，移动文件
             * 
             * @param pszSrcPath [in] 原路径
             * @param pszDestPath [in] 目标路径
             * @param bSameFileSystem [in] 是否为文件系统，如果不确定可以通过IsSameFileSystem来判断
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool Rename(const char * pszSrcPath, const char * pszDestPath, bool bSameFileSystem = true);
            /**
             * @brief 拷贝文件
             * 如果目的路径存在，则源路径拷贝为目的目录的子目录，反之源路径拷贝为目的路径
             * @param pszSrcDirPath [in] 原目录
             * @param pszDestDirPath [in] 目标目录
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool Copy(const char * pszSrcDirPath, const char * pszDestDirPath);
            /**
             * @brief 删除一个文件夹
             * 
             * @param pszDirPath [in] 目录路径
             * @param bForce [in] 是否强制删除目录，true表示强制删除非空目录，否则遇到非空目录删除会失败。
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool  Remove(const char *pszDirPath, bool bForce = false);
            /**
             * @brief 取得目录所在分区可用空间大小（字节数 ）
             *
             * @param pszDirPath [in] 目录路径
             * @param uiSize     [in] 目录所在分区可用空间大小
             * @return bool
             * @retval true 成功,false 失败
             */
            static bool GetDiskFreeSpace(const char *pszDirPath, MDB_UINT64& uiSize);

        };

        /**
         * @brief 文件扫描器
         * 
         */
        class TMdbNtcFileScanner:public TMdbNtcBaseObject
        {
            /** \example  example_TFileScanner.cpp
             * This is an example of how to use the TMdbNtcFileScanner class.
             * More details about this example.
             */
             MDB_ZF_DECLARE_OBJECT(TMdbNtcFileScanner);
        public:
            TMdbNtcFileScanner();
            ~TMdbNtcFileScanner();
            /**
             * @brief 添加过滤规则的通配符模式
             * 
             * @param pszPattern [in] 过滤的通配符模式(支持?和*通配符，也支持两个同时并存使用)
             *
             */
            void AddFilter(const char* pszPattern);
            /**
             * @brief 删除某一条过滤规则 
             * 
             * @param pszPattern [in] 过滤的通配符模式
             * @return bool
             * @retval true 成功
             */
            bool DelFilter(const char* pszPattern);
            /**
             * @brief 清空所有过滤规则
             * 
             */
            void ClearFilter()
            {
                m_lsFilterPattern.Clear();
            }
            /**
             * @brief 清空所有扫描到的文件
             * 
             */
            void ClearResult()
            {
                m_lsResultPath.Clear();
            }
            
            /**
             * @brief 清空成员数据 
             * 
             */
            void Clear();
            /**
             * @brief 扫描目录下的文件
             * 
             * 内部实现：对于递归查找，需要先保存子目录，关闭父目录句柄后，再挨个查找子目录，这样防止打开目录过多
             * @param pszScanPath    [in] 扫描路径     
             * @param bIncludeSubDir [in] 是否包含子目录
             * @param iMaxNum        [in] 最大数目限制,-1表示不限制
             * @param bClearPrevFlag [in] 是否清除以前的扫描结果(清除即此次扫描不叠加以前扫描结果/不清除即此次扫描叠加以前扫描结果)
             * @return bool
             * @retval true 成功;false 失败
             */
            bool ScanFile(const char* pszScanPath, bool bIncludeSubDir = false, int iMaxNum = -1, bool bClearPrevFlag = true)
            {
                if( bClearPrevFlag )
                    ClearResult();
                return scanFile(pszScanPath, bIncludeSubDir, iMaxNum);
            }

            /**
             * @brief 扫描目录下的子目录
             * 
             * 内部实现：对于递归查找，需要先保存子目录，关闭父目录句柄后，再挨个查找子目录，这样防止打开目录过多
             * @param pszScanPath    [in] 扫描路径     
             * @param bIncludeSubDir [in] 是否包含子目录
             * @param iMaxNum        [in] 最大数目限制,-1表示不限制
             * @param bClearPrevFlag [in] 是否清除以前的扫描结果(清除即此次扫描不叠加以前扫描结果/不清除即此次扫描叠加以前扫描结果)
             * @return bool
             * @retval true 成功;false 失败
             */
            bool ScanDir(const char* pszScanPath, bool bIncludeSubDir = false, int iMaxNum = -1, bool bClearPrevFlag = true)
            {
                if( bClearPrevFlag )
                    ClearResult();
                return scanDir(pszScanPath, bIncludeSubDir, iMaxNum);
            }

            /**
             * @brief 对扫描到的文件进行排序，元素类型是TStringObject
             * (详细说明)
             * @param bSortAsc [in] 升序与否
             * @param oCompare [in] 比较函数
             * @return void
             */
            void Sort(bool bSortAsc = true, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief 重新从第一个文件开始，针对GetNext
             * 不支持多线程
             * 
             * @return 无     
             */
            inline void StartOver()
            {
                m_itCur = m_lsResultPath.IterBegin();
            }
            /**
             * @brief 获取下一个扫描到的文件路径,从第一个开始
             * 不支持多线程
             * 
             * @return const char*
             * @retval 如果为NULL 表示结束
             */
            const char* GetNext();
            /**
             * @brief 得到搜索结果的路径链表
             * 
             * @return TMdbNtcBaseList&
             */
            TMdbNtcBaseList& GetResultList()
            {
                return m_lsResultPath;
            }
            /**
             * @brief 获得文件列表的开始迭代器
             *      
             * @return iterator
             * @retval 开始迭代器
             */
            inline TMdbNtcContainer::iterator IterBegin() const
            {
                return m_lsResultPath.IterBegin();
            }
            /**
             * @brief 获得文件列表的结束迭代器
             *      
             * @return iterator
             * @retval 结束迭代器
             */
            inline TMdbNtcContainer::iterator IterEnd() const
            {
                return m_lsResultPath.IterEnd();
            }
            /**
             * @brief 获取文件数目
             * 
             * @return MDB_UINT32
             */
            MDB_UINT32 GetCount() const
            {
                return m_lsResultPath.GetSize();
            }
            /**
             * @brief 判断名称是否满足条件
             * 
             * @param pszName [in] 文件或目录名称
             * @return bool
             * @retval true 满足
             */
            bool IsSatisfy(const char* pszName);
        private:
            bool scanFile(const char* pszScanPath, bool bIncludeSubDir, int iMaxNum);
            bool scanDir(const char* pszScanPath, bool bIncludeSubDir, int iMaxNum);
        protected:
            TMdbNtcStringBuffer     m_sScanPath;///< 扫描路径
            TMdbNtcBaseList   m_lsFilterPattern;///< 过滤模式的链表
            TMdbNtcBaseList   m_lsResultPath;///< 结果路径链表
            TMdbNtcBaseList::iterator m_itCur;///< 记录FilePath当前位置
        };

        /** 
         * @brief 文件锁封装类
         * 
         * 析构时自动释放锁
         */
        class TMdbNtcFileLock:public TMdbNtcBaseObject
        {
            /** \example  example_TFileLock.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
             MDB_ZF_DECLARE_OBJECT(TMdbNtcFileLock);
        public:
            /** 
             * @brief TFileLock构造函数
             *
             * TFileLock构造函数
             *
             * @param pszFilePath [in] 文件路径
             */
            TMdbNtcFileLock(const char *pszFilePath);

            /** 
             * @brief 析构函数
             *
             * 析构时，自动解除锁定
             *
             */    
            ~TMdbNtcFileLock();
            /** 
             * @brief 文件锁
             *      
             * @param bBlock [in] 是否阻塞模式
             * @return bool
             * @retval true 成功，false 失败
             */
            bool Lock(bool bBlock = false);

            /** 
             * @brief 释放文件锁  
             *
             * @return bool
             * @retval true 成功，false 失败
             */    
            bool Unlock();
        protected:
            TMdbNtcStringBuffer m_sFilePath;///< 文件路径
            bool    m_bLocked;  ///< 是否加过锁
        #ifndef OS_WINDOWS
            int     m_hFile;
        #else
            HANDLE  m_hFile;
        #endif
        };
//}
#endif

