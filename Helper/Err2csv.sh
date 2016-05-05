#!/bin/sh
echo "Error_code,Error_brief,Error_detail,Error_solution" > $QuickMDB_HOME/etc/qmdb_error_code.csv
grep '#define ERROR' mdbErr.h | awk -F '//' ' 
function getErrCode(code_desc)
{
	split(code_desc,parts," ")
	return 0-substr(parts[3],2,6)
}
{OFS=",";gsub("\t"," ",$2);gsub("\t"," ",$3);print getErrCode($1),$2,$2,$3}
'  >>  $QuickMDB_HOME/etc/qmdb_error_code.csv
