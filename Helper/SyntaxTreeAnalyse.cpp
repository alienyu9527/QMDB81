//#include "BillingSDK.h"
#include "Helper/SyntaxTreeAnalyse.h"
//namespace QuickMDB{
	char TSyntaxTreeAnalyse::m_sOffset[1024] = {0};

	//打印整个语法树
	int TSyntaxTreeAnalyse::Print(TMdbSqlParser * pSqlParser,int iLogType)
	{
//#if 0
		ST_SQL_STRUCT & stSqlStruct = pSqlParser->m_stSqlStruct;
		OutPutInfo(iLogType,"#################SqlStruct start################");
		OutPutInfo(iLogType,"==========================================");
		OutPutInfo(iLogType,"Insert(%d),delete(%d),update(%d),select(%d)",TK_INSERT,TK_DELETE,TK_UPDATE,TK_SELECT);
		OutPutInfo(iLogType,"[type = %d],[tablename = %s][alias = %s][pMdbTable = %p][pShmDSN= %p][pScanAllIndex= %p][vIndexUsed=%d]",
			stSqlStruct.iSqlType,
			ReplaceNull(stSqlStruct.sTableName),
			ReplaceNull(stSqlStruct.sAlias),
			stSqlStruct.pMdbTable,stSqlStruct.pShmDSN,
			stSqlStruct.pScanAllIndex,
			stSqlStruct.vIndexUsed.size());
		OutPutInfo(iLogType,"===========[index used]==================");
		PrintIndex(stSqlStruct.vIndexUsed,0,iLogType);
		OutPutInfo(iLogType,"===========[column list]==================");
		PrintExprList(stSqlStruct.pColList,0,iLogType);
		OutPutInfo(iLogType,"===========[where]========================");
		PrintExpr("root",stSqlStruct.pWhere,0,iLogType);
		OutPutInfo(iLogType,"===========[group by]========================");
		PrintExprList(stSqlStruct.pGroupby,0,iLogType);
		OutPutInfo(iLogType,"===========[having]========================");
		PrintExpr("root",stSqlStruct.pHaving,0,iLogType);
		OutPutInfo(iLogType,"===========[optimised where]==================");
		PrintWhereClause(iLogType,stSqlStruct.tWhereOrClause);
		OutPutInfo(iLogType,"===========[orderby]======================");
		PrintExprList(stSqlStruct.pOrderby,0,iLogType);
		OutPutInfo(iLogType,"===========[limit]========================");
		PrintExpr("root",stSqlStruct.pstLimit,0,iLogType);
		OutPutInfo(iLogType,"===========[offset]=======================");
		PrintExpr("root",stSqlStruct.pstOffset,0,iLogType);
		OutPutInfo(iLogType,"===========[idlist]=======================");
		PrintIdList(stSqlStruct.pstIdList,0,iLogType);
		OutPutInfo(iLogType,"==========================================");
		PrintValues(pSqlParser,iLogType);
		OutPutInfo(iLogType,"#################SqlStruct end#################");

    	_ST_DDL_SQL_STRUCT * pstDDLSqlStruct = pSqlParser->m_pDDLSqlStruct;
		OutPutInfo(iLogType,"###############DDLSqlStruct start##############");
    	if(pstDDLSqlStruct)
    	{
        	OutPutInfo(iLogType,"[sqltype = [%d][%d]][IfNE = %d]",pstDDLSqlStruct->iSqlType[0],pstDDLSqlStruct->iSqlType[1]
        															,pstDDLSqlStruct->bIfNE);
        	PrintTable(pstDDLSqlStruct->pTable,iLogType);
    	}
		OutPutInfo(iLogType,"###############DDLSqlStruct end################");
//#endif
		return 0;
	}

	//打印绑定变量主键信息
	int TSyntaxTreeAnalyse::PrintVariablePKValue(TMdbSqlParser * pSqlParser,int iLogType)
	{
//#if 0
		if(NULL == pSqlParser){return 0;}
		if(TK_SELECT == pSqlParser->m_stSqlStruct.iSqlType){return 0;}//查询不打印
		std::vector<_ST_MEM_VALUE *> & arrMemValue = pSqlParser->m_listInputVariable.vMemValue;
		std::vector<_ST_MEM_VALUE *>::iterator itor = arrMemValue.begin();
		TMdbTable * pTable = pSqlParser->m_stSqlStruct.pMdbTable;
		for(;itor != arrMemValue.end();++itor)
		{
			ST_MEM_VALUE * pstMemValue = *itor;
			if(NULL != pstMemValue->pColumnToSet)
			{
				int i = 0;
				for(i = 0;i < pTable->m_tPriKey.iColumnCounts;++i)
				{
					if(pstMemValue->pColumnToSet->iPos == pTable->m_tPriKey.iColumnNo[i])
					{
						OutPutInfo(iLogType,"[%-10s],Flag=[%d],lvalue=[%lld],sValue=[%s],iSize=[%d],",
							ReplaceNull(pstMemValue->sAlias),
							pstMemValue->iFlags,
							pstMemValue->lValue,
							ReplaceNull(pstMemValue->sValue),
							pstMemValue->iSize);
						break;
					}
				}
			}
		}
//#endif
		return 0;
	}


	//打印表达式列表
	int TSyntaxTreeAnalyse::PrintExprList(ST_EXPR_LIST * pstExprList,int offset,int iLogType)
	{
//#if 0
		if (NULL != pstExprList)
		{	
			OutPutInfo(iLogType,"%s======List start=====",GetOffSet(offset));
			int i = 0;
			for (i = 0;i<pstExprList->iItemNum;i++)
			{
				OutPutInfo(iLogType,"%sitem[%d]",GetOffSet(offset),i);
				ST_EXPR_ITEM & Item = pstExprList->pExprItems[i];
				OutPutInfo(iLogType,"%sname=[%-10s],span=[%s]",GetOffSet(offset),ReplaceNull(Item.sName),ReplaceNull(Item.sSpan));
				PrintExpr("root",Item.pExpr,offset,iLogType);
			}
			OutPutInfo(iLogType,"%s======List end======",GetOffSet(offset));
		}
//#endif
		return 0;
	}
	//打印表达式列表
	int TSyntaxTreeAnalyse::PrintExpr(const char  * sPrefix ,ST_EXPR * pstExpr,int offset,int iLogType)
	{
//#if 0
		if (NULL == pstExpr)
		{
			return 0;
		}
		OutPutInfo(iLogType,"%s[%s]op=[%s],addr[%p],calcflag=[%s],sTorken=[%s],valueaddr[%p],exprlValue[%lld],exprsValue[%s],valueSize[%d],exprailas[%s],pColumn[%p]",
			GetOffSet(offset),sPrefix,TokenName[pstExpr->iOpcode],pstExpr,
			DescribeExprCalcFlag(pstExpr).c_str(),
			ReplaceNull(pstExpr->sTorken),
			pstExpr->pExprValue,
			pstExpr->pExprValue->lValue,
			ReplaceNull(pstExpr->pExprValue->sValue),
			pstExpr->pExprValue->iSize,
			ReplaceNull(pstExpr->pExprValue->sAlias),
			pstExpr->pExprValue->pColumn);	

		PrintExpr("left",pstExpr->pLeft,offset+1, iLogType);
		PrintExpr("right",pstExpr->pRight,offset+1, iLogType);
		if (NULL != pstExpr->pFunc)
		{
			PrintExprList(pstExpr->pFunc->pFuncArgs,offset+1, iLogType);
		}
//#endif
		return 0;
	}


	//打印IDlist
	int TSyntaxTreeAnalyse::PrintIdList(ST_ID_LIST * pstIdList,int offset,int iLogType)
	{
//#if 0
		if (NULL == pstIdList)
		{
			return 0;
		}
		OutPutInfo(iLogType,"%s=======[%d][%d]=======",GetOffSet(offset),pstIdList->iIdNum,pstIdList->iAllocNum);
		for (int i = 0;i<pstIdList->iIdNum;i++)
		{
			ID_LIST_ITEM & item = pstIdList->pstItem[i];
			OutPutInfo(iLogType,"%sname=[%s],pColumn=[%p],pExpr=[%p]",GetOffSet(offset),ReplaceNull(item.zName),item.pColumn,item.pExpr);
		}
		OutPutInfo(iLogType,"%s======================",GetOffSet(offset));
//#endif
		return 0;
	}


	// 打印值信息
	int TSyntaxTreeAnalyse::PrintArrMemValue(std::vector<_ST_MEM_VALUE *> arrMemValue,int iLogType)
	{
//#if 0
		size_t i = 0;
		for(i = 0;i<arrMemValue.size();i++)
		{
			ST_MEM_VALUE * pstMemValue = arrMemValue[i];
			OutPutInfo(iLogType,"[%-10s],addr[%p],Flag=[%s],lvalue=[%lld],sValue=[%s],iSize=[%d],pColumn=[%p],\
								iColIndexToSet=[%d],pColumnToSet=[%p]",
								ReplaceNull(pstMemValue->sAlias),
								pstMemValue,
								DescribeMemValueFlag(pstMemValue).c_str(),
								pstMemValue->lValue,
								ReplaceNull(pstMemValue->sValue),
								pstMemValue->iSize,
								pstMemValue->pColumn,
								pstMemValue->iColIndexToSet,
								pstMemValue->pColumnToSet);
		}
//#endif
		return 0;
	}


	int TSyntaxTreeAnalyse::PrintValues(TMdbSqlParser * pSqlParser,int iLogType)
	{
//#if 0
		OutPutInfo(iLogType,"==========[m_arrInputVariable]============");
		PrintArrMemValue(pSqlParser->m_listInputVariable.vMemValue,iLogType);
		OutPutInfo(iLogType,"==========[m_arrInputCollist]=============");
		PrintArrMemValue(pSqlParser->m_listInputCollist.vMemValue,iLogType);
		OutPutInfo(iLogType,"==========[m_arrInputWhere]===============",iLogType);
		PrintArrMemValue(pSqlParser->m_listInputWhere.vMemValue,iLogType);
		OutPutInfo(iLogType,"==========[m_arrInputOrderby]============");
		PrintArrMemValue(pSqlParser->m_listInputOrderby.vMemValue,iLogType);
		OutPutInfo(iLogType,"==========[m_listInputLimit]============");
		PrintArrMemValue(pSqlParser->m_listInputLimit.vMemValue,iLogType);
		OutPutInfo(iLogType,"==========[m_listInputPriKey]============");
		PrintArrMemValue(pSqlParser->m_listInputPriKey.vMemValue,iLogType);

		OutPutInfo(iLogType,"==========[m_arrOutputCollist]============");
		PrintArrMemValue(pSqlParser->m_listOutputCollist.vMemValue,iLogType);
		OutPutInfo(iLogType,"==========[m_arrOutputOrderby]============");
		PrintArrMemValue(pSqlParser->m_listOutputOrderby.vMemValue,iLogType);
		OutPutInfo(iLogType,"==========[m_listOutputLimit]============");
		PrintArrMemValue(pSqlParser->m_listOutputLimit.vMemValue,iLogType);
		OutPutInfo(iLogType,"==========[m_listOutputPriKey]============");
		PrintArrMemValue(pSqlParser->m_listOutputPriKey.vMemValue,iLogType);
		OutPutInfo(iLogType,"==========================================");
//#endif
		return 0;
	}
	//打印表信息
	int TSyntaxTreeAnalyse::PrintTable(TMdbTable * pTable,int iLogType)
	{
//#if 0
		if(NULL == pTable){return 0;}
		printf("======Table=[%s] Start======\n", pTable->sTableName);
		//printf("    Table-ID      = %d\n", pTable->iTableID);
		printf("    sCreateTime   = %s\n", pTable->sCreateTime);
		printf("    sUpdateTime   = %s\n", pTable->sUpdateTime);
		printf("    cState        = %c(%c-unused, %c-using, %c-loading, %c-repping, %c-waitingRep)\n", pTable->cState, Table_unused, Table_running, Table_loading, Table_repping, Table_watingRep);
		printf("    TableSpace    = %s\n", pTable->m_sTableSpace);
		printf("    iColumnCounts = %d\n", pTable->iColumnCounts);
		printf("    iFullPageID   = %d\n", pTable->iFullPageID);
		printf("    iFreePageID   = %d\n", pTable->iFreePageID);
		printf("    iFullPages    = %d\n", pTable->iFullPages);
		printf("    iFreePages    = %d\n", pTable->iFreePages);
		printf("    iRepAttr      = %d\n", pTable->iRepAttr);
		printf("    iRecordCounts(Real) = %d\n", pTable->iCounts);
		printf("    iRecordCounts(Set)  = %d\n", pTable->iRecordCounts);
		printf("    iExpandRecords(Set) = %d\n", pTable->iExpandRecords);
		printf("    iCINCounts(real)    = %d\n", pTable->iCINCounts);
		printf("    iOneRecordSize      = %d\n", pTable->iOneRecordSize);
		printf("====Coloum Msg as like:\n");
		int i = 0;
		for(i=0; i<pTable->iColumnCounts; ++i)
		{
			printf("    Column-Name =[%s].\n", pTable->tColumn[i].sName);
			printf("    Column-Type =[%d].\n", pTable->tColumn[i].iDataType);
			printf("    Column-Len  =[%d].\n", pTable->tColumn[i].iColumnLen);
		}
		printf("====Primary-Key as like:\n");
		for(i=0; i<pTable->m_tPriKey.iColumnCounts; ++i)
		{
			printf("    Column-No =[%d].\n", pTable->m_tPriKey.iColumnNo[i]);
		}
		printf("====Index as like:\n");
		for(i=0;i<pTable->iIndexCounts;i++)
		{
			printf("	Index-Name=[%s], Column-No=[%d][%d][%d], iPriority=[%d],iHashType=[%d]\n",pTable->tIndex[i].sName,
				pTable->tIndex[i].iColumnNo[0],pTable->tIndex[i].iColumnNo[1],
				pTable->tIndex[i].iColumnNo[2],pTable->tIndex[i].iPriority,
				pTable->tIndex[i].m_iIndexType);
		}
		printf("======Table=[%s] Finish======\n", pTable->sTableName);
//#endif
		return 0;
	}

	//对于空字符串返回'nil' 防止sun下printf core.//fuck  ，sun就是蛋疼
	const char * TSyntaxTreeAnalyse::ReplaceNull(char * pStr)
	{
		if(NULL == pStr)
		{
			return "nil";
		}
		else
		{
			return pStr;
		}
	}

	int TSyntaxTreeAnalyse::OutPutInfo(int iLogType,const char * fmt, ...)
	{
//#if 0
		char sLogTemp[10240]="";
		va_list ap;
		va_start(ap,fmt);
		vsnprintf(sLogTemp, sizeof(sLogTemp), fmt, ap); 
		va_end (ap);
		switch(iLogType)
		{
		case TLOG_NORMAL :
			TADD_NORMAL("%s",sLogTemp);
			break;
		case TLOG_FLOW	 :
			TADD_FLOW("%s",sLogTemp);
			break;
		case TLOG_FUNC	 :
			TADD_FUNC("%s",sLogTemp);
			break;
		case TLOG_DETAIL :
			TADD_DETAIL("%s",sLogTemp);
			break;
		case TLOG_WARNING:
			TADD_WARNING("%s",sLogTemp);
			break;
		case TLOG_FATAL  :
			TADD_ERROR(-1,"%s",sLogTemp);
			break;
		default:
			printf("%s\n",sLogTemp);
			break;
		}
//#endif
		return 0;	
	}

	//获取偏移
	char * TSyntaxTreeAnalyse::GetOffSet(int n,char zOff)
	{
		memset(m_sOffset,0x00,sizeof(m_sOffset));
		for (int i = 0;i<n;i++)
		{
			m_sOffset[i] = zOff;
		}
		return m_sOffset;
	}

	//打印索引信息
	int TSyntaxTreeAnalyse::PrintIndex(std::vector< _ST_INDEX_VALUE> & vIndexValue,int offset,int iLogType)
	{
//#if 0
		std::vector< _ST_INDEX_VALUE>::iterator itor = vIndexValue.begin();
		for(;itor != vIndexValue.end();++itor)
		{
			OutPutInfo(iLogType,"%s index[%s]",GetOffSet(offset),itor->pstTableIndex->pIndexInfo->sName);
			int i = 0;
			for(i = 0; i < MAX_INDEX_COLUMN_COUNTS;++i)
			{
				if(itor->pExprArr[i] != NULL)
				{
					PrintExpr("Expr:",itor->pExprArr[i],offset +1 ,iLogType);
				}
			}
		}
//#endif
		return 0;
	}
	//解释memvalue中的flag
	std::string TSyntaxTreeAnalyse::DescribeMemValueFlag(int iFlags)
	{
		ST_MEM_VALUE stMemValue;
		stMemValue.iFlags = iFlags;
		return DescribeMemValueFlag(&stMemValue);
	}

	//解释memvalue中的flag
	std::string TSyntaxTreeAnalyse::DescribeMemValueFlag(ST_MEM_VALUE * pstMemValue)
	{
		int arrFlag[] = {MEM_Null,MEM_Str,MEM_Int,MEM_Float,MEM_Blob,MEM_Invalid,MEM_Variable,
			MEM_Term,MEM_Dyn,MEM_Date,MEM_Ephem,MEM_Agg,MEM_Zero};
		const char * arrDesc[] ={"MEM_Null","MEM_Str","MEM_Int","MEM_Float","MEM_Blob","MEM_Invalid","MEM_Variable",
			"MEM_Term","MEM_Dyn","MEM_Date","MEM_Ephem","MEM_Agg","MEM_Zero"}; 
		char sTemp[128] = {0};
		size_t i = 0;
		for(i = 0;i < sizeof(arrFlag)/sizeof(int);++i)
		{
			if(MemValueHasProperty(pstMemValue,arrFlag[i]))
			{
				snprintf(sTemp+strlen(sTemp),sizeof(sTemp),"%s,",arrDesc[i]);
			}
		}
		std::string sRet = sTemp;
		return sRet;
	}
	//打印where 段
	int TSyntaxTreeAnalyse::PrintWhereClause(int iLogType,ST_WHERE_OR_CLAUSE & stWhereOrClause)
	{
		int iRet = 0;
		size_t i = 0;
		for(i = 0;i < stWhereOrClause.m_vAndClause.size();++i)
		{
			OutPutInfo(iLogType,"++++++Or Clause[%u]+++++++",i);
			size_t j = 0;
			for(j = 0; j < stWhereOrClause.m_vAndClause[i].m_vExpr.size();++j)
			{
				PrintExpr("And Clause",stWhereOrClause.m_vAndClause[i].m_vExpr[j],1,iLogType);
			}
		}
		return iRet;
	}

	//校验where 中的index
	int TSyntaxTreeAnalyse::CheckWhereIndex(TMdbSqlParser * pSqlParser)
	{
		int iRet = 0;
//#if 0
		CHECK_OBJ(pSqlParser);
		std::vector< _ST_INDEX_VALUE> & vIndexValue = pSqlParser->m_stSqlStruct.vIndexUsed;
		std::vector< _ST_INDEX_VALUE>::iterator itor = vIndexValue.begin();
		for(;itor != vIndexValue.end();++itor)
		{
			printf("index[%s]",itor->pstTableIndex->pIndexInfo->sName);//使用的索引名
			int i = 0;
			for(i = 0; i < MAX_INDEX_COLUMN_COUNTS;++i)
			{
				if(itor->pExprArr[i] != NULL)
				{
					//每个索引所使用的表达式= itor->pExprArr[i]
					//PrintExpr("Expr:",itor->pExprArr[i],offset +1 ,iLogType);
				}
			}
		}
//#endif
		return iRet;
	}
	//解释expr中的calcflag
	std::string TSyntaxTreeAnalyse::DescribeExprCalcFlag(ST_EXPR * pstExpr)
	{
//#if 0
		int arrFlag[] = {CALC_NO_NEED,CALC_PER_Row,CALC_PER_Exec,CALC_PER_Query};
		const char * arrDesc[] ={"no_calc","row","exec","query"}; 
		char sTemp[128] = {0};
		size_t i = 0;
		for(i = 0;i < sizeof(arrFlag)/sizeof(int);++i)
		{
			if(ExprCalcHasProperty(pstExpr,arrFlag[i]))
			{
				snprintf(sTemp+strlen(sTemp),sizeof(sTemp),"%s,",arrDesc[i]);
			}
		}
		std::string sRet = sTemp;
		return sRet;
//#endif
		return "";
	}

//}

