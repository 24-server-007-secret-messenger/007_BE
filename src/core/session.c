#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "session.h"

#define MAX_ACTIVE_SESSIONS 100

typedef struct {
    char *username;
    ssh_session session;
} ActiveSession;

static ActiveSession active_sessions[MAX_ACTIVE_SESSIONS];
static pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

// 세션 추가
void add_session(const char *username, ssh_session session) {
    if (session == NULL) {
        fprintf(stderr, "Failed to add session: Invalid SSH session.\n");
        return;
    }

    pthread_mutex_lock(&session_mutex);

    for (int i = 0; i < MAX_ACTIVE_SESSIONS; i++) {
        if (active_sessions[i].username == NULL) {
            active_sessions[i].username = strdup(username);
            if (active_sessions[i].username == NULL) {
                fprintf(stderr, "Failed to add session: Memory allocation error.\n");
                pthread_mutex_unlock(&session_mutex);
                return;
            }
            active_sessions[i].session = session;
            printf("Session added for user: %s\n", username);
            pthread_mutex_unlock(&session_mutex);
            return;
        }
    }

    fprintf(stderr, "Failed to add session: Max sessions reached.\n");
    pthread_mutex_unlock(&session_mutex);
}

// 세션 제거
void remove_session(const char *username) {
    if (username == NULL) {
        fprintf(stderr, "Failed to remove session: Invalid username.\n");
        return;
    }

    pthread_mutex_lock(&session_mutex);

    for (int i = 0; i < MAX_ACTIVE_SESSIONS; i++) {
        if (active_sessions[i].username && strcmp(active_sessions[i].username, username) == 0) {
            ssh_disconnect(active_sessions[i].session);
            ssh_free(active_sessions[i].session);
            free(active_sessions[i].username);
            active_sessions[i].username = NULL;
            printf("Session removed for user: %s\n", username);
            pthread_mutex_unlock(&session_mutex);
            return;
        }
    }

    fprintf(stderr, "Failed to remove session: User not found.\n");
    pthread_mutex_unlock(&session_mutex);
}

// 세션 반환
ssh_session get_session(const char *username) {
    ssh_session session = NULL;

    pthread_mutex_lock(&session_mutex);

    for (int i = 0; i < MAX_ACTIVE_SESSIONS; i++) {
        if (active_sessions[i].username && strcmp(active_sessions[i].username, username) == 0) {
            session = active_sessions[i].session;
            break;
        }
    }

    pthread_mutex_unlock(&session_mutex);
    return session;
}
