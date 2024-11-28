#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

void broadcast_message(const char *message, int exclude_sock);
void start_chat_server();

#endif // CHAT_SERVER_H
