#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mongoose.h"
#include "decryption.h"
#include "aes.h"
#include "steganography.h"
#include "base64.h"
#include "db.h"

// CORS 설정
#define CORS_HEADERS "Access-Control-Allow-Origin: *\r\n" \
                     "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n" \
                     "Access-Control-Allow-Headers: Content-Type\r\n"

void handle_decrypt_message(struct mg_connection *conn, struct mg_http_message *hm) {
    char *sender = mg_json_get_str(hm->body, "$.sender");
    char *sent_at = mg_json_get_str(hm->body, "$.sent_at");
    char *key = mg_json_get_str(hm->body, "$.key");

    if (!sender || !sent_at || !key) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid input\"}\n");
        return;
    }
    printf("Sender: %s, Sent At: %s, Key: %s\n", sender, sent_at, key);

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed\"}\n");
        return;
    }

    // 쿼리 수정
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT encryption_key, output_image_name, aes_key, aes_iv, aes_len FROM secret_message "
             "WHERE sender='%s' "
             "AND sent_at = STR_TO_DATE('%s', '%%Y-%%m-%%d %%H:%%i:%%s')",
             sender, sent_at);
    // printf("Query: %s\n", query);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "MySQL Error: %s\n", mysql_error(db_conn));
        mysql_close(db_conn);
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to fetch secret message\"}\n");
        return;
    }

    MYSQL_RES *result = mysql_store_result(db_conn);
    if (!result || mysql_num_rows(result) == 0) {
        printf("Error: No result from query. MySQL Error: %s\n", mysql_error(db_conn));
        mysql_free_result(result);
        mysql_close(db_conn);
        mg_http_reply(conn, 404, CORS_HEADERS, "{\"error\": \"Message not found\"}\n");
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    const char *db_key = row[0];
    const char *output_image_name = row[1];
    char *aes_key_base64 = row[2];
    char *aes_iv_base64 = row[3];
    int aes_len = atoi(row[4]);

    if (strcmp(db_key, key) != 0) {
        mysql_free_result(result);
        mysql_close(db_conn);
        mg_http_reply(conn, 403, CORS_HEADERS, "{\"error\": \"Key does not match\"}\n");
        return;
    }
    printf("Key: %s, Output Image Name: %s\n", db_key, output_image_name);

    mysql_free_result(result);
    mysql_close(db_conn);

    // Base64 디코딩
    unsigned char aes_key[16];
    unsigned char aes_iv[AES_BLOCK_SIZE];
    base64_decode(aes_key_base64, aes_key);
    base64_decode(aes_iv_base64, aes_iv);

    // 이미지 경로 생성
    char output_image[256];
    snprintf(output_image, sizeof(output_image), "assets/output/image/%s.jpg", output_image_name);
    printf("Key: %s, Output Image Path: %s\n", db_key, output_image);

    // Steganography 복호화
    char hidden_message[128] = {0};
    if (extract_message_from_image(output_image, hidden_message, sizeof(hidden_message)) != 0) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to extract message from image\"}\n");
        return;
    }
    printf("Hidden message: %s\n", hidden_message);\

    // AES 복호화
    unsigned char decrypted_message[128] = {0};
    aes_decrypt((unsigned char *)hidden_message, decrypted_message, aes_key, aes_iv, aes_len);

    // 결과 반환
    mg_http_reply(conn, 200, CORS_HEADERS,
                  "{\"secret\": \"%s\"}\n",
                  decrypted_message);
}
