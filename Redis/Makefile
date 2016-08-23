include ../Makefile.incl

SERVER_OBJ=adlist.o anet.o ae.o networking.o server.o
clean:
	rm -f *.o *.$(TRUESUFFIX)

install:all
	cp -rf libmdbRedis.$(TRUESUFFIX) 	$(PROJECT_DIR)/lib 
all:	$(SERVER_OBJ) 
	$(AR) libmdbRedis.$(TRUESUFFIX)  $(SERVER_OBJ)

