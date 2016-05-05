/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbMemValue.h		
*@Description： SQL解析树的内存值计算与比较
*@Author:	 jin.shaohua
*@Date：	    2012.07
*@History:
******************************************************************************************/
#ifndef _MDB_MEM_VALUE_H_
#define _MDB_MEM_VALUE_H_
#include "Helper/SqlParserStruct.h"
//namespace QuickMDB
//{
	/*
	** memvalue的处理函数
	*/
	class TMdbMemValue
        {
        	public:
        		static  long long  CompareMemValueList(ST_MEM_VALUE_LIST & stLeftList,ST_MEM_VALUE_LIST & stRightList);
        		static  long long CompareExprValue(ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue,bool bNullFlag = true);//比较两个expr的值,默认需要再判断是否为null
        		static  long long CompareNullValue(ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue);//比较可能的null值
        		static  int GetIntValue(ST_MEM_VALUE*  pstMemValue,long long & lValue);//获取数值数据
        		static  int CalcMemValue(int iOpcode,ST_MEM_VALUE*  pstRetMemValue,ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue,bool bNullFlag = false);//计算memvalue值	
				static  void CalcMemValueForStar(ST_MEM_VALUE*	pstRetMemValue,ST_MEM_VALUE*  pstLeftMemValue,ST_MEM_VALUE *  pstRightMemValue,bool bNullFlag = false);//乘法运算
        };
//}

#endif
