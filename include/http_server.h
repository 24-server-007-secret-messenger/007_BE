#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "mongoose.h"

void handle_request(struct mg_connection *conn, int ev, void *ev_data);
void start_http_server();

#endif // HTTP_SERVER_H
