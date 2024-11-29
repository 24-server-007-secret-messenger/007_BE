#include <stdio.h>
#include "mongoose.h"
#include "auth.h"
#include "friend.h"
#include "chat_room.h"
#include "chat_history.h"

void handle_request(struct mg_connection *conn, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        if (mg_match(hm->uri, mg_str("/login"), NULL)) {
            handle_login(conn, hm);  // 로그인 처리
        }
        else if (mg_match(hm->uri, mg_str("/logout"), NULL)) {
            handle_logout(conn, hm);  // 로그아웃 처리
        }
        else if (mg_match(hm->uri, mg_str("/friend/request"), NULL)) {
            handle_friend_request(conn, hm);  // 친구 요청 처리
        }
        else if (mg_match(hm->uri, mg_str("/friend/accept"), NULL)) {
            handle_friend_accept(conn, hm);  // 친구 수락 처리
        }
        else if (mg_match(hm->uri, mg_str("/friend/reject"), NULL)) {
            handle_friend_reject(conn, hm);  // 친구 거절 처리
        }
        else if (mg_match(hm->uri, mg_str("/chat/start"), NULL)) {
            handle_chat_start(conn, hm);
        }
        else if (mg_match(hm->uri, mg_str("/chat/history"), NULL)) {
            handle_chat_history(conn, hm);
        }
        else {
            mg_http_reply(conn, 404, "", "Not Found\n");
        }
    }
}

void start_http_server() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    // HTTP 요청 처리
    if (mg_http_listen(&mgr, "http://localhost:8000", handle_request, &mgr) == NULL) {
        fprintf(stderr, "Error starting HTTP server on port 8000\n");
        return;
    }

    printf("HTTP server started on port 8000\n");

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
}