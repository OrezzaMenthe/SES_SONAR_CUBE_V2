package=ouistiti
version=3.3

include scripts.mk

CFLAGS+=-I$(srcdir)/include/ouistiti
export CFLAGS
#libhttpserver has to be static in all configuration
export SLIB_HTTPSERVER=y

ifeq ($(VTHREAD_TYPE),pthread)
 USE_PTHREAD=y
endif

download-$(LIBHTTPSERVER_DL)+=libhttpserver
libhttpserver_SOURCE=libhttpserver
libhttpserver_SITE=https://github.com/ouistiti-project/libhttpserver.git
libhttpserver_SITE_METHOD=git

LIBHTTPSERVER_DIR?=libhttpserver
export LIBHTTPSERVER_DIR

ifneq ($(wildcard $(LIBHTTPSERVER_DIR)/Makefile),)
subdir-y+=$(LIBHTTPSERVER_DIR)
LIBHTTPSERVER_LDFLAGS=-L$(builddir)$(LIBHTTPSERVER_DIR)/src/ -L$(builddir)$(LIBHTTPSERVER_DIR)/src/httpserver
LIBHTTPSERVER_CFLAGS=-I$(srcdir)$(LIBHTTPSERVER_DIR)/include
endif
LIBHTTPSERVER_NAME:=$(package)
export LIBHTTPSERVER_NAME
export LIBHTTPSERVER_LDFLAGS
export LIBHTTPSERVER_CFLAGS

ifneq ($(HTTPCLIENT_FEATURES),y)
override AUTHN_OAUTH2:=n
endif

subdir-y+=include
subdir-y+=staging
subdir-y+=src
subdir-y+=utils
