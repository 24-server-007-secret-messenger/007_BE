#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mongoose.h"
#include "db.h"

// 친구 요청 처리 함수
void handle_friend_request(struct mg_connection *conn, struct mg_http_message *hm) {
    char *from_user = mg_json_get_str(hm->body, "$.from");
    char *to_user = mg_json_get_str(hm->body, "$.to");

    if (!from_user || !to_user) {
        mg_http_reply(conn, 400, "", "Invalid JSON format.\n");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, "", "Database connection failed.\n");
        return;
    }

    char query[256];
    snprintf(query, sizeof(query), "INSERT INTO friend_requests (from_user, to_user, status) VALUES ('%s', '%s', 'pending')", from_user, to_user);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, "", "Failed to send friend request.\n");
    } else {
        mg_http_reply(conn, 200, "", "Friend request sent.\n");
    }

    db_disconnect(db_conn);
    free(from_user);
    free(to_user);
}

// 친구 수락 처리 함수
void handle_friend_accept(struct mg_connection *conn, struct mg_http_message *hm) {
    char *from_user = mg_json_get_str(hm->body, "$.from");
    char *to_user = mg_json_get_str(hm->body, "$.to");

    if (!from_user || !to_user) {
        mg_http_reply(conn, 400, "", "Invalid JSON format.\n");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, "", "Database connection failed.\n");
        return;
    }

    char query[256];
    snprintf(query, sizeof(query), "UPDATE friend_requests SET status='accepted' WHERE from_user='%s' AND to_user='%s'", from_user, to_user);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, "", "Failed to accept friend request.\n");
    } else {
        mg_http_reply(conn, 200, "", "Friend request accepted.\n");
    }

    db_disconnect(db_conn);
    free(from_user);
    free(to_user);
}

// 친구 거절 처리 함수
void handle_friend_reject(struct mg_connection *conn, struct mg_http_message *hm) {
    char *from_user = mg_json_get_str(hm->body, "$.from");
    char *to_user = mg_json_get_str(hm->body, "$.to");

    if (!from_user || !to_user) {
        mg_http_reply(conn, 400, "", "Invalid JSON format.\n");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, "", "Database connection failed.\n");
        return;
    }

    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM friend WHERE from_user='%s' AND to_user='%s'", from_user, to_user);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, "", "Failed to reject friend.\n");
    } else {
        mg_http_reply(conn, 200, "", "Friend rejected.\n");
    }

    db_disconnect(db_conn);
    free(from_user);
    free(to_user);
}
