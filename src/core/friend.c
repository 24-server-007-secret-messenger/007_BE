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

    // 중복 체크: 기존 친구 요청 또는 관계 확인
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT status FROM friend WHERE (from_user='%s' AND to_user='%s') OR (from_user='%s' AND to_user='%s')",
             from_user, to_user, to_user, from_user);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, "", "Failed to process friend request.\n");
        db_disconnect(db_conn);
        return;
    }

    MYSQL_RES *result = mysql_store_result(db_conn);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (row) {
        // 이미 관계가 존재할 경우
        const char *status = row[0];
        if (strcmp(status, "pending") == 0) {
            mg_http_reply(conn, 409, "", "Friend request already pending.\n");
        } else if (strcmp(status, "accepted") == 0) {
            mg_http_reply(conn, 409, "", "Already friends.\n");
        }
        mysql_free_result(result);
        db_disconnect(db_conn);
        free(from_user);
        free(to_user);
        return;
    }

    mysql_free_result(result);

    // 새로운 친구 요청 삽입
    snprintf(query, sizeof(query),
             "INSERT INTO friend (from_user, to_user, status) VALUES ('%s', '%s', 'pending')",
             from_user, to_user);

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

    // 친구 요청 상태 업데이트
    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE friend SET status='accepted' WHERE from_user='%s' AND to_user='%s' AND status='pending'",
             to_user, from_user); // 요청을 보낸 쪽과 수락한 쪽 반대

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, "", "Failed to accept friend request.\n");
        db_disconnect(db_conn);
        return;
    }

    if (mysql_affected_rows(db_conn) == 0) {
        mg_http_reply(conn, 404, "", "No pending friend request found.\n");
        printf("%s user doesn't request friendship to %s user.\n", from_user, to_user);
    } else {
        mg_http_reply(conn, 200, "", "Friend request accepted.\n");
    }

    db_disconnect(db_conn);
    free(from_user);
    free(to_user);
}

// 친구 거절 및 삭제 처리 함수
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

    // 친구 요청 또는 관계 삭제
    char query[256];
    snprintf(query, sizeof(query),
             "DELETE FROM friend WHERE (from_user='%s' AND to_user='%s') OR (from_user='%s' AND to_user='%s')",
             from_user, to_user, to_user, from_user);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, "", "Failed to reject friend.\n");
    } else if (mysql_affected_rows(db_conn) == 0) {
        mg_http_reply(conn, 404, "", "No friend relationship found to reject.\n");
    } else {
        mg_http_reply(conn, 200, "", "Friend relationship rejected/removed.\n");
    }

    db_disconnect(db_conn);
    free(from_user);
    free(to_user);
}
