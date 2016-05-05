include ../Makefile.incl

clean:
	rm -f *.o  libmdbdbflush.$(TRUESUFFIX)

ALL_OBJS=  mdbLoadFromDb.o mdbDAOLoad.o mdbChangeNotify.o mdbDAOBase.o mdbDAO.o mdbFileParser.o mdbDbLog.o mdbReadDbLog.o \
						mdbShadowFlushCtrl.o mdbShaowServerCtrl.o

ALL_EXES= mdbFlushFromDb.exe mdbClean.exe mdbDbRep.exe mdbFlushDb.exe mdbShadowFlush.exe mdbShadowServer.exe

all:	$(ALL_OBJS) 
	$(AR) libmdbdbflush.$(TRUESUFFIX)  $(ALL_OBJS) 
		
build: $(ALL_EXES)
	cp mdbFlushFromDb $(PROJECT_DIR)/bin
	cp mdbClean $(PROJECT_DIR)/bin
	cp mdbDbRep $(PROJECT_DIR)/bin
	cp mdbFlushDb $(PROJECT_DIR)/bin
	cp mdbShadowFlush $(PROJECT_DIR)/bin
	cp mdbShadowServer $(PROJECT_DIR)/bin

install: all
	cp -rf *.$(TRUESUFFIX)  $(PROJECT_DIR)/lib

