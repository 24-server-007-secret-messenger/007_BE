#include <libssh/libssh.h>
#include "auth.h"
#include "db.h"
#include "active_user.h"
#include "ssh_connection.h"
#include "session.h"

// HTTP 로그인 요청 핸들러
void handle_login(struct mg_connection *conn, struct mg_http_message *hm) {
    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, "", "Database connection failed.\n");
        return;
    }

    // 사용자 아이디과 비밀번호 추출
    char *username = mg_json_get_str(hm->body, "$.id");
    char *password = mg_json_get_str(hm->body, "$.password");

    // 사용자 아이디 또는 비밀번호가 없는 경우
    if (!username || !password) {
        mg_http_reply(conn, 400, "", "Invalid JSON format.\n");
        db_disconnect(db_conn);
        return;
    }

    // 사용자 정보 업데이트
    char query[256];
    snprintf(query, sizeof(query),
             "INSERT INTO user (username, password) VALUES ('%s', '%s') ON DUPLICATE KEY UPDATE password='%s'",
             username, password, password);
    // 사용자 정보 업데이트 실패 시 에러 출력
    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "Failed to update user: %s\n", mysql_error(db_conn));
    }

    // SSH 연결 생성 및 세션 추가
    ssh_session session = ssh_connection(username, password);
    if (session) {
        add_session(username, session); // 세션 추가
        mg_http_reply(conn, 200, "", "Login successful.\n");
    } else {
        mg_http_reply(conn, 403, "", "SSH connection failed.\n");
    }

    db_disconnect(db_conn);
    free(username);
    free(password);
}

// HTTP 로그아웃 요청 핸들러
void handle_logout(struct mg_connection *conn, struct mg_http_message *hm) {
    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, "", "Database connection failed.\n");
        return;
    }

    // 사용자 아이디 추출
    char *username = mg_json_get_str(hm->body, "$.id");
    if (!username) {
        mg_http_reply(conn, 400, "", "Invalid JSON format.\n");
        db_disconnect(db_conn);
        return;
    }

    // 세션 제거 및 SSH 연결 종료
    ssh_session session = get_session(username);
    if (session) {
        remove_session(username);
        mg_http_reply(conn, 200, "", "Logout successful.\n");
    } else {
        mg_http_reply(conn, 404, "", "User not logged in.\n");
    }

    // Active user에서 제거
    remove_active_user(db_conn, username);
    mg_http_reply(conn, 200, "", "Logout successful.\n");

    db_disconnect(db_conn);
    free(username);
}
