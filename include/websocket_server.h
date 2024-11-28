#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include "mongoose.h"

void websocket_handler(struct mg_connection *conn, int ev, void *ev_data);
void start_websocket_server();

#endif // WEBSOCKET_SERVER_H
