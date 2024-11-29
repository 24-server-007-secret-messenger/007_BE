#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include "db.h"

MYSQL *db_connect() {
    MYSQL *conn = mysql_init(NULL); // MySQL 연결 핸들러 초기화
    if (conn == NULL) {
        fprintf(stderr, "MySQL initialization failed\n");
        return NULL;
    }

    // MySQL 연결 설정
    // mysql_real_connect(mysql, host, user, passwd, db, port, unix_socket, clientflag); 
    if (mysql_real_connect(conn, "127.0.0.1", "user_007", "007", "db_007", 3308, NULL, 0) == NULL) { 
        fprintf(stderr, "MySQL connection error: %s\n", mysql_error(conn));

        mysql_close(conn); // 연결 해제
        return NULL;
    }

    // 세션 시간대를 KST로 설정
    if (mysql_query(conn, "SET time_zone = '+09:00'")) {
        fprintf(stderr, "Failed to set time zone: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    return conn;
}

void db_disconnect(MYSQL *conn) {
    if (conn) mysql_close(conn);
}
