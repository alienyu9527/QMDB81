/**
 * @file SysShareMQ.hxx
 * 进程间共享的消息队列（基于共享内存）
 * 
 * @author 
 * @version 1.0
 * @date 2013/05/20
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
 * @warning 
 */
#ifndef  _MDB_SYS_MESSAGES_HXX_
#define  _MDB_SYS_MESSAGES_HXX_
#include "Common/mdbSysShareMems.h"
#include "Common/mdbSysLocks.h"
//namespace QuickMDB
//{
        class TMdbNtcMessageQueue;
        /**
         * @brief 回调方式处理消息的函数类型声明
         * 
         * @param pMQ       [in] 发生覆盖的消息队列
         * @param uiType    [in] 消息类型
         * @param pBuffer   [in] 消息buffer
         * @param uiLength  [in] 消息长度
         * @param pArg      [in] 回调函数的参数
         */
        typedef void (*mdb_ntc_queue_msg_callback)(TMdbNtcMessageQueue* pMQ, MDB_UINT16 uiType, const void* pBuffer, MDB_UINT32 uiLength, void* pArg);
        /**
         * @brief 共享消息队列的组件
         * 
         */
        class TMdbNtcMessageQueue:public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcMessageQueue);
        protected:
            struct RECV_MSG
            {
                MDB_UINT16& uiType;
                TMdbNtcDataBuffer& sOut;
                RECV_MSG(MDB_UINT16& uiTypeParam, TMdbNtcDataBuffer& sOutParam)
                    :uiType(uiTypeParam), sOut(sOutParam)
                {
                }
            };
            static void recv_queue_msg_callback(TMdbNtcMessageQueue* pMQ, MDB_UINT16 uiType, const void* pBuffer, MDB_UINT32 uiLength, void* pArg);
        public:
            /**
             * @brief 创建消息队列
             * 
             * @param pszName  [in] 消息队列的名称
             * @param uiShmSize [in] 消息队列占用内存的大小，如果是附接的话，则无需传入size
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @param pParentNameObject [in] 所隶属的命名对象
             * @param bCheckVersion [in] 是否检查共享内存的版本，以免不同版本的共享内存结构差异导致访问异常
             * @return 无
             */
            TMdbNtcMessageQueue(const char* pszName, MDB_UINT32 uiShmSize = 0,
                const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject* pParentNameObject = NULL, bool bCheckVersion = true);
            ~TMdbNtcMessageQueue();
            /**
             * @brief 设置覆盖消息时的回调函数
             * 
             * @param func [in] 覆盖消息时的回调函数
             * @param pArg [in] 回调函数的参数
             */
            inline void SetMsgOverlappedCallback(mdb_ntc_queue_msg_callback func, void* pArg = NULL)
            {
                m_fnMsgOverlappedCallback = func;
                m_pMsgOverlappedArg = NULL;
            }
            /**
             * @brief 获得覆盖消息时的回调函数
             * 
             * @return msg_overlapped_callback
             * @retval 覆盖消息时的回调函数
             */
            inline mdb_ntc_queue_msg_callback GetMsgOverlappedCallback()
            {
                return m_fnMsgOverlappedCallback;
            }
            /**
             * @brief 判断当前消息队列是否仍然有效
             * 
             * @return bool
             * @retval true 有效
             */
            bool IsOK();
            /**
             * @brief 同步处理消息，无需拷贝buffer副本，提高效率
             * 如果处理比较耗时，可能引起消息积压较多，而导致覆盖方式压入消息时，阻塞至当前消息处理完毕。
             * 
             * @param deal_func     [in] 处理函数
             * @param pArg          [in] 回调时的参数
             * @param iMilliSeconds [in] 超时设置，单位毫秒,-1表示阻塞，0表示非阻塞，其他表示超时返回时间
             * @return bool
             * @retval true 收取到消息，并处理完毕
             */
            bool SyncDealMsg(mdb_ntc_queue_msg_callback deal_func, void* pArg = NULL, MDB_INT32 iMilliSeconds = -1);
            /**
             * @brief 压入一条消息
             *              
             * @param pBuffer [in] 要压入的消息buffer
             * @param uiLength   [in] 要压入的消息长度，-1表示以'\0'结尾
             * @param bAllowOverlap [in] 是否允许覆盖旧的未取出的消息
             * @return bool
             * @retval true 成功
             */
            inline bool Send(const void* pBuffer, MDB_UINT32 uiLength = -1, bool bAllowOverlap = false)
            {
                return Send(0, pBuffer, uiLength, bAllowOverlap);
            }

            /**
             * @brief 压入一条消息
             * 
             * @param uiType [in] 要压入的数据类型
             * @param pBuffer [in] 要压入的消息buffer
             * @param uiLength   [in] 要压入的消息长度，-1表示以'\0'结尾
             * @param bAllowOverlap [in] 是否允许覆盖旧的未取出的消息
             * @return bool
             * @retval true 成功
             */
            bool Send(MDB_UINT16 uiType, const void* pBuffer, MDB_UINT32 uiLength = -1, bool bAllowOverlap = false);

            /**
             * @brief 获取一条消息
             * 
             * @param sOut          [out] 输出的buffer
             * @param iMilliSeconds [in] 超时设置，单位毫秒,-1表示阻塞，0表示非阻塞，其他表示超时返回时间
             * @return bool
             * @retval true 成功
             */
            inline bool Receive(TMdbNtcDataBuffer& sOut, MDB_INT32 iMilliSeconds = -1)
            {
                MDB_UINT16 uiType = 0;
                RECV_MSG oRecvMsg(uiType, sOut);
                return SyncDealMsg(recv_queue_msg_callback, &oRecvMsg, iMilliSeconds);
            }

            /**
             * @brief 获取一条消息
             * 
             * @param uiType        [out] 数据的类型
             * @param sOut          [out] 输出的buffer
             * @param iMilliSeconds [in] 超时设置，单位毫秒,-1表示阻塞，0表示非阻塞，其他表示超时返回时间
             * @return bool
             * @retval true 成功
             */
            inline bool Receive(MDB_UINT16& uiType, TMdbNtcDataBuffer& sOut, MDB_INT32 iMilliSeconds = -1)
            {
                RECV_MSG oRecvMsg(uiType, sOut);
                return SyncDealMsg(recv_queue_msg_callback, &oRecvMsg, iMilliSeconds);
            }
            /**
             * @brief 判断当前队列是否为空
             * 
             * @return bool
             * @retval true 空队列
             */
            inline bool IsEmpty() const
            {
                return m_pMgrInfo?(m_pMgrInfo->uiPopTimes == m_pMgrInfo->uiPushTimes):true;
            }
            /**
             * @brief 获取队列中的消息数
             * 
             * @return unsigned int
             * @retval 消息数
             */
            MDB_UINT32 GetCount() const;

            /**
             * @brief 向队列中压入的消息数
             * 
             * @return MDB_UINT32
             * @retval 压入的消息数
             */
            inline MDB_UINT32 GetPushTimes() const
            {
                return m_pMgrInfo?m_pMgrInfo->uiPushTimes:0;
            }

            /**
             * @brief 从队列中取出的消息数
             * 
             * @return MDB_UINT32
             * @retval 取出的消息数
             */
            inline MDB_UINT32 GetPopTimes() const
            {
                return m_pMgrInfo?m_pMgrInfo->uiPopTimes:0;
            }
            /**
             * @brief 获得消息队列的容量字节数
             * 
             * @return MDB_UINT32
             * @retval 容量字节数
             */
            inline MDB_UINT32 GetTotalSize() const
            {
                return m_uiShmSize;
            }
            /**
             * @brief 获得当前消息队列已使用的字节数
             * 
             * @return MDB_UINT32
             * @retval 已使用的字节数
             */
            MDB_UINT32 GetUsedSize() const;
            /**
             * @brief 获得还剩余的字节数，可依次判断是否还放得下
             * 
             * @return MDB_UINT32
             */
            MDB_UINT32 GetFreeSize() const;
            /**
             * @brief 销毁消息队列
             * 
             * @return bool
             * @retval true 成功
             */
            virtual bool ManualFree();
        public://提供一些静态方法
            /**
             * @brief 检测消息队列是否存在
             * 
             * @param pszName [in] 消息队列的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 存在
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 销毁消息队列
             * 
             * @param pszName [in] 消息队列的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool Destroy(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        private:
            /**
             *@brief 设置消息头
             *
             *@param uiType [in] 消息类型
             *@param uiBodyLen [in] 消息体的长度
             *@return 无
             */
            void SetMsgHead( MDB_UINT16 uiType, MDB_UINT32 uiBodyLen);

            /**
             *@brief 获取消息头信息
             *
             *@param uiType [out] 消息类型
             *@param uiBodyLen [out] 消息体的长度
             *@return 无
             *@author zhou.qilong1@zte.com.cn
             */
            void GetMsgHead(MDB_UINT16& uiType, MDB_UINT32& uiBodyLen);
        private:
            /**
             *@brief 共享消息队列共享内存管理区的结构
             */
            struct TMQMgrInfo
            {
                MDB_UINT32  uiPushCursor;   ///< Push游标的值
                MDB_UINT32  uiPopCursor;    ///< Pop游标的值
                MDB_UINT32  uiPushTimes;    ///< 已Push的消息总数
                MDB_UINT32  uiPopTimes;     ///< 已Pop的消息总数
                MDB_UINT32  uiLastPushTime; ///< 最后压入的时间
                MDB_UINT32  uiLastPopTime;  ///< 最后获取的时间                
                char    szReserve[128]; ///< 保留一些空间，以备后用
            };
            TMdbNtcShareMem*      m_pShm;     ///< 消息队列上的消息所存放的共享内存
            TMdbNtcProcessLock*   m_pPushLock, *m_pPopLock;   ///< 消息队列上的互斥信号量
            TMdbNtcProcessEvent*  m_pEvent;   ///< 使用事件来完成进程间的通知
            TMQMgrInfo*     m_pMgrInfo;    ///<共享队列消息共享内存管理区信息
            unsigned char*  m_pContentAddr; ///<消息内容段的起始地址
            MDB_UINT32          m_uiShmSize;  ///<不包括统计区的消息共享队列的大小
            mdb_ntc_queue_msg_callback  m_fnMsgOverlappedCallback;///< 覆盖消息时回调函数
            void*           m_pMsgOverlappedArg;///< 覆盖消息时回调函数的参数
            static const MDB_UINT32 ms_uiMsgHeadLen;///< 每条消息的头长度
        };
//}
#endif
