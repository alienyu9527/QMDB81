include ../Makefile.incl


clean:
	rm -f *.o  libmdbCommon.$(TRUESUFFIX)

ALL_OBJS= mdbBaseObject.o mdbCommons.o mdbComponent.o mdbDataStructs.o mdbDateUtils.o mdbNtcSplit.o mdbFileUtils.o \
				mdbStrings.o mdbStrUtils.o mdbSysLocks.o mdbSysMessages.o mdbSysShareMems.o mdbSysSockets.o mdbSysThreads.o \
				mdbSysTimerManager.o mdbSysTimerSoftClock.o mdbSysTimerTaskExecute.o mdbSysUtils.o mdbIniFiles.o \
				mdbNTC.o mdbNtcEngine.o mdbPeerEvent.o mdbPeerInfo.o mdbPeerProactor.o mdbPeerProtocol.o mdbProtocolWinntTcp.o
ALL_EXES=

all:$(ALL_OBJS) 
	$(AR) libmdbCommon.$(TRUESUFFIX)  $(ALL_OBJS)
		
build: $(ALL_EXES)

install: all
	cp -rf *.$(TRUESUFFIX)  $(PROJECT_DIR)/lib

