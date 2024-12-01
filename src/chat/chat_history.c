#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mongoose.h"
#include "chat_history.h"
#include "db.h"

// CORS 헤더 추가
#define CORS_HEADERS "Access-Control-Allow-Origin: *\r\n" \
                     "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n" \
                     "Access-Control-Allow-Headers: Content-Type\r\n"

void handle_chat_history(struct mg_connection *conn, struct mg_http_message *hm) {
    char *user1 = mg_json_get_str(hm->body, "$.user1");
    char *user2 = mg_json_get_str(hm->body, "$.user2");

    if (!user1 || !user2) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid JSON format\"}\n");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed\"}\n");
        free(user1);
        free(user2);
        return;
    }

    int chat_room_id = get_chat_room_id(db_conn, user1, user2);
    if (chat_room_id == -1) {
        mg_http_reply(conn, 404, CORS_HEADERS, "{\"error\": \"Chat room not found\"}\n");
    } else {
        get_chat_history(db_conn, chat_room_id, conn);
    }

    db_disconnect(db_conn);
    free(user1);
    free(user2);
}

int get_chat_room_id(MYSQL *conn, const char *user1, const char *user2) {
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT id FROM chat_room "
             "WHERE (user1 = '%s' AND user2 = '%s') OR (user1 = '%s' AND user2 = '%s')",
             user1, user2, user2, user1);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Chat room ID retrieval failed: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        fprintf(stderr, "Failed to store result: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int chat_room_id = -1;

    if (row) {
        chat_room_id = atoi(row[0]);
    }
    mysql_free_result(result);

    return chat_room_id;
}

void get_chat_history(MYSQL *conn, int chat_room_id, struct mg_connection *conn_ws) {
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT sender, message, sent_at FROM chat_message WHERE chat_room_id = %d ORDER BY sent_at ASC",
             chat_room_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Message retrieval failed: %s\n", mysql_error(conn));
        mg_http_reply(conn_ws, 500, CORS_HEADERS, "{\"error\": \"Failed to fetch chat history\"}\n");
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        fprintf(stderr, "Failed to store result: %s\n", mysql_error(conn));
        mg_http_reply(conn_ws, 500, CORS_HEADERS, "{\"error\": \"Failed to fetch chat history\"}\n");
        return;
    }

    char *response = malloc(4096); // 임시 버퍼
    if (!response) {
        mg_http_reply(conn_ws, 500, CORS_HEADERS, "{\"error\": \"Memory allocation failed\"}\n");
        mysql_free_result(result);
        return;
    }

    strcpy(response, "{\"chat_history\":[");
    MYSQL_ROW row;
    
    int first = 1; // JSON 배열의 첫 번째 요소 여부를 추적
    while ((row = mysql_fetch_row(result))) {
        char entry[1024];
        snprintf(entry, sizeof(entry),
                 "%s{\"sender\":\"%s\",\"message\":\"%s\",\"sent_at\":\"%s\"}",
                 first ? "" : ",", row[0], row[1], row[2]);
        strcat(response, entry);
        first = 0;
    }
    strcat(response, "]}"); // JSON 배열 종료

    mg_http_reply(conn_ws, 200, CORS_HEADERS "Content-Type: application/json\r\n", "%s", response);

    free(response);
    mysql_free_result(result);
}
