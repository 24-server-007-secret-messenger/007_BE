#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

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

    unsigned char* buffer = malloc(file_size);
    fread(buffer, 1, file_size, file);
    fclose(file);

    char* b64text = base64_encode(buffer, file_size);
    free(buffer);

    return b64text;
}
