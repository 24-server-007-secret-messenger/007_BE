#ifndef ACTIVE_USER_H
#define ACTIVE_USER_H

#include <mysql/mysql.h>

void add_active_user(MYSQL *conn, const char *username);
void remove_active_user(MYSQL *conn, const char *username);
void clear_active_users(MYSQL *conn);

#endif
