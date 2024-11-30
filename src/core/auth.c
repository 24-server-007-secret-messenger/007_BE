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
        mg_http_reply(conn, 500,
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: application/json\r\n",
            "{\"error\": \"Database connection failed.\"}\n");
        return;
    }

    char *username = mg_json_get_str(hm->body, "$.id");
    char *password = mg_json_get_str(hm->body, "$.password");
    if (!username || !password) {
        mg_http_reply(conn, 400,
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: application/json\r\n",
            "{\"error\": \"Invalid JSON format.\"}\n");
        db_disconnect(db_conn);
        return;
    }

    ssh_session session = ssh_connection(username, password);
    if (session) {
        add_session(username, session);
    } else {
        mg_http_reply(conn, 403,
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: application/json\r\n",
            "{\"error\": \"Failed to authenticate.\"}\n");
        db_disconnect(db_conn);
        free(username);
        free(password);
        return;
    }

    char query[256];
    snprintf(query, sizeof(query),
             "INSERT INTO user (username, password) VALUES ('%s', '%s') ON DUPLICATE KEY UPDATE password='%s'",
             username, password, password);
    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "Failed to update user: %s\n", mysql_error(db_conn));
    }

    add_active_user(db_conn, username);
    mg_http_reply(conn, 200,
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: application/json\r\n",
        "{\"username\": \"%s\", \"message\": \"Login successful.\"}\n", username);

    db_disconnect(db_conn);
    free(username);
    free(password);
}

// HTTP 로그아웃 요청 핸들러
void handle_logout(struct mg_connection *conn, struct mg_http_message *hm) {
    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500,
            "Access-Control-Allow-Origin: *\r\n",
            "Database connection failed.\n");
        return;
    }

    char *username = mg_json_get_str(hm->body, "$.username");
    if (!username) {
        mg_http_reply(conn, 400,
            "Access-Control-Allow-Origin: *\r\n",
            "Invalid JSON format.\n");
        db_disconnect(db_conn);
        return;
    }

    ssh_session session = get_session(username);
    if (session) {
        remove_session(username);
    } else {
        mg_http_reply(conn, 404,
            "Access-Control-Allow-Origin: *\r\n",
            "Session not found.\n");
    }

    remove_active_user(db_conn, username);
    mg_http_reply(conn, 200,
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: text/plain\r\n",
        "Logout successful.\n");

    db_disconnect(db_conn);
    free(username);
}
