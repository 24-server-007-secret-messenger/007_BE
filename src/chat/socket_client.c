#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define TCP_SERVER_IP "0.0.0.0"
#define TCP_SERVER_PORT 9001

char *send_tcp_request(const char *operation, const char *message, const char *key) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];
    char *response = NULL;

    // 소켓 생성]
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_SERVER_PORT);

    // 서버 주소 설정
    if (inet_pton(AF_INET, TCP_SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(sock);
        return NULL;
    }

    // 서버 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return NULL;
    }

    // TCP 요청 작성
    snprintf(buffer, sizeof(buffer), "{\"operation\":\"%s\",\"message\":\"%s\",\"key\":\"%s\"}", operation, message, key);
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("Failed to send data");
        close(sock);
        return NULL;
    }

    // TCP 응답 수신
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        response = strdup(buffer);
    } else {
        perror("Failed to receive response");
    }

    close(sock);
    return response;
}
