# 컴파일러 설정
CC = gcc
CFLAGS = -Iinclude -Wall -pthread
LDFLAGS = -lssl -lcrypto -lpthread -lwebsockets -lssh -ljansson -lmongoose -lmysqlclient

# 운영 체제에 따라 경로 설정
ifeq ($(shell uname), Darwin) # macOS
    CFLAGS += -I/opt/homebrew/opt/openssl/include \
              -I/opt/homebrew/opt/libwebsockets/include \
              -I/opt/homebrew/opt/jansson/include \
              -I/opt/homebrew/opt/libssh/include \
              -I/opt/homebrew/opt/mongoose/include \
              -I/opt/homebrew/opt/mysql-client/include
    LDFLAGS += -L/opt/homebrew/opt/openssl/lib \
               -L/opt/homebrew/opt/libwebsockets/lib \
               -L/opt/homebrew/opt/jansson/lib \
               -L/opt/homebrew/opt/libssh/lib \
               -L/opt/homebrew/opt/mongoose/lib \
               -L/opt/homebrew/opt/mysql-client/lib
else ifeq ($(shell uname), Linux) # Linux
    CFLAGS += -I/usr/include/mysql
    LDFLAGS += -L/usr/lib/x86_64-linux-gnu
else ifeq ($(OS), Windows_NT) # Windows (MinGW 환경)
    CFLAGS += -IC:/mysql/include
    LDFLAGS += -LC:/mysql/lib
else
    $(error Unsupported platform: $(shell uname))
endif

# 디렉터리
SRC_DIR = src
BUILD_DIR = build
CHAT_DIR = $(BUILD_DIR)/chat
CORE_DIR = $(BUILD_DIR)/core
CRYPTO_DIR = $(BUILD_DIR)/crypto
SERVER_DIR = $(BUILD_DIR)/server

# 소스 및 오브젝트 파일
SRCS := $(wildcard $(SRC_DIR)/chat/*.c $(SRC_DIR)/core/*.c $(SRC_DIR)/crypto/*.c $(SRC_DIR)/server/*.c $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# 실행 파일 이름
TARGET = $(BUILD_DIR)/server_app

# 빌드 규칙
all: directories $(TARGET)

# 타겟 빌드 규칙
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# 개별 오브젝트 파일 빌드 규칙
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 디렉터리 생성
directories:
	mkdir -p $(BUILD_DIR) $(CHAT_DIR) $(CORE_DIR) $(CRYPTO_DIR) $(SERVER_DIR)

# 클린업 규칙
clean:
	rm -rf $(BUILD_DIR)

# 실행 규칙
run: $(TARGET)
	$(TARGET)
