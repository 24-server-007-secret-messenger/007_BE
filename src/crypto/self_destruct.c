#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "mongoose.h"

// CORS 설정
#define CORS_HEADERS "Access-Control-Allow-Origin: *\r\n" \
                     "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n" \
                     "Access-Control-Allow-Headers: Content-Type\r\n"

// 타이머 데이터를 저장할 구조체
typedef struct {
    struct mg_connection *conn;
    struct tms start_time;
    clock_t start_ticks;
} timer_data_t;

// 스레드 함수
void* timer_thread(void* arg) {
    timer_data_t *data = (timer_data_t *)arg;

    // 10초 대기
    printf("Timer thread started. Waiting for 10 seconds...\n");
    sleep(10);

    // 종료 시간 계산
    struct tms end_time;
    clock_t end_ticks = times(&end_time);
    clock_t elapsed_ticks = end_ticks - data->start_ticks; // 경과 시간(틱 단위)
    long clock_ticks = sysconf(_SC_CLK_TCK); // 1초당 클럭 틱 수

    double elapsed_seconds = (double)elapsed_ticks / clock_ticks; // 실행 시간(초 단위)

    // JSON 응답 생성
    const char* json_response =
        "{"
        "\"message\": \"Boom!\","
        "\"elapsed_time\": %.1f"
        "}";
    char response[256];
    snprintf(response, sizeof(response), json_response, elapsed_seconds);

    // 클라이언트에 응답
    mg_http_reply(data->conn, 200, CORS_HEADERS, "%s\n", response);

    // 메모리 해제
    free(data);
    printf("Timer thread completed. Elapsed time: %.1f seconds\n", elapsed_seconds);
    return NULL;
}

// 타이머 핸들러 함수
void handle_timer(struct mg_connection *conn, struct mg_http_message *hm) {
    timer_data_t *data = malloc(sizeof(timer_data_t));
    if (!data) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Memory allocation failed\"}\n");
        return;
    }

    // 시작 시간 기록
    data->start_ticks = times(&data->start_time);
    data->conn = conn;

    // 스레드 생성
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, timer_thread, data) != 0) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to create thread\"}\n");
        free(data);
        return;
    }

    // 스레드 분리
    pthread_detach(thread_id);

    // mg_http_reply(conn, 200, CORS_HEADERS, "{\"message\": \"Timer started\"}\n");
}
