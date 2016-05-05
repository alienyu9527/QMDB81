/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbMalloc.cpp		
*@Description： mdb内存申请与释放的统一接口。单例模式
*@Author:			jin.shaohua
*@Date：	    2012.05
*@History:
******************************************************************************************/
#ifndef _MDB_MALLOC_H_
#define _MDB_MALLOC_H_
#include "Helper/SqlParserStruct.h"
//namespace QuickMDB{
	//内存分配管理
	class TMdbMalloc
	{
	public:
		~TMdbMalloc();
		static TMdbMalloc * Instance();
		void * ReAlloc(void * p,int iOldSize,int iNewSize);//重新分配大小
		void * MallocRaw(int n); //分配空间
		char * NameFromToken(const Token * pToken);
		int NameFromToken(const Token * pToken,char * sName,int iMaxLen);//从token获取名字
		char * CopyFromStr(const char * sStr);
		void ReleaseStr(char * & sStr);
		void * ArrayAllocate(
			void *pArray,     /* Array of objects.  Might be reallocated */
			int szEntry,      /* Size of each object in the array */
			int initSize,     /* Suggested initial allocation, in elements */
			int *pnEntry,     /* Number of objects currently in use */
			int *pnAlloc,     /* Current size of the allocation, in elements */
			int *pIdx         /* Write the index of a new slot here */
		);


		//分配ST_EXPR
		ST_EXPR * AllocExpr();
		int ReleaseExpr(ST_EXPR * & pstExpr);
		//分配ST_EXPR_LIST
		ST_EXPR_LIST * AllocExprList();
		int ReleaseExprList(ST_EXPR_LIST * & pstExprList);

		//分配ST_EXPR_FUNC
		ST_EXPR_FUNC * AllocExprFunc();
		int ReleaseExprFunc(ST_EXPR_FUNC * & pstExprFunc);

		//分配ST_ID_LIST
		ST_ID_LIST * AllocIdList();
		int ReleaseIdList(ST_ID_LIST * & pstIdList);

		//分配memvalue
		ST_MEM_VALUE * AllocMemValue(ST_MEM_VALUE * pstOld);
		int ReleaseMemValue(ST_MEM_VALUE * & pstMemValue);
		ST_MEM_VALUE * ResetMemValue(ST_MEM_VALUE * & pstMemValue);

		//分配memvalue *
		 ST_MEM_VALUE ** AllocMemValueArray(int iSize);
		 void ReleaseMemValueArray(ST_MEM_VALUE ** & pstMemValueArray);

	protected:
		TMdbMalloc();
	private:
		static TMdbMalloc * m_pMdbmalloc;
	public:
		class CGarbo //它的唯一工作就是在析构函数中删除CSingleton的实例
		{
				public:
					~CGarbo()
					{
						if( TMdbMalloc::m_pMdbmalloc)
						{
							delete TMdbMalloc::m_pMdbmalloc;
							TMdbMalloc::m_pMdbmalloc = NULL;
						}

					}
		};
	private:
		static CGarbo Garbo; //定义一个静态成员，程序结束时，系统会自动调用它的析构函数
	};
	#define QMDB_MALLOC TMdbMalloc::Instance()

	//动态内存块，可自增长
	class TDynamicMemBlock
	{
	public:
		TDynamicMemBlock();
		~TDynamicMemBlock();
		int Init(size_t iMinSize,size_t iMaxSize,size_t iIncreaseSize);//初始化
		int InsertData(char * pDataAddr,size_t iDataSize);//插入数据，可能需要扩展空间
		int Clear();//清理数据
		 char * GetDataAddr(){return m_pAddr;}
	private:
		size_t m_iCurSize;//当前大小
		size_t m_iMaxSize;//最大大小
		size_t m_iDataPos;//数据地址
		size_t m_iIncreaseSize;//增长大小
		char * m_pAddr;//内存地址
	};

//}
#endif

