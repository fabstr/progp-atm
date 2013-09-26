.PHONY: all wc clean run-server-valgrind run-client-valgrind test

# the source files
SRCCOMMON = protocol.c mlog.c
SRCSERVER = server.c serverdb.c $(SRCCOMMON)
SRCCLIENT = client.c clientdb.c $(SRCCOMMON)
SRCMANAGE = manage.c $(SRCCOMMON)

# enable or disable logging (comment out to disable)
LOGGING = -DLOGGING

# the objects files
SERVEROBJECTS = $(SRCSERVER:.c=.o)
CLIENTOBJECTS = $(SRCCLIENT:.c=.o)
MANAGEOBJECTS = $(SRCMANAGE:.c=.o)

# all objects
OBJECTS = $(SERVEROBJECTS) $(CLIENTOBJECTS) $(MANAGEOBJECTS)

# all binaries
BINS = server client manage

# the compiler flags add -Wno-deprecated to use openssl on os x
CFLAGS = -g -O0 -Wall -D_GNU_SOURCE $(LOGGING)

# the libraries used
LDLIBS = -lpthread -lreadline -lsqlite3
#LDLIBS = -lcrypto

# the c99 compiler
#CC = gcc # use this on linux
CC = clang # use this on os x

# the output files
all: server client manage
server: $(SERVEROBJECTS)
	$(CC) $(CFLAGS) -o $@ $(SERVEROBJECTS) $(LDLIBS)
client: $(CLIENTOBJECTS)
	$(CC) $(CFLAGS) -o $@ $(CLIENTOBJECTS) $(LDLIBS) 
manage: $(MANAGEOBJECTS)
	$(CC) $(CFLAGS) -o $@ $(MANAGEOBJECTS) $(LDLIBS)

# other files to clean
OTHERCLEANING = valgrind.log manage.log client.log server.log

# dSYM's 
DSYMS = $(BINS:=.dSYM) 

# the dependency files
DEPS = $(SRCSERVER:.c=.DEP) $(SRCCLIENT:.c=.DEP) $(SRCMANAGE:.c=.DEP)

# the valgrind flags
VLGDFLAGS = --leak-check=full --track-origins=yes #--log-file=valgrind.log 

run-server-valgrind: server server.dSYM
	@rm -f valgrind.log
	valgrind $(VLGDFLAGS) ./server
run-client-valgrind: client client.dSYM
	@rm -f valgrind.log
	valgrind $(VLGDFLAGS) ./client localhost

wc:
	@wc -l *.c *.h | grep -i -v -e total -e sqlite3.h -e sqlite3.c | awk '{print $$1}' | \
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
