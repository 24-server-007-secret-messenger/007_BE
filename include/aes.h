#ifndef AES_H
#define AES_H

#define AES_KEY_SIZE 16   // AES-128에서 키 크기는 16바이트
#define AES_BLOCK_SIZE 16 // AES 블록 크기는 16바이트

// AES 암호화 및 복호화 함수 원형
void aes_encrypt(const unsigned char *input, const unsigned char *key, const unsigned char *iv, unsigned char *output, int *output_len);
void aes_decrypt(const unsigned char *input, unsigned char *output, const unsigned char *aes_key, const unsigned char *aes_iv, int aes_len);

#endif // AES_H
