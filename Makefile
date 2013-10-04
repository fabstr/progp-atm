.PHONY: all wc clean run-server-valgrind run-client-valgrind test

# the source files
SRCCOMMON = networking.c mlog.c
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

# for some reason openssl is deprecated on os x
CFLAGS = -g -O0 -Wall -Wno-deprecated -D_GNU_SOURCE $(LOGGING)

# the libraries used
LDLIBS = -lpthread -lreadline -lsqlite3 -lpolarssl

# the c compiler, set to gcc on linux and clang on os x
CC = clang

# the output files
all: server client manage
server: $(SERVEROBJECTS)
	$(CC) $(CFLAGS) -o $@ $(SERVEROBJECTS) $(LDLIBS)
client: $(CLIENTOBJECTS)
	$(CC) $(CFLAGS) -o $@ $(CLIENTOBJECTS) $(LDLIBS) 
manage: $(MANAGEOBJECTS)
	$(CC) $(CFLAGS) -o $@ $(MANAGEOBJECTS) $(LDLIBS)

# other files to clean
OTHERCLEANING = valgrind.log manage.log client.log server.log *.d

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
	rm -f server.log client.log
	clear
	./server &
	./client localhost 
	killall server
	@echo There is info in server.log and client.log

test-manage: all
	rm -f server.log client.log manage.log
	clear
	./server &
	./manage
	killall server
	@echo There is info in server.log, client.log and manage.log


docs: Doxyfile
	doxygen Doxyfile

-include $(DEPS)
