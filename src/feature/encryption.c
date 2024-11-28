#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rand.h>
#include "encryption.h"
#include "utility.h"
#include "aes.h"
#include "steganography.h"

void encryption() {
    unsigned char key[AES_KEY_SIZE];              // AES 키
    unsigned char plaintext[128];                 // 사용자 입력 메시지
    unsigned char encrypted[128];                 // 암호화된 메시지
    unsigned char iv[AES_BLOCK_SIZE];             // IV
    int encrypted_len;

    // 사용자로부터 암호화 키와 메시지 입력받기
    get_user_input((char *)key, sizeof(key), (char *)plaintext, sizeof(plaintext));

    // IV 생성 (AES_BLOCK_SIZE 크기)
    if (!RAND_bytes(iv, AES_BLOCK_SIZE)) {
        perror("Error generating IV");
        return;
    }

    // AES 암호화
    aes_encrypt(plaintext, key, iv, encrypted, &encrypted_len);
    printf("Message encrypted.\n");

    // 암호화된 메시지와 IV를 각각 파일에 저장
    FILE *temp_file = fopen("encrypted_msg.txt", "wb");
    FILE *iv_file = fopen("iv.txt", "wb");
    if (!temp_file || !iv_file) {
        perror("Error opening file for encrypted message or IV");
        return;
    }
    fwrite(encrypted, sizeof(unsigned char), encrypted_len, temp_file);
    fwrite(iv, sizeof(unsigned char), AES_BLOCK_SIZE, iv_file);
    fclose(temp_file);
    fclose(iv_file);

    // JPEG 이미지에 암호화된 메시지 숨기기
    hide_message_in_jpeg("input.jpg", "output.jpg", "encrypted_msg.txt");
    printf("Encrypted message and IV saved. Encrypted message hidden in JPEG as output.jpg.\n");
}
