#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include "db.h"

void add_active_user(MYSQL *conn, const char *username) {
    char query[256];
    snprintf(query, sizeof(query), "INSERT IGNORE INTO active_user (username) VALUES ('%s')", username);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to add active user: %s\n", mysql_error(conn));
    }
}

void remove_active_user(MYSQL *conn, const char *username) {
    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM active_user WHERE username='%s'", username);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to remove active user: %s\n", mysql_error(conn));
    }
}

void clear_active_users(MYSQL *conn) {
    if (mysql_query(conn, "TRUNCATE TABLE active_user")) {
        fprintf(stderr, "Failed to clear active users: %s\n", mysql_error(conn));
    }
}
