#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mongoose.h"
#include "db.h"
#include "chat_room.h"

// CORS 헤더 추가
#define CORS_HEADERS "Access-Control-Allow-Origin: *\r\n" \
                     "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n" \
                     "Access-Control-Allow-Headers: Content-Type\r\n"

int create_or_get_chat_room(MYSQL *conn, const char *user1, const char *user2) {
    // 사용자 정렬: 항상 user1 < user2로 정렬
    const char *first_user = (strcmp(user1, user2) < 0) ? user1 : user2;
    const char *second_user = (strcmp(user1, user2) < 0) ? user2 : user1;

    // 친구 관계와 active_user 확인
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT COUNT(*) FROM friend f "
             "JOIN active_user au1 ON f.from_user = au1.username "
             "JOIN active_user au2 ON f.to_user = au2.username "
             "WHERE ((f.from_user = '%s' AND f.to_user = '%s') OR "
             "(f.from_user = '%s' AND f.to_user = '%s')) "
             "AND f.status = 'accepted'",
             first_user, second_user, second_user, first_user);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Friend and active_user check failed: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        fprintf(stderr, "Failed to store result: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int is_friend_and_active = atoi(row[0]);
    mysql_free_result(result);

    if (!is_friend_and_active) {
        fprintf(stderr, "Users are not friends or not active.\n");
        return -1;
    }

    // 채팅방 생성 또는 가져오기: 항상 정렬된 순서로 삽입
    snprintf(query, sizeof(query),
             "INSERT INTO chat_room (user1, user2) "
             "VALUES ('%s', '%s') "
             "ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id)",
             first_user, second_user);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Chat room creation failed: %s\n", mysql_error(conn));
        return -1;
    }

    return (int)mysql_insert_id(conn);
}

void handle_chat_room_list(struct mg_connection *conn, struct mg_http_message *hm) {
    // JSON 본문에서 username 추출
    char *username = mg_json_get_str(hm->body, "$.username");
    if (!username) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid JSON format. 'username' is required.\"}\n");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed.\"}\n");
        free(username);
        return;
    }

    // SQL 쿼리 작성: chat_room 테이블에서 username과 일치하지 않는 user1 또는 user2 선택
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT DISTINCT CASE "
             "WHEN user1 = '%s' THEN user2 "
             "WHEN user2 = '%s' THEN user1 "
             "END AS partner "
             "FROM chat_room "
             "WHERE user1 = '%s' OR user2 = '%s'",
             username, username, username, username);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db_conn));
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to retrieve chat partners.\"}\n");
        db_disconnect(db_conn);
        free(username);
        return;
    }

    MYSQL_RES *result = mysql_store_result(db_conn);
    if (!result) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to process database result.\"}\n");
        db_disconnect(db_conn);
        free(username);
        return;
    }

    // JSON 응답 생성
    char response[2048] = "{\"partners\":["; // JSON 시작
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
    free(username);
}


void handle_chat_start(struct mg_connection *conn, struct mg_http_message *hm) {
    char *user1 = mg_json_get_str(hm->body, "$.user1");
    char *user2 = mg_json_get_str(hm->body, "$.user2");

    if (!user1 || !user2) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid JSON format\"}\n");
        return;
    }

    // 사용자 정렬
    if (strcmp(user1, user2) > 0) {
        char *temp = user1;
        user1 = user2;
        user2 = temp;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed\"}\n");
        free(user1);
        free(user2);
        return;
    }

    int chat_room_id = create_or_get_chat_room(db_conn, user1, user2);
    if (chat_room_id == -1) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to create chat room\"}\n");
    } else {
        mg_http_reply(conn, 200, CORS_HEADERS, "{\"chat_room_id\": %d}\n", chat_room_id);
    }

    db_disconnect(db_conn);
    free(user1);
    free(user2);
}
