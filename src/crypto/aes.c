#include <openssl/evp.h>
#include <string.h>
#include "aes.h"

// AES CBC 모드 암호화
void aes_encrypt(const unsigned char *input, const unsigned char *key, const unsigned char *iv, unsigned char *output, int *output_len) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len;
    int plaintext_len = strlen((char *)input); // 입력 데이터 길이 계산

    // 암호화 초기화
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1) {
        fprintf(stderr, "Error initializing encryption context\n");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    // 암호화 업데이트
    if (EVP_EncryptUpdate(ctx, output, &len, input, plaintext_len) != 1) {
        fprintf(stderr, "Error during encryption update\n");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    *output_len = len;

    // 암호화 완료
    if (EVP_EncryptFinal_ex(ctx, output + len, &len) != 1) {
        fprintf(stderr, "Error during encryption finalization\n");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    EVP_CIPHER_CTX_free(ctx);
}

// AES CBC 모드 복호화
void aes_decrypt(const unsigned char *input, unsigned char *output, const unsigned char *aes_key, const unsigned char *aes_iv, int aes_len) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len, plaintext_len;

    if (!ctx) {
        fprintf(stderr, "Error creating decryption context\n");
        return;
    }

    // 복호화 초기화
    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1) {
        fprintf(stderr, "Error initializing decryption context\n");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    // 복호화 업데이트
    if (EVP_DecryptUpdate(ctx, output, &len, input, input_len) != 1) {
        fprintf(stderr, "Error during decryption update\n");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    plaintext_len = len;

    // 복호화 완료
    if (EVP_DecryptFinal_ex(ctx, output + len, &len) != 1) {
        fprintf(stderr, "Error during decryption finalization\n");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    plaintext_len += len;

    // NULL 문자를 추가해 문자열로 만든다
    output[plaintext_len] = '\0';

    EVP_CIPHER_CTX_free(ctx);
}
