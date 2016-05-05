#ifndef _SQL_PARSER_HELPER_H
#define _SQL_PARSER_HELPER_H
#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include <map>


//namespace QuickMDB{

/**
* SQL parser�Ĺ�����
*/
class TSqlParserHelper
{
public:
	TSqlParserHelper();
	~TSqlParserHelper();
	int SetDB(TMdbShmDSN * pShmDsn);//����DB
	TMdbShmDSN * GetMdbShmDSN();//
	TMdbTable * GetMdbTable(const char * sTablename);//��ȡmdbtableָ��
	TMdbColumn * GetMdbColumn(TMdbTable * pMdbTable,const char * sColumnName);//��ȡ��ָ��
	int GetMdbColumnPos(TMdbTable * pMdbTable,const char * sColumnName);//��ȡ��λ��
	//int BuildColumIndexMap(TMdbTable * pMdbTable);//����column index map
	//int GetIndexByColumnPos(int iColummPos);//��ȡiColumnPos����Ӧ��indexPos
	bool IsPrimaryKey(TMdbTable * pMdbTable,TMdbColumn *pColumn);//�ж����Ƿ�������
	TMdbTableSpace* GetMdbTablespace(const char * sTablespaceName);
	TMemSeq * GetSequenceByName(const char* pSeqName);//��ȡ�ڴ�����ֵ
	int GetSeqNextValue(const char* pSeqName);//��ȡ�ڴ�����NEXTֵ
	int CheckPrimaryKeyColumn(TMdbTable* pTable);//У�������������Ƿ�Ϸ�
private:
	TMdbShmDSN * m_pShmDsn;//ShmDSN  ��Ϣ
	TMdbDSN    * m_pMdbDsn;//DSN ��Ϣ������
	TMdbTable * m_pMdbTable;//mdb tableָ��
	TMdbTableSpace* m_pMdbTablespace;//mdb tablespaceָ��
	//std::map<int,int> m_mapColumnToIndex;//column��index��ӳ�䡣
};


//}
#endif

