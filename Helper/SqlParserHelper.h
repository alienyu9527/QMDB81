#ifndef _SQL_PARSER_HELPER_H
#define _SQL_PARSER_HELPER_H
#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include <map>


//namespace QuickMDB{

/**
* SQL parser的工具类
*/
class TSqlParserHelper
{
public:
	TSqlParserHelper();
	~TSqlParserHelper();
	int SetDB(TMdbShmDSN * pShmDsn);//设置DB
	TMdbShmDSN * GetMdbShmDSN();//
	TMdbTable * GetMdbTable(const char * sTablename);//获取mdbtable指针
	TMdbColumn * GetMdbColumn(TMdbTable * pMdbTable,const char * sColumnName);//获取列指针
	int GetMdbColumnPos(TMdbTable * pMdbTable,const char * sColumnName);//获取列位置
	//int BuildColumIndexMap(TMdbTable * pMdbTable);//构建column index map
	//int GetIndexByColumnPos(int iColummPos);//获取iColumnPos所对应的indexPos
	bool IsPrimaryKey(TMdbTable * pMdbTable,TMdbColumn *pColumn);//判断列是否是主键
	TMdbTableSpace* GetMdbTablespace(const char * sTablespaceName);
	TMemSeq * GetSequenceByName(const char* pSeqName);//获取内存序列值
	int GetSeqNextValue(const char* pSeqName);//获取内存序列NEXT值
	int CheckPrimaryKeyColumn(TMdbTable* pTable);//校验主键列配置是否合法
private:
	TMdbShmDSN * m_pShmDsn;//ShmDSN  信息
	TMdbDSN    * m_pMdbDsn;//DSN 信息管理区
	TMdbTable * m_pMdbTable;//mdb table指针
	TMdbTableSpace* m_pMdbTablespace;//mdb tablespace指针
	//std::map<int,int> m_mapColumnToIndex;//column对index的映射。
};


//}
#endif

