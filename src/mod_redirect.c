/*****************************************************************************
 * mod_redirect.c: Redirect the request on 404 error
 * this file is part of https://github.com/ouistiti-project/ouistiti
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
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "httpserver/httpserver.h"
#include "httpserver/utils.h"
#include "mod_redirect.h"

#define err(format, ...) fprintf(stderr, "\x1B[31m"format"\x1B[0m\n",  ##__VA_ARGS__)
#define warn(format, ...) fprintf(stderr, "\x1B[35m"format"\x1B[0m\n",  ##__VA_ARGS__)
#ifdef DEBUG
#define dbg(format, ...) fprintf(stderr, "\x1B[32m"format"\x1B[0m\n",  ##__VA_ARGS__)
#else
#define dbg(...)
#endif

#ifndef RESULT_204
#define RESULT_204 204
#endif

typedef struct _mod_redirect_s _mod_redirect_t;

static void *_mod_redirect_getctx(void *arg, http_client_t *ctl, struct sockaddr *addr, int addrsize);
static void _mod_redirect_freectx(void *vctx);
static int _mod_redirect_connector(void *arg, http_message_t *request, http_message_t *response);

const char str_redirect[] = "redirect";
#ifndef __STR_HTTPS
static const char str_https[] = "https";
#endif
static const char str_wss[] = "wss";

struct _mod_redirect_s
{
	mod_redirect_t	*config;
	char *vhost;
	http_server_t *server;
	int result;
};

void *mod_redirect_create(http_server_t *server, char *vhost, mod_redirect_t *config)
{
	_mod_redirect_t *mod;

	if (!config)
		return NULL;

	mod = calloc(1, sizeof(*mod));
	mod->config = config;
	mod->vhost = vhost;
	mod->server = server;
	if (config->options & REDIRECT_PERMANENTLY)
		mod->result = RESULT_301;
	else if (config->options & REDIRECT_TEMPORARY)
		mod->result = RESULT_307;
	else
		mod->result = RESULT_302;

	httpserver_addmod(server, _mod_redirect_getctx, _mod_redirect_freectx, mod, str_redirect);
	return mod;
}

void mod_redirect_destroy(void *arg)
{
	_mod_redirect_t *mod = (_mod_redirect_t *)arg;
	free(mod);
}

static void *_mod_redirect_getctx(void *arg, http_client_t *ctl, struct sockaddr *addr, int addrsize)
{
	_mod_redirect_t *mod = (_mod_redirect_t *)arg;
	mod_redirect_t *config = mod->config;

	httpclient_addconnector(ctl, mod->vhost, _mod_redirect_connector, arg, str_redirect);
	return mod;
}

static void _mod_redirect_freectx(void *vctx)
{
}

static int _mod_redirect_connector(void *arg, http_message_t *request, http_message_t *response)
{
	_mod_redirect_t *mod = (_mod_redirect_t *)arg;
	mod_redirect_t *config = mod->config;

	if (config->options & REDIRECT_HSTS)
	{
#if 1
		httpmessage_addheader(response, "Strict-Transport-Security", "max-age=31536000; includeSubDomains");
#else
		const char *scheme = str_https;
		if (scheme != NULL)
		{
			const char *scheme = httpserver_INFO(mod->server, "scheme");
			const char *host = httpserver_INFO(mod->server, "host");
			const char *port = httpserver_INFO(mod->server, "port");
			const char *path = httpserver_INFO(mod->server, "uri");
			const char *portseparator = "";
			if (port[0] != '\0')
				portseparator = ":";

			char location[1024];
			snprintf(location, 1024, "%s://%s%s%s/%s",
						scheme, host, portseparator, port, path);
			httpmessage_addheader(response, str_location, location);
			httpmessage_result(response, RESULT_301);
			return ESUCCESS;
		}
#endif
	}
	if (config->options & REDIRECT_GENERATE204)
	{
		const char *path = httpmessage_REQUEST(request, "uri");
		if (utils_searchexp(path, "generate_204") == ESUCCESS)
		{
			httpmessage_result(response, RESULT_204);
			return ESUCCESS;
		}
	}
	if (config->options & REDIRECT_LINK)
	{
		char *uri = utils_urldecode(httpmessage_REQUEST(request, "uri"));
		if (uri == NULL)
		{
			return EREJECT;
		}
		//const char *uri = httpmessage_REQUEST(request, "uri");
		mod_redirect_link_t *link = config->links;
		while (link != NULL)
		{
			if (utils_searchexp(uri, link->origin) == ESUCCESS)
			{
				if (link->destination != NULL &&
						utils_searchexp(uri, link->destination) != ESUCCESS)
				{
					httpmessage_addheader(response, str_location, link->destination);
					httpmessage_result(response, mod->result);
					free(uri);
					return ESUCCESS;
				}
				else if (link->options & REDIRECT_GENERATE204)
				{
					httpmessage_result(response, RESULT_204);
					free(uri);
					return ESUCCESS;
				}
			}
			link = link->next;
		}
		free(uri);
	}
	return EREJECT;
}

const module_t mod_redirect =
{
	.name = str_redirect,
	.create = (module_create_t)mod_redirect_create,
	.destroy = mod_redirect_destroy
};
#ifdef MODULES
extern module_t mod_info __attribute__ ((weak, alias ("mod_redirect")));
#endif
