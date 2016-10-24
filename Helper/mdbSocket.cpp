////////////////////////////////////////////////
// Name: Socket.cpp
// Author: jiang.mingjun
// Date: 2010/12/2
// Description: Socket ��װ��
////////////////////////////////////////////////
#include "mdbSocket.h"
#include "Helper/mdbErr.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbDateTime.h"

#ifndef WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#else
#include <windows.h>
#endif



using namespace std;

//namespace QuickMDB{

    /******************************************************************************
    * ��������	:  Create()
    * ��������	:  ����socket
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�<0
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int Socket::Create()
    {
        m_iListenSocketID = ::socket(AF_INET, SOCK_STREAM, 0);
        if(m_iListenSocketID == -1)
        {
            TADD_ERROR(ERR_NET_CREATE_SOCKET,"socket() failed. errno=%d, errmsg=%s.", errno, strerror(errno));
            return ERR_NET_CREATE_SOCKET;
        }
        return 0;
    }

    /******************************************************************************
    * ��������	:  Bind()
    * ��������	:  �󶨶˿�
    * ����		:  iPortID, �˿�
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�<0
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int  Socket::Bind(int iPortID)
    {
        memset(&m_tAddr, 0, sizeof(m_tAddr));
        m_tAddr.sin_family      = AF_INET;
        m_tAddr.sin_port        = htons((uint16_t)iPortID);
        m_tAddr.sin_addr.s_addr = INADDR_ANY;

        //��������״̬����
        int iTempBool = -1;
        int iRet = setsockopt(m_iListenSocketID, SOL_SOCKET, SO_REUSEADDR, (char *)&iTempBool, sizeof(int));
        if(iRet == -1)
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"setsockopt() failed. errno=%d, errmsg=%s.",
                       errno, strerror(errno));
            return ERR_NET_SET_ATTR;
        }
        
        //��
        iRet = ::bind(m_iListenSocketID, (struct sockaddr *)&m_tAddr, sizeof(m_tAddr));
        if(iRet == -1)
        {
            TADD_ERROR(ERR_NET_BIND_PORT,"Failed to bind port[%d], errno=%d, errmsg=%s.",iPortID,errno, strerror(errno));
            return ERR_NET_BIND_PORT;
        }

        return 0;
    }

    /******************************************************************************
    * ��������	:  Listen()
    * ��������	:  ����������(�����ʹ��)
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�<0
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int Socket::Listen()
    {

        //����
        int iRet = listen(m_iListenSocketID, FD_SETSIZE);
        if(iRet == -1)
        {
            TADD_ERROR(ERR_NET_LISTEN_PORT,"listen() failed. errno=%d, errmsg=%s.",
                       errno, strerror(errno));
            return ERR_NET_LISTEN_PORT;
        }
        return 0;
    }

    /******************************************************************************
    * ��������	:  Accept()
    * ��������	:  ��ȡ��������
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0/1(�����ڴ�����Ϊ0,���ڷ�����Ϊ1)�����򷵻�<0
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int Socket::Accept()
    {
        //�Է���ַ
        int iLen = sizeof(struct sockaddr);
#ifdef WIN32
        int iConnectfd = accept(m_iListenSocketID, (struct sockaddr*)&m_tAddTmp, &iLen);
#else
#ifdef _HP
        int iConnectfd = accept(m_iListenSocketID, (struct sockaddr*)&m_tAddTmp, &iLen);
#else
        int iConnectfd = accept(m_iListenSocketID, (struct sockaddr*)&m_tAddTmp, (socklen_t*)&iLen);
#endif
#endif

        //if(iConnectfd == -1)
        if(iConnectfd < 0)
        {
            TADD_ERROR(ERR_NET_CREATE_SOCKET,"accept failed. errno=%d, errmsg=%s.",
                       errno, strerror(errno));
            CloseListenSocket();
            return ERR_NET_CREATE_SOCKET;

        }

        m_iSocketID = iConnectfd;
        int iRet = 0;


    //#ifndef _AIX
#if defined(_LINUX)||defined(_WIN32)
#ifndef _LINUX
        int iTimeout=300;//300��
        int  timeout = iTimeout*1000;//1000=1��
#else
        int iTimeout=300;//300��
        struct timeval timeout = {iTimeout,0};
#endif
        //int nNetTimeout=1000;//1�� // AIXƽ̨�ݲ�֧�ֳ�ʱ����
        iRet = setsockopt(m_iSocketID, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout,sizeof(timeout));
        if (iRet == -1)
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"set server SO_RCVTIMEO failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
            return ERR_NET_SET_ATTR;
        }
        iRet = setsockopt(m_iSocketID, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout,sizeof(timeout));
        if (iRet == -1)
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"set server SO_SNDTIMEO failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
            return ERR_NET_SET_ATTR;
        }
#endif

        int nZero=32*1024;
        iRet = setsockopt(m_iSocketID,SOL_SOCKET,SO_SNDBUF,(char *)&nZero,sizeof(int));
        if (iRet == -1)
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"set server SO_SNDBUF failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
            return ERR_NET_SET_ATTR;
        }
        iRet = setsockopt(m_iSocketID,SOL_SOCKET,SO_RCVBUF,(char *)&nZero,sizeof(int));
        if (iRet == -1)
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"set server SO_RCVBUF failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
            return ERR_NET_SET_ATTR;
        }

        //#ifdef _LINUX
        // Disable the Nagle (TCP No Delay) algorithm
        int flag = 1;
        iRet = setsockopt(m_iSocketID, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
        if (iRet == -1)
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"Disable server Nagle failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
            return ERR_NET_SET_ATTR;
        }
        //#endif

        struct linger mylinger;
        mylinger.l_onoff  = 1;
        mylinger.l_linger = 0;

        iRet = setsockopt(m_iSocketID, SOL_SOCKET, SO_LINGER, (char *) &mylinger,sizeof(struct linger));
        if (iRet == -1)
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"set server linger failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
            return ERR_NET_SET_ATTR;
        }

        int optval = 1;
        iRet = setsockopt(m_iSocketID, SOL_SOCKET, SO_KEEPALIVE, (char *) &optval,sizeof(int));
        if (iRet == -1)
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"set server KEEPALIVE failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
            return ERR_NET_SET_ATTR;
        }


        return iConnectfd;
    }

    /******************************************************************************
    * ��������	:  Close()
    * ��������	:  �ر�socket
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  jiang.mingjun
    *******************************************************************************/
    void Socket::Close()
    {
        shutdown( m_iSocketID,2 );
        close( m_iSocketID );
        m_iSocketID = -1;
    }

    /******************************************************************************
    * ��������	:  CloseListenSocket()
    * ��������	:  �رռ�ص�socket
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  jiang.mingjun
    *******************************************************************************/
    void Socket::CloseListenSocket()
    {
        shutdown( m_iListenSocketID,2 );
        close( m_iListenSocketID );
        m_iListenSocketID = -1;
        FD_ZERO(&m_rSet);
    }

    /******************************************************************************
    * ��������	:  connect()
    * ��������	:  ��TCP������������
    * ����		:  sRemoteHostIP, �����IP��ַ
    * ����		:  iRemotePortID, ����˼����˿ڣ�iTimeout ��ʱʱ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����socket���Ӿ��,���򷵻�<0
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int Socket::connect(char *sRemoteHostIP, int iRemotePortID,int iTimeout)
    {
        struct sockaddr_in ServerAddr;
        struct linger mylinger;
        int iSocketId,iResult,optval=1;
        //int nNetTimeout= 1000*iTimeout;//10000=10��
        int nZero=32*1024;
        //int nZero=0;
        ServerAddr.sin_family         = AF_INET;
        ServerAddr.sin_addr.s_addr    = inet_addr( sRemoteHostIP );
        ServerAddr.sin_port           = htons( (uint16_t)iRemotePortID );
        iSocketId = socket(AF_INET,SOCK_STREAM,0);
        if(iSocketId < 0)
        {
            TADD_ERROR (ERR_NET_CREATE_SOCKET,"Create Socket[%d] error,errno=%d,strerror=%s\n",iSocketId,errno,strerror(errno));
            return ERR_NET_CREATE_SOCKET;
        }
        while( 1 )
        {
            int iRet = 0;

            mylinger.l_onoff  = 1;
            mylinger.l_linger = 0;

            iRet = setsockopt(iSocketId, SOL_SOCKET, SO_LINGER,
                              (char *) &mylinger,sizeof(struct linger));
            if (iRet == -1)
            {
                TADD_ERROR(ERR_NET_SET_ATTR,"set client linger failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
                return ERR_NET_SET_ATTR;
            }


#ifndef DEF_LINUX
            sigset(SIGPIPE,SIG_IGN);
#else
            signal(SIGPIPE,SIG_IGN);
#endif
            optval = 1;
            iRet = setsockopt(iSocketId, SOL_SOCKET, SO_KEEPALIVE, (char *) &optval,sizeof(int));
            if (iRet == -1)
            {
                TADD_ERROR(ERR_NET_SET_ATTR,"set client KEEPALIVE failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
                return ERR_NET_SET_ATTR;
            }

    //#ifndef _AIX
#if defined(_LINUX)||defined(_WIN32)
#ifndef _LINUX
            int  timeout = iTimeout*1000;//1000=1��
#else
            struct timeval timeout = {iTimeout,0};
#endif
            //int nNetTimeout=1000;//1�� // AIX SUN��ƽ̨�ݲ�֧��
            iRet = setsockopt(iSocketId, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout,sizeof(timeout));
            if (iRet == -1)
            {
                TADD_ERROR(ERR_NET_SET_ATTR,"set client SO_RCVTIMEO failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
                return ERR_NET_SET_ATTR;
            }
            iRet = setsockopt(iSocketId, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout,sizeof(timeout));
            if (iRet == -1)
            {
                TADD_ERROR(ERR_NET_SET_ATTR,"set client SO_SNDTIMEO failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
                return ERR_NET_SET_ATTR;
            }
#endif
            //int nZero=0;
            //���⻺��������
            iRet = setsockopt(iSocketId,SOL_SOCKET,SO_SNDBUF,(char *)&nZero,sizeof(int));
            if (iRet == -1)
            {
                TADD_ERROR(ERR_NET_SET_ATTR,"set client SO_SNDBUF failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
                return ERR_NET_SET_ATTR;
            }
            iRet = setsockopt(iSocketId,SOL_SOCKET,SO_RCVBUF,(char *)&nZero,sizeof(int));
            if (iRet == -1)
            {
                TADD_ERROR(ERR_NET_SET_ATTR,"set client SO_RCVBUF failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
                return ERR_NET_SET_ATTR;
            }

            // Disable the Nagle (TCP No Delay) algorithm
            int flag = 1;
            iRet = setsockopt(iSocketId, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
            if (iRet == -1)
            {
                TADD_ERROR(ERR_NET_SET_ATTR,"Disable server Nagle failed,setsockopt() failed. errno=%d, errmsg=%s.",errno, strerror(errno));
                return ERR_NET_SET_ATTR;
            }
            iResult = /*std*/::connect(iSocketId,(struct sockaddr*)&ServerAddr,
                                       sizeof(ServerAddr));
            if( iResult != 0 )
            {
                shutdown( iSocketId,2 );
                close( iSocketId );
                if( errno == ECONNREFUSED )
                {
                    sleep(5);
                    continue;
                }
                //printf("Connect error\n");
                TADD_ERROR (ERR_NET_PEER_REFUSE,"[%s:%d] Connect error,errno=%d,strerror=%s\n",__FILE__,__LINE__,errno,strerror(errno));
                return ERR_NET_PEER_REFUSE;
            }
            else break;
        }
        m_iSocketID = iSocketId;
        return iSocketId;

    }

    /******************************************************************************
    * ��������	:  Select()
    * ��������	:  ����ĳ����ĳЩ�����״̬�仯����ȡ�ɶ����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0/1(�����ڴ�����Ϊ0,���ڷ�����Ϊ1)�����򷵻�<0
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int Socket::Select()
    {
        struct timeval tv;
        tv.tv_sec = 0;
        //tv.tv_usec = 50*1000;//΢������֮һ��
        tv.tv_usec = 20*10000;//΢������֮һ��


        FD_ZERO(&m_rSet);
        FD_SET(m_iListenSocketID, &m_rSet);

        int iReady = select(m_iListenSocketID+1, &m_rSet, NULL, NULL, &tv);
        return iReady;

    }

    /******************************************************************************
    * ��������	:  FdIsSet()
    * ��������	:  �ж�ĳ��socket�Ƿ���fd_set���͵ļ�����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  true �ڣ�false ����
    * ����		:  jiang.mingjun
    *******************************************************************************/
    bool Socket::FdIsSet()
    {
        if(FD_ISSET(m_iListenSocketID, &m_rSet))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    Socket::Socket()
    {
        m_iSocketID = -1;
        m_iListenSocketID = -1;
    }

    /******************************************************************************
    * ��������	:  SetSocketID()
    * ��������	:  ����socket
    * ����		:  iSocketID, socket���
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  jiang.mingjun
    *******************************************************************************/
    void Socket::SetSocketID(int iSocketID)
    {
        m_iSocketID = iSocketID;
    }

    Socket::~Socket()
    {
        m_iSocketID = -1;
    }

    /******************************************************************************
    * ��������	:  SetBlock()
    * ��������	:  ���ñ��׽ӿڵķ�������־
    * ����		:  iSocketID, socket���
    * ���		:  ��
    * ����ֵ	:  ����0 ���ɹ�  -1 ������
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int Socket::SetBlock(int iSocketID)
    {
        int iRet = -1;
#ifdef _SUN
        iRet = 0;
#else
        int ul = 0;
        iRet = ioctl(iSocketID, FIONBIO, &ul);
        if (iRet == -1)
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"Socket set unblock failed.\n");
            return ERR_NET_SET_ATTR;
        }
#endif
        return iRet;
    }


    /******************************************************************************
    * ��������	:  read()
    * ��������	:  ��socket
    * ����		:  iLen, ��ȡ�ֽ���
    * ���		:  sBuffer, ��ȡ������
    * ����ֵ	:  ���ؽ��>0 ��ʾ��ȡ���ֽ��������򷵻�ʧ����
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int Socket::read(unsigned char *sBuffer, int iLen)
    {
        int ret = -1;
        if (m_iSocketID < 0)
        {
            TADD_ERROR(ERR_NET_RECV_FAILED,"Socket not connected m_iSocketID=[%d]\n",m_iSocketID);
            return ERR_NET_RECV_FAILED;
        }
        int num=0;
        int iTest = 0;
        while( num < iLen )
        {
            ret = recv(m_iSocketID, &sBuffer[num], (size_t)(iLen - num),0);
            int iErrNo = errno;
            if ( ret < 0 )
            {
                //errno = EINTR ��ζ������
                if(iErrNo == EINTR || iErrNo == EWOULDBLOCK || iErrNo == EAGAIN)
                {
                    continue;
                }
                TADD_NORMAL("socket[%d] read error ,ret = %d ,errno=%d,strerror=%s\n",m_iSocketID,ret,iErrNo,strerror(iErrNo));
                ret = ERR_NET_RECV_FAILED;
                break;
            }
            else if ( ret == 0 && iErrNo!=0)
            {
                TADD_NORMAL ("socket[%d],read error=%d,msg=%s",m_iSocketID,iErrNo,strerror(iErrNo));
                ret = ERR_NET_SOCKET_CLOSE;
                break;
            }
            else
            {
                if(ret == 0)
                {
                    iTest ++;
                    TMdbDateTime::MSleep(10);
                    if(iTest > 0 && iTest % 100 == 0)
                    {
                        TADD_DETAIL("socket[%d],iTest=%d,error=%d,msg=%s",m_iSocketID,iTest,errno,strerror(errno));
                        ret = ERR_NET_SOCKET_CLOSE;
                        break;
                    }
                    continue;
                }
            }
            num+=ret;
        }
        return ret;
    }

    int Socket::read(int iSocket,char *sBuffer,int iLen)
    {
        m_iSocketID = iSocket;
        int ret = recv(m_iSocketID,sBuffer, iLen-1,0);
        if(ret < 0)
        {
            if(EWOULDBLOCK == errno || errno == EINTR || errno == EAGAIN)
            {
                return 0;
            }
            TADD_ERROR(ERR_NET_RECV_FAILED,"Socket::read error errno=%d,strerror=%s\n",errno,strerror(errno));
            ret = ERR_NET_RECV_FAILED;
            return ret;
        }
        else if ( ret == 0)
        {
            TADD_ERROR (ERR_NET_SOCKET_CLOSE,"Socket::read error=%d,msg=%s",errno,strerror(errno));
            ret = ERR_NET_SOCKET_CLOSE;
            return ret;
        }

        return ret;
    }


    /******************************************************************************
    * ��������	:  Recv()
    * ��������	:  ��socket
    * ����		:  iLen, buffer����
    * ���		:  sBuffer, ��������������Ŷ�ȡ������
    * ����ֵ	:  ���ؽ��>0 ��ʾ��ȡ���ֽ��������򷵻�ʧ����
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int Socket::Recv(unsigned char *sBuffer, int iLen)
    {
        int ret = -1;

        if (m_iSocketID < 0)
        {
            TADD_ERROR(ERR_NET_RECV_FAILED,"Socket not connected m_iSocketID=[%d]\n",m_iSocketID);
            return ERR_NET_RECV_FAILED;
        }
        while(true)
        {
            ret = recv(m_iSocketID, sBuffer,iLen,0);
            int iErrNo = errno;
            if ( ret < 0 )
            {
                //errno = EINTR ��ζ������
                if(iErrNo == EINTR || iErrNo == EWOULDBLOCK || iErrNo == EAGAIN)
                {
                    continue;
                }
                TADD_NORMAL("socket[%d] read error ,ret = %d ,errno=%d,strerror=%s\n",m_iSocketID,ret,iErrNo,strerror(iErrNo));
                ret = ERR_NET_RECV_FAILED;
                break;
            }
            else if ( ret == 0 && iErrNo!=0)
            {
                TADD_NORMAL ("socket[%d],read error=%d,msg=%s",m_iSocketID,iErrNo,strerror(iErrNo));
                ret = ERR_NET_SOCKET_CLOSE;
                break;
            }
            else
            {
                if(ret == 0)//�Զ������ر�
                {
                    TADD_WARNING("socket[%d],error=%d,msg=%s",m_iSocketID,errno,strerror(errno));
                    ret = ERR_NET_SOCKET_CLOSE;
                    break;
                }
            }
            break;
        }

        return ret;
    }


    /******************************************************************************
    * ��������	:  write()
    * ��������	:  дsocket
    * ����		:  sBuffer �������ݣ�iLen �����ֽڳ���
    * ���		:  ��
    * ����ֵ	:  ʵ�ʷ��͵��ֽ���
    * ����		:  jiang.mingjun
    *******************************************************************************/
    int Socket::write(unsigned char *sBuffer, int iLen)
    {
        //m_pPerfTestSend->begin();
        int num=0;
        int ret = 0;
        /*
        if(NULL == sBuffer)
        {
        	TADD_ERROR ("[%s:%d] Input parma is null.\n",__FILE__,__LINE__);
        	return ERR_APP_INVALID_PARAM;
        }
        */
        if (m_iSocketID < 0)
        {
            TADD_ERROR (ERR_NET_SEND_FAILED,"[%s:%d] Socket not connected m_iSocketID=[%d]\n",__FILE__,__LINE__,m_iSocketID);
            //return ERR_NET_CREATE_SOCKET;
            return ERR_NET_SEND_FAILED;
        }

        while( num < iLen )
        {
            errno = 0;
            ret = /*std*/::send (m_iSocketID, &sBuffer[num], (size_t)(iLen-num),0);
            if( ret < 0 )
            {
                if ((ret == -1) && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
                {
                    TADD_DETAIL("socket[%d],ret=[%d],len=[%d] read error errno=%d,strerror=%s\n",m_iSocketID,ret,iLen,errno,strerror(errno));
                    continue;
                }

                TADD_NORMAL("Socket::write error,ret = %d,errno=%d,strerror=%s\n",ret,errno,strerror(errno));
                return ERR_NET_SEND_FAILED;
            }
            num+=ret;
        }
        //m_pPerfTestSend->end();
        return num;
    }



    /******************************************************************************
    * ��������	:  Send()
    * ��������	:  ���ͳ���ΪiLen������
    * ����		:  sBuffer �������ݣ�iLen �����ֽڳ���
    * ���		:  ��
    * ����ֵ	:  ʵ�ʷ��͵��ֽ���
    * ����		:  zhang.lin
    *******************************************************************************/
    int Socket::Send(unsigned char *sBuffer, int iLen)
    {
        int num=0;
        int ret;
        if (m_iSocketID < 0)
        {
            TADD_ERROR (ERR_NET_SEND_FAILED,"Socket not connected m_iSocketID=[%d]\n",m_iSocketID);
            return ERR_NET_SEND_FAILED;
        }

        while( num < iLen )
        {
            errno = 0;
            ret = /*std*/::send (m_iSocketID, &sBuffer[num], (size_t)(iLen-num),0);
            if( ret < 0 )
            {
                TADD_NORMAL("Socket::write error,ret = %d,errno=%d,strerror=%s\n",ret,errno,strerror(errno));
                return ERR_NET_SEND_FAILED;
            }
            num+=ret;
        }
        return num;
    }

//}
