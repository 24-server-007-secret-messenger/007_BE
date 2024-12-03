#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#define MAX_READ_SIZE 450000 // 최대 읽기 크기 정의

// Base64 인코딩
char* base64_encode(const unsigned char* buffer, size_t length) {
    BIO *bio, *b64;
    BUF_MEM *buffer_ptr;

    // Base64 인코딩
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // 개행 없이 Base64 생성
    bio = BIO_push(b64, bio);

    // 인코딩된 데이터를 메모리에 저장
    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);

    // 메모리 할당 및 복사
    char* b64text = malloc(buffer_ptr->length + 1);
    if (!b64text) {
        fprintf(stderr, "Failed to allocate memory for base64 encode\n");
        BIO_free_all(bio);
        return NULL;
    }

    // NULL 문자 추가
    memcpy(b64text, buffer_ptr->data, buffer_ptr->length);
    b64text[buffer_ptr->length] = '\0';

    BIO_free_all(bio);

    return b64text;
}

// Base64 디코딩
size_t base64_decode(const char* base64text, unsigned char* buffer) {
    BIO *bio, *b64;
    size_t length = strlen(base64text);

    // Base64 디코딩
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(base64text, -1);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // 개행 처리 없음
    bio = BIO_push(b64, bio);

    // 디코딩된 데이터를 버퍼에 저장
    size_t decoded_length = BIO_read(bio, buffer, length);
    if (decoded_length <= 0) {
        fprintf(stderr, "Failed to decode base64 string\n");
        BIO_free_all(bio);
        return 0;
    }

    BIO_free_all(bio);
    return decoded_length;
}

// 파일을 읽어 Base64로 인코딩
char* encode_file_to_base64(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    // 파일 크기 계산
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file); // 파일 포인터를 처음으로 이동

    // 파일 크기가 MAX_READ_SIZE를 초과하면 MAX_READ_SIZE만 읽음
    size_t read_size = file_size > MAX_READ_SIZE ? MAX_READ_SIZE : file_size;

    // 파일 내용을 읽어 메모리에 저장
    unsigned char* buffer = malloc(read_size);
    if (!buffer) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // 파일 읽기
    fread(buffer, 1, read_size, file);
    fclose(file);

    // Base64 인코딩
    char* b64text = base64_encode(buffer, read_size);
    free(buffer);

    return b64text;
}
