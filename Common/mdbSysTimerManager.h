/**
 * @file SysTimerManager.hxx
 * ��ʱ�����
 * 
 * @author jiang.jinzhou
 * @version 1.0
 * @date 2013/12/20
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
 * @warning 
 */
#ifndef _MDB_SYS_TIMER_MANAGER_H_
#define _MDB_SYS_TIMER_MANAGER_H_

#include "Common/mdbCommons.h"
#include "Common/mdbComponent.h"

#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include <sys/stat.h>

//namespace  QuickMDB
//{
        /**
         * @brief ��ʱ��������
         * 
         */
        class TMdbNtcTimerManager : public TMdbNtcComponentObject
        {
        public:
            /**
              * @brief ��ʼ��
              * @param bDynamic [in] �Ƿ���Ҫ��̬����
              * @retval  true�ɹ�/falseʧ��
              */            
            static bool Initialize( bool bDynamic = false );

            /**
              * @brief ������Զ�ʱ��
              * @param ATimerName [in] ��ʱ������
              * @param ALoopFlag [in] �Ƿ�ѭ����ʱ��
              * @param ACount100ms [in] ��ʱ����ִ�м��(��λ���ٺ���)
              * @param AEventFunc [in] ��ʱ������ָ��
              * @param pFuncParam [in] ��ʱ����������
              * @retval  >0�ɹ�/0ʧ��
              */
            static unsigned int AddRelativeTimer(const char *ATimerName,bool ALoopFlag,unsigned int ACount100ms,OnMdbEventFunc AEventFunc,void *pFuncParam);

            /**
              * @brief �������Զ�ʱ��
              * @param ATimerName [in] ��ʱ������
              * @param ATimerStr [in] ��ʱ����ִ��ʱ��
              * @param AEventFunc [in] ��ʱ������ָ��
              * @param pFuncParam [in] ��ʱ����������
              * @retval >0�ɹ�/0ʧ��
              */
            static unsigned int AddAbsTimer(const char *ATimerName,const char *ATimerStr,OnMdbEventFunc AEventFunc,void *pFuncParam);

            /**
              * @brief ɾ����ʱ��
              * @param ATimerName [in] ��ʱ������
              * @param ppFuncParam [out] ��ʱ������
              * @retval true�ɹ�/falseʧ��
              */
            static bool DelTimerByName(const char *ATimerName,void **ppFuncParam = NULL);

            /**
              * @brief ɾ����ʱ��
              * @param uiTimerId [in] ��ʱ�����
              * @param ppFuncParam [out] ��ʱ������
              * @retval true�ɹ�/falseʧ��
              */
            static bool DelTimerById(const unsigned int uiTimerId,void **ppFuncParam = NULL);
            
            /**
              * @brief ɾ���붨ʱ����ص������߳�
            */            
            static void KillAllThr();
        public:
            static unsigned int TimerId;
            static bool bHasInitialize;
        };

//}

#endif
