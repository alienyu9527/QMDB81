include ../Makefile.incl

pre:
	$(CCC)  -o lemon lemon.c
	-./lemon -p parser.y
	mv parser.h parser.h.temp
	awk -f ./addopcodes.awk parser.h.temp >parser.h
	$(CCC)  -o mkkeyword mkkeywordhash.c 
	./mkkeyword > keywordhash.h
	chmod +x Err2csv.sh

clean:
	rm -f *.o  libmdbHelper.$(TRUESUFFIX)

ALL_OBJS= mdbBase.o mdbHashTable.o mdbCacheTable.o mdbConfig.o mdbDateTime.o mdbDictionary.o TMsqDBQuery.o TDBFactory.o \
		  mdbExpr.o mdbFunc.o mdbMalloc.o  mdbMemValue.o mdbMutex.o mdbRepRecd.o \
		  TBaseException.o mdbDBInterface.o TOraDBQuery.o mdbXML.o mdbXMLError.o mdbXMLParser.o  \
		  mdbShm.o mdbSQLParser.o mdbQueue.o SqlParserHelper.o mdbFileList.o mdbErrorHelper.o \
		  SqlTypeCheck.o SyntaxTreeAnalyse.o tinyxml2.o Tokenize.o TThreadLog.o Walker.o TMutex.o mdbCacheLog.o mdbOS.o mdbEncrypt.o mdbHashAlgo.o \
		  mdbPerfInfo.o mdbCSPParser.o mdbSocket.o mdbCspAvpMgr.o mdbMultiProtector.o string_ex.o mdbCommandlineParser.o mdbThreadBase.o mdbConfigAlter.o


ALL_EXES=

all:pre $(ALL_OBJS) 
	$(AR) libmdbHelper.$(TRUESUFFIX)  $(ALL_OBJS) $(DATABASE_LIBS)
		
build: $(ALL_EXES)

install: all
	cp -rf *.$(TRUESUFFIX)  $(PROJECT_DIR)/lib

