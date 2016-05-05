include ../Makefile.incl

clean:
	rm -f *.o  libmdbReplication.$(TRUESUFFIX)

ALL_OBJS= mdbRepClientCtrl.o mdbRepCtrl.o mdbRepFileParser.o mdbRepFlushDao.o mdbRepLoadData.o mdbRepLog.o mdbRepNTC.o\
          mdbRepServerCtrl.o mdbServerConfig.o mdbServerCtrl.o mdbQueueLog.o


ALL_EXES= mdbRep.exe mdbRepClient.exe mdbRepServer.exe mdbServer.exe mdbFlushRep.exe

all:	$(ALL_OBJS) 
	$(AR) libmdbReplication.$(TRUESUFFIX)  $(ALL_OBJS) 
		
build: $(ALL_EXES)	
	cp mdbRep $(PROJECT_DIR)/bin
	cp mdbRepClient $(PROJECT_DIR)/bin
	cp mdbRepServer $(PROJECT_DIR)/bin
	cp mdbServer $(PROJECT_DIR)/bin
	cp mdbFlushRep $(PROJECT_DIR)/bin
	
install: all
	cp -rf *.$(TRUESUFFIX)  $(PROJECT_DIR)/lib
