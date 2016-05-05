/**
 * @file IniFiles.hxx
 * @brief ��ini�ļ�������صķ�װ
 *
 * ��ini�ļ�������صķ�װ
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
         * @brief �����ļ���, ����֮ǰ�Ľӿ�
         * 
         * ÿ�ζ�ȡ���Ǵ��ļ�ͷ��ʼ���ң��ʺ�ֻ��ȡ�����ļ���Ϣ
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
             * @brief ���ļ�
             * 
             * @param pszFilePath [in] ini�ļ�·��
             * @param cCommentSep [in]ע�ͷ�
             */
            bool OpenFile(const char* pszFilePath, char cCommentSep = '#');
            /**
             * @brief �ر��ļ�
             *      
             */
            void CloseFile();
            /**
             * @brief ��ȡ������������ȡֵΪ�գ���ȡĬ��ֵ
             * 
             * @param pszSection [in] ����
             * @param pszKey [in] ����
             * @param iDefVal [in] Ĭ��ֵ
             * @return int
             * @retval ��ֵ
             */
            int ReadInteger(const char *pszSection, const char *pszKey, int iDefVal = 0);
            /**
             * @brief ��ȡ����������
             * 
             * @param pszSection [in] ����
             * @param pszKey [in] ����
             * @param pszDefVal [in] Ĭ��ֵ
             * @return TMdbNtcStringBuffer
             * @retval ��ֵ
             */
            TMdbNtcStringBuffer ReadString(const char *pszSection, const char *pszKey, const char *pszDefVal = "");
            /**
             * @brief �ж�section�Ƿ����
             * 
             * @param pszSection [in] ����
             * @return bool
             * @retval true ����
             */
            bool SectionCheck(const char *pszSection);
            /**
             * @brief �жϼ����Ƿ����
             * 
             * @param pszSection [in] ����
             * @param pszKey [in] ����
             * @return bool
             * @retval true ����
             */
            bool KeyCheck(const char *pszSection, const char *pszKey);
			/** @brief ���ļ�������������
			 */
			char *Decrypt(char * password);
        private:
            FILE* m_fp;///< �ļ�ָ��
            char  m_cCommentSep; ///ע�ͷ�
        };

        /**
         * @brief �����ļ���,һ���Խ������������ڶ�ȡ�������Ϣ���ٶȿ�
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
                TMdbNtcStringBuffer sSectionName;///< ����
                TMdbNtcBaseList lsKeyInfo;///< ����Ϣ����
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
                TMdbNtcStringBuffer sKeyName;///< ����
                TMdbNtcStringBuffer sKeyValue;///< ��ֵ
                TMdbNtcStringBuffer sComment;///< ע����Ϣ
                virtual TMdbNtcStringBuffer ToString( ) const;
                virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const;
            };
        public:
            TMdbNtcIniParser();
            ~TMdbNtcIniParser();
            /**
             * @brief ����һ��ini�����ļ�
             * 
             * @param pszFilePath [in] �ļ�·��
             * @param cCommentChar [in] ע�ͷָ���,�˷��ź��ʾע����Ϣ
             * @return int
             * @retval 0 �ɹ�
             */
            bool LoadFromFile(const char* pszFilePath, char cCommentChar = '#');
            /**
             * @brief ���¼��������ļ�
             * 
             * @return int
             * @retval 0 �ɹ�
             */
            bool Reload();
            /**
             * @brief ɾ���ڵ���Ϣ
             * 
             * @return void
             */
            void Relese();
            /**
             * @brief ����һ��ini�����ļ�
             * 
             * @param pszFilePath [in] �ļ�·��
             * @param cCommentChar [in] ע�ͷָ���,�˷��ź��ʾע����Ϣ
             * @return bool
             * @retval true �ɹ�
             */
            bool SaveToFile(const char* pszFilePath, char cCommentChar = '#');
            /**
             * @brief ��ȡ������������ȡֵΪ�գ���ȡĬ��ֵ
             * 
             * @param pszSection [in] ����
             * @param pszKey [in] ����
             * @param iDefVal [in] Ĭ��ֵ
             * @return int
             * @retval ��ֵ
             */
            int ReadInteger(const char *pszSection, const char *pszKey, int iDefVal = 0);
            /**
             * @brief ��ȡ�ַ���������
             * 
             * @param pszSection [in] ����
             * @param pszKey [in] ����
             * @param pszDefVal [in] Ĭ��ֵ
             * @return TMdbNtcStringBuffer
             * @retval ��ֵ
             */
            TMdbNtcStringBuffer ReadString(const char *pszSection, const char *pszKey, const char *pszDefVal = "");
            /**
             * @brief д��������������ȡֵΪ�գ���ȡĬ��ֵ
             * 
             * @param pszSection [in] ����
             * @param pszKey [in] ����
             * @param iVal [in] ��ֵ
             */
            void WriteInteger(const char *pszSection, const char *pszKey, int iVal);
            /**
             * @brief д���ַ���������
             * 
             * @param pszSection [in] ����
             * @param pszKey [in] ����
             * @param pszVal [in] ��ֵ
             */
            void WriteString(const char *pszSection, const char *pszKey, const char *pszVal);
            /**
             * @brief �ж�section�Ƿ����
             * 
             * @param pszSection [in] ����
             * @return bool
             * @retval true ����
             */
            bool SectionCheck(const char *pszSection);
            /**
             * @brief �жϼ����Ƿ����
             * 
             * @param pszSection [in] ����
             * @param pszKey [in] ����
             * @return bool
             * @retval true ����
             */
            bool KeyCheck(const char *pszSection, const char *pszKey);
            /**
             * @brief �Ƴ�section
             * 
             * @param pszSection [in] ����
             */
            void RemoveSection(const char *pszSection);
            /**
             * @brief �Ƴ���
             * 
             * @param pszSection [in] ����
             * @param pszKey [in] ����
             */
            void RemoveKey(const char *pszSection, const char *pszKey);
        protected:
            /**
             * @brief����keyֵ���Ҷ�Ӧ�ĵ�����
             * 
             * @param list [in] �����list�ϲ���keyֵ
             * @param pszKey [in] ����
             * @return TMdbNtcBaseList::iterator
             */
            TMdbNtcBaseList::iterator GetKeyIterator(TMdbNtcBaseList &list, const char *pszKey);

            /**
             * @brief ����keyֵ���Ҷ�Ӧ�ĵ�����
             * 
             * @param pszSection [in] ����
             * @return TMdbNtcBaseList::iterator
             */
            TMdbNtcBaseList::iterator GetSectionIterator(const char *pszSection);
        protected:
            TMdbNtcStringBuffer m_sFilePath;///< ���ص��ļ�·��            
            TMdbNtcBaseList m_lsSectionInfo;///< ��section��ɵ����������ļ�˳��
            char m_cCommentSep;
        };
//}
#endif
