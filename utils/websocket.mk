WS_SRC:=websocket/
lib-$(WEBSOCKET)+=ouistiti_ws
ouistiti_ws_SOURCES+=websocket.c utils.c
ouistiti_ws_LIBS+=dl
ouistiti_ws_LIBS+=websocket
ouistiti_ws_CFLAGS+=-I../libhttpserver/include
ouistiti_ws_CFLAGS-$(DEBUG)+=-g -DDEBUG

bin-$(WS_ECHO)+=websocket_echo
websocket_echo_SOURCES+=$(WS_SRC)echo.c
ifeq ($(WEBSOCKET_RT), y)
websocket_echo_CFLAGS-$(WEBSOCKET)+=-DPTHREAD
websocket_echo_CFLAGS-$(WEBSOCKET)+=-DSOCKDOMAIN="(AF_MAX+0X100)"
websocket_echo_CFLAGS-$(WEBSOCKET)+=-DSOCKPROTOCOL=153
websocket_echo_LDFLAGS-$(WEBSOCKET)+=-nodefaultlibs
websocket_echo_LIBS-$(WEBSOCKET)+=ouistiti_ws websocket c pthread
endif

websocket_echo_CFLAGS-$(DEBUG)+=-g -DDEBUG

bin-$(WS_CHAT)+=websocket_chat
websocket_chat_SOURCES+=$(WS_SRC)chat.c
ifeq ($(WEBSOCKET_RT), y)
websocket_chat_CFLAGS-$(WEBSOCKET)+=-DPTHREAD
websocket_chat_CFLAGS-$(WEBSOCKET)+=-DSOCKDOMAIN="(AF_MAX+0X100)"
websocket_chat_CFLAGS-$(WEBSOCKET)+=-DSOCKPROTOCOL=153
websocket_chat_LDFLAGS-$(WEBSOCKET)+=-nodefaultlibs
websocket_chat_LIBS-$(WEBSOCKET)+=ouistiti_ws websocket c pthread
endif

websocket_chat_CFLAGS-$(DEBUG)+=-g -DDEBUG

bin-$(WS_JSONRPC)+=websocket_jsonrpc
websocket_jsonrpc_SOURCES+=$(WS_SRC)jsonrpc.c
websocket_jsonrpc_SOURCES+=jsonrpc/jsonrpc.c
websocket_jsonrpc_LIBS+=dl jansson
websocket_jsonrpc_CFLAGS-$(DEBUG)+=-g -DDEBUG

modules-$(WS_JSONRPC)+=jsonsql
jsonsql_SOURCES+=$(WS_SRC)jsonsql.c
jsonsql_LIBS+=sqlite3

bin-$(WS_SYSLOGD)+=websocket_syslogd
websocket_syslogd_SOURCES+=$(WS_SRC)syslogd.c
ifeq ($(WEBSOCKET_RT), y)
websocket_syslogd_CFLAGS-$(WEBSOCKET)+=-DPTHREAD
websocket_syslogd_CFLAGS-$(WEBSOCKET)+=-DSOCKDOMAIN="(AF_MAX+0X100)"
websocket_syslogd_CFLAGS-$(WEBSOCKET)+=-DSOCKPROTOCOL=153
websocket_syslogd_LDFLAGS-$(WEBSOCKET)+=-nodefaultlibs
websocket_syslogd_LIBS-$(WEBSOCKET)+=ouistiti_ws websocket c pthread
endif

websocket_syslogd_CFLAGS-$(DEBUG)+=-g -DDEBUG
