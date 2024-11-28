#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>

MYSQL *db_connect();
void db_disconnect(MYSQL *conn);

#endif // DB_H
