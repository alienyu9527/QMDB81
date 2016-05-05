/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbMalloc.cpp		
*@Description�� mdb�ڴ��������ͷŵ�ͳһ�ӿڡ�����ģʽ
*@Author:			jin.shaohua
*@Date��	    2012.05
*@History:
******************************************************************************************/
#ifndef _MDB_MALLOC_H_
#define _MDB_MALLOC_H_
#include "Helper/SqlParserStruct.h"
//namespace QuickMDB{
	//�ڴ�������
	class TMdbMalloc
	{
	public:
		~TMdbMalloc();
		static TMdbMalloc * Instance();
		void * ReAlloc(void * p,int iOldSize,int iNewSize);//���·����С
		void * MallocRaw(int n); //����ռ�
		char * NameFromToken(const Token * pToken);
		int NameFromToken(const Token * pToken,char * sName,int iMaxLen);//��token��ȡ����
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


		//����ST_EXPR
		ST_EXPR * AllocExpr();
		int ReleaseExpr(ST_EXPR * & pstExpr);
		//����ST_EXPR_LIST
		ST_EXPR_LIST * AllocExprList();
		int ReleaseExprList(ST_EXPR_LIST * & pstExprList);

		//����ST_EXPR_FUNC
		ST_EXPR_FUNC * AllocExprFunc();
		int ReleaseExprFunc(ST_EXPR_FUNC * & pstExprFunc);

		//����ST_ID_LIST
		ST_ID_LIST * AllocIdList();
		int ReleaseIdList(ST_ID_LIST * & pstIdList);

		//����memvalue
		ST_MEM_VALUE * AllocMemValue(ST_MEM_VALUE * pstOld);
		int ReleaseMemValue(ST_MEM_VALUE * & pstMemValue);
		ST_MEM_VALUE * ResetMemValue(ST_MEM_VALUE * & pstMemValue);

		//����memvalue *
		 ST_MEM_VALUE ** AllocMemValueArray(int iSize);
		 void ReleaseMemValueArray(ST_MEM_VALUE ** & pstMemValueArray);

	protected:
		TMdbMalloc();
	private:
		static TMdbMalloc * m_pMdbmalloc;
	public:
		class CGarbo //����Ψһ��������������������ɾ��CSingleton��ʵ��
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
		static CGarbo Garbo; //����һ����̬��Ա���������ʱ��ϵͳ���Զ�����������������
	};
	#define QMDB_MALLOC TMdbMalloc::Instance()

	//��̬�ڴ�飬��������
	class TDynamicMemBlock
	{
	public:
		TDynamicMemBlock();
		~TDynamicMemBlock();
		int Init(size_t iMinSize,size_t iMaxSize,size_t iIncreaseSize);//��ʼ��
		int InsertData(char * pDataAddr,size_t iDataSize);//�������ݣ�������Ҫ��չ�ռ�
		int Clear();//��������
		 char * GetDataAddr(){return m_pAddr;}
	private:
		size_t m_iCurSize;//��ǰ��С
		size_t m_iMaxSize;//����С
		size_t m_iDataPos;//���ݵ�ַ
		size_t m_iIncreaseSize;//������С
		char * m_pAddr;//�ڴ��ַ
	};

//}
#endif

