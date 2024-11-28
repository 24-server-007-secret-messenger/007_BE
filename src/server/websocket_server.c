#include <stdio.h>
#include "mongoose.h"
#include "websocket_server.h"
#include "active_user.h"
#include "db.h"

// WebSocket 핸들러
void websocket_handler(struct mg_connection *conn, int ev, void *ev_data) {
    MYSQL *db_conn = db_connect();

    switch (ev) {
        case MG_EV_ACCEPT: {
            printf("New WebSocket connection accepted.\n");
            break;
        }
        case MG_EV_WS_OPEN: {
            printf("WebSocket connection opened.\n");
            break;
        }
        case MG_EV_WS_MSG: {
            struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;

            // 메시지 데이터 추출
            char *message = strndup(wm->data.buf, wm->data.len);
            printf("WebSocket message received: %s\n", message);

            if (strncmp(message, "pong", wm->data.len) == 0) {
                printf("Pong received from client.\n");
                conn->is_closing = 0; // 연결 유지
            } else {
                // 일반 메시지는 클라이언트로 에코
                mg_ws_send(conn, wm->data.buf, wm->data.len, WEBSOCKET_OP_TEXT);
            }

            free(message); // 동적 메모리 해제
            break;
        }
        case MG_EV_CLOSE: {
            printf("WebSocket connection closed.\n");
            break;
        }
        default:
            break;
    }

    db_disconnect(db_conn);
}

// WebSocket 서버 시작
void start_websocket_server() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    // WebSocket 요청 핸들러 등록
    struct mg_connection *conn = mg_listen(&mgr, "tcp://0.0.0.0:8001", websocket_handler, NULL);
    if (conn == NULL) {
        fprintf(stderr, "Error starting WebSocket server on port 8001\n");
        return;
    }

    printf("WebSocket server started on port 8001\n");

    // 주기적으로 Ping 메시지 전송
    for (;;) {
        mg_mgr_poll(&mgr, 1000);
        struct mg_connection *c;
        for (c = mgr.conns; c != NULL; c = c->next) {
            if (c->is_websocket && !c->is_closing) {
                mg_ws_send(c, "ping", 4, WEBSOCKET_OP_PING);
            }
        }
    }

    mg_mgr_free(&mgr);
}
