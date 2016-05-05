/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbBitMap.h		
*@Description：  位示图
*@Author:		jin.shaohua
*@Date：	    2014/03/27
*@History:
******************************************************************************************/
#ifndef _MDB_BIT_MAP_H_
#define _MDB_BIT_MAP_H_

//namespace QuickMDB{
	template<int SIZE>
	class TMdbBitMap
	{
	public:
		TMdbBitMap();
		void Clear();   //清理所有
		bool IsSet(int iPos); //某个位置是否设置了flag = true
		void Set(int iPos,bool bFlag);//设置某位置的flag true/false
	protected:
	private:
		char m_sBM[SIZE/8+1];
	};
	template<int SIZE>
	TMdbBitMap<SIZE>::TMdbBitMap()
	{
		Clear();
	}
	/******************************************************************************
	* 函数名称	:  Clear
	* 函数描述	: 清理所有
	* 输入		:  
	* 输出		:  
	* 返回值	:  
	* 作者		:  jin.shaohua
	*******************************************************************************/
	template<int SIZE>
	void TMdbBitMap<SIZE>::Clear()
	{
		memset(m_sBM,0,sizeof(m_sBM));
	}
	/******************************************************************************
	* 函数名称	:  IsSet
	* 函数描述	: 某个位置是否设置了flag = true
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	template<int SIZE>
	bool TMdbBitMap<SIZE>::IsSet(int iPos)
	{
		iPos = iPos%SIZE;
		return (m_sBM[iPos/8] & (1 < iPos%8)) != 0;
	}
	/******************************************************************************
	* 函数名称	:  Set
	* 函数描述	: //设置某位置的flag true/false
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	template<int SIZE>
	void TMdbBitMap<SIZE>::Set(int iPos,bool bFlag)
	{
		iPos = iPos%SIZE;
		char cTemp = 1 < iPos%8;
		if(bFlag)
		{//设为1
			m_sBM[iPos/8] |= cTemp;
		}
		else
		{//设为0
			m_sBM[iPos/8] &= (cTemp ^ 255);
		}
	}
//}
#endif