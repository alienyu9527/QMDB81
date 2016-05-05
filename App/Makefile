include ../Makefile.incl

clean:
	rm -f *.o  libmdbApp.$(TRUESUFFIX) QuickMDB 

ALL_OBJS= mdbCtrl.o mdbMonitor.o QuickMDB.o    

ALL_EXES= QuickMDB.exe mdbAgent.exe mdbJob.exe mdbFlushSequence.exe mdbCheckPoint.exe

all:	$(ALL_OBJS) 
	$(AR) libmdbApp.$(TRUESUFFIX)  $(ALL_OBJS) 
		
build: $(ALL_EXES)
	cp QuickMDB $(PROJECT_DIR)/bin
	cp mdbAgent $(PROJECT_DIR)/bin
	cp mdbJob $(PROJECT_DIR)/bin
	cp mdbFlushSequence $(PROJECT_DIR)/bin
	cp mdbCheckPoint $(PROJECT_DIR)/bin

install: all
	cp -rf *.$(TRUESUFFIX)  $(PROJECT_DIR)/lib

