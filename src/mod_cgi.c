/*****************************************************************************
 * mod_cgi.c: callbacks and management of connection
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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libgen.h>
#include <netinet/in.h>
#include <sched.h>

#include "httpserver/httpserver.h"
#include "httpserver/uri.h"
#include "httpserver/utils.h"
#include "mod_cgi.h"

#define err(format, ...) fprintf(stderr, "\x1B[31m"format"\x1B[0m\n",  ##__VA_ARGS__)
#ifdef DEBUG
#define warn(format, ...) fprintf(stderr, "\x1B[35m"format"\x1B[0m\n",  ##__VA_ARGS__)
#define dbg(format, ...) fprintf(stderr, "\x1B[32m"format"\x1B[0m\n",  ##__VA_ARGS__)
#else
#define warn(...)
#define dbg(...)
#endif

#define cgi_dbg(...)

static const char str_cgi[] = "cgi";

typedef struct _mod_cgi_config_s _mod_cgi_config_t;
typedef struct _mod_cgi_s _mod_cgi_t;
typedef struct mod_cgi_ctx_s mod_cgi_ctx_t;

static void *_mod_cgi_getctx(void *arg, http_client_t *ctl, struct sockaddr *addr, int addrsize);
static void _mod_cgi_freectx(void *vctx);
static int _cgi_connector(void **arg, http_message_t *request, http_message_t *response);

struct mod_cgi_ctx_s
{
	enum
	{
		STATE_SETUP,
		STATE_START,
		STATE_INFINISH,
		STATE_HEADERCOMPLETE,
		STATE_CONTENTCOMPLETE,
		STATE_OUTFINISH,
		STATE_END,
		STATE_SHUTDOWN = 0x0100,
	} state;
	_mod_cgi_t *mod;
	http_client_t *ctl;

	char *cgipath;
	char *path_info;

	pid_t pid;
	int tocgi[2];
	int fromcgi[2];

	char *chunk;
};

struct _mod_cgi_s
{
	http_server_t *server;
	mod_cgi_config_t *config;
};

void *mod_cgi_create(http_server_t *server, mod_cgi_config_t *modconfig)
{
	_mod_cgi_t *mod;

	if (!modconfig)
		return NULL;

	mod = calloc(1, sizeof(*mod));
	mod->config = modconfig;
	mod->server = server;
	if (modconfig->timeout == 0)
		modconfig->timeout = 3;

	httpserver_addmod(server, _mod_cgi_getctx, _mod_cgi_freectx, mod, str_cgi);

	return mod;
}

void mod_cgi_destroy(void *arg)
{
	_mod_cgi_t *mod = (_mod_cgi_t *)arg;
	free(mod);
}

static void *_mod_cgi_getctx(void *arg, http_client_t *ctl, struct sockaddr *addr, int addrsize)
{
	_mod_cgi_t *mod = (_mod_cgi_t *)arg;
	mod_cgi_ctx_t *ctx = calloc(1, sizeof(*ctx));

	ctx->ctl = ctl;
	ctx->mod = mod;
	if (mod->config->chunksize == 0)
		mod->config->chunksize = 64;
	ctx->chunk = malloc(mod->config->chunksize + 1);
	httpclient_addconnector(ctl, _cgi_connector, ctx, str_cgi);

	return ctx;
}

static void _mod_cgi_freectx(void *vctx)
{
	mod_cgi_ctx_t *ctx = (mod_cgi_ctx_t *)vctx;
	if (ctx->cgipath)
		free(ctx->cgipath);
	if (ctx->path_info)
		free(ctx->path_info);
	if (ctx->chunk)
		free(ctx->chunk);
	if (ctx->fromcgi[0])
	{
		if (ctx->fromcgi[0])
			close(ctx->fromcgi[0]);
		if (ctx->tocgi[1] > 0)
			close(ctx->tocgi[1]);
	}
	free(ctx);
}

static int _mod_cgi_fork(mod_cgi_ctx_t *ctx, http_message_t *request)
{
	pipe(ctx->tocgi);
	pipe(ctx->fromcgi);
	pid_t pid = fork();
	if (pid)
	{
		/* keep only input of the pipe */
		close(ctx->tocgi[0]);
		/* keep only output of the pipe */
		close(ctx->fromcgi[1]);
	}
	else /* into child */
	{
		int flags;
		flags = fcntl(ctx->tocgi[0],F_GETFD);
		fcntl(ctx->tocgi[0],F_SETFD, flags | FD_CLOEXEC);
		flags = fcntl(ctx->fromcgi[1],F_GETFD);
		fcntl(ctx->fromcgi[1],F_SETFD, flags | FD_CLOEXEC);
		/* send data from server to the stdin of the cgi */
		close(ctx->tocgi[1]);
		dup2(ctx->tocgi[0], STDIN_FILENO);
		close(ctx->tocgi[0]);
		/* send data from the stdout of the cgi to server */
		close(ctx->fromcgi[0]);
		dup2(ctx->fromcgi[1], STDOUT_FILENO);
		close(ctx->fromcgi[1]);

		int sock = httpmessage_keepalive(request);

		char *argv[2];
		argv[0] = basename(ctx->cgipath);
		argv[1] = NULL;

		char **env = NULL;
		env = cgi_buildenv(ctx->mod->config, request, ctx->cgipath);

		close(sock);

		char *dirpath;
		dirpath = dirname(ctx->cgipath);
		if (dirpath)
		{
			chdir(dirpath);
		}
		setbuf(stdout, 0);
		sched_yield();
		execve(argv[0], argv, env);
		err("cgi error: %s", strerror(errno));
		exit(0);
	}
	return pid;
}

static int _cgi_connector(void **arg, http_message_t *request, http_message_t *response)
{
	int ret = EINCOMPLETE;
	mod_cgi_ctx_t *ctx = (mod_cgi_ctx_t *)*arg;
	mod_cgi_config_t *config = ctx->mod->config;

	if (ctx->state == STATE_SETUP)
	{
		if (ctx->pid == -1)
		{
			warn("cgi: pid -1");
			return EREJECT;
		}

		if (ctx->pid == 0)
		{
			if (ctx->path_info)
				free(ctx->path_info);
			ctx->path_info = utils_urldecode(httpmessage_REQUEST(request,"uri"));
			char *str = ctx->path_info;
			if (str && config->docroot && ctx->cgipath == NULL)
			{
				int length = strlen(str);
				length += strlen(config->docroot) + 1;

				char *filepath;
				filepath = calloc(1, length + 1);
				snprintf(filepath, length + 1, "%s/%s", config->docroot, str);

				if (utils_searchexp(str, config->deny) == ESUCCESS &&
					utils_searchexp(str, config->allow) != ESUCCESS)
				{
					dbg("cgi: %s forbidden extension", ctx->path_info);
					free(filepath);
					return EREJECT;
				}
				struct stat filestat;
				int ret = stat(filepath, &filestat);
				if (ret != 0)
				{
					dbg("cgi: %s not found", ctx->path_info);
					free(filepath);
					return EREJECT;
				}
				if (S_ISDIR(filestat.st_mode))
				{
					dbg("cgi: %s is directory", ctx->path_info);
					free(filepath);
					return EREJECT;
				}
				/* at least user or group may execute the CGI */
				if ((filestat.st_mode & (S_IXUSR | S_IXGRP)) != (S_IXUSR | S_IXGRP))
				{
					httpmessage_result(response, RESULT_404);
					warn("cgi: %s access denied", ctx->path_info);
					free(filepath);
					return ESUCCESS;
				}
				dbg("cgi: run %s", filepath);
				ctx->cgipath = filepath;
			}
			ctx->pid = _mod_cgi_fork(ctx, request);
			ctx->state = STATE_START;
		}
	}
	else if (ctx->tocgi[1] > 0 && ctx->state == STATE_START)
	{
		char *input;
		int inputlen;
		unsigned long long rest;
		inputlen = httpmessage_content(request, &input, &rest);
		if (inputlen > 0)
		{
			write(ctx->tocgi[1], input, inputlen);
		}
		else if (rest != EINCOMPLETE)
			ctx->state = STATE_INFINISH;
		if (ctx->state == STATE_INFINISH)
		{
			close(ctx->tocgi[1]);
			ctx->tocgi[1] = -1;
		}
	}
	/**
	 * when the request is complete the module must check the CGI immedialty
	 * otherwise the client will wait more data from request
	 */
	if (ctx->state >= STATE_INFINISH && ctx->state < STATE_CONTENTCOMPLETE)
	{
		int sret;
		fd_set rfds;
		struct timeval timeout = { config->timeout,0 };

		FD_ZERO(&rfds);
		FD_SET(ctx->fromcgi[0], &rfds);
		sret = select(ctx->fromcgi[0] + 1, &rfds, NULL, NULL, &timeout);
		if (sret > 0 && FD_ISSET(ctx->fromcgi[0], &rfds))
		{
			int size = config->chunksize;
			size = read(ctx->fromcgi[0], ctx->chunk, size);
			if (size < 1)
			{
				ctx->state = STATE_CONTENTCOMPLETE;
				ret = ECONTINUE;
				if (size < 0)
				{
					err("cgi read %s", strerror(errno));
				}
			}
			else
			{
				ctx->chunk[size] = 0;
				cgi_dbg("cgi: receive (%d)", size);
				cgi_dbg("cgi: receive (%d)\n%s", size, ctx->chunk);
				/**
				 * if content_length is not null, parcgi is able to
				 * create the content.
				 * But the cgi know the length at the end, is too late
				 * to set the header.
				 */
				int rest = size;
				if (rest > 0)
				{
					ret = httpmessage_parsecgi(response, ctx->chunk, &rest);
					cgi_dbg("cgi: parse %d data %d %d", ret, size, rest);
					cgi_dbg("cgi: parse %d data %d %d\n%s#", ret, size, rest, ctx->chunk);
					if (ret == ECONTINUE && ctx->state < STATE_HEADERCOMPLETE)
					{
#if defined(RESULT_302)
						/**
						 * RFC 3875 : 6.2.3
						 */
						const char *location = httpmessage_REQUEST(response, str_location);
						if (location != NULL && location[0] != '\0')
							httpmessage_result(response, RESULT_302);
#endif
						ctx->state = STATE_HEADERCOMPLETE;
					}
					if (ret == ESUCCESS)
					{
						ctx->state = STATE_CONTENTCOMPLETE;
						//parse_cgi is complete but not this module
					}
				}
			}
			/**
			 * the request is completed to not wait more data the module
			 * must returns ECONTINUE or ESUCCESS now
			 */
			ret = ECONTINUE;
		}
		else
		{
			ctx->state = STATE_OUTFINISH | STATE_SHUTDOWN;
			kill(ctx->pid, SIGTERM);
			dbg("cgi complete");
			ret = ECONTINUE;
		}
	}
	else if (ctx->state == STATE_CONTENTCOMPLETE)
	{
		close(ctx->fromcgi[0]);
		httpmessage_parsecgi(response, NULL, 0);
		ret = ECONTINUE;
		ctx->state = STATE_OUTFINISH | STATE_SHUTDOWN;
	}
	else if ((ctx->state & STATE_OUTFINISH) == STATE_OUTFINISH)
	{
		long long length;
		ret = httpmessage_content(response, NULL, &length);
		dbg("content len %d %lld", ret, length);
		if (ret == 0)
			ctx->state = STATE_END | STATE_SHUTDOWN;
		ret = ECONTINUE;
	}
	else if ((ctx->state & STATE_END) == STATE_END)
	{
		ret = ESUCCESS;
	}
	/* this mod returns EINCOMPLETE
	 * because it needs to wait the end
	 * to know the length of the content */
	return ret;
}

const module_t mod_cgi =
{
	.name = str_cgi,
	.create = (module_create_t)mod_cgi_create,
	.destroy = mod_cgi_destroy
};
#ifdef MODULES
extern module_t mod_info __attribute__ ((weak, alias ("mod_cgi")));
#endif
