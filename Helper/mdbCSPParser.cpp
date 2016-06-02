/****************************************************************************************
*@Copyrights  2010�����������Ͼ�����������޹�˾ ������ CCB��Ŀ��
*@                   All rights reserved.
*@Name��	      mdbCSPParser.cpp
*@Description��   CSP�������
*@Author:		  li.shugang,jiangmingjun
*@Date��	      2010��10��21��
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

    #define SKIP_TAB(_text,_pos) while(_text[_pos] == '\t' || _text[_pos] == ' ') {++_pos;}  //����ǰ��Ŀո��TAB 
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
    * ��������	:  TMdbCSPException
    * ��������	:  CSP Э������쳣
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  TMdbAvpHead
    * ��������	:  AVPͷ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
    *******************************************************************************/
    TMdbAvpHead::TMdbAvpHead()
    {
        //sHeadName[6] = '\0';    //����ͷ��ʼ
        memset(sHeadName,0,sizeof(sHeadName));
        strncpy(sHeadName,"<QMDB>",6);
        memset(sKeyValue,0,sizeof(sKeyValue)); //��ʼ��key
        iVersion  = MDB_CS_USE_OCP;        //Version(1���ֽ�)
        iLen      = 0x0;        //Message Length(4���ֽ�������ʾ)
        isequence = 0;
        iCmdCode  = CSP_APP_ERROR;//Command-Code(2���ֽ�)
        iSessionId = 0x0;       //sessionId (4���ֽ�)
        memset(sSendTime,0,6);  //date-time(6�ֽ��ַ�����ʾʱ����)
        iCurrentAvpFlag = 0;
        iAnsCodePos = 0;
        //iIsBigEndian = 0;
    }

    /******************************************************************************
    * ��������	:  SetCmdCode
    * ��������	:  ����������
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:
    *******************************************************************************/
    void TMdbAvpHead::SetCmdCode(int ComId)
    {
        iCmdCode = ComId;
    }
    /******************************************************************************
    * ��������	:  GetSequence
    * ��������	:  ��ȡ����ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
    *******************************************************************************/
    unsigned int TMdbAvpHead::GetSequence()
    {
        return isequence;
    }

    /******************************************************************************
    * ��������	:  SetSequence
    * ��������	:  ��������ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
    *******************************************************************************/
    void TMdbAvpHead::SetSequence(unsigned int iseq)
    {
        isequence = iseq;
    }
    /******************************************************************************
    * ��������	:  print
    * ��������	:  ��ӡ��Ϣ��ͷ��Ϣ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
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
    * ��������	:  Clear
    * ��������	:  ����
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
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
    * ��������	:  CnvtToBin
    * ��������	:  ���ı�����ת�ɶ����ư�
    * ����		:  pszMsg - ��Ϣ  SessionId - �ػ���ʶ(Ψһ)
    * ���		:
    * ����ֵ	:
    * ����		:
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
        //ʱ��
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
        //ʱ��
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
    * ��������	:  BinToCnvt
    * ��������	:  �����ư�ת���ı�����
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
    *******************************************************************************/
    void TMdbAvpHead::BinToCnvt(unsigned char* pszMsg)
    {
        //��ȡ����ͷ�����<QMDB>
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
        //��ȡ����ͷ�����<QMDB>
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
    * ��������	:  TMdbAvpItem
    * ��������	:  AVP ��Ĺ��캯��
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
    *******************************************************************************/
    TMdbAvpItem::TMdbAvpItem()
    {
        //memset(sName,  0, sizeof(sName));
        sName[0]  = 0;
        iCode     = 0;              //AVP-Code
        iLen      = 0;              //AVP����
        iType     = CSP_TYPE_UNKOWN; //��������
        bIsExist  = false;          //�Ƿ����
        bIsMul    = false;          //�Ƿ������ж��
        bIsM      = false;
        iLevel    = 1;              //Ĭ��Ϊ��һ��
        pszValue  = NULL;
        iValue    = 0;
        uiValue   = 0;
        llValue   = -1;
        ullValue  = 0;

        pFatherItem = NULL;
        pChildItem  = NULL;   //�ӽڵ�
        pNextItem   = NULL;   //�ֵܽڵ�
        m_bNULLValue = false;
        iUpdateFlag = 0;
    }

    /******************************************************************************
    * ��������	:  ~TMdbAvpItem
    * ��������	:  ����
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
    *******************************************************************************/
    TMdbAvpItem::~TMdbAvpItem()
    {
        SAFE_DELETE_ARRAY(pszValue);
        SAFE_DELETE(pChildItem);
        SAFE_DELETE(pNextItem);
    }
    /******************************************************************************
    * ��������	: Clear
    * ��������	:  ����һ��avp ���е�����
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
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
    * ��������	:  GetInfo
    * ��������	:  ��ȡ���ڵ��Լ��ӽڵ��ȫ�����ݣ��������л����ַ���
    * ����		:
    * ���		:  pszOut -  ������ַ���
    * ����ֵ	:
    * ����		:
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
        //�����趨AVP-Code / AVP-Len
        pszOut[0] = (iCode>>8) & 0xff;
        pszOut[1] = iCode & 0xff;
        pszOut[2] = (iLen>>8) & 0xff;
        pszOut[3] = iLen & 0xff;
        int iPos = 4; //ƫ��4���ֽ�
        if(iLen == 4) return iRet;
        //����������ԣ�����Ҫ��ȡȫ��������
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
    * ��������	:  SetAvp
    * ��������	:  ����avp�ڵ���Ϣ
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:
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
            {//NULL ֵ
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
        else    //�����Group���ͣ���λ�ӽڵ�
        {
            bIsExist = true;
            iLen     = iLenIn;
            int iAvp = 0;
            int iTempLen = 0;
            TMdbAvpItem* pItem = pChildItem;
            while(true)
            {
                pParser->GetAvpItemCodeLen(pszDCC + iPos,iAvp,iTempLen);
                //�ҵ�AVP��Ϣ
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
        iUpdateFlag = pParser->m_tHead.iCurrentAvpFlag;//����flag
        return 0;
    }


#if 0
    /******************************************************************************
    * ��������	:  GetTotalLen
    * ��������	:  ��ȡ���ڵ��Լ��ӽڵ�ĳ��Ⱥ�
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
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
    * ��������	:  FinishFill
    * ��������	:  ����������,��group�ڵ���ã����¼�����len
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbAvpItem::FinishFill()
    {
        bIsExist = true;
        GetTotalLen();
        return 0;
    }
#endif


    /******************************************************************************
    * ��������	:  TMdbCspParser
    * ��������	:  ����
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbCspParser::TMdbCspParser()
    {
        m_pRootAvpItem = NULL;
        m_iTotalLen = 0;

    }

    /******************************************************************************
    * ��������	:  ~TMdbCspParser
    * ��������	:  ����
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbCspParser::~TMdbCspParser()
    {
        SAFE_DELETE(m_pRootAvpItem);
    }
    /******************************************************************************
    * ��������	:  Init
    * ����		:  iType - Э�������  bRequest- true(request) false(answer)
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  ParserConfig
    * ��������	:  ���������ļ������ɰ��ṹ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        //��ǰAVP��
        TMdbAvpItem* pCurItem = NULL;
        //fseek(fp,0,SEEK_SET );
        TMdbNtcSplit tSplit;
        tSplit.SplitString(CSP_AVP_STRING, '\n');
        //�����ļ�
        //while(fgets(sLineText, sizeof(sLineText),fp))
        for(unsigned int i = 0; i<tSplit.GetFieldCount(); i++)
        {
            SAFESTRCPY(sLineText, sizeof(sLineText), tSplit[i]);
            TMdbNtcStrFunc::Trim(sLineText);
            if ( ( sLineText[0] == '\n' ) || strlen(sLineText) == 0) //����ֱ�ӹ���
                continue;
            /* into the section */
            if ( !nAct && strstr( sLineText, pszIdemt ) )
            {
                nAct = 1;
                continue;
            }
            //��ȡ�����зָ��˳�.
            if(nAct && ( sLineText[0] == '#' )&& ( sLineText[1] == '#' ) )
            {
                break;
            }
            /* front of the section */
            if ( !nAct )
                continue;
            //������һ��
            if(iLinePos == 0)
            {
                ++iLinePos;
                continue;
            }
            //�ѵ�ǰ���ֽ���ΪAVP��
            TMdbAvpItem* pItem = ParserLine(sLineText);
            if(pItem == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Invalid Line = [%s].", sLineText);
                return -1;
            }
            //��AVP����뵱ǰ�б���
            AddItem(pCurItem,pItem);
        }
        //SAFE_CLOSE(fp);
        return iRet;
    }
    /******************************************************************************
    * ��������	:  AddItem
    * ��������	:  �����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::AddItem(TMdbAvpItem * &pCurItem,TMdbAvpItem* pItem)
    {
        if(pCurItem == NULL)
        {
            m_pRootAvpItem = pItem; //m_pRootAvpItem ��¼��ini�ļ��е�һ�е�avp
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
    * ��������	:  GetStr
    * ��������	:  ��ȡstrֵ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  GetInt
    * ��������	:  ��ȡintֵ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::GetItemCode(const char* pszLineText,int & iPos,TMdbAvpItem* pItem)
    {
        for(int i=iPos; pszLineText[i] != ' ' &&  pszLineText[i] != '\t'; ++i)
        {
            //��������֣������˳�
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
    * ��������	:  ParserLine
    * ��������	:  �ѵ�ǰ���ֽ���ΪAVP��
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        //�Ƿ�����ظ�
        SET_T_F(pszLineText,iPos,'*',pItem->bIsMul);
        //��������ȡ����
        if(GetItemName(pszLineText,iPos,pItem) != 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Invalid Char=[%c].", pszLineText[iPos]);
            delete pItem;
            return NULL;
        }
        //�ҵ����ڵڼ���
        SKIP_TAB(pszLineText,iPos);
        pItem->iLevel = pszLineText[iPos]-'0';
        ++iPos;
        //�������ǿո�+����AVP-CODE
        SKIP_TAB(pszLineText,iPos);
        pItem->iCode = 0;
        if(GetItemCode(pszLineText,iPos, pItem) != 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Invalid Char=[%c].", pszLineText[iPos]);
            delete pItem;
            return NULL;
        }
        ++iPos;
        //�������ǿո�+�Ƿ��ѡ
        SKIP_TAB(pszLineText,iPos);
        pItem->bIsM = (pszLineText[iPos++]=='M' || pszLineText[iPos++]=='m');
        //�������ǿո�+��������
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
        m_tHead.Print();//ͷ��Ϣ��ӡ
        TMdbAvpHelper::PrintTotalAvp(0,m_pRootAvpItem);//avp��Ϣ����Ϣ��ӡ
    }

    /******************************************************************************
    * ��������	:  Serialize
    * ��������	:  ��һ�����ݰ����л��ɶ����ƴ�
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        //Clear(); //���
        return m_tHead.iLen;
    }

    /******************************************************************************
    * ��������	:  DeSerialize
    * ��������	:  ���������ƴ�
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::DeSerialize(unsigned char* pszDCC,int iDCCLen) throw (TMdbCSPException)
    {
        int iRet = 0;
        m_tHead.BinToCnvt(pszDCC);//��ͷ����
        Clear();//��������
        int iPos  = SIZE_MSG_AVP_HEAD;
        int iCode = 0;
        int iLen  = 0 ;
        TMdbAvpItem * pTempAvpItem = m_pRootAvpItem;
        while(iPos < iDCCLen)
        {
            GetAvpItemCodeLen(pszDCC + iPos,iCode,iLen);
            pTempAvpItem = GetFreeAvpItem(m_pRootAvpItem,iCode);//��ȡһ�����е�avp
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
    * ��������	:  GetTotalLen
    * ��������	:  ��ȡ�ܳ���
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*
    int TMdbCspParser::GetTotalLen()
    {
        int iLen = 6+16+1+4+2+4+6+4;
        iLen += GetItemLen(m_pRootAvpItem);
        return iLen;
    }
    */
    //��ȡ�ܳ���
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
    * ��������	:  Clear
    * ��������	:  ��������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbCspParser::Clear()
    {
        m_tHead.UpdateAvpFlag();//����head�е�avp��ʶ���ϻ�������
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
    * ��������	:  SetItemValue
    * ��������	:  ����item��ֵ�����ݲ���һ�����нڵ�(�ýڵ�bIsExist = false),
    				   ÿ���ڵ�ֻ�ܱ�����һ�Ρ���������������Ͳ���Ҳ���׳��쳣��
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    //���������⴦��
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
    * ��������	:  SetItemValue
    * ��������	:  ����NULLvalue
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        pTempItem->iLen = 4 ;//+ strlen(pTempItem->pszValue)+1;û��valueֵ
        m_iTotalLen +=pTempItem->iLen;
        m_iIncreaseLen += pTempItem->iLen;
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SetItemValue
    * ��������	:  ����item��ֵ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  CopyItem
    * ��������	:  copy item
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        //�������ڵ�����
        SAFESTRCPY(pItemNode->sName,sizeof(pItemNode->sName),pItem->sName);
        pItemNode->iCode    = pItem->iCode;			//AVP-Code
        pItemNode->iLen	   = pItem->iLen;			//AVP����
        pItemNode->iType    = pItem->iType;			//��������
        pItemNode->bIsExist = false; 				//�Ƿ����
        pItemNode->bIsMul   = pItem->bIsMul; 		//�Ƿ������ж��
        pItemNode->bIsM	   = pItem->bIsM;		    //�Ƿ�������
        pItemNode->iLevel   = pItem->iLevel; 	   //������Σ�����Ϊ1.2.3.....
        pItemNode->iUpdateFlag = 0;
        //�����������ͷ����ڴ�
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
        pItemNode->pFatherItem = pItem->pFatherItem;  //���׽ڵ�
        pItemNode->pChildItem  = NULL;  //�ӽڵ�
        pItemNode->pNextItem   = NULL;  //�ֵܽڵ�
        //����ǵ��ڵ㣬ֱ�ӷ���;��������νڵ㣬��ݹ鴦��ÿ���ֽڵ�
        if(pItemNode->iType == CSP_TYPE_GROUP && pItemNode->iCode != AVP_PARAM_STR_GROUP)
        {
            TMdbAvpItem* pItemNodeChild = pItem->pChildItem;
            TMdbAvpItem * pTailItem	   = NULL;//��¼β�ڵ㣬��߲�ʹ��ͷ�壬��֤��ͱ�copy ��Ԫ��˳��һ��
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
    * ��������	:  GetFreeAvpItem
    * ��������	:  ��ȡһ������avp item ��
    				   ����group��ҪԤ��һ�����е�(���ں�����copy item)��
    				   ֻ��group�ڵ�ſ���copy���½ڵ�
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
                    //����group���ҪԤ��һ�����е�
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
    * ��������	:  GetStringValue
    * ��������	:  ��ȡ�ַ���ֵ����pStartItem��ʼ����ΪiAvpCode�Ľڵ㡣
    				   �ýڵ��bIsExist = true.����ȡֵ�����������Ͳ���Ҳ���׳��쳣
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    //���������⴦��
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
    * ��������	:  GetUINT32Value
    * ��������	:  ��ȡUINT32ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  GetINT32Value
    * ��������	:  ��ȡINT32ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  GetINT64Value
    * ��������	:  ��ȡINT64ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  GetUINT64Value
    * ��������	:  ��ȡUINT64ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  IsNullValue
    * ��������	:  �ж��Ƿ���null value
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  FindExistAvpItem
    * ��������	:  Ѱ�Ҵ��ڵ�avp item
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
                break;   //�ҵ�
            }
            if(CSP_TYPE_GROUP == pRetItem->iType && iAvpCode != AVP_PARAM_STR_GROUP)
            {//��������Ҫ���ӽڵ�
                //������ڵ�������ӽڵ�
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
    * ��������	:  GetExistGroupItem
    * ��������	:  Ѱ�Ҵ���avpgroup
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    //��ȡ�׸���������
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
    * ��������	:  PrintTotalAvp
    * ��������	:  ��ӡ����avp
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  PrintAvpItem
    * ��������	:  ��ӡavp
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
            //������
            PrintTotalAvp(iOffset + 1,pOneAvp->pChildItem);
        }
    }

    /******************************************************************************
    * ��������	:  TypeToStr
    * ��������	:
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  StrToType
    * ��������	:
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  GetAvpValue
    * ��������	:  ��ȡavp ��ֵ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  CopyItem
    * ��������	:  copy item
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbAvpItem* TMdbAvpHelper::CopyItem(TMdbAvpItem* pItem)
    {
        if(pItem == NULL)
            return NULL;
        TMdbAvpItem* pItemNode = new TMdbAvpItem();
        //�������ڵ�����
        SAFESTRCPY(pItemNode->sName,sizeof(pItemNode->sName),pItem->sName);
        pItemNode->iCode    = pItem->iCode;			//AVP-Code
        pItemNode->iLen	   = pItem->iLen;			//AVP����
        pItemNode->iType    = pItem->iType;			//��������
        pItemNode->bIsExist = false; 				//�Ƿ����
        pItemNode->bIsMul   = pItem->bIsMul; 		//�Ƿ������ж��
        pItemNode->bIsM	   = pItem->bIsM;		    //�Ƿ�������
        pItemNode->iLevel   = pItem->iLevel; 	   //������Σ�����Ϊ1.2.3.....
        //�����������ͷ����ڴ�
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
        pItemNode->pFatherItem = pItem->pFatherItem;  //���׽ڵ�
        pItemNode->pChildItem  = NULL;  //�ӽڵ�
        pItemNode->pNextItem   = NULL;  //�ֵܽڵ�
        //����ǵ��ڵ㣬ֱ�ӷ���;��������νڵ㣬��ݹ鴦��ÿ���ֽڵ�
        if(pItemNode->iType == CSP_TYPE_GROUP)
        {
            TMdbAvpItem* pItemNodeChild = pItem->pChildItem;
            TMdbAvpItem * pTailItem	   = NULL;//��¼β�ڵ㣬��߲�ʹ��ͷ�壬��֤��ͱ�copy ��Ԫ��˳��һ��
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
    * ��������	:  GetFreeAvpItem
    * ��������	:  ��ȡһ������avp item ��
    				   ����group��ҪԤ��һ�����е�(���ں�����copy item)��
    				   ֻ��group�ڵ�ſ���copy���½ڵ�
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
                    //����group���ҪԤ��һ�����е�
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
    * ��������	:  GetStringValue
    * ��������	:  ��ȡ�ַ���ֵ����pStartItem��ʼ����ΪiAvpCode�Ľڵ㡣
    				   �ýڵ��bIsExist = true.����ȡֵ�����������Ͳ���Ҳ���׳��쳣
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  GetUINT32Value
    * ��������	:  ��ȡUINT32ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  GetINT32Value
    * ��������	:  ��ȡINT32ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  GetINT64Value
    * ��������	:  ��ȡINT64ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  GetUINT64Value
    * ��������	:  ��ȡUINT64ֵ
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  IsNullValue
    * ��������	:  �ж��Ƿ���null value
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  FindExistAvpItem
    * ��������	:  Ѱ�Ҵ��ڵ�avp item
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
                break;   //�ҵ�
            }
            if(CSP_TYPE_GROUP == pRetItem->iType)
            {
                //������ڵ�������ӽڵ�
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
    * ��������	:  GetExistGroupItem
    * ��������	:  Ѱ�Ҵ���avpgroup
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
