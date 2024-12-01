#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <mysql/mysql.h>
#include "mongoose.h"

void handle_encrypt_message(struct mg_connection *conn, struct mg_http_message *hm);

#endif // ENCRYPTION_H
