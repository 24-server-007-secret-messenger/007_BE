#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <mysql/mysql.h>
#include <openssl/rand.h>
#include "encryption.h"
#include "aes.h"
#include "steganography.h"
#include "mongoose.h"
#include "db.h"

// CORS 설정
#define CORS_HEADERS "Access-Control-Allow-Origin: *\r\n" \
                     "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n" \
                     "Access-Control-Allow-Headers: Content-Type\r\n"

// 랜덤 이미지 선택 함수
const char* select_random_image(const char *directory) {
    struct dirent *entry;
    char *file_list[100];
    int count = 0;

    DIR *dir = opendir(directory);
    if (!dir) {
        perror("Failed to open image directory");
        return NULL;
    }

    // 디렉토리에서 JPEG 파일 수집
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".jpeg")) {
            file_list[count] = malloc(strlen(directory) + strlen(entry->d_name) + 2);
            sprintf(file_list[count], "%s/%s", directory, entry->d_name);
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        fprintf(stderr, "No image files found in directory: %s\n", directory);
        return NULL;
    }

    // 랜덤 선택
    srand((unsigned int)time(NULL));
    int random_index = rand() % count;

    const char *selected_image = strdup(file_list[random_index]);

    // 메모리 해제
    for (int i = 0; i < count; i++) {
        free(file_list[i]);
    }

    return selected_image;
}

// 암호화 API
void handle_encrypt_message(struct mg_connection *conn, struct mg_http_message *hm) {
    char *from = mg_json_get_str(hm->body, "$.from");
    char *to = mg_json_get_str(hm->body, "$.to");
    char *secret_message = mg_json_get_str(hm->body, "$.message");
    char *key = mg_json_get_str(hm->body, "$.key");

    if (!from || !to || !secret_message || !key) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid input\"}\n");
        return;
    }

    unsigned char encrypted[128], iv[AES_BLOCK_SIZE];
    int encrypted_len;

    // IV 생성
    if (!RAND_bytes(iv, AES_BLOCK_SIZE)) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to generate IV\"}\n");
        return;
    }

    // AES 암호화
    aes_encrypt((unsigned char *)secret_message, (unsigned char *)key, iv, encrypted, &encrypted_len);

    // 임시 파일에 암호화된 데이터와 IV 저장
    FILE *encrypted_file = fopen("tmp/encrypted_message.txt", "wb");
    fwrite(encrypted, 1, encrypted_len, encrypted_file);
    fwrite(iv, 1, AES_BLOCK_SIZE, encrypted_file);
    fclose(encrypted_file);

    // 랜덤 이미지 선택
    const char *input_directory = "image/input";
    const char *input_image = select_random_image(input_directory);
    if (!input_image) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"No valid image files found\"}\n");
        return;
    }

    // 출력 경로 설정
    const char *output_directory = "image/output";
    const char *output_image = "image/output/encrypted_image.jpg";

    // Steganography 적용
    hide_message_in_jpeg(input_image, output_image, "tmp/encrypted_message.txt");

    // 암호화된 이미지를 데이터베이스에 저장
    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed\"}\n");
        return;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO chat_message (chat_room_id, sender, message, sent_at) "
             "VALUES ((SELECT id FROM chat_room WHERE (user1='%s' AND user2='%s') OR (user1='%s' AND user2='%s')), '%s', LOAD_FILE('%s'), NOW())",
             from, to, to, from, from, output_image);

    if (mysql_query(db_conn, query)) {
        mysql_close(db_conn);
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to save chat message\"}\n");
        return;
    }

    int chat_message_id = mysql_insert_id(db_conn);
    mysql_close(db_conn);

    mg_http_reply(conn, 200, CORS_HEADERS, "{\"chat_message_id\": %d, \"file\": \"%s\"}\n", chat_message_id, output_image);

    free((void *)input_image);  // strdup으로 할당된 메모리 해제
}
