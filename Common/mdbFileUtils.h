/**
 * @file FileUtils.hxx
 * @brief �ļ���Ŀ¼������Ŀ¼ɨ�裬�ļ�����ص���
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
         * @brief �ļ�/Ŀ¼·��������
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
             * @brief �ж��Ƿ������ļ���Ŀ¼�Ƿ�����ͬ�ļ�ϵͳ
             * 
             * @param pszPath1 [in] ·��1
             * @param pszPath2 [in] ·��2
             * @return bool
             * @retval true ͬһ�ļ�ϵͳ
             */
            static bool IsSameFileSystem(const char *pszPath1, const char *pszPath2);
            /**
             * @brief ����ļ�����ʱ��
             * 
             * @param pszFilePath [in] �ļ�·��
             * @return MDB_INT64
             * @retval -1 ��ȡʧ��
             */
            static MDB_INT64 GetCreateTime(const char *pszFilePath);
            /**
             * @brief ����ļ��޸�ʱ��
             * 
             * @param pszFilePath [in] �ļ�·��
             * @return MDB_INT64
             * @retval -1 ��ȡʧ��
             */
            static MDB_INT64 GetModifyTime(const char *pszFilePath);
            /**
             * @brief ����ļ�������ʱ��
             * 
             * @param pszFilePath [in] �ļ�·��
             * @return MDB_INT64
             * @retval -1 ��ȡʧ��
             */
            static MDB_INT64 GetAccessTime(const char *pszFilePath);
            /**
             * @brief �ж�һ��·���Ƿ����
             * 
             * @param pszPath [in] ·��
             * @return bool
             * @retval true ����,false ������
             */    
            static bool IsExist(const char *pszPath);
            /**
             * @brief �ж��ļ��Ƿ����
             * 
             * @param pszFilePath [in] �ļ�·��
             * @return bool
             * @retval true ���ڣ�·�����������ļ���
             */    
            static bool IsFileExist(const char *pszFilePath);
            /**
             * @brief �ж�Ŀ¼�Ƿ����
             * 
             * @param pszDirPath [in] Ŀ¼·��
             * @return bool
             * @retval true ���ڣ�·����������Ŀ¼��
             */    
            static bool IsDirExist(const char *pszDirPath);
            /**
             * @brief ���ָ��Ŀ¼���Ƿ����ĳһ�ļ�(win32�²����ִ�Сд)
             * 
             * @param pszDirPath [in] Ŀ¼·��
             * @param pszFileName [in] �ļ�����,win32�²����ִ�Сд
             * @param bSearchSub [in] �Ƿ�������Ŀ¼
             * @return TMdbNtcStringBuffer
             * @retval �����Ϊ�գ����ʾ�ҵ����ļ�ȫ·��
             */
            static TMdbNtcStringBuffer SearchFile(const char* pszDirPath, const char* pszFileName, bool bSearchSub = false);
            /**
             * @brief ���ָ��Ŀ¼���Ƿ����ĳһ��Ŀ¼(win32�²����ִ�Сд)
             * 
             * @param pszDirPath [in] ��ǰĿ¼·��
             * @param pszSubDirName [in] ��Ŀ¼����,win32�²����ִ�Сд
             * @param bSearchSub [in] �Ƿ�������Ŀ¼
             * @return TMdbNtcStringBuffer
             * @retval �����Ϊ�գ����ʾ�ҵ�����Ŀ¼ȫ·��
             */
            static TMdbNtcStringBuffer SearchDir(const char* pszDirPath, const char* pszSubDirName, bool bSearchSub = false);
            /**
             * @brief ��õ�ǰ���ڵ�Ŀ¼
             * 
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetCurrentPath();
            /**
             * @brief ����ָ��·����ø���·��������'/'��'\\'�ָ�
             * 
             * @param pszPath [in] ·��
             * @param iLength [in] ·�����ȣ�-1��ʾ��'\0'��β�Զ�����
             * @return TMdbNtcStringBuffer
             * @retval ����·��
             */
            static TMdbNtcStringBuffer GetParentPath(const char* pszPath, int iLength  = -1 );
            /**
             * @brief ����ָ��·����ø���·��������'/'��'\\'�ָ�
             * 
             * @param sPath [in] ԭ·��
             * @return TMdbNtcStringBuffer
             * @retval ����·��
             */
            static TMdbNtcStringBuffer GetParentPath(TMdbNtcStringBuffer sPath)
            {
                return GetParentPath(sPath.c_str(), (int)sPath.GetLength());
            }
        };

        /**
         * @brief �ļ������ӿ���
         * 
         * ����ƽ����֮ǰ����������޷���þ��������Ϣ������
         */
        class TMdbNtcFileOper:public TMdbNtcPathOper
        {
            /** \example  example_TFileOper.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
        public:
            /**
             * @brief �ж�һ���ļ��Ƿ����
             * 
             * @param pszPath [in] �ļ�·��
             * @return bool
             * @retval true ����,false ������
             */    
            inline static bool IsExist(const char *pszPath)
            {
                return IsFileExist(pszPath);
            }
            /**
             * @brief �����ļ�������ļ������ڣ���ᴴ����СΪ0���ļ�
             * 
             * @param pszFilePath [in] Ŀ¼·��             
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool MakeFile(const char *pszFilePath);
            /**
             * @brief �������ļ����ƶ��ļ�
             * 
             * @param pszSrcFilePath [in] ԭ·��
             * @param pszDestFilePath [in] Ŀ��·��
             * @param bSameFileSystem [in] �Ƿ�Ϊ�ļ�ϵͳ�������ȷ������ͨ��IsSameFileSystem���ж�
             * @param uiFileSize [in] Դ�ļ����ʱ�Ÿ���(0����Դ�ļ���󶼸�������0���Դ�ļ���Сû�ﵽuiFileSize�˴�Rename���ò�����)
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool Rename(const char * pszSrcFilePath, const char * pszDestFilePath, bool bSameFileSystem = true, MDB_UINT64 uiFileSize = 0);
            /**
             * @brief ɾ���ļ�
             * 
             * @param pszFilePath [in] �ļ�·��
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool  Remove(const char *pszFilePath);
            /**
             * @brief �����ļ�
             * 
             * @param pszSrcFilePath [in] ԭ·��
             * @param pszDestFilePath [in] Ŀ��·��
             * @param bFailIfExists [in] ��������Ƿ�ʧ�ܣ�Ĭ���Ǵ����򸲸�
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool Copy(const char * pszSrcFilePath, const char * pszDestFilePath, bool bFailIfExists = false);
            /**
             * @brief ����ļ�������
             * 
             * @param pszFilePath [in] �ļ�·��
             * @param sContent    [out] ��õ����ļ�����
             * @return bool
             * @retval true �ɹ�
             */
            static bool ReadContent(const char *pszFilePath, TMdbNtcStringBuffer& sContent);
            /**
             * @brief ���ļ����Ը��Ƿ�ʽд������
             * 
             * @param pszFilePath [in] �ļ�·��
             * @param pszContent  [in] ��Ҫд�������
             * @param iLength  [in] ��Ҫд��ĳ��ȣ�-1��ʾ������'\0'��β
             * @return bool
             * @retval true �ɹ�
             */
            static bool WriteContent(const char *pszFilePath, const void* pszContent, int iLength = -1);
            /**
             * @brief ���ļ�����׷�ӷ�ʽд������
             * 
             * @param pszFilePath [in] �ļ�·��
             * @param pszContent  [in] ��Ҫд�������
             * @param iLength  [in] ��Ҫд��ĳ��ȣ�-1��ʾ������'\0'��β
             * @return bool
             * @retval true �ɹ�
             */
            static bool AppendContent(const char *pszFilePath, const void* pszContent, int iLength = -1);
            /**
             * @brief ��ȡ�ļ���׺��
             * 
             * @param pszFilePath [in] �ļ�·��
             * @return const char*
             * @retval NULL û�к�׺��
             */
            static const char* GetFileExt(const char * pszFilePath);
            /**
             * @brief ��·����ȡ���ļ���
             * 
             * @param pszFilePath [in] �ļ�·��
             * @return const char*
             */
            static const char* GetFileName(const char * pszFilePath);
            /**
             * @brief �õ�����·�����ļ���������Trimȥ��������Ч�ַ�
             * 
             * @param pszFilePath [in] �ļ�·��
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetPureFileName(const char * pszFilePath);
            /**
             * @brief ��ȡ�ļ���С
             * 
             * @param pszFilePath [in] �ļ�·��
             * @param iSize [out] �ļ���С
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool GetFileSize(const char *pszFilePath, MDB_UINT64& uiSize);
        #ifndef OS_WINDOWS
            /**
             * @brief UNIX ƽ̨���滻ftok����·����ID��ȡIPCΨһ��ʶ
             * 
             * @param pszFilePath [in] �ļ�·��
             * @return key_t
             */
            static key_t Ftok(const char *pszFilePath);
        #endif
        };

        /**
         * @brief Ŀ¼�����ӿ���
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
             * @brief �ж�һ��Ŀ¼�Ƿ����
             * 
             * @param pszPath [in] Ŀ¼·��
             * @return bool
             * @retval true ����,false ������
             */    
            inline static bool IsExist(const char *pszPath)
            {
                return IsDirExist(pszPath);
            }
            /**
             * @brief ������Ŀ¼
             * 
             * @param pszDirPath [in] Ŀ¼·��             
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool MakeDir(const char *pszDirPath);
            /**
             * @brief ������������Ŀ¼���൱��linux/unix�µ�make -p,��֧�ֿ�ƽ̨
             * 
             * @param pszDirPath [in] Ŀ¼·��
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool MakeFullDir(const char *pszDirPath);
            /**
             * @brief �������ļ����ƶ��ļ�
             * 
             * @param pszSrcPath [in] ԭ·��
             * @param pszDestPath [in] Ŀ��·��
             * @param bSameFileSystem [in] �Ƿ�Ϊ�ļ�ϵͳ�������ȷ������ͨ��IsSameFileSystem���ж�
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool Rename(const char * pszSrcPath, const char * pszDestPath, bool bSameFileSystem = true);
            /**
             * @brief �����ļ�
             * ���Ŀ��·�����ڣ���Դ·������ΪĿ��Ŀ¼����Ŀ¼����֮Դ·������ΪĿ��·��
             * @param pszSrcDirPath [in] ԭĿ¼
             * @param pszDestDirPath [in] Ŀ��Ŀ¼
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool Copy(const char * pszSrcDirPath, const char * pszDestDirPath);
            /**
             * @brief ɾ��һ���ļ���
             * 
             * @param pszDirPath [in] Ŀ¼·��
             * @param bForce [in] �Ƿ�ǿ��ɾ��Ŀ¼��true��ʾǿ��ɾ���ǿ�Ŀ¼�����������ǿ�Ŀ¼ɾ����ʧ�ܡ�
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool  Remove(const char *pszDirPath, bool bForce = false);
            /**
             * @brief ȡ��Ŀ¼���ڷ������ÿռ��С���ֽ��� ��
             *
             * @param pszDirPath [in] Ŀ¼·��
             * @param uiSize     [in] Ŀ¼���ڷ������ÿռ��С
             * @return bool
             * @retval true �ɹ�,false ʧ��
             */
            static bool GetDiskFreeSpace(const char *pszDirPath, MDB_UINT64& uiSize);

        };

        /**
         * @brief �ļ�ɨ����
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
             * @brief ��ӹ��˹����ͨ���ģʽ
             * 
             * @param pszPattern [in] ���˵�ͨ���ģʽ(֧��?��*ͨ�����Ҳ֧������ͬʱ����ʹ��)
             *
             */
            void AddFilter(const char* pszPattern);
            /**
             * @brief ɾ��ĳһ�����˹��� 
             * 
             * @param pszPattern [in] ���˵�ͨ���ģʽ
             * @return bool
             * @retval true �ɹ�
             */
            bool DelFilter(const char* pszPattern);
            /**
             * @brief ������й��˹���
             * 
             */
            void ClearFilter()
            {
                m_lsFilterPattern.Clear();
            }
            /**
             * @brief �������ɨ�赽���ļ�
             * 
             */
            void ClearResult()
            {
                m_lsResultPath.Clear();
            }
            
            /**
             * @brief ��ճ�Ա���� 
             * 
             */
            void Clear();
            /**
             * @brief ɨ��Ŀ¼�µ��ļ�
             * 
             * �ڲ�ʵ�֣����ڵݹ���ң���Ҫ�ȱ�����Ŀ¼���رո�Ŀ¼������ٰ���������Ŀ¼��������ֹ��Ŀ¼����
             * @param pszScanPath    [in] ɨ��·��     
             * @param bIncludeSubDir [in] �Ƿ������Ŀ¼
             * @param iMaxNum        [in] �����Ŀ����,-1��ʾ������
             * @param bClearPrevFlag [in] �Ƿ������ǰ��ɨ����(������˴�ɨ�費������ǰɨ����/��������˴�ɨ�������ǰɨ����)
             * @return bool
             * @retval true �ɹ�;false ʧ��
             */
            bool ScanFile(const char* pszScanPath, bool bIncludeSubDir = false, int iMaxNum = -1, bool bClearPrevFlag = true)
            {
                if( bClearPrevFlag )
                    ClearResult();
                return scanFile(pszScanPath, bIncludeSubDir, iMaxNum);
            }

            /**
             * @brief ɨ��Ŀ¼�µ���Ŀ¼
             * 
             * �ڲ�ʵ�֣����ڵݹ���ң���Ҫ�ȱ�����Ŀ¼���رո�Ŀ¼������ٰ���������Ŀ¼��������ֹ��Ŀ¼����
             * @param pszScanPath    [in] ɨ��·��     
             * @param bIncludeSubDir [in] �Ƿ������Ŀ¼
             * @param iMaxNum        [in] �����Ŀ����,-1��ʾ������
             * @param bClearPrevFlag [in] �Ƿ������ǰ��ɨ����(������˴�ɨ�費������ǰɨ����/��������˴�ɨ�������ǰɨ����)
             * @return bool
             * @retval true �ɹ�;false ʧ��
             */
            bool ScanDir(const char* pszScanPath, bool bIncludeSubDir = false, int iMaxNum = -1, bool bClearPrevFlag = true)
            {
                if( bClearPrevFlag )
                    ClearResult();
                return scanDir(pszScanPath, bIncludeSubDir, iMaxNum);
            }

            /**
             * @brief ��ɨ�赽���ļ���������Ԫ��������TStringObject
             * (��ϸ˵��)
             * @param bSortAsc [in] �������
             * @param oCompare [in] �ȽϺ���
             * @return void
             */
            void Sort(bool bSortAsc = true, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief ���´ӵ�һ���ļ���ʼ�����GetNext
             * ��֧�ֶ��߳�
             * 
             * @return ��     
             */
            inline void StartOver()
            {
                m_itCur = m_lsResultPath.IterBegin();
            }
            /**
             * @brief ��ȡ��һ��ɨ�赽���ļ�·��,�ӵ�һ����ʼ
             * ��֧�ֶ��߳�
             * 
             * @return const char*
             * @retval ���ΪNULL ��ʾ����
             */
            const char* GetNext();
            /**
             * @brief �õ����������·������
             * 
             * @return TMdbNtcBaseList&
             */
            TMdbNtcBaseList& GetResultList()
            {
                return m_lsResultPath;
            }
            /**
             * @brief ����ļ��б�Ŀ�ʼ������
             *      
             * @return iterator
             * @retval ��ʼ������
             */
            inline TMdbNtcContainer::iterator IterBegin() const
            {
                return m_lsResultPath.IterBegin();
            }
            /**
             * @brief ����ļ��б�Ľ���������
             *      
             * @return iterator
             * @retval ����������
             */
            inline TMdbNtcContainer::iterator IterEnd() const
            {
                return m_lsResultPath.IterEnd();
            }
            /**
             * @brief ��ȡ�ļ���Ŀ
             * 
             * @return MDB_UINT32
             */
            MDB_UINT32 GetCount() const
            {
                return m_lsResultPath.GetSize();
            }
            /**
             * @brief �ж������Ƿ���������
             * 
             * @param pszName [in] �ļ���Ŀ¼����
             * @return bool
             * @retval true ����
             */
            bool IsSatisfy(const char* pszName);
        private:
            bool scanFile(const char* pszScanPath, bool bIncludeSubDir, int iMaxNum);
            bool scanDir(const char* pszScanPath, bool bIncludeSubDir, int iMaxNum);
        protected:
            TMdbNtcStringBuffer     m_sScanPath;///< ɨ��·��
            TMdbNtcBaseList   m_lsFilterPattern;///< ����ģʽ������
            TMdbNtcBaseList   m_lsResultPath;///< ���·������
            TMdbNtcBaseList::iterator m_itCur;///< ��¼FilePath��ǰλ��
        };

        /** 
         * @brief �ļ�����װ��
         * 
         * ����ʱ�Զ��ͷ���
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
             * @brief TFileLock���캯��
             *
             * TFileLock���캯��
             *
             * @param pszFilePath [in] �ļ�·��
             */
            TMdbNtcFileLock(const char *pszFilePath);

            /** 
             * @brief ��������
             *
             * ����ʱ���Զ��������
             *
             */    
            ~TMdbNtcFileLock();
            /** 
             * @brief �ļ���
             *      
             * @param bBlock [in] �Ƿ�����ģʽ
             * @return bool
             * @retval true �ɹ���false ʧ��
             */
            bool Lock(bool bBlock = false);

            /** 
             * @brief �ͷ��ļ���  
             *
             * @return bool
             * @retval true �ɹ���false ʧ��
             */    
            bool Unlock();
        protected:
            TMdbNtcStringBuffer m_sFilePath;///< �ļ�·��
            bool    m_bLocked;  ///< �Ƿ�ӹ���
        #ifndef OS_WINDOWS
            int     m_hFile;
        #else
            HANDLE  m_hFile;
        #endif
        };
//}
#endif

