#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mongoose.h"
#include "websocket_server.h"
#include "db.h"
#include "chat_room.h"
#include "encryption.h"
#include "socket_client.h"

// 전역 MySQL 연결
static MYSQL *global_db_conn = NULL;

// 메시지 처리
void process_message(struct mg_connection *conn, struct mg_ws_message *wm) {
    struct mg_str message = wm->data;
    printf("Received raw message: %.*s\n", (int)message.len, message.buf);

    // JSON 데이터 파싱
    const char *from = mg_json_get_str(message, "$.from");
    const char *to = mg_json_get_str(message, "$.to");
    const char *text = mg_json_get_str(message, "$.message");
    bool encrypt = false;
    mg_json_get_bool(message, "$.encrypt", &encrypt);
    const char *key = mg_json_get_str(message, "$.key");

    if (!from || !to || !text) {
        mg_ws_printf(conn, WEBSOCKET_OP_TEXT, "{\"error\": \"Invalid JSON message format\"}");
        return;
    }

    printf("Parsed from: %s, to: %s, message: %s, encrypt: %s\n", from, to, text, encrypt ? "true" : "false");

    // 채팅방 생성 또는 가져오기
    int chat_room_id = create_or_get_chat_room(global_db_conn, from, to);
    if (chat_room_id == -1) {
        fprintf(stderr, "Failed to create or get chat room.\n");
        mg_ws_printf(conn, WEBSOCKET_OP_TEXT, "{\"error\": \"Failed to create or get chat room\"}");
        return;
    }

    if (encrypt) {
        // 암호화된 메시지 처리
        if (key) {
            char *response = send_tcp_request("encrypt", text, key); // TCP로 암호화 요청
            char *stego_image_name = handle_encrypt_message(conn, from, to, text, key, chat_room_id);
            if (stego_image_name) {
                printf("Steganography image: %s\n", stego_image_name);
                save_message(global_db_conn, chat_room_id, from, stego_image_name, encrypt);
                free(stego_image_name);  // 동적 메모리 해제
            } else {
                mg_ws_printf(conn, WEBSOCKET_OP_TEXT, "{\"error\": \"Failed to process encrypted message\"}");
            }


        }
        else {
            mg_ws_printf(conn, WEBSOCKET_OP_TEXT, "{\"error\": \"Key is required for encryption\"}");
        }
    } else {
        // 평문 메시지 처리
        char *response = send_tcp_request("plain", text, NULL); // TCP로 일반 메시지 처리 요청
        if (response) {
            save_message(global_db_conn, chat_room_id, from, response, encrypt);
            mg_ws_printf(conn, WEBSOCKET_OP_TEXT, "{\"message\": \"%s\"}", response);
            free(response);
        } else {
            mg_ws_printf(conn, WEBSOCKET_OP_TEXT, "{\"error\": \"Failed to process plain message\"}");
        }
    }
}

// 메시지 저장
void save_message(MYSQL *conn, int chat_room_id, const char *sender, const char *message, bool encrypt) {
    char query[1024];
    snprintf(query, sizeof(query),
             "INSERT INTO chat_message (chat_room_id, sender, message, encrypt) VALUES (%d, '%s', '%s', %d)",
             chat_room_id, sender, message, encrypt);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Message save failed: %s\n", mysql_error(conn));
    } else {
        printf("Message saved: %s\n", message);
    }
}

// WebSocket 핸들러
void websocket_handler(struct mg_connection *conn, int ev, void *ev_data) {
    switch (ev) {
        case MG_EV_HTTP_MSG: { // Full HTTP request/response | struct mg_http_message *
            struct mg_http_message *hm = (struct mg_http_message *)ev_data;
            // URI 매칭: "/ws" 경로 확인
            if (strncmp(hm->uri.buf, "/ws", hm->uri.len) == 0 && hm->uri.len == 3) {
                mg_ws_upgrade(conn, hm, NULL);  // WebSocket 핸드셰이크
                printf("WebSocket handshake completed.\n");
            }
            break;
        }
        case MG_EV_OPEN: // Connection created | NULL
            printf("WebSocket connection created.\n");
            break;
        case MG_EV_CLOSE: // Connection closed | NULL
            printf("WebSocket connection closed.\n");
            break;
        case MG_EV_WS_OPEN: // Websocket handshake done | struct mg_http_message *
            printf("WebSocket handshake done.\n");
            mg_ws_printf(conn, WEBSOCKET_OP_TEXT, "{\"message\": \"Connection established\"}");
            break;
        case MG_EV_WS_MSG: { // Websocket msg, text or bin | struct mg_ws_message *
            struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
            uint8_t msg_type = wm->flags & 0x0F;
            if (msg_type == WEBSOCKET_OP_TEXT) {
                printf("WebSocket text message received: %.*s\n", (int)wm->data.len, wm->data.buf);
                process_message(conn, wm);  // JSON 데이터 처리
            } else {
                printf("Unsupported WebSocket message type: %d\n", msg_type);
            }
            break;
        }
        default:
            // printf("Unhandled WebSocket event: %d\n", ev);
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

    struct mg_connection *conn = mg_http_listen(&mgr, "http://0.0.0.0:8001", websocket_handler, NULL);
    if (!conn) {
        fprintf(stderr, "Failed to start WebSocket server\n");
        return;
    }
    printf("WebSocket server started on ws://0.0.0.0:8001/ws\n");

    for (;;) mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    db_disconnect(global_db_conn);
}
