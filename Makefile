.PHONY: all wc clean run-server-valgrind run-client-valgrind test

# the source files
SRCCOMMON = protocol.c mlog.c 
SRCSERVER = server.c serverdb.c sqlite3.c $(SRCCOMMON)
SRCCLIENT = client.c $(SRCCOMMON)

# enable or disable logging (comment out to disable)
LOGGING = -DLOGGING

# the objects files
SERVEROBJECTS = $(SRCSERVER:.c=.o)
CLIENTOBJECTS = $(SRCCLIENT:.c=.o)

# all objects
OBJECTS = $(SERVEROBJECTS) $(CLIENTOBJECTS)

# all binaries
BINS = server client

# the compiler flags
CFLAGS = -g -O0 -Wall -Wno-deprecated $(LOGGING)

# the libraries used
LDLIBS = -lpthread -ldl -lreadline
#LDLIBS = -lcrypto -lsqlite3

# the c99 compiler
CC = gcc # use this on linux
#CC = clang # use this on os x

# the output files
all: server client
server: $(SERVEROBJECTS)
	$(CC) $(CFLAGS) -o $@ $(SERVEROBJECTS) $(LDLIBS)
client: $(CLIENTOBJECTS)
	$(CC) $(CFLAGS) -o $@ $ $(CLIENTOBJECTS) $(LDLIBS) 

# other files to clean
OTHERCLEANING = valgrind.log

# dSYM's 
DSYMS = $(BINS:=.dSYM) 

# the dependency files
DEPS = $(SRCSERVER:.c=.DEP) $(SRCCLIENT:.c=.DEP)

# the valgrind flags
VLGDFLAGS = --leak-check=full --log-file=valgrind.log --track-origins=yes 

run-server-valgrind: server server.dSYM
	valgrind $(VLGDFLAGS) server
run-client-valgrind: client client.dSYM
	valgrind $(VLGDFLAGS) client

wc:
	@wc -l *.c *.h | grep -i -v -e total | awk '{print $$1}' | \
		awk '{sum += $$1} END {print sum " total"}'

clean:
	@rm -f $(OBJECTS) # the object files
	@rm -f $(BINS) # binary files
	@rm -f $(OTHERCLEANING) # other stuff
	@rm -f $(DEPS)  # dependencies
	@rm -rf $(DSYMS) # debugging info

%.dSYM : % 
	dsymutil $* -o $*.dSYM

# compile the object files, and dependencies if necessary
%.o : %.c
	$(COMPILE.c) -MD -o $@ $<
	@cp $*.d $*.DEP; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.DEP; \
		rm -f $*.d

test: all
	rm -f server.log
	clear
	make
	./server 1234 &
	./client localhost 1234
	killall server
	cat server.log

-include $(DEPS)
