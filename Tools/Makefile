include ../Makefile.incl

clean:
	rm -f *.o  libmdbTools.$(TRUESUFFIX)

ALL_OBJS= mdbInfo.o mdbChangeLog.o mdbMutexCtrl.o mdbDataCmp.o mdbBulkCtrl.o\
					mdbOverdueDataCtrl.o mdbCheckState.o mdbDataExport.o mdbDataPumpCtrl.o mdbImage.o mdbDbFlushMdb.o mdbAlterInfo.o


ALL_EXES= mdbSQL.exe mdbSetLog.exe mdbShow.exe mdbObserve.exe mdbStartCheck.exe mdbUnlock.exe mdbDataCheck.exe \
					mdbIndexLook.exe mdbCleanRestoreTool.exe mdbStat.exe mdbDbTableRecordStat.exe mdbReLoad.exe mdbExport.exe \
					mdbDataPump.exe mdbDataImage.exe mdbFlushCache.exe mdbCreateTable.exe mdbDropTable.exe mdbBulkCp.exe \
					mdbServerTool.exe mdbAlterTool.exe mdbSize.exe mdbChkPnt.exe mdbParseRepLog.exe

all:	$(ALL_OBJS) 
	$(AR) libmdbTools.$(TRUESUFFIX)  $(ALL_OBJS) 
		
build: $(ALL_EXES)	
	cp mdbSQL $(PROJECT_DIR)/bin
	cp mdbSetLog $(PROJECT_DIR)/bin
	cp mdbShow $(PROJECT_DIR)/bin
	cp mdbObserve $(PROJECT_DIR)/bin
	cp mdbStartCheck $(PROJECT_DIR)/bin
	cp mdbUnlock $(PROJECT_DIR)/bin
	cp mdbDataCheck $(PROJECT_DIR)/bin
	cp mdbIndexLook $(PROJECT_DIR)/bin
	cp mdbCleanRestoreTool $(PROJECT_DIR)/bin
	cp mdbStat $(PROJECT_DIR)/bin
	cp mdbDbTableRecordStat $(PROJECT_DIR)/bin
	cp mdbReLoad $(PROJECT_DIR)/bin
	cp mdbExport $(PROJECT_DIR)/bin
	cp mdbDataPump $(PROJECT_DIR)/bin
	cp mdbDataImage $(PROJECT_DIR)/bin
	cp mdbFlushCache $(PROJECT_DIR)/bin
	cp mdbCreateTable $(PROJECT_DIR)/bin
	cp mdbDropTable $(PROJECT_DIR)/bin
	cp mdbBulkCp $(PROJECT_DIR)/bin
	cp mdbServerTool $(PROJECT_DIR)/bin
	cp mdbAlterTool $(PROJECT_DIR)/bin
	cp mdbSize $(PROJECT_DIR)/bin
	cp mdbChkPnt $(PROJECT_DIR)/bin
	cp mdbParseRepLog $(PROJECT_DIR)/bin
install: all
	cp -rf *.$(TRUESUFFIX)  $(PROJECT_DIR)/lib

