#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <uuid/uuid.h>
#include <openssl/rand.h>
#include <mysql/mysql.h>
#include "encryption.h"
#include "aes.h"
#include "steganography.h"
#include "mongoose.h"
#include "db.h"
#include "base64.h"

// CORS 설정
#define CORS_HEADERS "Access-Control-Allow-Origin: *\r\n" \
                     "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n" \
                     "Access-Control-Allow-Headers: Content-Type\r\n"

// AES Key와 IV를 생성하는 함수
void generate_aes_key_and_iv(unsigned char *aes_key, unsigned char *aes_iv) {
    if (RAND_bytes(aes_key, sizeof(aes_key)) != 1) {
        fprintf(stderr, "Failed to generate AES key\n");
        exit(EXIT_FAILURE);
    }
    if (RAND_bytes(aes_iv, sizeof(aes_iv)) != 1) {
        fprintf(stderr, "Failed to generate AES IV\n");
        exit(EXIT_FAILURE);
    }

    printf("AES key: ");
    for (size_t i = 0; i < sizeof(aes_key); i++) {
        printf("%02x", aes_key[i]);
    }
    printf("\n");

    printf("AES IV: ");
    for (size_t i = 0; i < sizeof(aes_iv); i++) {
        printf("%02x", aes_iv[i]);
    }
    printf("\n");
}

// 환경 변수 설정 함수
void set_file_path_env_vars() {
    setenv("IMAGE_INPUT_DIR", "assets/input", 1);
    setenv("IMAGE_OUTPUT_DIR", "assets/output/image", 1);
    setenv("TEXT_OUTPUT_DIR", "assets/output/txt", 1);
}

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

    // 랜덤 선택Í
    srand((unsigned int)time(NULL));
    int random_index = rand() % count;

    // 선택된 이미지 복사
    const char *selected_image = strdup(file_list[random_index]);

    // 메모리 해제
    for (int i = 0; i < count; i++) {
        free(file_list[i]);
    }

    return selected_image;
}

// 암호화 메시지 처리 함수
char *handle_encrypt_message(struct mg_connection *conn, const char *from, const char *to, const char *input_message, const char *key, int chat_room_id) {
    printf("From: %s, To: %s, Message: %s, Key: %s\n", from, to, input_message, key);

    // 환경 변수 설정
    set_file_path_env_vars();

    const char *input_directory = getenv("IMAGE_INPUT_DIR");
    const char *output_image_directory = getenv("IMAGE_OUTPUT_DIR");
    const char *output_text_directory = getenv("TEXT_OUTPUT_DIR");

    unsigned char aes_key[16];
    unsigned char aes_iv[AES_BLOCK_SIZE];
    unsigned char aes_message[128];
    int aes_len;

    // AES 키와 IV 생성
    generate_aes_key_and_iv(aes_key, aes_iv);

    // AES 암호화
    aes_encrypt((unsigned char *)input_message, aes_key, aes_iv, aes_message, &aes_len);
    printf("AES Encrypted Message: %s\n", aes_message);

    // AES 키와 IV를 Base64로 인코딩
    char *aes_key_base64 = base64_encode(aes_key, sizeof(aes_key));
    char *aes_iv_base64 = base64_encode(aes_iv, sizeof(aes_iv));

    if (!aes_key_base64 || !aes_iv_base64) {
        fprintf(stderr, "Failed to encode AES key or IV to Base64\n");
        return NULL;
    }

    // 랜덤 이미지 선택
    const char *input_image = select_random_image(input_directory);
    if (!input_image) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"No valid image files found\"}\n");
        return NULL;
    }
    printf("Selected input image: %s\n", input_image);

    // input 파일 이름 추출
    const char *file_name_ptr = strrchr(input_image, '/');
    if (file_name_ptr) {
        file_name_ptr++; // '/' 이후의 파일명
    } else {
        file_name_ptr = input_image; // 전체가 파일명
    }

    // input_image_name에 파일 이름 복사
    char *input_image_name = strdup(file_name_ptr);
    if (!input_image_name) {
        fprintf(stderr, "Failed to allocate memory for input_image_name\n");
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Memory allocation failed\"}\n");
        return NULL;
    }

    // 확장자 제거
    char *dot = strrchr(input_image_name, '.'); // '.'의 위치 찾기
    if (dot && (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)) {
        *dot = '\0'; // 문자열을 확장자 이전까지만 유지
    }
    printf("Input image name: %s\n", input_image_name);

    // 스테가노그래피용 출력 이미지 이름 생성
    uuid_t uuid;
    char uuid_str[37];  // UUID는 36자 + null 문자
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_str);
    printf("UUID: %s\n", uuid_str);

    // NULL인지 확인
    if (strlen(uuid_str) == 0) {
        fprintf(stderr, "Failed to generate UUID string\n");
        return NULL;
    }

    // 확장자를 제거한 output_image_name 생성
    char *output_image_name = malloc(128); // 확장자를 제거한 이름만 저장
    if (!output_image_name) {
        fprintf(stderr, "Failed to allocate memory for output image name\n");
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Memory allocation failed\"}\n");
        return NULL;
    }

    snprintf(output_image_name, 128, "%s_%s", input_image_name, uuid_str); // 이름 생성
    printf("Output image name: %s\n", output_image_name);

    // 전체 경로와 확장자를 포함한 output_image 생성
    char output_image[512];
    snprintf(output_image, sizeof(output_image), "%s/%s.jpeg", output_image_directory, output_image_name);
    printf("Output image: %s\n", output_image);

    // 메시지를 이미지에 숨김
    if (embed_message_in_image(input_image, output_image, (const char *)aes_message) != 0) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to hide message in image\"}\n");
        free(output_image_name); // 메모리 해제
        free((void *)input_image);
        return NULL;
    }
    printf("Stego image: %s\n", output_image);

    // Base64로 변환
    char *base64_image = encode_file_to_base64(output_image);
    // printf("Base64 image: %s\n", base64_image);

    if (!base64_image) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to convert image to Base64\"}\n");
        free((void *)input_image);
        return NULL;
    }

    // Base64 이미지를 같은 이름의 .txt 파일로 저장
    // Base64 이미지를 같은 이름의 .txt 파일로 저장
    char txt_file_path[512];
    snprintf(txt_file_path, sizeof(txt_file_path), "%s/%s.txt", output_text_directory, output_image_name);

    // 파일 열기
    FILE *txt_file = fopen(txt_file_path, "w");
    if (!txt_file) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to create text file\"}\n");
        free(base64_image);
        free((void *)input_image);
        free(output_image_name); // 메모리 해제
        return NULL;
    }

    // Base64 데이터를 파일에 기록
    fprintf(txt_file, "data:image/jpeg;base64,%s", base64_image);
    fclose(txt_file);

    printf("Base64 data saved to txt file path: %s\n", txt_file_path);

    // 데이터베이스에 저장
    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed\"}\n");
        free(base64_image);
        free((void *)input_image);
        return NULL;
    }

    char query[1024];
    snprintf(query, sizeof(query),
             "INSERT INTO secret_message (chat_room_id, sender, original_message, encryption_key, aes_key, aes_iv, aes_len, input_image_name, output_image_name, sent_at) "
             "VALUES (%d, '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', NOW())",
             chat_room_id, from, input_message, key, aes_key_base64, aes_iv_base64, aes_len, input_image_name, output_image_name);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "Database query failed: %s\n", mysql_error(db_conn));
        mysql_close(db_conn);
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to save secret message\"}\n");
        free(base64_image);
        free((void *)input_image);
        return NULL;
    }

    mysql_close(db_conn);

    free(base64_image);
    free((void *)input_image);

    return output_image_name;
}
