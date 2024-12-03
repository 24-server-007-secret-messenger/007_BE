#include <stdio.h>
#include "mongoose.h"
#include "auth.h"
#include "friend.h"
#include "chat_room.h"
#include "chat_history.h"
#include "decryption.h"
#include "self_destruct.h"

void handle_request(struct mg_connection *conn, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;

        // OPTIONS 요청 처리 (CORS Preflight 요청)
        if (mg_strcmp(hm->method, mg_str("OPTIONS")) == 0) {
            mg_http_reply(conn, 204,
                          "Access-Control-Allow-Origin: *\r\n"
                          "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                          "Access-Control-Allow-Headers: Content-Type\r\n",
                          "");
            return;
        }

        if (mg_match(hm->uri, mg_str("/login"), NULL)) {
            handle_login(conn, hm); // 로그인 처리
        }
        else if (mg_match(hm->uri, mg_str("/logout"), NULL)) {
            handle_logout(conn, hm); // 로그아웃 처리
        }
        else if (mg_match(hm->uri, mg_str("/friend/request"), NULL)) {
            handle_friend_request(conn, hm); // 친구 요청 처리
        }
        else if (mg_match(hm->uri, mg_str("/friend/accept"), NULL)) {
            handle_friend_accept(conn, hm); // 친구 수락 처리
        }
        else if (mg_match(hm->uri, mg_str("/friend/reject"), NULL)) {
            handle_friend_reject(conn, hm); // 친구 거절 처리
        }
        else if (mg_match(hm->uri, mg_str("/friend/list"), NULL)) {
            handle_friend_list(conn, hm); // 전체 친구 리스트 전달
        }
        else if (mg_match(hm->uri, mg_str("/friend/active_list"), NULL)) {
            handle_active_friends_list(conn, hm); // 활동 중인 친구 리스트 전달
        }
        else if (mg_match(hm->uri, mg_str("/friend/requested_list"), NULL)) {
            handle_requested_friend_list(conn, hm); // 친구 요청이 온 친구 리스트 전달
        }
        else if (mg_match(hm->uri, mg_str("/chat/start"), NULL)) {
            handle_chat_start(conn, hm); // 채팅 시작
        }
        else if (mg_match(hm->uri, mg_str("/chat/history"), NULL)) {
            handle_chat_history(conn, hm); // 채팅 기록 전달
        }
        else if (mg_match(hm->uri, mg_str("/chat/room_list"), NULL)) {
            handle_chat_room_list(conn, hm); // 채팅방 리스트 전달
        }
        else if (mg_match(hm->uri, mg_str("/chat/decrypt"), NULL)) {
            handle_decrypt_message(conn, hm); // 비밀 메시지 복호화
        }
        else if (mg_match(hm->uri, mg_str("/chat/boom"), NULL)) {
            handle_timer(conn, hm); // 타이머 엔드포인트
        }
        else {
            // 404 Not Found 응답
            mg_http_reply(conn, 404,
                          "Access-Control-Allow-Origin: *\r\n",
                          "Not Found\n");
        }
    }
}

void start_http_server() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    // HTTP 요청 처리
    if (mg_http_listen(&mgr, "http://0.0.0.0:8000", handle_request, &mgr) == NULL) {
        fprintf(stderr, "Error starting HTTP server on port 8000\n");
        return;
    }

    printf("HTTP server started on port 8000\n");

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
}
