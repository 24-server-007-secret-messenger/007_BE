
#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

void handle_client(int client_sock);
void *client_handler_thread(void *arg);
void start_tcp_server();

#endif // SOCKET_SERVER_H
