include ../Makefile.incl

clean:
	rm -f *.o  libmdbControl.$(TRUESUFFIX)

ALL_OBJS= mdbMgrShm.o mdbPageCtrl.o mdbVarcharMgr.o \
          mdbSocketTry.o mdbTableSpaceCtrl.o mdbTimeThread.o mdbSysTableThread.o  mdbLogInfo.o mdbIndexCtrl.o mdbTableWalker.o mdbTableInfoCtrl.o\
          mdbExecuteEngine.o  mdbLimitCtrl.o mdbFlush.o mdbObserveCtrl.o mdbRowCtrl.o mdbDDChangeNotify.o mdbDDLExecuteEngine.o mdbScript.o\
          mdbAutoUnlockThread.o mdbJobCtrl.o mdbStorageEngine.o mdbProcCtrl.o  mdbLinkCtrl.o mdbRepCommon.o mdbHashIndexCtrl.o mdbMHashIndexCtrl.o \
          mdbCaptureLog.o mdbTrieIndexCtrl.o

ALL_EXES= 

all:	$(ALL_OBJS) 
	$(AR) libmdbControl.$(TRUESUFFIX)  $(ALL_OBJS) 
		
build: $(ALL_EXES)

install: all
	cp -rf *.$(TRUESUFFIX)  $(PROJECT_DIR)/lib

