modules-$(MODULES)+=mod_methodlock
slib-y+=mod_methodlock
mod_methodlock_SOURCES-$(METHODLOCK)+=mod_methodlock.c
mod_methodlock_CFLAGS+=-I../libhttpserver/include
mod_methodlock_CFLAGS+=-DMETHODLOCK
mod_methodlock_CFLAGS-$(AUTH)+=-DAUTH

mod_methodlock_CFLAGS-$(DEBUG)+=-g -DDEBUG

