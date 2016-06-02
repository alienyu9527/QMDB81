/****************************************************************************************
*@Copyrights  2010，中兴软创（南京）计算机有限公司 开发部 CCB项目组
*@                   All rights reserved.
*@Name：	      mdbCSPParser.cpp
*@Description：   CSP编码解码
*@Author:		  li.shugang,jiangmingjun
*@Date：	      2010年10月21日
*@History:
******************************************************************************************/
#include "Helper/mdbCSPParser.h"
#include "Helper/mdbBase.h"
#include "Helper/mdbDateTime.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbDictionary.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    #define ERROR_TO_THROW(_code,...)\
        TADD_ERROR(_code,__VA_ARGS__);\
        throw TMdbCSPException(_code,__VA_ARGS__);

    #define SKIP_TAB(_text,_pos) while(_text[_pos] == '\t' || _text[_pos] == ' ') {++_pos;}  //跳过前面的空格和TAB 
    #define SET_T_F(_text,_pos,_check,_set) if(_text[_pos] == (_check)){_set = true;++_pos;}else{_set = false;}
    #define SET_0(P) if(P){P[0] = 0;}
 
    const char * TMdbAvpHelper::m_cspTypeStr[] = 
    {
        "UNKOWN",     // 0
        "OctetString",// 1
        "Integer32",  // 2
        "Unsigned32", // 3
        "Integer64",  // 4
        "Unsigned64", // 5
        "Grouped",    // 6
        "Time",       // 7
    };

    /******************************************************************************
    * 函数名称	:  TMdbCSPException
    * 函数描述	:  CSP 协议解析异常
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbCSPException::TMdbCSPException(const int lErrCode, const char* pszFormat, ...)
    {
        va_list args;

        memset(m_sErrMsg,0,sizeof(m_sErrMsg));
        m_lErrCode=lErrCode;

        va_start(args, pszFormat);
        vsprintf(m_sErrMsg, pszFormat, args);
        va_end(args);

        m_sErrMsg[sizeof(m_sErrMsg)-1] = '\0';
    }


    /******************************************************************************
    * 函数名称	:  TMdbAvpHead
    * 函数描述	:  AVP头
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    TMdbAvpHead::TMdbAvpHead()
    {
        //sHeadName[6] = '\0';    //报文头开始
        memset(sHeadName,0,sizeof(sHeadName));
        strncpy(sHeadName,"<QMDB>",6);
        memset(sKeyValue,0,sizeof(sKeyValue)); //初始化key
        iVersion  = MDB_CS_USE_OCP;        //Version(1个字节)
        iLen      = 0x0;        //Message Length(4个字节整数表示)
        isequence = 0;
        iCmdCode  = CSP_APP_ERROR;//Command-Code(2个字节)
        iSessionId = 0x0;       //sessionId (4个字节)
        memset(sSendTime,0,6);  //date-time(6字节字符串表示时分秒)
        iCurrentAvpFlag = 0;
        iAnsCodePos = 0;
        //iIsBigEndian = 0;
    }

    /******************************************************************************
    * 函数名称	:  SetCmdCode
    * 函数描述	:  设置命令码
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    void TMdbAvpHead::SetCmdCode(int ComId)
    {
        iCmdCode = ComId;
    }
    /******************************************************************************
    * 函数名称	:  GetSequence
    * 函数描述	:  获取序列值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    unsigned int TMdbAvpHead::GetSequence()
    {
        return isequence;
    }

    /******************************************************************************
    * 函数名称	:  SetSequence
    * 函数描述	:  设置序列值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    void TMdbAvpHead::SetSequence(unsigned int iseq)
    {
        isequence = iseq;
    }
    /******************************************************************************
    * 函数名称	:  print
    * 函数描述	:  打印消息包头信息
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    void TMdbAvpHead::Print()
    {
        /*
        printf("HeadName[%-6s],KeyValue[%-16s],Version[%-4d],Len[%-4d],CmdCode[%-4d],SessionId[%-4d],sSendTime[%-6s],isequence[%-4d]\n",
        						sHeadName,sKeyValue,
        						iVersion,
        						iLen,iCmdCode,
        						iSessionId,sSendTime,
        						isequence
        						);
        */
        TADD_NORMAL("HeadName[%-6s],KeyValue[%-16s],Version[%-4d],Len[%-4d],CmdCode[%-4d],SessionId[%-4d],sSendTime[%-6s],isequence[%-4d],iCurrentAvpFlag[%u]\n",
                    sHeadName,sKeyValue,
                    iVersion,
                    iLen,iCmdCode,
                    iSessionId,sSendTime,
                    isequence,
                    iCurrentAvpFlag
                   );
    }
    /******************************************************************************
    * 函数名称	:  Clear
    * 函数描述	:  清理
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    void TMdbAvpHead::Clear()
    {
        sHeadName[6] = '\0';
        sKeyValue[16] = '\0';
        iVersion = -1;
        iLen = 0;
        iCmdCode = 0;
        iSessionId = 0;
        sSendTime[6] = '\0';
        isequence = 0;
        iAnsCodePos = 0;
    }

    /******************************************************************************
    * 函数名称	:  CnvtToBin
    * 函数描述	:  将文本数据转成二进制包
    * 输入		:  pszMsg - 消息  SessionId - 回话标识(唯一)
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    void TMdbAvpHead::CnvtToBin(unsigned char* pszMsg,int SessionId)
    {
        //memcpy(&pszMsg[0],"<QMDB>",6);
        pszMsg[0]='<';
        pszMsg[1]='Q';
        pszMsg[2]='M';
        pszMsg[3]='D';
        pszMsg[4]='B';
        pszMsg[5]='>';
        //strncpy(sHeadName,"<QMDB>",6);
        //sHeadName[6] = '\0';
        //sKeyValue[16] = '\0';
        //version
        //pszMsg[21] = iIsBigEndian&0xff;
        pszMsg[22] = iVersion &0xff;
        //pszMsg[22] = iIsBigEndian &0xff;
        
        //Length
        pszMsg[23] = (iLen>>24) & 0xff;
        pszMsg[24] = (iLen>>16) & 0xff;
        pszMsg[25] = (iLen>>8) & 0xff;
        pszMsg[26] = iLen & 0xff;
        //command-Code
        pszMsg[27] = (iCmdCode>>8) & 0xff;
        pszMsg[28] = iCmdCode & 0xff;
        //sessionId
        iSessionId = SessionId;
        pszMsg[29] = (iSessionId>>24) & 0xff;
        pszMsg[30] = (iSessionId>>16) & 0xff;
        pszMsg[31] = (iSessionId>>8) & 0xff;
        pszMsg[32] = iSessionId & 0xff;
        //时间
        pszMsg[33] = '0';
        pszMsg[34] = '0';
        pszMsg[35] = '0';
        pszMsg[36] = '0';
        pszMsg[37] = '0';
        pszMsg[38] = '0';
        //isequence
        pszMsg[39] = (isequence>>24) & 0xff;
        pszMsg[40] = (isequence>>16) & 0xff;
        pszMsg[41] = (isequence>>8) & 0xff;
        pszMsg[42] = isequence & 0xff;
    }

    void TMdbAvpHead::CnvtToBinPlus(unsigned char* pszMsg,unsigned int SessionId)
    {
        pszMsg[0]='<';
        pszMsg[1]='Q';
        pszMsg[2]='M';
        pszMsg[3]='D';
        pszMsg[4]='B';
        pszMsg[5]='>';
        //version
        //pszMsg[21] = iIsBigEndian&0xff;
        pszMsg[22] = iVersion &0xff;
        //pszMsg[22] = iIsBigEndian &0xff;
        
        //Length
        memcpy(&pszMsg[23],&iLen,sizeof(iLen));
        //command-Code
        
        memcpy(&pszMsg[27],&iCmdCode,sizeof(iCmdCode));
        //sessionId
        iSessionId = SessionId;
        memcpy(&pszMsg[29],&iSessionId,sizeof(iSessionId));
        //时间
        pszMsg[33] = '0';
        pszMsg[34] = '0';
        pszMsg[35] = '0';
        pszMsg[36] = '0';
        pszMsg[37] = '0';
        pszMsg[38] = '0';
        //isequence
        memcpy(&pszMsg[39],&isequence,sizeof(isequence));
    }

    /******************************************************************************
    * 函数名称	:  BinToCnvt
    * 函数描述	:  二进制包转成文本数据
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    void TMdbAvpHead::BinToCnvt(unsigned char* pszMsg)
    {
        //获取报文头部标记<QMDB>
        FastMemcpy(sHeadName,&pszMsg[0],6);
        sHeadName[6] = '\0';
        sKeyValue[16] = '\0';
        iAnsCodePos = pszMsg[10]*NUM1 + pszMsg[11]*NUM2 + pszMsg[12]*256 + pszMsg[13];;
        //version
        //iIsBigEndian = pszMsg[21];
        iVersion = pszMsg[22];
        //Length
        //iIsBigEndian = pszMsg[22];
        
        iLen     = pszMsg[23]*NUM1 + pszMsg[24]*NUM2 + pszMsg[25]*256 + pszMsg[26];
        //commandcode
        iCmdCode = pszMsg[27]*256 + pszMsg[28];
        //sessionId
        iSessionId = pszMsg[29]*NUM1 + pszMsg[30]*NUM2 + pszMsg[31]*256 + pszMsg[32];
        //date-time
        sSendTime[6] = '\0';
        isequence = pszMsg[39]*NUM1 + pszMsg[40]*NUM2 + pszMsg[41]*256 + pszMsg[42];
    }

    void TMdbAvpHead::BinToCnvtPlus(unsigned char* pszMsg)
    {
        //获取报文头部标记<QMDB>
        //FastMemcpy(sHeadName,&pszMsg[0],6);
        //cpp check modify
        int iCpyLen = 0;
		iCpyLen =  6;
        memcpy(sHeadName,&pszMsg[0],iCpyLen);
        sHeadName[6] = '\0';
        sKeyValue[16] = '\0';
        
        memcpy(&iAnsCodePos,&pszMsg[10],sizeof(iAnsCodePos));
        //version
        
        //iIsBigEndian = pszMsg[22];
        iVersion = pszMsg[22];
        //Length
        memcpy(&iLen,&pszMsg[23],sizeof(iLen));
        //iLen     = pszMsg[23]*NUM1 + pszMsg[24]*NUM2 + pszMsg[25]*256 + pszMsg[26];
        
        //iLen     = pszMsg[26]*NUM1 + pszMsg[25]*NUM2 + pszMsg[24]*256 + pszMsg[23];
        //commandcode
        
        memcpy(&iCmdCode,&pszMsg[27],sizeof(iCmdCode));
        //iCmdCode = pszMsg[27]*256 + pszMsg[28];
        
        //iCmdCode = pszMsg[28]*256 + pszMsg[27];
        //sessionId
        memcpy(&iSessionId,&pszMsg[29],sizeof(iSessionId));
        //iSessionId = pszMsg[29]*NUM1 + pszMsg[30]*NUM2 + pszMsg[31]*256 + pszMsg[32];
        //iSessionId = pszMsg[32]*NUM1 + pszMsg[31]*NUM2 + pszMsg[30]*256 + pszMsg[29];
        //date-time
        sSendTime[6] = '\0';
        memcpy(&isequence,&pszMsg[39],sizeof(isequence));
        //isequence = pszMsg[39]*NUM1 + pszMsg[40]*NUM2 + pszMsg[41]*256 + pszMsg[42];
        //isequence = pszMsg[42]*NUM1 + pszMsg[41]*NUM2 + pszMsg[40]*256 + pszMsg[39];
    }


    /******************************************************************************
    * 函数名称	:  TMdbAvpItem
    * 函数描述	:  AVP 项的构造函数
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    TMdbAvpItem::TMdbAvpItem()
    {
        //memset(sName,  0, sizeof(sName));
        sName[0]  = 0;
        iCode     = 0;              //AVP-Code
        iLen      = 0;              //AVP长度
        iType     = CSP_TYPE_UNKOWN; //数据类型
        bIsExist  = false;          //是否存在
        bIsMul    = false;          //是否允许有多个
        bIsM      = false;
        iLevel    = 1;              //默认为第一层
        pszValue  = NULL;
        iValue    = 0;
        uiValue   = 0;
        llValue   = -1;
        ullValue  = 0;

        pFatherItem = NULL;
        pChildItem  = NULL;   //子节点
        pNextItem   = NULL;   //兄弟节点
        m_bNULLValue = false;
        iUpdateFlag = 0;
    }

    /******************************************************************************
    * 函数名称	:  ~TMdbAvpItem
    * 函数描述	:  析构
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    TMdbAvpItem::~TMdbAvpItem()
    {
        SAFE_DELETE_ARRAY(pszValue);
        SAFE_DELETE(pChildItem);
        SAFE_DELETE(pNextItem);
    }
    /******************************************************************************
    * 函数名称	: Clear
    * 函数描述	:  清理一个avp 项中的数据
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    //you bug
    void TMdbAvpItem::Clear()
    {
        //if(bIsExist == true)
        {
            if(iType == CSP_TYPE_GROUP )
            {
                bIsExist = false;
                iUpdateFlag = 0;
                if(pChildItem != NULL)
                {
                    pChildItem->Clear();
                    TMdbAvpItem* pCNextItem = pChildItem->pNextItem;
                    while(pCNextItem != NULL)
                    {
                        pCNextItem->Clear();
                        pCNextItem = pCNextItem->pNextItem;
                    }
                }
            }
            else
            {
                iUpdateFlag = 0;
                bIsExist = false;
                iValue    = -1;
                uiValue   = 0;
                llValue   = -1;
                ullValue  = 0;
                m_bNULLValue  = false;
                if(NULL != pszValue)
                {
                    pszValue[0] = 0;
                }
            }
        }
    }

    /******************************************************************************
    * 函数名称	:  GetInfo
    * 函数描述	:  获取本节点以及子节点的全部数据，将其序列化成字符串
    * 输入		:
    * 输出		:  pszOut -  输出的字符串
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    int TMdbAvpItem::Serialize(unsigned char* pszOut,TMdbCspParser * pParser) throw (TMdbCSPException)
    {
        int iRet = 0;
        if(iLen < 4)
        {
            return 0;
        }
        TADD_DETAIL("Serialize item[%d],ilen=[%d],iUpdateFlag[%u],",iCode,iLen,iUpdateFlag);
        bIsExist = false;
        //首先设定AVP-Code / AVP-Len
        pszOut[0] = (iCode>>8) & 0xff;
        pszOut[1] = iCode & 0xff;
        pszOut[2] = (iLen>>8) & 0xff;
        pszOut[3] = iLen & 0xff;
        int iPos = 4; //偏移4个字节
        if(iLen == 4) return iRet;
        //如果是组属性，则需要提取全部的数据
        if(iType == CSP_TYPE_GROUP && iCode != AVP_PARAM_STR_GROUP)
        {
            TMdbAvpItem* pTempItem = pChildItem;
            while(pTempItem)
            {
                if(false == pParser->IsAvpFree(pTempItem)/*pTempItem->bIsExist*/)
                {
                    pTempItem->Serialize(&pszOut[iPos],pParser);
                    iPos += pTempItem->iLen;
                }
                pTempItem = pTempItem->pNextItem;
            }
        }
        else
        {
            switch(iType)
            {
            case CSP_TYPE_STRING:
            case CSP_TYPE_TIME:
            case CSP_TYPE_GROUP://AVP_PARAM_GROUP
            {

                FastMemcpy((void *)&pszOut[iPos],(void*)pszValue,iLen-iPos);
                //pszOut[iPos+iLen] = '\0';
                break;
            }
            case CSP_TYPE_INT32:
            {
                TMdbAvpConvert::Int32ToNet(&pszOut[iPos], iValue);
                break;
            }
            case CSP_TYPE_INT64:
            {
                TMdbAvpConvert::Int64ToNet(&pszOut[iPos], llValue);
                break;
            }
            case CSP_TYPE_UINT32:
            {
                TMdbAvpConvert::UInt32ToNet(&pszOut[iPos], uiValue);
                break;
            }
            case CSP_TYPE_UINT64:
            {
                TMdbAvpConvert::UInt64ToNet(&pszOut[iPos], ullValue);
                break;
            }
            default:
            {
                ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Error Data-Type=%d.\n",iType);
                //break;
            }
            }
        }
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  SetAvp
    * 函数描述	:  设置avp节点信息
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    int TMdbAvpItem::SetAvp(unsigned char* pszDCC, int iLenIn,TMdbCspParser * pParser) throw (TMdbCSPException)
    {
        iLen     = iLenIn;//NEW ADD avp lenght
        //clear
        m_bNULLValue = false;
        SET_0(pszValue);
        int iPos = 4;
        if(iType != CSP_TYPE_GROUP || iCode == AVP_PARAM_STR_GROUP)
        {
            if(iPos == iLenIn)
            {//NULL 值
                bIsExist = true;
                m_bNULLValue = true;
            }
            else
            {
                switch(iType)
                {
                case CSP_TYPE_STRING:
                case CSP_TYPE_TIME:
                case CSP_TYPE_GROUP://AVP_PARAM_STR_GROUP
                {
                    if(pszValue == NULL)
                    {
						pszValue = new(std::nothrow) char[MAX_AVP_VALUE_LEN];
                        if(pszValue == NULL)
                        {
                            ERROR_TO_THROW(ERR_OS_NO_MEMROY,"Mem Not Enough.");
                        }
                    }
                    FastMemcpy((void*)pszValue,(void*)&pszDCC[iPos],iLenIn-iPos);
                    pszValue[iLenIn-iPos] = 0;
                    bIsExist = true;
                    break;
                }
                case CSP_TYPE_INT32:
                {
                    TMdbAvpConvert::NetToInt32(&pszDCC[iPos], iValue);
                    bIsExist = true;
                    break;
                }
                case CSP_TYPE_INT64:
                {
                    TMdbAvpConvert::NetToInt64(&pszDCC[iPos], llValue);
                    bIsExist = true;
                    break;
                }
                case CSP_TYPE_UINT32:
                {
                    TMdbAvpConvert::NetToUInt32(&pszDCC[iPos], uiValue);
                    bIsExist = true;
                    break;
                }
                case CSP_TYPE_UINT64:
                {
                    TMdbAvpConvert::NetToUInt64(&pszDCC[iPos], ullValue);
                    bIsExist = true;
                    break;
                }
                default:
                {
                    ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Error Data-Type= %d",iType);
                    //break;
                }
               }
           }
        }
        else    //如果是Group类型，定位子节点
        {
            bIsExist = true;
            iLen     = iLenIn;
            int iAvp = 0;
            int iTempLen = 0;
            TMdbAvpItem* pItem = pChildItem;
            while(true)
            {
                pParser->GetAvpItemCodeLen(pszDCC + iPos,iAvp,iTempLen);
                //找到AVP信息
                pItem = pParser->GetFreeAvpItem(pChildItem,iAvp);
                if(pItem == NULL)
                {
                    ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find AVP=[%d], iPos=[%d],iLenIn=[%d]",iAvp, iPos,iLenIn);
                }
                pItem->SetAvp(&pszDCC[iPos], iTempLen,pParser);
                iPos += iTempLen;
                if(iLenIn <= iPos)
                    break;
            }
        }
        iUpdateFlag = pParser->m_tHead.iCurrentAvpFlag;//更新flag
        return 0;
    }


#if 0
    /******************************************************************************
    * 函数名称	:  GetTotalLen
    * 函数描述	:  获取本节点以及子节点的长度和
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:
    *******************************************************************************/
    int TMdbAvpItem::GetTotalLen()
    {

        if(CSP_TYPE_GROUP == iType)
        {
            iLen = 4;
            TMdbAvpItem * pTempItem = pChildItem;
            while(pTempItem)
            {
                if(pTempItem->bIsExist)
                {
                    iLen += pTempItem->GetTotalLen();
                }
                pTempItem = pTempItem->pNextItem;
            }
        }
        return iLen;
    }
    /******************************************************************************
    * 函数名称	:  FinishFill
    * 函数描述	:  完成数据填充,给group节点调用，重新计算其len
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbAvpItem::FinishFill()
    {
        bIsExist = true;
        GetTotalLen();
        return 0;
    }
#endif


    /******************************************************************************
    * 函数名称	:  TMdbCspParser
    * 函数描述	:  构造
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbCspParser::TMdbCspParser()
    {
        m_pRootAvpItem = NULL;
        m_iTotalLen = 0;

    }

    /******************************************************************************
    * 函数名称	:  ~TMdbCspParser
    * 函数描述	:  析构
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbCspParser::~TMdbCspParser()
    {
        SAFE_DELETE(m_pRootAvpItem);
    }
    /******************************************************************************
    * 函数名称	:  Init
    * 输入		:  iType - 协议包类型  bRequest- true(request) false(answer)
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::Init(int iType,bool bRequest)
    {
        int iRet = 0;
        m_tHead.SetCmdCode(iType);
        char pszIdemt[MAX_NAME_LEN];
        pszIdemt[0] = 0;
        int i  = 0;
        for(i = 0; i< (int)(sizeof(g_cspAppType)/sizeof(ST_CSP_APP_TYPE)); i++)
        {
            if(iType == g_cspAppType[i].iAppType)
            {
                if(bRequest)
                {
                    SAFESTRCPY(pszIdemt,sizeof(pszIdemt),g_cspAppType[i].sRequestName);

                }
                else
                {
                    SAFESTRCPY(pszIdemt,sizeof(pszIdemt),g_cspAppType[i].sAnswerNem);
                }
                break;
            }
        }
        if(0 == pszIdemt[0])
        {
            CHECK_RET(-1,"Invalid type=[%d].",iType);
        }
        CHECK_RET(ParserConfig(pszIdemt),"ParserConfig [%s] error",pszIdemt);
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ParserConfig
    * 函数描述	:  解析配置文件，生成包结构
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::ParserConfig(const char *pszIdemt)
    {
        int iRet = 0;
        //char pszFileName[256]= {0};
        //sprintf(pszFileName, "%s/etc/csp-avp-config.ini", getenv("QuickMDB_HOME"));
        //pszFileName[strlen(pszFileName)] = '\0';
        //FILE * fp = fopen(pszFileName, "r");
        //if(NULL == fp)
        //{
        //    CHECK_RET(-1,"Can't open file=[%s],errno=%d,errmsg=%s.",pszFileName,errno,strerror(errno));
        //}
        char sLineText[1024];
        sLineText[0] = 0;
        int iLinePos = 0;
        int  nAct = 0;
        //当前AVP项
        TMdbAvpItem* pCurItem = NULL;
        //fseek(fp,0,SEEK_SET );
        TMdbNtcSplit tSplit;
        tSplit.SplitString(CSP_AVP_STRING, '\n');
        //遍历文件
        //while(fgets(sLineText, sizeof(sLineText),fp))
        for(unsigned int i = 0; i<tSplit.GetFieldCount(); i++)
        {
            SAFESTRCPY(sLineText, sizeof(sLineText), tSplit[i]);
            TMdbNtcStrFunc::Trim(sLineText);
            if ( ( sLineText[0] == '\n' ) || strlen(sLineText) == 0) //空行直接过滤
                continue;
            /* into the section */
            if ( !nAct && strstr( sLineText, pszIdemt ) )
            {
                nAct = 1;
                continue;
            }
            //读取到空行分割退出.
            if(nAct && ( sLineText[0] == '#' )&& ( sLineText[1] == '#' ) )
            {
                break;
            }
            /* front of the section */
            if ( !nAct )
                continue;
            //跳过第一行
            if(iLinePos == 0)
            {
                ++iLinePos;
                continue;
            }
            //把当前文字解析为AVP项
            TMdbAvpItem* pItem = ParserLine(sLineText);
            if(pItem == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Invalid Line = [%s].", sLineText);
                return -1;
            }
            //把AVP项插入当前列表中
            AddItem(pCurItem,pItem);
        }
        //SAFE_CLOSE(fp);
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  AddItem
    * 函数描述	:  添加项
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::AddItem(TMdbAvpItem * &pCurItem,TMdbAvpItem* pItem)
    {
        if(pCurItem == NULL)
        {
            m_pRootAvpItem = pItem; //m_pRootAvpItem 记录下ini文件中第一行的avp
            pCurItem       = pItem;
        }
        else
        {
            if(pItem->iLevel == pCurItem->iLevel)
            {
                pCurItem->pNextItem = pItem;
                pItem->pFatherItem  = pCurItem->pFatherItem;
            }
            else if(pItem->iLevel > pCurItem->iLevel)
            {
                pCurItem->pChildItem = pItem;
                pItem->pFatherItem  = pCurItem;
            }
            else
            {
                while(1)
                {
                    pCurItem = pCurItem->pFatherItem;
                    if(pCurItem == NULL)
                    {
                        break;
                    }
                    if(pCurItem->iLevel == pItem->iLevel)
                    {
                        break;
                    }
                }
                pItem->pFatherItem  = pCurItem->pFatherItem;
                pCurItem->pNextItem = pItem;
            }
            pCurItem = pItem;
        }
        return 0;
    }

    /******************************************************************************
    * 函数名称	:  GetStr
    * 函数描述	:  获取str值
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::GetItemName(const char* pszLineText,int & iPos,TMdbAvpItem* pItem)
    {
        int iNamePos = 0;
        if(pszLineText[iPos] == '[' || pszLineText[iPos] == '<' || pszLineText[iPos] == '{')
        {
            for(int i=iPos+1; pszLineText[i]!='\0'; ++i)
            {
                if(pszLineText[i] == ']' || pszLineText[i] == '>' || pszLineText[i] == '}' || pszLineText[i] == ' ')
                {
                    pItem->sName[iNamePos] = '\0';
                    iPos = i+1;
                    break;
                }
                pItem->sName[iNamePos] = pszLineText[i];
                ++iNamePos;
            }
        }
        else
        {
            return -1;
        }
        return 0;
    }
    /******************************************************************************
    * 函数名称	:  GetInt
    * 函数描述	:  获取int值
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::GetItemCode(const char* pszLineText,int & iPos,TMdbAvpItem* pItem)
    {
        for(int i=iPos; pszLineText[i] != ' ' &&  pszLineText[i] != '\t'; ++i)
        {
            //如果非数字，报错退出
            if(pszLineText[i] < '0' || pszLineText[i] > '9')
            {
                return -1;
            }
            else
            {
                pItem->iCode = pItem->iCode*10 + (pszLineText[i] - '0');
            }
            iPos = i;
        }
        return 0;
    }

    /******************************************************************************
    * 函数名称	:  ParserLine
    * 函数描述	:  把当前文字解析为AVP项
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/

    TMdbAvpItem* TMdbCspParser::ParserLine(const char* pszLineText)
    {
        TMdbAvpItem* pItem = new(std::nothrow) TMdbAvpItem();
        if(NULL == pItem) 
        {
            TADD_ERROR(ERROR_UNKNOWN,"Mem not Enough.");
            return NULL;
        }
        int iPos = 0;
        SKIP_TAB(pszLineText,iPos);
        //是否可以重复
        SET_T_F(pszLineText,iPos,'*',pItem->bIsMul);
        //接下来获取名称
        if(GetItemName(pszLineText,iPos,pItem) != 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Invalid Char=[%c].", pszLineText[iPos]);
            delete pItem;
            return NULL;
        }
        //找到处于第几层
        SKIP_TAB(pszLineText,iPos);
        pItem->iLevel = pszLineText[iPos]-'0';
        ++iPos;
        //接下来是空格+数字AVP-CODE
        SKIP_TAB(pszLineText,iPos);
        pItem->iCode = 0;
        if(GetItemCode(pszLineText,iPos, pItem) != 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Invalid Char=[%c].", pszLineText[iPos]);
            delete pItem;
            return NULL;
        }
        ++iPos;
        //接下来是空格+是否必选
        SKIP_TAB(pszLineText,iPos);
        pItem->bIsM = (pszLineText[iPos++]=='M' || pszLineText[iPos++]=='m');
        //接下来是空格+数据类型
        SKIP_TAB(pszLineText,iPos);
        char sType[32];
        memset(sType, 0, sizeof(sType));
        int iTPos = 0;
        for(int i=iPos; pszLineText[i] != ' ' &&  pszLineText[i] != '\t' && pszLineText[i] != '\n' && pszLineText[i] != '\r' && pszLineText[i] != '\0'; ++i)
        {
            sType[iTPos] = pszLineText[i];
            ++iTPos;
        }
        sType[iTPos] = '\0';
        pItem->iType = TMdbAvpHelper::StrToType(sType);
        if(pItem->iType == CSP_TYPE_UNKOWN)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Invalid iType=[%s].", sType);
            delete pItem;
            return NULL;
        }
        return pItem;
    }

    void TMdbCspParser::Print()
    {
        m_tHead.Print();//头信息打印
        TMdbAvpHelper::PrintTotalAvp(0,m_pRootAvpItem);//avp消息体信息打印
    }

    /******************************************************************************
    * 函数名称	:  Serialize
    * 函数描述	:  将一个数据包序列化成二进制串
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::Serialize(unsigned char* pszDCC,int SessionId,int iSequence) throw (TMdbCSPException)
    {
        //int iRet = 0;
        m_tHead.iLen = GetTotalLen();
        if(m_tHead.iLen > MAX_CSP_LEN)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Msg size[%d] > MAX_SIZE[%d]",m_tHead.iLen,MAX_CSP_LEN);
        }
        m_tHead.SetSequence(iSequence);
        m_tHead.CnvtToBin(pszDCC,SessionId);
        int iPos = SIZE_MSG_AVP_HEAD;
        TMdbAvpItem* pItem = m_pRootAvpItem;
        while(pItem != NULL)
        {
           // if(pItem->bIsExist == true)
            if(false == IsAvpFree(pItem))
            {
                pItem->Serialize((unsigned char*)&pszDCC[iPos],this);
                iPos += pItem->iLen;
            }
            pItem = pItem->pNextItem;
        }
        //Clear(); //清空
        return m_tHead.iLen;
    }

    /******************************************************************************
    * 函数名称	:  DeSerialize
    * 函数描述	:  解析二进制串
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::DeSerialize(unsigned char* pszDCC,int iDCCLen) throw (TMdbCSPException)
    {
        int iRet = 0;
        m_tHead.BinToCnvt(pszDCC);//包头解析
        Clear();//清理数据
        int iPos  = SIZE_MSG_AVP_HEAD;
        int iCode = 0;
        int iLen  = 0 ;
        TMdbAvpItem * pTempAvpItem = m_pRootAvpItem;
        while(iPos < iDCCLen)
        {
            GetAvpItemCodeLen(pszDCC + iPos,iCode,iLen);
            pTempAvpItem = GetFreeAvpItem(m_pRootAvpItem,iCode);//获取一个空闲的avp
            if(NULL == pTempAvpItem)
            {
                ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find free avp node,code[%d].",iCode);
            }
            pTempAvpItem->SetAvp(pszDCC + iPos,iLen,this);
            iPos += iLen;
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetTotalLen
    * 函数描述	:  获取总长度
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*
    int TMdbCspParser::GetTotalLen()
    {
        int iLen = 6+16+1+4+2+4+6+4;
        iLen += GetItemLen(m_pRootAvpItem);
        return iLen;
    }
    */
    //获取总长度
    int TMdbCspParser::GetItemLen(TMdbAvpItem* pStartItem)
    {
        int iLen = 0;
        TMdbAvpItem* pItem = pStartItem;
        while(pItem != NULL)
        {
            if(false ==IsAvpFree(pItem))
            {
                if(CSP_TYPE_GROUP ==pItem->iType)
                {
                    pItem->iLen = 4;
                    pItem->iLen += GetItemLen( pItem->pChildItem);
                    /*
                    TMdbAvpItem * pTempItem = pItem->pChildItem;
                    while(pTempItem)
                    {
                        if(false ==IsAvpFree(pTempItem))
                        {
                            pItem->iLen += GetItemLen(pTempItem);
                        }
                        pTempItem = pTempItem->pNextItem;
                    }   
                    */
                }
                iLen += pItem->iLen;
            }
            pItem = pItem->pNextItem;
        }
        return iLen;
    }
    void TMdbCspParser::FinishFillGroup(TMdbAvpItem* pStartItem,int iGroupLen)
    {
        pStartItem->iUpdateFlag = m_tHead.iCurrentAvpFlag;
        pStartItem->iLen = 4 + iGroupLen;
        m_iTotalLen+=4;
        //GetItemLen(pStartItem);
    }

    /******************************************************************************
    * 函数名称	:  Clear
    * 函数描述	:  清理数据
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::Clear()
    {
        m_tHead.UpdateAvpFlag();//更新head中的avp标识，老化其数据
        m_iTotalLen = SIZE_MSG_AVP_HEAD;
        #if 0
        TMdbAvpItem* pItem = m_pRootAvpItem;
        while(pItem != NULL)
        {
            //if(pItem->bIsExist == true)
            {
                pItem->Clear();
            }
            pItem = pItem->pNextItem;
        }
        #endif
        return 0;
    }




    /******************************************************************************
    * 函数名称	:  SetItemValue
    * 函数描述	:  设置item的值，根据查找一个空闲节点(该节点bIsExist = false),
    				   每个节点只能被设置一次。若传入的数据类型不对也会抛出异常。
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::SetItemValue(TMdbAvpItem* pStartItem,int iCode,const char * sValue) throw (TMdbCSPException)
    {
        int iRet  = 0;
        if(NULL == sValue)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"avp[%d] value = NULL.",iCode);
        }
        TMdbAvpItem * pTempItem = GetFreeAvpItem(pStartItem,iCode);
        if(NULL == pTempItem)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find free avp[%d].",iCode);
        }
        if(CSP_TYPE_STRING != pTempItem->iType && CSP_TYPE_TIME != pTempItem->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"SetValue code[%d] iType[%d] error.",iCode,pTempItem->iType);
        }
        pTempItem->bIsExist = true;
        pTempItem->iUpdateFlag = m_tHead.iCurrentAvpFlag;
        if(pTempItem->pszValue == NULL)
        {
            pTempItem->pszValue = new(std::nothrow) char[MAX_AVP_VALUE_LEN];
            if(NULL == pTempItem->pszValue )
            {
                ERROR_TO_THROW(ERR_OS_NO_MEMROY,"Mem not Enough.");
            }
        }
        SAFESTRCPY(pTempItem->pszValue,MAX_AVP_VALUE_LEN,sValue);
        pTempItem->iLen = 4 + strlen(pTempItem->pszValue)+1;
        m_iTotalLen +=pTempItem->iLen;
        m_iIncreaseLen += pTempItem->iLen;
        return iRet;
    }
    //批处理特殊处理
    int TMdbCspParser::SetItemValueForParamGroup(TMdbAvpItem* pStartItem,int iCode,const char * sValue) throw (TMdbCSPException)
    {
        int iRet  = 0;
        if(NULL == sValue)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"avp[%d] value = NULL.",iCode);
        }
        TMdbAvpItem * pTempItem = GetFreeAvpItem(pStartItem,iCode);
        if(NULL == pTempItem)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find free avp[%d].",iCode);
        }
        if(CSP_TYPE_GROUP != pTempItem->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"SetValue code[%d] iType[%d] error.",iCode,pTempItem->iType);
        }
        pTempItem->bIsExist = true;
        pTempItem->iUpdateFlag = m_tHead.iCurrentAvpFlag;
        if(pTempItem->pszValue == NULL)
        {
            pTempItem->pszValue = new(std::nothrow) char[MAX_AVP_VALUE_LEN];
            if(NULL == pTempItem->pszValue )
            {
                ERROR_TO_THROW(ERR_OS_NO_MEMROY,"Mem not Enough.");
            }
        }
        SAFESTRCPY(pTempItem->pszValue,MAX_AVP_VALUE_LEN,sValue);
        pTempItem->iLen = (int)(4 + strlen(pTempItem->pszValue)+1);
        m_iTotalLen +=pTempItem->iLen;
        m_iIncreaseLen += pTempItem->iLen;
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  SetItemValue
    * 函数描述	:  设置NULLvalue
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::SetItemValueNULL(TMdbAvpItem* pStartItem,int iCode) throw (TMdbCSPException)
    {
         int iRet  = 0;
        TMdbAvpItem * pTempItem = GetFreeAvpItem(pStartItem,iCode);
        if(NULL == pTempItem)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find free avp[%d].",iCode);
        }
        pTempItem->bIsExist = true;
        pTempItem->iUpdateFlag = m_tHead.iCurrentAvpFlag;
        /*
        if(pTempItem->pszValue == NULL)
        {

            pTempItem->pszValue = new char[MAX_AVP_VALUE_LEN];
            if(NULL == pTempItem->pszValue )
            {
                ERROR_TO_THROW(ERR_OS_NO_MEMROY,"Mem not Enough.");
            }
        }
        */
        pTempItem->iLen = 4 ;//+ strlen(pTempItem->pszValue)+1;没有value值
        m_iTotalLen +=pTempItem->iLen;
        m_iIncreaseLen += pTempItem->iLen;
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SetItemValue
    * 函数描述	:  设置item的值
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::SetItemValue(TMdbAvpItem* pStartItem,int iCode,unsigned int iValue) throw (TMdbCSPException)
    {
        int iRet = 0;
        TMdbAvpItem * pTempItem = GetFreeAvpItem(pStartItem,iCode);
        if(NULL == pTempItem)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find free avp[%d].",iCode);
        }
        if(CSP_TYPE_UINT32 != pTempItem->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"SetValue code[%d] iType[%d] error.",iCode,pTempItem->iType);
        }
        pTempItem->bIsExist = true;
        pTempItem->iUpdateFlag = m_tHead.iCurrentAvpFlag;
        pTempItem->uiValue  = iValue;
        pTempItem->iLen     = 4 + 4;
        m_iTotalLen +=pTempItem->iLen;
        m_iIncreaseLen += pTempItem->iLen;
        return iRet;
    }

    int TMdbCspParser::SetItemValue(TMdbAvpItem* pStartItem,int iCode,int iValue) throw (TMdbCSPException)
    {
        int iRet = 0;
        TMdbAvpItem * pTempItem = GetFreeAvpItem(pStartItem,iCode);
        if(NULL == pTempItem)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find free avp[%d].",iCode);
        }
        if(CSP_TYPE_INT32 != pTempItem->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"SetValue code[%d] iType[%d] error.",iCode,pTempItem->iType);
        }
        pTempItem->bIsExist = true;
        pTempItem->iUpdateFlag = m_tHead.iCurrentAvpFlag;
        pTempItem->iValue  = iValue;
        pTempItem->iLen     = 4 + 4;
        m_iTotalLen +=pTempItem->iLen;
        m_iIncreaseLen += pTempItem->iLen;
        return iRet;

    }
    int TMdbCspParser::SetItemValue(TMdbAvpItem* pStartItem,int iCode,unsigned long long iValue) throw (TMdbCSPException)
    {
        int iRet = 0;
        TMdbAvpItem * pTempItem = GetFreeAvpItem(pStartItem,iCode);
        if(NULL == pTempItem)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find free avp[%d].",iCode);
        }
        if(CSP_TYPE_UINT64 != pTempItem->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"SetValue code[%d] iType[%d] error.",iCode,pTempItem->iType);
        }
        pTempItem->bIsExist = true;
        pTempItem->iUpdateFlag = m_tHead.iCurrentAvpFlag;
        pTempItem->ullValue  = iValue;
        pTempItem->iLen     = 4 + 8;
        m_iTotalLen +=pTempItem->iLen;
        m_iIncreaseLen += pTempItem->iLen;
        return iRet;

    }
    int TMdbCspParser::SetItemValue(TMdbAvpItem* pStartItem,int iCode,long long iValue) throw (TMdbCSPException)
    {
        int iRet = 0;
        TMdbAvpItem * pTempItem = GetFreeAvpItem(pStartItem,iCode);
        if(NULL == pTempItem)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find free avp[%d].",iCode);
        }
        if(CSP_TYPE_INT64 != pTempItem->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"SetValue code[%d] iType[%d] error.",iCode,pTempItem->iType);
        }
        pTempItem->bIsExist = true;
        pTempItem->iUpdateFlag = m_tHead.iCurrentAvpFlag;
        pTempItem->llValue  = iValue;
        pTempItem->iLen     = 4 + 8;
        m_iTotalLen +=pTempItem->iLen;
        m_iIncreaseLen += pTempItem->iLen;
        return iRet;

    }

    /******************************************************************************
    * 函数名称	:  CopyItem
    * 函数描述	:  copy item
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbAvpItem* TMdbCspParser::CopyItem(TMdbAvpItem* pItem)
    {
        if(pItem == NULL)
            return NULL;
        TMdbAvpItem* pItemNode = new(std::nothrow) TMdbAvpItem();
        if(NULL == pItemNode) 
        {
            TADD_ERROR(ERROR_UNKNOWN,"Mem not Enough");
            return NULL;
        }    
        //拷贝本节点数据
        SAFESTRCPY(pItemNode->sName,sizeof(pItemNode->sName),pItem->sName);
        pItemNode->iCode    = pItem->iCode;			//AVP-Code
        pItemNode->iLen	   = pItem->iLen;			//AVP长度
        pItemNode->iType    = pItem->iType;			//数据类型
        pItemNode->bIsExist = false; 				//是否存在
        pItemNode->bIsMul   = pItem->bIsMul; 		//是否允许有多个
        pItemNode->bIsM	   = pItem->bIsM;		    //是否必须存在
        pItemNode->iLevel   = pItem->iLevel; 	   //所处层次，依次为1.2.3.....
        pItemNode->iUpdateFlag = 0;
        //根据数据类型分配内存
        if(pItemNode->iType == CSP_TYPE_STRING || pItemNode->iCode == AVP_PARAM_STR_GROUP)
        {
            pItemNode->pszValue = new(std::nothrow) char[MAX_AVP_VALUE_LEN];
            if( pItemNode->pszValue == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Mem not Enough");
                return NULL;
            }
            pItemNode->pszValue[0] = '\0';
        }
        else if(pItemNode->iType == CSP_TYPE_TIME)
        {
            pItemNode->pszValue = new(std::nothrow) char[MAX_AVP_TIME_LEN];
            if( pItemNode->pszValue == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Mem not Enough");
                return NULL;
            }
            pItemNode->pszValue[0] = '\0';
        }
        else
        {
            pItemNode->pszValue = NULL;
        }
        pItemNode->iValue   = -1;
        pItemNode->uiValue  = 0;
        pItemNode->llValue  = -1;
        pItemNode->ullValue = 0;
        pItemNode->pFatherItem = pItem->pFatherItem;  //父亲节点
        pItemNode->pChildItem  = NULL;  //子节点
        pItemNode->pNextItem   = NULL;  //兄弟节点
        //如果是单节点，直接返回;如果是树形节点，则递归处理每个字节点
        if(pItemNode->iType == CSP_TYPE_GROUP && pItemNode->iCode != AVP_PARAM_STR_GROUP)
        {
            TMdbAvpItem* pItemNodeChild = pItem->pChildItem;
            TMdbAvpItem * pTailItem	   = NULL;//记录尾节点，这边不使用头插，保证其和被copy 的元素顺序一致
            while(pItemNodeChild != NULL)
            {
                TMdbAvpItem* pItemNew = CopyItem(pItemNodeChild);
                if(pItemNode->pChildItem == NULL)
                {
                    pItemNode->pChildItem = pItemNew;
                    pTailItem = pItemNew;
                }
                else
                {
                    pTailItem->pNextItem = pItemNew;
                }
                pTailItem	  = pItemNew;
                pItemNodeChild = pItemNodeChild->pNextItem;
            }
        }
        return pItemNode;
    }

    /******************************************************************************
    * 函数名称	:  GetFreeAvpItem
    * 函数描述	:  获取一个空闲avp item ，
    				   若是group则要预留一个空闲的(用于后续的copy item)。
    				   只有group节点才可以copy出新节点
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbAvpItem * TMdbCspParser::GetFreeAvpItem(TMdbAvpItem * pStartItem, int iAvpCode)
    {
        if(NULL == pStartItem)
        {
            return NULL;
        }
        TMdbAvpItem * pRetItem	= NULL;
        TMdbAvpItem * pTempItem = pStartItem;
        while(NULL != pTempItem)
        {
            //if(pTempItem->iCode == iAvpCode && false == pTempItem->bIsExist)
            if(pTempItem->iCode == iAvpCode &&  IsAvpFree(pTempItem))
            {
                pRetItem = pTempItem;
                if(CSP_TYPE_GROUP == pTempItem->iType &&
                        (NULL == pTempItem->pNextItem || pTempItem->pNextItem->iCode != iAvpCode)
                  )
                {
                    //对于group的项，要预留一个空闲的
                    TMdbAvpItem * pNewItem = TMdbCspParser::CopyItem(pTempItem);
                    if(NULL == pNewItem)
                    {
                        TADD_ERROR(ERROR_UNKNOWN,"copy item failed.");
                        return NULL;
                    }
                    pNewItem->pNextItem = pTempItem->pNextItem;
                    pTempItem->pNextItem = pNewItem;
                }
                break;
            }
            pTempItem = pTempItem->pNextItem;
        }
        return pRetItem;
    }

    /******************************************************************************
    * 函数名称	:  GetStringValue
    * 函数描述	:  获取字符串值，从pStartItem开始查找为iAvpCode的节点。
    				   该节点的bIsExist = true.并获取值。若数据类型不对也会抛出异常
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/

    char *		TMdbCspParser::GetStringValue(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_TIME != pTemp->iType && CSP_TYPE_STRING != pTemp->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->pszValue;

    }
    //批处理特殊处理
    char* TMdbCspParser::GetStringValueForParamGroup(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_GROUP != pTemp->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->pszValue;

    }
    /******************************************************************************
    * 函数名称	:  GetUINT32Value
    * 函数描述	:  获取UINT32值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    unsigned int TMdbCspParser::GetUINT32Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_UINT32!= pTemp->iType )
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->uiValue;

    }
    /******************************************************************************
    * 函数名称	:  GetINT32Value
    * 函数描述	:  获取INT32值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int	TMdbCspParser::GetINT32Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_INT32!= pTemp->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->iValue;

    }
    /******************************************************************************
    * 函数名称	:  GetINT64Value
    * 函数描述	:  获取INT64值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    long long	TMdbCspParser::GetINT64Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_INT64 != pTemp->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->llValue;

    }
    /******************************************************************************
    * 函数名称	:  GetUINT64Value
    * 函数描述	:  获取UINT64值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    unsigned long long TMdbCspParser::GetUINT64Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_UINT64 != pTemp->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->ullValue;
    }
    /******************************************************************************
    * 函数名称	:  IsNullValue
    * 函数描述	:  判断是否是null value
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/

    bool TMdbCspParser::IsNullValue(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        return pTemp->m_bNULLValue;
    }

    /******************************************************************************
    * 函数名称	:  FindExistAvpItem
    * 函数描述	:  寻找存在的avp item
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbAvpItem * TMdbCspParser::FindExistAvpItem(TMdbAvpItem * pStartItem, int iAvpCode)
    {
        if(NULL == pStartItem)
        {
            return NULL;
        }
        TMdbAvpItem * pRetItem = pStartItem;
        TMdbAvpItem * pTempItem = NULL;
        while(NULL != pRetItem)
        {
            //if(iAvpCode == pRetItem->iCode && pRetItem->bIsExist)
            if(iAvpCode == pRetItem->iCode && false == IsAvpFree(pRetItem))
            {
                break;   //找到
            }
            if(CSP_TYPE_GROUP == pRetItem->iType && iAvpCode != AVP_PARAM_STR_GROUP)
            {//批处理不需要找子节点
                //若是租节点查找其子节点
                pTempItem = FindExistAvpItem(pRetItem->pChildItem,iAvpCode);
                if(NULL != pTempItem)
                {
                    pRetItem = pTempItem;
                    break;
                }
            }
            pRetItem = pRetItem->pNextItem;
        }
        return pRetItem;
    }

    /******************************************************************************
    * 函数名称	:  GetExistGroupItem
    * 函数描述	:  寻找存在avpgroup
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::GetExistGroupItem(TMdbAvpItem * pStartItem,int iAvpCode,std::vector<TMdbAvpItem * > &vGroupAvpItem)
    {
        int iRet = 0;
        if(NULL == pStartItem)
        {
            return iRet;
        }
        TMdbAvpItem * pTempItem = pStartItem;
        while(NULL != pTempItem)
        {
            if(CSP_TYPE_GROUP == pTempItem->iType && iAvpCode == pTempItem->iCode && 
                /*pTempItem->bIsExist*/ false == IsAvpFree(pTempItem))
            {
                vGroupAvpItem.push_back(pTempItem);
            }
            pTempItem = pTempItem->pNextItem;
        }
        return iRet;
    }
    //获取首个批处理组
    TMdbAvpItem * TMdbCspParser::GetExistGroupItemForParam(TMdbAvpItem * pStartItem,int iAvpCode)
    {
        if(NULL == pStartItem)
        {
            return NULL;
        }
        TMdbAvpItem * pTempItem = pStartItem;
        TMdbAvpItem * pRetItem = NULL;
        while(NULL != pTempItem)
        {
            if(CSP_TYPE_GROUP   == pTempItem->iType 
                && iAvpCode     == pTempItem->iCode  
                && false        == IsAvpFree(pTempItem))
            {
                pRetItem = pTempItem;
                break;
            }
            pTempItem = pTempItem->pNextItem;
        }
        return pRetItem;
    }

    /******************************************************************************
    * 函数名称	:  PrintTotalAvp
    * 函数描述	:  打印所有avp
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    void TMdbAvpHelper::PrintTotalAvp(int iOffset,TMdbAvpItem * pRootAvp)
    {
        if(NULL == pRootAvp)
        {
            return;
        }
        TMdbAvpItem * pTemp = pRootAvp;
        while(pTemp)
        {
            PrintAvpItem(iOffset,pTemp);
            pTemp = pTemp->pNextItem;
        }
        return;
    }

    /******************************************************************************
    * 函数名称	:  PrintAvpItem
    * 函数描述	:  打印avp
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    void TMdbAvpHelper::PrintAvpItem(int iOffset,TMdbAvpItem * pOneAvp)
    {
        if(NULL == pOneAvp)
        {
            return;
        }
        PrintOffset(iOffset);
        char sValue[4096] = {0};
        /*
        printf("[%-20s]:code[%-5d],type[%-15s],len[%-3d],isExist[%d],IsMul[%d],IsM[%d],level[%d],value[%s],\n",
        						pOneAvp->sName,pOneAvp->iCode,
        						TMdbAvpHelper::TypeToStr(pOneAvp->iType),
        						pOneAvp->iLen,pOneAvp->bIsExist,
        						pOneAvp->bIsMul,pOneAvp->bIsM,pOneAvp->iLevel,
        						GetAvpValue(pOneAvp,sValue)
        						);
        */
        TADD_NORMAL("[%-20s]:code[%-5d],type[%-15s],len[%-3d],isExist[%d],IsMul[%d],IsM[%d],level[%d],value[%s],iUpdateFlag[%u]\n",
                    pOneAvp->sName,pOneAvp->iCode,
                    TMdbAvpHelper::TypeToStr(pOneAvp->iType),
                    pOneAvp->iLen,pOneAvp->bIsExist,
                    pOneAvp->bIsMul,pOneAvp->bIsM,pOneAvp->iLevel,
                    GetAvpValue(pOneAvp,sValue),pOneAvp->iUpdateFlag
                   );
        if(CSP_TYPE_GROUP == pOneAvp->iType && AVP_PARAM_STR_GROUP != pOneAvp->iCode)
        {
            //组类型
            PrintTotalAvp(iOffset + 1,pOneAvp->pChildItem);
        }
    }

    /******************************************************************************
    * 函数名称	:  TypeToStr
    * 函数描述	:
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    const char * TMdbAvpHelper::TypeToStr(int iType)
    {
        if(iType < 0 || iType >=(int)(sizeof(m_cspTypeStr)/sizeof(char *)))
        {
            return "error type";
        }
        return m_cspTypeStr[iType];
    }

    /******************************************************************************
    * 函数名称	:  StrToType
    * 函数描述	:
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbAvpHelper::StrToType(const char * sTypeStr)
    {
        int i = 0;
        for(i = 0; i <(int)(sizeof(m_cspTypeStr)/sizeof(char *)); i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(sTypeStr,m_cspTypeStr[i]) == 0)
            {
                return i;
            }
        }
        return CSP_TYPE_UNKOWN;
    }


    /******************************************************************************
    * 函数名称	:  GetAvpValue
    * 函数描述	:  获取avp 的值
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    const char * TMdbAvpHelper::GetAvpValue(TMdbAvpItem * pAvpItem,char * sValue)
    {
        if(NULL == pAvpItem)
        {
            return "";
        }
        switch(pAvpItem->iType)
        {
        case CSP_TYPE_STRING:
        case CSP_TYPE_TIME:
        case CSP_TYPE_GROUP:
        {
            return pAvpItem->pszValue?pAvpItem->pszValue:"nil";
        }
        case CSP_TYPE_INT32:
        {
            sprintf(sValue,"%d",pAvpItem->iValue);
            break;
        }
        case CSP_TYPE_INT64:
        {
            sprintf(sValue,"%lld",pAvpItem->llValue);
            break;
        }
        case CSP_TYPE_UINT32:
        {
            sprintf(sValue,"%u",pAvpItem->uiValue);
            break;
        }
        case CSP_TYPE_UINT64:
        {
            sprintf(sValue,"%llu",pAvpItem->ullValue);
            break;
        }
        default:
        {
            break;
        }
        }
        return sValue;
    }
#if 0
    /******************************************************************************
    * 函数名称	:  CopyItem
    * 函数描述	:  copy item
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbAvpItem* TMdbAvpHelper::CopyItem(TMdbAvpItem* pItem)
    {
        if(pItem == NULL)
            return NULL;
        TMdbAvpItem* pItemNode = new TMdbAvpItem();
        //拷贝本节点数据
        SAFESTRCPY(pItemNode->sName,sizeof(pItemNode->sName),pItem->sName);
        pItemNode->iCode    = pItem->iCode;			//AVP-Code
        pItemNode->iLen	   = pItem->iLen;			//AVP长度
        pItemNode->iType    = pItem->iType;			//数据类型
        pItemNode->bIsExist = false; 				//是否存在
        pItemNode->bIsMul   = pItem->bIsMul; 		//是否允许有多个
        pItemNode->bIsM	   = pItem->bIsM;		    //是否必须存在
        pItemNode->iLevel   = pItem->iLevel; 	   //所处层次，依次为1.2.3.....
        //根据数据类型分配内存
        if(pItemNode->iType == CSP_TYPE_STRING)
        {
            pItemNode->pszValue = new char[MAX_AVP_VALUE_LEN];
            if( pItemNode->pszValue == NULL)
            {
                TADD_ERROR("Mem not Enough");
                return NULL;
            }
            pItemNode->pszValue[0] = '\0';
        }
        else if(pItemNode->iType == CSP_TYPE_TIME)
        {
            pItemNode->pszValue = new char[MAX_AVP_TIME_LEN];
            if( pItemNode->pszValue == NULL)
            {
                TADD_ERROR("Mem not Enough");
                return NULL;
            }
            pItemNode->pszValue[0] = '\0';
        }
        else
        {
            pItemNode->pszValue = NULL;
        }
        pItemNode->iValue   = -1;
        pItemNode->uiValue  = 0;
        pItemNode->llValue  = -1;
        pItemNode->ullValue = 0;
        pItemNode->pFatherItem = pItem->pFatherItem;  //父亲节点
        pItemNode->pChildItem  = NULL;  //子节点
        pItemNode->pNextItem   = NULL;  //兄弟节点
        //如果是单节点，直接返回;如果是树形节点，则递归处理每个字节点
        if(pItemNode->iType == CSP_TYPE_GROUP)
        {
            TMdbAvpItem* pItemNodeChild = pItem->pChildItem;
            TMdbAvpItem * pTailItem	   = NULL;//记录尾节点，这边不使用头插，保证其和被copy 的元素顺序一致
            while(pItemNodeChild != NULL)
            {
                TMdbAvpItem* pItemNew = CopyItem(pItemNodeChild);
                if(pItemNode->pChildItem == NULL)
                {
                    pItemNode->pChildItem = pItemNew;
                    pTailItem = pItemNew;
                }
                else
                {
                    pTailItem->pNextItem = pItemNew;
                }
                pTailItem	  = pItemNew;
                pItemNodeChild = pItemNodeChild->pNextItem;
            }
        }
        return pItemNode;
    }

    /******************************************************************************
    * 函数名称	:  GetFreeAvpItem
    * 函数描述	:  获取一个空闲avp item ，
    				   若是group则要预留一个空闲的(用于后续的copy item)。
    				   只有group节点才可以copy出新节点
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbAvpItem * TMdbAvpHelper::GetFreeAvpItem(TMdbAvpItem * pStartItem, int iAvpCode)
    {
        if(NULL == pStartItem)
        {
            return NULL;
        }
        TMdbAvpItem * pRetItem	= NULL;
        TMdbAvpItem * pTempItem = pStartItem;
        while(NULL != pTempItem)
        {
            if(pTempItem->iCode == iAvpCode && false == pTempItem->bIsExist)
            {
                pRetItem = pTempItem;
                if(CSP_TYPE_GROUP == pTempItem->iType &&
                        (NULL == pTempItem->pNextItem || pTempItem->pNextItem->iCode != iAvpCode)
                  )
                {
                    //对于group的项，要预留一个空闲的
                    TMdbAvpItem * pNewItem = TMdbAvpHelper::CopyItem(pTempItem);
                    if(NULL == pNewItem)
                    {
                        TADD_ERROR("copy item failed.");
                        return NULL;
                    }
                    pNewItem->pNextItem = pTempItem->pNextItem;
                    pTempItem->pNextItem = pNewItem;
                }
                break;
            }
            pTempItem = pTempItem->pNextItem;
        }
        return pRetItem;
    }

    /******************************************************************************
    * 函数名称	:  GetStringValue
    * 函数描述	:  获取字符串值，从pStartItem开始查找为iAvpCode的节点。
    				   该节点的bIsExist = true.并获取值。若数据类型不对也会抛出异常
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/

    char *		TMdbAvpHelper::GetStringValue(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_TIME != pTemp->iType && CSP_TYPE_STRING != pTemp->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->pszValue;

    }
    /******************************************************************************
    * 函数名称	:  GetUINT32Value
    * 函数描述	:  获取UINT32值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    unsigned int TMdbAvpHelper::GetUINT32Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_UINT32!= pTemp->iType )
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->uiValue;

    }
    /******************************************************************************
    * 函数名称	:  GetINT32Value
    * 函数描述	:  获取INT32值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int	TMdbAvpHelper::GetINT32Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_INT32!= pTemp->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->iValue;

    }
    /******************************************************************************
    * 函数名称	:  GetINT64Value
    * 函数描述	:  获取INT64值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    long long	TMdbAvpHelper::GetINT64Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_INT64 != pTemp->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->llValue;

    }
    /******************************************************************************
    * 函数名称	:  GetUINT64Value
    * 函数描述	:  获取UINT64值
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    unsigned long long TMdbAvpHelper::GetUINT64Value(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        if(CSP_TYPE_UINT64 != pTemp->iType)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"Get code[%d] type [%d] error",iAvpCode,pTemp->iType);
        }
        return pTemp->ullValue;
    }
    /******************************************************************************
    * 函数名称	:  IsNullValue
    * 函数描述	:  判断是否是null value
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/

    bool TMdbAvpHelper::IsNullValue(TMdbAvpItem * pStartItem, int iAvpCode)throw (TMdbCSPException)
    {
        TMdbAvpItem * pTemp = FindExistAvpItem(pStartItem,iAvpCode);
        if(NULL == pTemp)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,"not find Exist avp[%d].",iAvpCode);
        }
        return pTemp->m_bNULLValue;
    }

    /******************************************************************************
    * 函数名称	:  FindExistAvpItem
    * 函数描述	:  寻找存在的avp item
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbAvpItem * TMdbAvpHelper::FindExistAvpItem(TMdbAvpItem * pStartItem, int iAvpCode)
    {
        if(NULL == pStartItem)
        {
            return NULL;
        }
        TMdbAvpItem * pRetItem = pStartItem;
        TMdbAvpItem * pTempItem = NULL;
        while(NULL != pRetItem)
        {
            if(iAvpCode == pRetItem->iCode && pRetItem->bIsExist)
            {
                break;   //找到
            }
            if(CSP_TYPE_GROUP == pRetItem->iType)
            {
                //若是租节点查找其子节点
                pTempItem = FindExistAvpItem(pRetItem->pChildItem,iAvpCode);
                if(NULL != pTempItem)
                {
                    pRetItem = pTempItem;
                    break;
                }
            }
            pRetItem = pRetItem->pNextItem;
        }
        return pRetItem;
    }

    /******************************************************************************
    * 函数名称	:  GetExistGroupItem
    * 函数描述	:  寻找存在avpgroup
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbAvpHelper::GetExistGroupItem(TMdbAvpItem * pStartItem,int iAvpCode,std::vector<TMdbAvpItem * > &vGroupAvpItem)
    {
        int iRet = 0;
        if(NULL == pStartItem)
        {
            return iRet;
        }
        TMdbAvpItem * pTempItem = pStartItem;
        while(NULL != pTempItem)
        {
            if(CSP_TYPE_GROUP == pTempItem->iType && iAvpCode == pTempItem->iCode && pTempItem->bIsExist)
            {
                vGroupAvpItem.push_back(pTempItem);
            }
            pTempItem = pTempItem->pNextItem;
        }
        return iRet;
    }

#endif

//}
