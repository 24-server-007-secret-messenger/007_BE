#ifndef AUTH_H
#define AUTH_H

#include "mongoose.h"

void handle_login(struct mg_connection *conn, struct mg_http_message *hm);
void handle_logout(struct mg_connection *conn, struct mg_http_message *hm);

#endif // AUTH_H
