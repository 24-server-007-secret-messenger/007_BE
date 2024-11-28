#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"

// 사용자로부터 키와 메시지를 입력받는 함수
void get_user_input(char *key, int key_size, char *message, int message_size) {
    printf("Enter AES encryption key (16 characters): ");
    fgets(key, key_size, stdin);
    key[strcspn(key, "\n")] = '\0';  // 입력 끝의 개행 문자 제거

    printf("Enter message to encrypt: ");
    fgets(message, message_size, stdin);
    message[strcspn(message, "\n")] = '\0';  // 입력 끝의 개행 문자 제거
}

// 복호화 후 자동으로 파일 열기 (OS 호환성 포함)
void open_recovered_file(const char *filename) {
    char command[256];
    
    #ifdef __APPLE__
        snprintf(command, sizeof(command), "open %s", filename); // MacOS
    #elif __linux__
        snprintf(command, sizeof(command), "xdg-open %s", filename); // Linux
    #elif _WIN32
        snprintf(command, sizeof(command), "start %s", filename); // Windows
    #else
        fprintf(stderr, "Unsupported OS for automatic file opening.\n");
        return;
    #endif
    
    system(command);
}
