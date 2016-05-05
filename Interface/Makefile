include ../Makefile.incl

clean:
	rm -f *.o  libmdbInterface.$(TRUESUFFIX)

ALL_OBJS=  mdbQuery.o  mdbRollback.o mdbSequance.o mdbClientQuery.o mdbNosqlQuery.o

ALL_EXES= 

all:	$(ALL_OBJS) 
	$(AR) libmdbInterface.$(TRUESUFFIX)  $(ALL_OBJS) 
		
build: $(ALL_EXES)

install: all
	cp -rf *.$(TRUESUFFIX)  $(PROJECT_DIR)/lib

