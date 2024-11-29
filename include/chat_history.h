#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <mysql/mysql.h>

void handle_chat_history(struct mg_connection *conn, struct mg_http_message *hm);
int get_chat_room_id(MYSQL *conn, const char *user1, const char *user2);
void get_chat_history(MYSQL *conn, int chat_room_id, struct mg_connection *conn_ws);

#endif // CHAT_MESSAGE_H
