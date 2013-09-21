#ifndef SERVERDB_H
#define SERVERDB_H

#include <sqlite3.h>

#define DBPATH "db.sqlite"

#define CREATE_TABLE_STRING "CREATE TABLE accounts(id INTEGER PRIMARY KEY AUTOINCREMENT, cardnumber INTEGER, pinhash TEXT, balance INTEGER);"

#define BALANCE_STRING "SELECT balance FROM accounts WHERE cardnumber LIKE :cardnumber AND pinhash LIKE :pin"

#define UPDATE_STRING "UPDATE accounts SET balance = :newbalance WHERE cardnumber LIKE :cardnumber AND pinhash LIKE :pin"

#define INSERT_STRING "INSERT INTO accounts (cardnumber, pinhash, balance) VALUES (?1, ?2, ?3)"



int setup_db();
int close_db();
int getBalance(Message *m, uint16_t *balance);
int insert(Message *m);
int update(Message *m);

#endif /* SERVERDB_H */
