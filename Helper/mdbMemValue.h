/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbMemValue.h		
*@Description�� SQL���������ڴ�ֵ������Ƚ�
*@Author:	 jin.shaohua
*@Date��	    2012.07
*@History:
******************************************************************************************/
#ifndef _MDB_MEM_VALUE_H_
#define _MDB_MEM_VALUE_H_
#include "Helper/SqlParserStruct.h"
//namespace QuickMDB
//{
	/*
	** memvalue�Ĵ�����
	*/
	class TMdbMemValue
        {
        	public:
        		static  long long  CompareMemValueList(ST_MEM_VALUE_LIST & stLeftList,ST_MEM_VALUE_LIST & stRightList);
        		static  long long CompareExprValue(ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue,bool bNullFlag = true);//�Ƚ�����expr��ֵ,Ĭ����Ҫ���ж��Ƿ�Ϊnull
        		static  long long CompareNullValue(ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue);//�ȽϿ��ܵ�nullֵ
        		static  int GetIntValue(ST_MEM_VALUE*  pstMemValue,long long & lValue);//��ȡ��ֵ����
        		static  int CalcMemValue(int iOpcode,ST_MEM_VALUE*  pstRetMemValue,ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue,bool bNullFlag = false);//����memvalueֵ	
				static  void CalcMemValueForStar(ST_MEM_VALUE*	pstRetMemValue,ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue,bool bNullFlag = false);//�˷�����
        };
//}

#endif
