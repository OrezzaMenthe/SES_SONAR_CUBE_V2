
modules-$(DYNAMIC)+=mod_server
slib-$(STATIC)+=mod_server
mod_server_SOURCES-$(SERVERHEADER)+=mod_server.c
mod_server_CFLAGS+=-I../libhttpserver/include
mod_server_CFLAGS+=-DSERVERHEADER

mod_server_CFLAGS-$(DEBUG)+=-g -DDEBUG

