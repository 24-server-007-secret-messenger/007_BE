#include <libssh/libssh.h>
#include <stdio.h>
#include "ssh_connection.h"

#define SSH_HOST "swist2.cbnu.ac.kr"
#define SSH_PORT 221

// SSH 연결 함수
ssh_session ssh_connection(const char *username, const char *password) {
    ssh_session session = ssh_new();
    // SSH 세션 생성 실패
    if (session == NULL) {
        return NULL;
    }

    int port = SSH_PORT;
    ssh_options_set(session, SSH_OPTIONS_HOST, SSH_HOST);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_USER, username);

    // SSH 연결
    int rc = ssh_connect(session);
    // SSH 연결 실패 시 에러 코드 반환
    if (rc != SSH_OK) {
        fprintf(stderr, "SSH connection failed: %s\n", ssh_get_error(session));
        ssh_free(session);
        return NULL;
    }

    // 비밀번호 인증
    rc = ssh_userauth_password(session, NULL, password);
    // 비밀번호 인증 실패 시 에러 코드 반환
    if (rc != SSH_AUTH_SUCCESS) {
        fprintf(stderr, "SSH authentication failed: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        ssh_free(session);
        return NULL;
    }

    printf("SSH connection successful for user: %s\n", username);
    return session;
}
