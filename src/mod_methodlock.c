/*****************************************************************************
 * mod_methodlock.c: callbacks and management of request method
 *****************************************************************************
 * Copyright (C) 2016-2017
 *
 * Authors: Marc Chalain <marc.chalain@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>

#include "httpserver/httpserver.h"
#include "httpserver/uri.h"
#include "mod_methodlock.h"

#define err(format, ...) fprintf(stderr, "\x1B[31m"format"\x1B[0m\n",  ##__VA_ARGS__)
#define warn(format, ...) fprintf(stderr, "\x1B[35m"format"\x1B[0m\n",  ##__VA_ARGS__)
#ifdef DEBUG
#define dbg(format, ...) fprintf(stderr, "\x1B[32m"format"\x1B[0m\n",  ##__VA_ARGS__)
#else
#define dbg(...)
#endif

typedef struct _mod_methodlock_s _mod_methodlock_t;

struct _mod_methodlock_s
{
	void *vhost;
};

static int methodlock_connector(void *arg, http_message_t *request, http_message_t *response)
{
	int ret = ESUCCESS;

	char *method = httpmessage_REQUEST(request, "method");

	if (method && (method[0] == 'G' || method[0] == 'H' || !strcmp(method, "POST")))
	{
		ret = EREJECT;
	}
#ifdef AUTH
	else if (!strcmp("superuser", httpmessage_SESSION(request, "%authrights",NULL)))
	{
		if (method && (method[0] == 'D' || !strcmp(method, "PUT")))
		{
			ret = EREJECT;
		}
		else
		{
			httpmessage_addheader(response, "Allow", "GET, POST, HEAD, PUT, DEL");
			httpmessage_result(response, RESULT_405);
		}
	}
#endif
	else
	{
		httpmessage_addheader(response, "Allow", "GET, POST, HEAD");
		httpmessage_result(response, RESULT_405);
	}
	return ret;
}

static void *_mod_methodlock_getctx(void *arg, http_client_t *ctl, struct sockaddr *addr, int addrsize)
{
	_mod_methodlock_t *mod = (_mod_methodlock_t *)arg;

	httpclient_addconnector(ctl, mod->vhost, methodlock_connector, NULL);

	return NULL;
}

void *mod_methodlock_create(http_server_t *server, char *vhost, void *config)
{
	_mod_methodlock_t *mod = calloc(1, sizeof(*mod));

	mod->vhost = vhost;
	httpserver_addmod(server, _mod_methodlock_getctx, NULL, mod);

	return mod;
}

void mod_methodlock_destroy(void *data)
{
	free(data);
}
