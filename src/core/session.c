#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "session.h"

#define MAX_ACTIVE_SESSIONS 100

typedef struct {
    char *username;
    ssh_session session;
} ActiveSession;

static ActiveSession active_sessions[MAX_ACTIVE_SESSIONS];

// 세션 추가
void add_session(const char *username, ssh_session session) {
    for (int i = 0; i < MAX_ACTIVE_SESSIONS; i++) {
        if (active_sessions[i].username == NULL) {
            active_sessions[i].username = strdup(username);
            active_sessions[i].session = session;
            printf("Session added for user: %s\n", username);
            return;
        }
    }
    fprintf(stderr, "Failed to add session: Max sessions reached.\n");
}

// 세션 제거
void remove_session(const char *username) {
    for (int i = 0; i < MAX_ACTIVE_SESSIONS; i++) {
        if (active_sessions[i].username && strcmp(active_sessions[i].username, username) == 0) {
            ssh_disconnect(active_sessions[i].session);
            ssh_free(active_sessions[i].session);
            free(active_sessions[i].username);
            active_sessions[i].username = NULL;
            printf("Session removed for user: %s\n", username);
            return;
        }
    }
    fprintf(stderr, "Failed to remove session: User not found.\n");
}

// 세션 반환
ssh_session get_session(const char *username) {
    for (int i = 0; i < MAX_ACTIVE_SESSIONS; i++) {
        if (active_sessions[i].username && strcmp(active_sessions[i].username, username) == 0) {
            return active_sessions[i].session;
        }
    }
    return NULL;
}
