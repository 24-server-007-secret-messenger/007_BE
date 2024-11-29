#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mongoose.h"
#include "websocket_server.h"
#include "db.h"
#include "chat_room.h"

// 전역 MySQL 연결
static MYSQL *global_db_conn = NULL;

void process_message(struct mg_connection *conn, struct mg_ws_message *wm) {
    struct mg_str message = wm->data;

    printf("Received raw message: %.*s\n", (int)message.len, message.buf); // 디버깅용

    // JSON 데이터 파싱
    const char *user1 = mg_json_get_str(message, "$.user1");
    const char *user2 = mg_json_get_str(message, "$.user2");
    const char *text = mg_json_get_str(message, "$.message");

    if (!user1 || !user2 || !text) {
        mg_ws_send(conn, "{\"error\": \"Invalid JSON message format\"}", 37, WEBSOCKET_OP_TEXT);
        return;
    }

    printf("Parsed user1: %s, user2: %s, message: %s\n", user1, user2, text); // 디버깅용

    // 채팅방 생성 또는 가져오기
    int chat_room_id = create_or_get_chat_room(global_db_conn, user1, user2);
    if (chat_room_id == -1) {
        fprintf(stderr, "Invalid JSON message format: %.*s\n", (int)message.len, message.buf); // 디버깅용
        mg_ws_send(conn, "{\"error\": \"Failed to create or get chat room\"}", 45, WEBSOCKET_OP_TEXT);
        return;
    }

    // 메시지 저장
    save_message(global_db_conn, chat_room_id, user1, text);

    // 메시지 브로드캐스트
    struct mg_connection *c;
    for (c = conn->mgr->conns; c != NULL; c = c->next) {
        if (c->is_websocket) {
            mg_ws_send(c, wm->data.buf, wm->data.len, WEBSOCKET_OP_TEXT);
        }
    }
}

void save_message(MYSQL *conn, int chat_room_id, const char *sender, const char *message) {
    char query[1024];
    snprintf(query, sizeof(query),
             "INSERT INTO chat_message (chat_room_id, sender, message) VALUES (%d, '%s', '%s')",
             chat_room_id, sender, message);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Message save failed: %s\n", mysql_error(conn));
    } else {
        printf("Message saved: %s\n", message);
    }
}

// WebSocket 핸들러
void websocket_handler(struct mg_connection *conn, int ev, void *ev_data) {
    switch (ev) {
        case MG_EV_WS_OPEN:
            printf("WebSocket connection opened.\n");
            break;
        case MG_EV_WS_MSG:
            printf("WebSocket message received: %.*s\n", 
                   (int)((struct mg_ws_message *)ev_data)->data.len,
                   ((struct mg_ws_message *)ev_data)->data.buf);
            process_message(conn, (struct mg_ws_message *)ev_data);
            break;
        case MG_EV_CLOSE:
            printf("WebSocket connection closed.\n");
            break;
        default:
            printf("Unhandled WebSocket event: %d\n", ev);
            break;
    }
}

// WebSocket 서버 시작
void start_websocket_server() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    global_db_conn = db_connect();
    if (!global_db_conn) {
        fprintf(stderr, "Failed to connect to database\n");
        return;
    }

    struct mg_connection *conn = mg_listen(&mgr, "ws://localhost:8001", websocket_handler, NULL);
    if (!conn) {
        fprintf(stderr, "Failed to start WebSocket server\n");
        return;
    }
    printf("WebSocket server started on ws://localhost:8001\n");

    for (;;) mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    db_disconnect(global_db_conn);
}
