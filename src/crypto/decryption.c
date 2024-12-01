#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mongoose.h"
#include "decryption.h"
#include "aes.h"
#include "steganography.h"
#include "self_destruct.h"
#include "db.h"

// CORS 설정
#define CORS_HEADERS "Access-Control-Allow-Origin: *\r\n" \
                     "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n" \
                     "Access-Control-Allow-Headers: Content-Type\r\n"

void handle_decrypt_message(struct mg_connection *conn, struct mg_http_message *hm) {
    char *from = mg_json_get_str(hm->body, "$.from");
    char *to = mg_json_get_str(hm->body, "$.to");
    char *key = mg_json_get_str(hm->body, "$.key");
    int chat_message_id = 0;
    mg_json_get_long(hm->body, "$.chat_message_id", chat_message_id);

    if (!from || !to || !key || chat_message_id <= 0) {
        mg_http_reply(conn, 400, CORS_HEADERS, "{\"error\": \"Invalid input\"}\n");
        return;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Database connection failed\"}\n");
        return;
    }

    // 데이터베이스에서 암호화된 이미지 가져오기
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT message FROM chat_message WHERE id=%d", chat_message_id);

    if (mysql_query(db_conn, query)) {
        mysql_close(db_conn);
        mg_http_reply(conn, 500, CORS_HEADERS, "{\"error\": \"Failed to fetch chat message\"}\n");
        return;
    }

    MYSQL_RES *result = mysql_store_result(db_conn);
    if (!result || mysql_num_rows(result) == 0) {
        mysql_free_result(result);
        mysql_close(db_conn);
        mg_http_reply(conn, 404, CORS_HEADERS, "{\"error\": \"Message not found\"}\n");
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    const char *encrypted_image_path = row[0];
    mysql_free_result(result);
    mysql_close(db_conn);

    // Steganography로 암호화된 메시지 추출
    const char *output_file = "tmp/recovered_message.txt";
    extract_message_from_jpeg(encrypted_image_path, output_file);

    // 암호화된 메시지와 IV 읽기
    FILE *file = fopen(output_file, "rb");
    unsigned char encrypted[128], iv[AES_BLOCK_SIZE];
    fread(encrypted, 1, sizeof(encrypted), file);
    fread(iv, 1, AES_BLOCK_SIZE, file);
    fclose(file);

    // AES 복호화
    unsigned char decrypted[128];
    aes_decrypt(encrypted, (unsigned char *)key, iv, decrypted, sizeof(encrypted));

    mg_http_reply(conn, 200, CORS_HEADERS, "{\"message\": \"%s\"}\n", decrypted);
}
