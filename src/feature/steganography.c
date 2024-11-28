#include <stdio.h>
#include <stdlib.h>

// JPEG 이미지에 암호화된 파일 숨기기
void hide_message_in_jpeg(const char *input_jpeg, const char *output_jpeg, const char *file_to_hide) {
    char command[512];
    snprintf(command, sizeof(command), "./jphs/jphide %s %s %s", input_jpeg, output_jpeg, file_to_hide);
    int result = system(command);
    if (result == 0) {
        printf("Message hidden successfully in JPEG.\n");
    } else {
        perror("Failed to hide message in JPEG");
    }
}

// JPEG 이미지에서 숨겨진 파일 추출하기
void extract_message_from_jpeg(const char *input_jpeg, const char *output_file) {
    char command[512];
    snprintf(command, sizeof(command), "./jphs/jpseek %s %s", input_jpeg, output_file);
    int result = system(command);
    if (result == 0) {
        printf("Message extracted successfully from JPEG.\n");
    } else {
        perror("Failed to extract message from JPEG");
    }
}