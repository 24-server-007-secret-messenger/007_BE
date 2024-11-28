#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <libssh/libssh.h>

void add_session(const char *username, ssh_session session);
void remove_session(const char *username);
ssh_session get_session(const char *username);

#endif // SESSION_MANAGER_H
