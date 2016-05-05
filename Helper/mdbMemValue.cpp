/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbMemValue.cpp		
*@Description�� SQL���������ڴ�ֵ������Ƚ�
*@Author:	 jin.shaohua
*@Date��	    2012.07
*@History:
******************************************************************************************/
//#include "BillingSDK.h"
#include "Helper/mdbMemValue.h"
#include "Helper/mdbDateTime.h"
//using namespace ZSmart::BillingSDK;
//namespace QuickMDB{
    /******************************************************************************
    * ��������	:  GetIntValue
    * ��������	:  ��ȡ��ֵ����
    * ����		:  pstMemValue
    * ���		:  lValue ��ֵ����
    * ����ֵ	:  0:��ȡ�ɹ���<0:��ȡʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbMemValue::GetIntValue(ST_MEM_VALUE*  pstMemValue,long long & lValue)
    {
    	int iRet = 0;
    	if(NULL == pstMemValue)
    	{
    		return -1;
    	}

        if(pstMemValue->IsNull())
        {
            TADD_ERROR(ERROR_UNKNOWN,"pstMemValue is null value");
            return -1;
        }
    	if(MemValueHasProperty(pstMemValue,MEM_Str))
    	{
    		if(TMdbNtcStrFunc::IsDigital(pstMemValue->sValue))
    		{
    			lValue = TMdbNtcStrFunc::StrToInt(pstMemValue->sValue);
    		}
    		else
    		{
    			return -1;
    		}
    	}
    	else
    	{
    		lValue = pstMemValue->lValue;

    	}
    	return iRet;
    }
    /******************************************************************************
    * ��������	:  CompareExprValue
    * ��������	:  �Ƚ���ֵ����ֵ
    * ����		:  
    * ����		:  pstLeftMemValue - ��ֵ pstRightMemValue - ��ֵ
    * ���		:  
    * ����ֵ	:  0 - ��ͬ >0 - left>right   <0 -left<right  
    * ����		:  jin.shaohua
    *******************************************************************************/
    long long TMdbMemValue::CompareExprValue(ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue,bool bNullFlag)
    {
    	TADD_FUNC("Start.pstLeftMemValue=[%p],pstRightMemValue=[%p].",pstLeftMemValue,pstRightMemValue);
    	long long llRet = 0;
    	int iRet = 0;
    	if(NULL == pstLeftMemValue || NULL == pstRightMemValue){return -1;}
        if(bNullFlag)
        {//�Ƿ���Ҫ�ж�null
            if(pstLeftMemValue->IsNull() || pstRightMemValue->IsNull())
            {//��ֵΪNULL
                return CompareNullValue(pstLeftMemValue, pstRightMemValue);
            }
        }
    	//else 
    	if(MemValueHasProperty(pstLeftMemValue,MEM_Int) || MemValueHasProperty(pstRightMemValue,MEM_Int))
    	{//������һ������ֵ��
    		long long llLeftValue = 0;
    		long long llRightValue = 0;
    		CHECK_RET(GetIntValue(pstLeftMemValue,llLeftValue),"GetIntValue error.");
    		CHECK_RET(GetIntValue(pstRightMemValue,llRightValue),"GetIntValue error.");
    		TADD_DETAIL("llLeftValue=[%lld],llRightValue=[%lld]",llLeftValue,llRightValue);
    		return (llLeftValue - llRightValue);
    	}
    	else
    	{
    		TADD_DETAIL("LeftValue=[%s],RightValue=[%s]",pstLeftMemValue->sValue,pstRightMemValue->sValue);
    		return strcmp(pstLeftMemValue->sValue,pstRightMemValue->sValue);
    	}
    	TADD_FUNC("Finish.");
    	return 0==iRet?llRet:iRet;
    }

    /******************************************************************************
    * ��������	:  CompareMemValueList
    * ��������	:  �Ƚ�memvaluelist
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - ��ͬ >0 - left>right   <0 -left<right 
    * ����		:  jin.shaohua
    *******************************************************************************/
    long long TMdbMemValue::CompareMemValueList(ST_MEM_VALUE_LIST & stLeftList,ST_MEM_VALUE_LIST & stRightList)
    {
    	TADD_FUNC("Start.");
    	long long llRet = 0;
    	llRet = stLeftList.iItemNum - stRightList.iItemNum;
    	if(0 == llRet)
    	{//����һֱ���ܼ����Ƚ�
    		int i = 0;
    		for(i = 0;i<stLeftList.iItemNum;++i)
    		{
    			llRet = CompareExprValue(stLeftList.pValueArray[i],stRightList.pValueArray[i]);
    			if(0 != llRet){break;}
    		}
    	}
    	TADD_FUNC("Finish.");
    	return llRet;
    }

    /******************************************************************************
    * ��������	:  CalcMemValue
    * ��������	:  ����MemValueֵ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbMemValue::CalcMemValue(int iOpcode,ST_MEM_VALUE*	pstRetMemValue,ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue,bool bNullFlag)
    {
    	TADD_FUNC("Start.iOpcode=[%d][%s]",iOpcode,TokenName[iOpcode]);
        if(bNullFlag && (pstLeftMemValue->IsNull() || pstRightMemValue->IsNull()))	
        {
            pstRetMemValue->SetNull();
            return 0;
        }
    	int iRet = 0;
        int iOneDaySec = 24*60*60;//һ�������
    	if(MemValueHasProperty(pstRetMemValue,MEM_Date))
    	{//ʱ����������
    		if(MemValueHasProperty(pstLeftMemValue,MEM_Date) && MemValueHasProperty(pstRightMemValue,MEM_Int))
    		{
    			TADD_DETAIL("Date[%s],int[%lld]",pstLeftMemValue->sValue,pstRightMemValue->lValue);
                         SAFESTRCPY(pstRetMemValue->sValue,pstRetMemValue->iSize,pstLeftMemValue->sValue);
                         switch(iOpcode)
    			{
    				case TK_PLUS://+
    					TMdbDateTime::AddDay(pstRetMemValue->sValue,pstRightMemValue->lValue);
    					break;
    				case TK_MINUS://-
    					TMdbDateTime::AddDay(pstRetMemValue->sValue,0-pstRightMemValue->lValue);
    					break;
    				default:
    					CHECK_RET(-1,"opcode[%d] not support now",iOpcode);
    					break;
    			}
                        /*
    			time_t iTime = TMdbDateTime::StringToTimeT(pstLeftMemValue->sValue);
    			switch(iOpcode)
    			{
    				case TK_PLUS://+
    					iTime += pstRightMemValue->lValue*iOneDaySec;
    					break;
    				case TK_MINUS://-
    					iTime -= pstRightMemValue->lValue*iOneDaySec;
    					break;
    				default:
    					CHECK_RET(-1,"opcode[%d] not support now",iOpcode);
    					break;
    			}
    			TMdbDateTime::TimeToString(iTime,pstRetMemValue->sValue);
    			*/
    		}
    		else if(MemValueHasProperty(pstRightMemValue,MEM_Date) && MemValueHasProperty(pstLeftMemValue,MEM_Int))
    		{
    			TADD_DETAIL("int[%lld],date[%s]",pstLeftMemValue->lValue,pstRightMemValue->sValue);
                         SAFESTRCPY(pstRetMemValue->sValue,pstRetMemValue->iSize,pstRightMemValue->sValue);
                         switch(iOpcode)
    			{
    				case TK_PLUS://+
    					TMdbDateTime::AddDay(pstRetMemValue->sValue,pstLeftMemValue->lValue);
    					break;
    				case TK_MINUS://-
    					CHECK_RET(-1,"num - date is error.");
    					break;
    				default:
    					CHECK_RET(-1,"opcode[%d] not support now",iOpcode);
    					break;
    			}
                         /*
    			time_t iTime = TMdbDateTime::StringToTimeT(pstRightMemValue->sValue);
    			switch(iOpcode)
    			{
    				case TK_PLUS://+
    					iTime += pstLeftMemValue->lValue*iOneDaySec;
    					break;
    				case TK_MINUS://-
    					CHECK_RET(-1,"num - date is error.");
    					break;
    				default:
    					CHECK_RET(-1,"opcode[%d] not support now",iOpcode);
    					break;
    			}
    			TMdbDateTime::TimeToString(iTime,pstRetMemValue->sValue);
    			*/
    		}
    		else if(MemValueHasProperty(pstRightMemValue,MEM_Int) && MemValueHasProperty(pstLeftMemValue,MEM_Int))
    		{
    			TADD_DETAIL("int[%lld],int[%lld]",pstLeftMemValue->lValue,pstRightMemValue->lValue);	
    			switch(iOpcode)
    			{
    				case TK_PLUS://+
    					pstRetMemValue->lValue = pstLeftMemValue->lValue + pstRightMemValue->lValue;
    					break;
    				case TK_MINUS://-
    					pstRetMemValue->lValue = pstLeftMemValue->lValue - pstRightMemValue->lValue;
    					break;
    				default:
    					CHECK_RET(-1,"opcode[%d] not support now",iOpcode);
    					break;
    			}
    		}
    		else
    		{
    			CHECK_RET(-1,"subtype error left[%d],right[%d],ret[%d]",pstLeftMemValue->iFlags,
    							pstRightMemValue->iFlags,pstRetMemValue->iFlags);
    		}
    	}
    	else
    	{//return = int
    	       if(MemValueHasProperty(pstLeftMemValue,MEM_Date) && MemValueHasProperty(pstRightMemValue,MEM_Date))
                  {
                        TADD_DETAIL("left date [%s],right date[%s]",pstLeftMemValue->sValue,pstRightMemValue->sValue);
                        
                        time_t iLeftTime = TMdbDateTime::StringToTime(pstLeftMemValue->sValue,0); //ʹ��GMTʱ��
                        time_t iRightTime = TMdbDateTime::StringToTime(pstRightMemValue->sValue,0);//ʹ��GMT ʱ��
    			switch(iOpcode)
    			{
    				case TK_PLUS://+
    					CHECK_RET(-1,"opcode[%d]:date + date not allow",iOpcode);
    					break;
    				case TK_MINUS://- //date��������Ľ�������Ӿ���
    					pstRetMemValue->lValue = (iLeftTime - iRightTime)/iOneDaySec;
    					pstRetMemValue->dValue = (iLeftTime - iRightTime)*1.0/iOneDaySec;
    					MemValueSetProperty(pstRetMemValue,MEM_Float);
    					break;
    				default:
    					CHECK_RET(-1,"opcode[%d] not support now",iOpcode);
    					break;
    			}
                  }
                  else
                  {
                      TADD_DETAIL("left int[%lld],right int[%lld]",pstLeftMemValue->lValue,pstRightMemValue->lValue);    
                      switch(iOpcode)
                      {
                          case TK_PLUS://+
                              pstRetMemValue->lValue = pstLeftMemValue->lValue + pstRightMemValue->lValue;
                              break;
                          case TK_MINUS://-
                              pstRetMemValue->lValue = pstLeftMemValue->lValue - pstRightMemValue->lValue;
                              break;
                          default:
                              CHECK_RET(-1,"opcode[%d] not support now",iOpcode);
                              break;
                      }
                  }
    	}
    	TADD_FUNC("Finish.");
    	return iRet;
    }

    /******************************************************************************
    * ��������	:  CompareNullValue
    * ��������	:  �Ƚ�����NULLֵ
    * ����		:  
    * ����		:  pstLeftMemValue - ��ֵ pstRightMemValue - ��ֵ
    * ���		:  
    * ����ֵ	:  0 - ��ͬ >0 - left>right   <0 -left<right  
    * ����		:  jin.shaohua
    *******************************************************************************/
    long long TMdbMemValue::CompareNullValue(ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue)
    {
        long long llRet = 0;
        if(pstLeftMemValue->IsNull() && pstRightMemValue->IsNull())
        {
            llRet = 0;
        }
        else if(true == pstLeftMemValue->IsNull() && false == pstRightMemValue->IsNull())
        {//��null���ҷ�null
            llRet = -1;
        }
        else if(false == pstLeftMemValue->IsNull() && true == pstRightMemValue->IsNull())
        {//��null����ҷ�null
            llRet = 1;
        }
        return llRet;
    }

/******************************************************************************
* ��������	:  CalcMemValueForStar
* ��������	:  �˷�����--(date-date)*number ���Ӿ���
* ����		:  
* ����		:  pstLeftMemValue - ��ֵ pstRightMemValue - ��ֵ
* ���		:  
* ����ֵ	: 
* ����		:  
*******************************************************************************/
void TMdbMemValue::CalcMemValueForStar(ST_MEM_VALUE*	pstRetMemValue,ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue,bool bNullFlag)
{
    if(bNullFlag && (pstLeftMemValue->IsNull() || pstRightMemValue->IsNull())) 
    {
        pstRetMemValue->SetNull();
    }
    else if(MemValueHasAnyProperty(pstLeftMemValue,MEM_Float))
    {
        pstRetMemValue->dValue = pstLeftMemValue->dValue * pstRightMemValue->lValue;
        pstRetMemValue->lValue = pstRetMemValue->dValue;
        MemValueSetProperty(pstRetMemValue,MEM_Float);
    }
    else if(MemValueHasAnyProperty(pstRightMemValue,MEM_Float))
    {
        pstRetMemValue->dValue = pstLeftMemValue->lValue * pstRightMemValue->dValue;
        pstRetMemValue->lValue = pstRetMemValue->dValue;
        MemValueSetProperty(pstRetMemValue,MEM_Float);
    }
    else
    {
        pstRetMemValue->lValue = pstLeftMemValue->lValue * pstRightMemValue->lValue;
    }
    return;
}
//}
