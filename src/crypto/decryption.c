#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decryption.h"
#include "utility.h"
#include "aes.h"
#include "steganography.h"
#include "self_destruct.h"

void decryption() {
    unsigned char key[AES_KEY_SIZE];              // AES 키
    unsigned char decrypted[128];                 // 복호화된 메시지 저장 공간
    unsigned char plaintext[128];                 // 복구된 메시지
    unsigned char iv[AES_BLOCK_SIZE];             // IV

    // 사용자로부터 복호화 키 입력받기
    printf("Enter AES decryption key (16 characters): ");
    fgets((char *)key, sizeof(key), stdin);
    key[strcspn((char *)key, "\n")] = '\0';  // 개행 문자 제거

    // JPEG에서 숨겨진 메시지 복구
    extract_message_from_jpeg("output.jpg", "recovered_message.txt");

    // 복구한 메시지 파일과 IV 파일 열기
    FILE *recovered_file = fopen("recovered_message.txt", "rb");
    FILE *iv_file = fopen("iv.txt", "rb");
    if (!recovered_file || !iv_file) {
        perror("Failed to open recovered message or IV file");
        return;
    }

    // 암호화된 메시지와 IV 읽기
    fread(decrypted, sizeof(unsigned char), sizeof(decrypted), recovered_file);
    fread(iv, sizeof(unsigned char), AES_BLOCK_SIZE, iv_file);
    fclose(recovered_file);
    fclose(iv_file);

    // AES 복호화
    aes_decrypt(decrypted, key, iv, plaintext, sizeof(decrypted));
    printf("Recovered message: %s\n", plaintext);

    // 원본 파일 자동 열기
    open_recovered_file("recovered_message.txt");

    // 복구된 파일 카운트다운 후 삭제
    countdown_and_delete("recovered_message.txt");
}
