#ifndef SERVERDB_H
#define SERVERDB_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "protocol.h"
#include "mlog.h"
#include "sqlite3.h"

#define DBPATH "db.sqlite"

int setup_db();
int close_db();
int getBalance(Message *m, uint16_t *balance);
int insert(Message *m);
int update(Message *m);
int updateOnetimekey(Message *m);
int getNextOnetimekey(Message *m);

#endif /* SERVERDB_H */
