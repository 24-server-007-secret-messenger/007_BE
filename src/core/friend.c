#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mongoose.h"
#include "db.h"
#include "session.h"

// CORS 헤더 추가
#define CORS_HEADERS "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST, GET, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\n"

// 친구 요청 처리 함수
void handle_friend_request(struct mg_connection *conn, struct mg_http_message *hm) {
    char *from_user = mg_json_get_str(hm->body, "$.from");
    char *to_user = mg_json_get_str(hm->body, "$.to");

    if (!from_user || !to_user) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid JSON format.\"}");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed.\"}");
        return;
    }

    // 중복 체크: 기존 친구 요청 또는 관계 확인
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT status FROM friend WHERE (from_user='%s' AND to_user='%s') OR (from_user='%s' AND to_user='%s')",
             from_user, to_user, to_user, from_user);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to process friend request.\"}");
        db_disconnect(db_conn);
        return;
    }

    MYSQL_RES *result = mysql_store_result(db_conn);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (row) {
        // 이미 관계가 존재할 경우
        const char *status = row[0];
        if (strcmp(status, "pending") == 0) {
            mg_http_reply(conn, 409, CORS_HEADERS, "{\"error\": \"Friend request already pending.\"}");
        } else if (strcmp(status, "accepted") == 0) {
            mg_http_reply(conn, 409, CORS_HEADERS, "{\"error\": \"Already friends.\"}");
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
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to send friend request.\"}");
    } else {
        mg_http_reply(conn, 200, CORS_HEADERS, "{\"message\": \"Friend request sent.\"}");
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
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid JSON format.\"}");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed.\"}");
        return;
    }

    // 친구 요청 상태 업데이트
    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE friend SET status='accepted' WHERE from_user='%s' AND to_user='%s' AND status='pending'",
             to_user, from_user); // 요청을 보낸 쪽과 수락한 쪽 반대

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to accept friend request.\"}");
        db_disconnect(db_conn);
        return;
    }

    if (mysql_affected_rows(db_conn) == 0) {
        mg_http_reply(conn, 404, CORS_HEADERS, "{\"error\": \"No pending friend request found.\"}");
    } else {
        mg_http_reply(conn, 200, CORS_HEADERS, "{\"message\": \"Friend request accepted.\"}");
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
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid JSON format.\"}");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed.\"}");
        return;
    }

    // 친구 요청 또는 관계 삭제
    char query[256];
    snprintf(query, sizeof(query),
             "DELETE FROM friend WHERE (from_user='%s' AND to_user='%s') OR (from_user='%s' AND to_user='%s')",
             from_user, to_user, to_user, from_user);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to reject friend.\"}");
    } else if (mysql_affected_rows(db_conn) == 0) {
        mg_http_reply(conn, 404, CORS_HEADERS, "{\"error\": \"No friend relationship found to reject.\"}");
    } else {
        mg_http_reply(conn, 200, CORS_HEADERS, "{\"message\": \"Friend relationship rejected/removed.\"}");
    }

    db_disconnect(db_conn);
    free(from_user);
    free(to_user);
}

// 친구 목록 추출 함수
void handle_friend_list(struct mg_connection *conn, struct mg_http_message *hm) {
    char *username = mg_json_get_str(hm->body, "$.username");
    if (!username) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid JSON format. 'username' required.\"}");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed.\"}");
        return;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT DISTINCT CASE "
             "WHEN from_user = '%s' THEN to_user "
             "WHEN to_user = '%s' THEN from_user "
             "END AS username "
             "FROM friend "
             "WHERE (from_user = '%s' OR to_user = '%s') AND status = 'accepted'",
             username, username, username, username);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to retrieve friends.\"}");
        db_disconnect(db_conn);
        return;
    }

    MYSQL_RES *result = mysql_store_result(db_conn);
    if (!result) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to process database result.\"}");
        db_disconnect(db_conn);
        return;
    }

    // JSON 결과 생성
    char response[2048] = "{\"friends\":["; // JSON 시작
    MYSQL_ROW row;
    int first = 1;

    while ((row = mysql_fetch_row(result))) {
        if (!first) strcat(response, ",");
        first = 0;
        snprintf(response + strlen(response), sizeof(response) - strlen(response), "{\"username\": \"%s\"}", row[0]);
    }
    strcat(response, "]}"); // JSON 끝

    mg_http_reply(conn, 200, CORS_HEADERS, "%s", response);

    mysql_free_result(result);
    db_disconnect(db_conn);
}

void handle_active_friends_list(struct mg_connection *conn, struct mg_http_message *hm) {
    // JSON 본문에서 username 추출
    char *username = mg_json_get_str(hm->body, "$.username");
    if (!username) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid JSON format. 'username' required.\"}");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed.\"}");
        return;
    }

    // SQL 쿼리 작성: 입력받은 username을 제외한 active friends를 선택
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT DISTINCT u.username "
             "FROM user u "
             "JOIN active_user au ON u.username = au.username "
             "JOIN friend f ON (f.from_user = u.username OR f.to_user = u.username) "
             "WHERE f.status = 'accepted' AND u.username != '%s'", username);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to retrieve active friends.\"}");
        db_disconnect(db_conn);
        return;
    }

    MYSQL_RES *result = mysql_store_result(db_conn);
    if (!result) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to process database result.\"}");
        db_disconnect(db_conn);
        return;
    }

    // JSON 응답 생성
    char response[2048] = "{\"activeUsers\":["; // JSON 시작
    MYSQL_ROW row;
    int first = 1;

    while ((row = mysql_fetch_row(result))) {
        if (!first) strcat(response, ",");
        first = 0;
        snprintf(response + strlen(response), sizeof(response) - strlen(response),
                 "{\"username\": \"%s\"}", row[0]);
    }
    strcat(response, "]}"); // JSON 끝

    mg_http_reply(conn, 200, CORS_HEADERS, "%s", response);

    mysql_free_result(result);
    db_disconnect(db_conn);
}

void handle_requested_friend_list(struct mg_connection *conn, struct mg_http_message *hm) {
    char *username = mg_json_get_str(hm->body, "$.username");
    if (!username) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid JSON format.\"}");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed.\"}");
        free(username);
        return;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT to_user FROM friend WHERE from_user='%s' AND status='pending'", username);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to retrieve pending friend requests.\"}");
        db_disconnect(db_conn);
        free(username);
        return;
    }

    MYSQL_RES *result = mysql_store_result(db_conn);
    if (!result) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to process database result.\"}");
        db_disconnect(db_conn);
        free(username);
        return;
    }

    char response[1024] = "{\"requestedList\":["; // JSON 시작
    MYSQL_ROW row;
    int first = 1;

    while ((row = mysql_fetch_row(result))) {
        if (!first) strcat(response, ",");
        first = 0;
        snprintf(response + strlen(response), sizeof(response) - strlen(response), "{\"sender\": \"%s\"}", row[0]);
    }
    strcat(response, "]}"); // JSON 끝

    mg_http_reply(conn, 200, CORS_HEADERS, "%s", response);

    mysql_free_result(result);
    db_disconnect(db_conn);
    free(username);
}
