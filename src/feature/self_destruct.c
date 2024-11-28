#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 단어 수 계산
int count_words(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }
    int word_count = 0;
    char ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == ' ' || ch == '\n') word_count++;
    }
    fclose(file);
    return word_count;
}

// 카운트다운 및 파일 삭제
void countdown_and_delete(const char *filename) {
    int word_count = count_words(filename);
    if (word_count < 0) return; // 파일 열기 오류 시 중단

    int countdown = word_count * 1; // 단어당 1초로 설정
    printf("File will self-destruct in %d seconds...\n", countdown);
    sleep(countdown);

    if (unlink(filename) == 0) {
        printf("Message file destroyed.\n");
    } else {
        perror("Error deleting file");
    }
}
