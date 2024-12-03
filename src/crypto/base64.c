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

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);

    char* b64text = malloc(buffer_ptr->length + 1);
    if (!b64text) {
        fprintf(stderr, "Failed to allocate memory for base64 encode\n");
        BIO_free_all(bio);
        return NULL;
    }

    memcpy(b64text, buffer_ptr->data, buffer_ptr->length);
    b64text[buffer_ptr->length] = '\0';

    BIO_free_all(bio);

    return b64text;
}

char* encode_file_to_base64(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    // 파일 크기가 MAX_READ_SIZE를 초과하면 MAX_READ_SIZE만 읽음
    size_t read_size = file_size > MAX_READ_SIZE ? MAX_READ_SIZE : file_size;

    unsigned char* buffer = malloc(read_size);
    if (!buffer) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, read_size, file);
    fclose(file);

    char* b64text = base64_encode(buffer, read_size);
    free(buffer);

    return b64text;
}
