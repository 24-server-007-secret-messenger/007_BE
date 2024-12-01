#include <openssl/evp.h>
#include <string.h>
#include "aes.h"

// AES CBC 모드 암호화
void aes_encrypt(const unsigned char *input, const unsigned char *key, const unsigned char *iv, unsigned char *output, int *output_len) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len;

    // 암호화 초기화
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
    // 암호화 업데이트
    EVP_EncryptUpdate(ctx, output, &len, input, strlen((char *)input));
    *output_len = len;

    // 암호화 완료
    EVP_EncryptFinal_ex(ctx, output + len, &len);
    *output_len += len;

    EVP_CIPHER_CTX_free(ctx);
}

// AES CBC 모드 복호화
void aes_decrypt(const unsigned char *input, const unsigned char *key, const unsigned char *iv, unsigned char *output, int input_len) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len, plaintext_len;

    // 복호화 초기화
    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
    // 복호화 업데이트
    EVP_DecryptUpdate(ctx, output, &len, input, input_len);
    plaintext_len = len;

    // 복호화 완료
    EVP_DecryptFinal_ex(ctx, output + len, &len);
    plaintext_len += len;

    // NULL 문자를 추가해 문자열로 만든다
    output[plaintext_len] = '\0';

    EVP_CIPHER_CTX_free(ctx);
}
