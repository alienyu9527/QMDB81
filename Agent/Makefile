include ../Makefile.incl

clean:
	rm -f *.o  libmdbAgent.$(TRUESUFFIX)

ALL_OBJS= mdbAgentServer.o mdbInfoCollect.o 

ALL_EXES= 

all:	$(ALL_OBJS) 
	$(AR) libmdbAgent.$(TRUESUFFIX)  $(ALL_OBJS) 

build: $(ALL_EXES)


install: all
	cp -rf *.$(TRUESUFFIX)  $(PROJECT_DIR)/lib

