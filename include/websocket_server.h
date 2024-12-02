#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <mysql/mysql.h>
#include "mongoose.h"

void process_message(struct mg_connection *conn, struct mg_ws_message *wm);
void save_message(MYSQL *conn, int chat_room_id, const char *sender, const char *message, bool encrypt);
void websocket_handler(struct mg_connection *conn, int ev, void *ev_data);
void start_websocket_server();

#endif // WEBSOCKET_SERVER_H
