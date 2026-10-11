#---------------------------------------------------------------------------------------------------------------------
# 文件名称          Makefile
# 说明              主机端单元测试（纯逻辑模块）构建脚本，供 Linux / MSYS / MinGW 环境使用
#                   用法：make        （编译并运行）
#                        make build  （只编译）
#                        make clean
#                   注意：这些文件不参与 MounRiver 目标端编译（.cproject 已排除 host_test）
#---------------------------------------------------------------------------------------------------------------------
CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O1 -Wall -Wextra -DAPP_HOST_TEST -I../User
USER    := ../User
SRC     := test_main.c test_crc32.c test_params.c test_menu.c test_key.c test_text.c test_ui.c \
           stub_screen.c stub_time.c \
           $(USER)/app_crc32.c $(USER)/app_params.c $(USER)/app_menu.c $(USER)/app_key.c \
           $(USER)/app_text.c $(USER)/app_ui.c
OUT     := host_test.exe

.PHONY: all build run clean

all: run

build: $(OUT)

$(OUT): $(SRC) test_util.h
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

run: $(OUT)
	./$(OUT)

clean:
	$(RM) $(OUT)
