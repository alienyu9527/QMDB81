/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbBitMap.h		
*@Description��  λʾͼ
*@Author:		jin.shaohua
*@Date��	    2014/03/27
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
		void Clear();   //��������
		bool IsSet(int iPos); //ĳ��λ���Ƿ�������flag = true
		void Set(int iPos,bool bFlag);//����ĳλ�õ�flag true/false
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
	* ��������	:  Clear
	* ��������	: ��������
	* ����		:  
	* ���		:  
	* ����ֵ	:  
	* ����		:  jin.shaohua
	*******************************************************************************/
	template<int SIZE>
	void TMdbBitMap<SIZE>::Clear()
	{
		memset(m_sBM,0,sizeof(m_sBM));
	}
	/******************************************************************************
	* ��������	:  IsSet
	* ��������	: ĳ��λ���Ƿ�������flag = true
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	template<int SIZE>
	bool TMdbBitMap<SIZE>::IsSet(int iPos)
	{
		iPos = iPos%SIZE;
		return (m_sBM[iPos/8] & (1 < iPos%8)) != 0;
	}
	/******************************************************************************
	* ��������	:  Set
	* ��������	: //����ĳλ�õ�flag true/false
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	template<int SIZE>
	void TMdbBitMap<SIZE>::Set(int iPos,bool bFlag)
	{
		iPos = iPos%SIZE;
		char cTemp = 1 < iPos%8;
		if(bFlag)
		{//��Ϊ1
			m_sBM[iPos/8] |= cTemp;
		}
		else
		{//��Ϊ0
			m_sBM[iPos/8] &= (cTemp ^ 255);
		}
	}
//}
#endif