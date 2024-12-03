#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 9001

void handle_client(int client_sock) {
    char buffer[1024];
    int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("Failed to receive data");
        close(client_sock);
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Received from client: %s\n", buffer);

    // 응답 작성 (간단히 메시지 리턴)
    send(client_sock, "Processed", strlen("Processed"), 0);

    close(client_sock);
}

// 클라이언트 연결을 처리하는 쓰레드 함수
void *client_handler_thread(void *arg) {
    int client_sock = *(int *)arg;
    free(arg); // 동적으로 할당된 메모리 해제

    handle_client(client_sock);
    return NULL;
}

// TCP 서버 실행 함수
void start_tcp_server() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓 바인딩
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 클라이언트 연결 대기
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("TCP Server listening on port %d\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New client connected\n");

        // 클라이언트 처리를 위한 새로운 쓰레드 생성
        pthread_t client_thread;
        int *client_sock_ptr = malloc(sizeof(int)); // 동적으로 메모리 할당
        if (client_sock_ptr == NULL) {
            perror("Memory allocation failed");
            close(client_sock);
            continue;
        }
        *client_sock_ptr = client_sock;

        if (pthread_create(&client_thread, NULL, client_handler_thread, client_sock_ptr) != 0) {
            perror("Failed to create client thread");
            free(client_sock_ptr);
            close(client_sock);
        } else {
            pthread_detach(client_thread); // 쓰레드 리소스를 자동으로 회수
        }
    }

    close(server_sock);
}
