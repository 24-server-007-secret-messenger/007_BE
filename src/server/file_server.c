// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include "file_server.h"

// #define PORT 8081
// #define BUFFER_SIZE 1024

// void receive_file(int client_socket) {
//     FILE *file = fopen("received_input.jpg", "wb");
//     if (file == NULL) {
//         perror("Error opening file");
//         close(client_socket);
//         return;
//     }

//     char buffer[BUFFER_SIZE];
//     int bytes_received;

//     // 클라이언트로부터 데이터 수신 및 파일 저장
//     while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
//         fwrite(buffer, sizeof(char), bytes_received, file);
//     }

//     fclose(file);
//     close(client_socket);
//     printf("File received successfully and saved as 'received_input.jpg'.\n");
// }

// int start_file_server() {
//     int server_socket, client_socket;
//     struct sockaddr_in server_addr, client_addr;
//     socklen_t addr_len = sizeof(client_addr);

//     // 소켓 생성
//     server_socket = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_socket == -1) {
//         perror("Could not create socket");
//         return -1;
//     }

//     // 서버 주소 설정
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = INADDR_ANY;
//     server_addr.sin_port = htons(PORT);

//     // 소켓 바인딩
//     if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         perror("Bind failed");
//         close(server_socket);
//         return -1;
//     }

//     // 클라이언트 연결 대기
//     listen(server_socket, 5);
//     printf("File Server listening on port %d\n", PORT); // 8081

//     // 클라이언트 연결 수락 및 파일 수신
//     client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
//     if (client_socket < 0) {
//         perror("Accept failed");
//         close(server_socket);
//         return -1;
//     }

//     printf("Client connected.\n");
//     receive_file(client_socket);  // 파일 수신

//     close(server_socket);
//     return 0;
// }
