#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <mysql/mysql.h>
#include "mongoose.h"

void generate_aes_key_and_iv();
void get_aes_key_and_iv(struct mg_connection *conn, struct mg_http_message *hm);
const char* select_random_image(const char *directory);
char *handle_encrypt_message(struct mg_connection *conn, const char *from, const char *to, const char *input_message, const char *key, int chat_room_id);

#endif // ENCRYPTION_H
