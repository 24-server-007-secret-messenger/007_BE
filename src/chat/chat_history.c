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
             "SELECT sender, message, sent_at, encrypt FROM chat_message WHERE chat_room_id = %d ORDER BY sent_at ASC",
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

    char *response = malloc(3000000);
    
    if (!response) {
        mg_http_reply(conn_ws, 500, CORS_HEADERS, "{\"error\": \"Memory allocation failed\"}\n");
        mysql_free_result(result);
        return;
    }

    strcpy(response, "{\"chat_history\":[");
    MYSQL_ROW row;

    int first = 1; // JSON 배열의 첫 번째 요소 여부를 추적
    while ((row = mysql_fetch_row(result))) {
        const char *sender = row[0];
        const char *message_or_image = row[1];
        const char *sent_at = row[2];
        int encrypt = atoi(row[3]); // `encrypt` 필드를 int로 변환

        if (encrypt == 0) {
            // 암호화되지 않은 메시지
            char entry[1024];
            snprintf(entry, sizeof(entry),
                     "%s{\"sender\":\"%s\",\"message\":\"%s\",\"sent_at\":\"%s\"}",
                     first ? "" : ",", sender, message_or_image, sent_at);
            strcat(response, entry);
        } else {
            // 암호화된 메시지
            char txt_path[512];
            snprintf(txt_path, sizeof(txt_path), "assets/output/txt/%s.txt", message_or_image);

            FILE *txt_file = fopen(txt_path, "r");
            if (!txt_file) {
                fprintf(stderr, "Failed to open base64 file: %s\n", txt_path);
                continue; // 파일을 열지 못하면 무시하고 다음 항목으로 진행
            }

            // 파일 크기 계산
            fseek(txt_file, 0, SEEK_END);
            long long file_size = ftell(txt_file);
            rewind(txt_file);

            // 파일 내용을 저장할 버퍼 할당
            char *base64_content = malloc(file_size + 1); // 널 종료를 위해 추가 공간 할당
            if (!base64_content) {
                fprintf(stderr, "Memory allocation failed for base64 content\n");
                fclose(txt_file);
                continue; // 메모리 할당 실패 시 무시하고 다음 항목으로 진행
            }

            // 파일 내용 읽기
            fread(base64_content, 1, file_size, txt_file);
            base64_content[file_size] = '\0'; // 널 종료
            fclose(txt_file);

            // JSON에 추가
            char entry[500000];
            snprintf(entry, sizeof(entry),
                    "%s{\"sender\":\"%s\",\"base64\":\"%s\",\"sent_at\":\"%s\"}",
                    first ? "" : ",", sender, base64_content, sent_at);
            strcat(response, entry);

            free(base64_content); // 동적 메모리 해제
        }
        first = 0; // 첫 번째 요소 이후로는 콤마 추가
    }

    strcat(response, "]}"); // JSON 배열 종료

    mg_http_reply(conn_ws, 200, CORS_HEADERS "Content-Type: application/json\r\n", "%s", response);

    free(response);
    mysql_free_result(result);
}
