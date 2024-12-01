#ifndef DECRYPTION_H
#define DECRYPTION_H

#include <mysql/mysql.h>
#include "mongoose.h"

void handle_decrypt_message(struct mg_connection *conn, struct mg_http_message *hm);

#endif // DECRYPTION_H
