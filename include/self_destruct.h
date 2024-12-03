#ifndef SELF_DESTRUCT_H
#define SELF_DESTRUCT_H

void* timer_thread(void* arg);
void handle_timer(struct mg_connection *conn, struct mg_http_message *hm);

#endif // SELF_DESTRUCT_H
