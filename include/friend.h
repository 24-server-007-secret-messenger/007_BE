#ifndef FRIEND_H
#define FRIEND_H

#include "mongoose.h"

void handle_friend_request(struct mg_connection *conn, struct mg_http_message *hm);
void handle_friend_accept(struct mg_connection *conn, struct mg_http_message *hm);
void handle_friend_reject(struct mg_connection *conn, struct mg_http_message *hm);

#endif // FRIEND_H
