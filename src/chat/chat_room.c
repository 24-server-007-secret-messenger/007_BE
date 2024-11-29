#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mongoose.h"
#include "db.h"
#include "chat_room.h"

void handle_chat_start(struct mg_connection *conn, struct mg_http_message *hm) {
    char *user1 = mg_json_get_str(hm->body, "$.user1");
    char *user2 = mg_json_get_str(hm->body, "$.user2");

    if (!user1 || !user2) {
        mg_http_reply(conn, 400, "", "Invalid JSON format\n");
        return;
    }

    // 사용자 정렬
    if (strcmp(user1, user2) > 0) {
        char *temp = user1;
        user1 = user2;
        user2 = temp;
    }

    MYSQL *db_conn = db_connect();
    if (!db_conn) {
        mg_http_reply(conn, 500, "", "Database connection failed\n");
        free(user1);
        free(user2);
        return;
    }

    int chat_room_id = create_or_get_chat_room(db_conn, user1, user2);
    if (chat_room_id == -1) {
        mg_http_reply(conn, 500, "", "Failed to create chat room\n");
    } else {
        mg_http_reply(conn, 200, "", "{\"chat_room_id\":%d}\n", chat_room_id);
    }

    db_disconnect(db_conn);
    free(user1);
    free(user2);
}

int create_or_get_chat_room(MYSQL *conn, const char *user1, const char *user2) {
    // 친구 관계와 active_user 확인
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT COUNT(*) FROM friend f "
             "JOIN active_user au1 ON f.from_user = au1.username "
             "JOIN active_user au2 ON f.to_user = au2.username "
             "WHERE ((f.from_user = '%s' AND f.to_user = '%s') OR "
             "(f.from_user = '%s' AND f.to_user = '%s')) "
             "AND f.status = 'accepted'",
             user1, user2, user2, user1);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Friend and active_user check failed: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    int is_friend_and_active = atoi(row[0]);
    mysql_free_result(result);

    if (!is_friend_and_active) {
        fprintf(stderr, "Users are not friends or not active.\n");
        return -1;
    }

    // 채팅방 생성 또는 가져오기
    snprintf(query, sizeof(query),
             "INSERT INTO chat_room (user1, user2) "
             "VALUES ('%s', '%s') "
             "ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id)",
             user1, user2);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Chat room creation failed: %s\n", mysql_error(conn));
        return -1;
    }

    return (int)mysql_insert_id(conn);
}

