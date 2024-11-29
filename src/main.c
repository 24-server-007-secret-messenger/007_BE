#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
// #include "file_server.h"
#include "http_server.h"
#include "websocket_server.h"
#include "db.h"
#include "active_user.h"

// Graceful Shutdown 핸들러
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nSignal received (%d). Cleaning up and shutting down...\n", sig);

        MYSQL *db_conn = db_connect();
        if (db_conn) {
            clear_active_users(db_conn);
            db_disconnect(db_conn);
            printf("Active user table cleaned up successfully during shutdown.\n");
        } else {
            fprintf(stderr, "Error connecting to database during shutdown.\n");
        }
        exit(0);
    }
}

int main() {
    pthread_t http_thread, websocket_thread;
    MYSQL *db_conn;

    // Signal 핸들러 설정
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 서버 시작 시 active_user 초기화
    db_conn = db_connect();
    if (db_conn) {
        clear_active_users(db_conn);
        db_disconnect(db_conn);
        printf("Active user table initialized successfully.\n");
    } else {
        fprintf(stderr, "Error connecting to database during initialization.\n");
        return 1;
    }

    // 각 서버 쓰레드 생성
    if (pthread_create(&http_thread, NULL, (void *(*)(void *))start_http_server, NULL) != 0) {
        perror("Error creating HTTP server thread");
        return 1;
    }
    if (pthread_create(&websocket_thread, NULL, (void *(*)(void *))start_websocket_server, NULL) != 0) {
        perror("Error creating WebSocket server thread");
        return 1;
    }
    // if (pthread_create(&file_thread, NULL, (void *(*)(void *))start_file_server, NULL) != 0) {
    //     perror("Error creating File server thread");
    //     return 1;
    // }

    // 쓰레드 종료 대기
    pthread_join(http_thread, NULL);
    pthread_join(websocket_thread, NULL);
    // pthread_join(file_thread, NULL);

    // 서버 종료 시 active_user 초기화
    db_conn = db_connect();
    if (db_conn) {
        clear_active_users(db_conn);
        db_disconnect(db_conn);
        printf("Active user table cleaned up successfully.\n");
    } else {
        fprintf(stderr, "Error connecting to database for cleanup.\n");
    }

    return 0;
}
