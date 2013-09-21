#!/bin/bash
rm -rf db.sqlite
echo "
CREATE TABLE accounts(id INTEGER PRIMARY KEY AUTOINCREMENT, cardnumber INTEGER, pinhash TEXT, balance INTEGER);
INSERT INTO accounts (cardnumber, pinhash, balance) VALUES (1000, 1234, 500);
INSERT INTO accounts (cardnumber, pinhash, balance) VALUES (1001, 2345, 1500);
INSERT INTO accounts (cardnumber, pinhash, balance) VALUES (1002, 3456, 2500);
" | sqlite3 db.sqlite 
