#ifndef CHAT_ROOM_H
#define CHAT_ROOM_H

#include <mysql/mysql.h>

void handle_chat_start(struct mg_connection *conn, struct mg_http_message *hm);
int create_or_get_chat_room(MYSQL *conn, const char *user1, const char *user2);
void handle_chat_room_list(struct mg_connection *conn, struct mg_http_message *hm);

#endif // CHAT_ROOM_H
