/*****************************************************************************
 * mod_static_file.c: callbacks and management of files
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

#include "httpserver/httpserver.h"
#include "httpserver/uri.h"
#include "utils.h"
#include "mod_static_file.h"
#include "mod_dirlisting.h"

#define err(format, ...) fprintf(stderr, "\x1B[31m"format"\x1B[0m\n",  ##__VA_ARGS__)
#define warn(format, ...) fprintf(stderr, "\x1B[35m"format"\x1B[0m\n",  ##__VA_ARGS__)
#ifdef DEBUG
#define dbg(format, ...) fprintf(stderr, "\x1B[32m"format"\x1B[0m\n",  ##__VA_ARGS__)
#else
#define dbg(...)
#endif

/**
 * USE_PRIVATE is used to keep a sample of cade which uses
 * the httpmessage_private function
 */
typedef struct _static_file_connector_s static_file_connector_t;

struct _mod_static_file_mod_s
{
	mod_static_file_t *config;
	void *vhost;
	mod_transfer_t transfer;
};

int mod_send(static_file_connector_t *private, http_message_t *response);

static mod_static_file_t default_config = 
{
	.docroot = "/srv/www/htdocs",
	.accepted_ext = ".html,.xhtml,.htm,.css",
	.ignored_ext = ".php",
};

static int static_file_connector(void *arg, http_message_t *request, http_message_t *response)
{
	int ret =  EREJECT;
	static_file_connector_t *private = (static_file_connector_t *)arg;
	_mod_static_file_mod_t *mod = private->mod;
	mod_static_file_t *config = (mod_static_file_t *)mod->config;

	if (private->fd == -1)
	{
		warn("static file: -1");
	}
	else if (private->fd == 0)
	{
		struct stat filestat;
		char *filepath;
		if (private->path_info == NULL)
			private->path_info = utils_urldecode(httpmessage_REQUEST(request,"uri"));
		filepath = utils_buildpath(config->docroot, private->path_info, "", "", &filestat);
		if (filepath == NULL)
		{
			dbg("static file: %s not exist", private->path_info);
			return ret;
		}
		else if (S_ISDIR(filestat.st_mode))
		{
			int length = strlen(filepath);
			if (filepath[length - 1] != '/')
			{
				free(filepath);
#ifndef HTTP_STATUS_PARTIAL
				char *location = calloc(1, strlen(private->path_info) + 2);
				sprintf(location, "%s/", private->path_info);
				httpmessage_addheader(response, str_location, location);
				httpmessage_result(response, RESULT_301);
				free(location);
				if (private->path_info)
				{
					free(private->path_info);
					private->path_info = NULL;
				}
				return ESUCCESS;
#else
				if (private->path_info)
				{
					free(private->path_info);
					private->path_info = NULL;
				}
				return EREJECT;
#endif
			}
			char ext_str[64];
			ext_str[63] = 0;
			strncpy(ext_str, config->accepted_ext, 63);
			char *ext = ext_str;
			char *ext_end = strchr(ext, ',');
			if (ext_end)
				*ext_end = 0;
			
			while(ext != NULL)
			{
				filepath = utils_buildpath(config->docroot, private->path_info, "index", ext, &filestat);
				if (filepath && !S_ISDIR(filestat.st_mode))
				{
					ret = ECONTINUE;
					break;
				}
				else if (ext_end)
					ext = ext_end + 1;
				else
				{
					break;
				}
				ext_end = strchr(ext, ',');
				if (ext_end)
					*ext_end = 0;
			}
		}
		else
			ret = ECONTINUE;
		if (ret != ECONTINUE)
		{
			dbg("static file: %s not found (%s)", private->path_info, strerror(errno));
			if (filepath)
				free(filepath);
			if (private->path_info)
			{
				free(private->path_info);
				private->path_info = NULL;
			}
			return EREJECT;
		}
		if (utils_searchext(filepath, config->ignored_ext) == ESUCCESS)
		{
			warn("static file: forbidden extension");
			free(filepath);
			if (private->path_info)
			{
				free(private->path_info);
				private->path_info = NULL;
			}
			return  EREJECT;
		}
		private->size = filestat.st_size;

		/**
		 * file is found
		 * check the extension
		 */
		const char *mime = NULL;
		if (utils_searchext(filepath, config->accepted_ext) == ESUCCESS)
		{
			mime = utils_getmime(filepath);
		}
		else
		{
			warn("static file: forbidden extension");
			free(filepath);
			if (private->path_info)
			{
				free(private->path_info);
				private->path_info = NULL;
			}
			return  EREJECT;
		}
		private->fd = open(filepath, O_RDONLY);
		private->offset = 0;
		if (private->fd < 0)
		{
			ret = EREJECT;
			if (private->path_info)
			{
				free(private->path_info);
				private->path_info = NULL;
			}
		}
		else
		{
			dbg("static file: send %s (%d)", filepath, private->size);
			httpmessage_addcontent(response, (char *)mime, NULL, private->size);
		}
		free(filepath);
		return ret;
	}
	else
	{
		ret = mod->transfer(private, response);
		if (ret < 1)
		{
			close(private->fd);
			private->fd = 0;
			if (ret == 0)
			{
				if (private->path_info)
				{
					free(private->path_info);
					private->path_info = NULL;
				}
				return ESUCCESS;
			}
			else
				return EREJECT;
		}
		else
			private->offset += ret;
	}
	return ECONTINUE;
}

int mod_send_read(static_file_connector_t *private, http_message_t *response)
{
	int ret, size;

	char content[CONTENTCHUNK];
	size = sizeof(content) - 1;
	ret = read(private->fd, content, size);
	if (ret > 0)
	{
		content[size] = 0;
		httpmessage_addcontent(response, NULL, content, ret);
	}
	return ret;
}

static void *_mod_static_file_getctx(void *arg, http_client_t *ctl, struct sockaddr *addr, int addrsize)
{
	_mod_static_file_mod_t *mod = (_mod_static_file_mod_t *)arg;
	mod_static_file_t *config = mod->config;
	static_file_connector_t *ctx = calloc(1, sizeof(*ctx));

	ctx->mod = mod;
	int length;
	char *ext = config->transfertype;

	while (ext != NULL)
	{
		length = strlen(ext);
		char *ext_end = strchr(ext, ',');
		if (ext_end)
		{
			length -= strlen(ext_end + 1) + 1;
			ext_end++;
		}
#ifdef SENDFILE
		if (!strncmp(ext, "sendfile", length))
		{
			mod->transfer = mod_send_sendfile;
		}
#endif
#ifdef DIRLISTING
		if (!strncmp(ext, "dirlisting", length))
		{
			httpclient_addconnector(ctl, mod->vhost, dirlisting_connector, ctx);
		}
#endif
		ext = ext_end;
	}
	httpclient_addconnector(ctl, mod->vhost, static_file_connector, ctx);

	return ctx;
}

static void _mod_static_file_freectx(void *vctx)
{
	static_file_connector_t *ctx = vctx;
	if (ctx->path_info)
		free(ctx->path_info);
	free(ctx);
}

void *mod_static_file_create(http_server_t *server, char *vhost, mod_static_file_t *config)
{
	_mod_static_file_mod_t *mod = calloc(1, sizeof(*mod));

	if (!config)
		config = &default_config;
	if (!config->docroot)
		config->docroot = default_config.docroot;
	if (!config->accepted_ext)
		config->accepted_ext = default_config.accepted_ext;
	if (!config->ignored_ext)
		config->ignored_ext = default_config.ignored_ext;
	mod->config = config;
	mod->vhost = vhost;
	mod->transfer = mod_send_read;
	httpserver_addmod(server, _mod_static_file_getctx, _mod_static_file_freectx, mod);

	return mod;
}

void mod_static_file_destroy(void *data)
{
	free(data);
}
