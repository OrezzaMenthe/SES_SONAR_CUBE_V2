
modules-$(MODULES)+=mod_document
slib-y+=mod_document
mod_document_SOURCES+=mod_document.c
mod_document_CFLAGS+=-DSTATIC_FILE -I../libhttpserver/include
mod_document_CFLAGS-$(MODULES)+=-DMODULES
mod_document_CFLAGS-$(AUTH)+=-DAUTH

mod_document_SOURCES-$(SENDFILE)+=mod_sendfile.c
mod_document_CFLAGS-$(SENDFILE)+=-DSENDFILE

mod_document_SOURCES-$(DIRLISTING)+=mod_dirlisting.c
mod_document_CFLAGS-$(DIRLISTING)+=-DDIRLISTING

mod_document_SOURCES-$(RANGEREQUEST)+=mod_range.c
mod_document_CFLAGS-$(RANGEREQUEST)+=-DRANGEREQUEST

mod_document_SOURCES-$(DOCUMENTREST)+=mod_documentrest.c
mod_document_CFLAGS-$(DOCUMENTREST)+=-DDOCUMENTREST

mod_document_CFLAGS-$(DOCUMENTHOME)+=-DDOCUMENTHOME

mod_document_CFLAGS-$(DEBUG)+=-g -DDEBUG

