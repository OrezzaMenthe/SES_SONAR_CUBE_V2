/*****************************************************************************
 * cgi_env.c: Generate environment variables for CGI
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
#include <libgen.h>

#include "httpserver/httpserver.h"
#include "mod_cgi.h"
#include "mod_auth.h"

#define err(format, ...) fprintf(stderr, "\x1B[31m"format"\x1B[0m\n",  ##__VA_ARGS__)
#ifdef DEBUG
#define warn(format, ...) fprintf(stderr, "\x1B[35m"format"\x1B[0m\n",  ##__VA_ARGS__)
#define dbg(format, ...) fprintf(stderr, "\x1B[32m"format"\x1B[0m\n",  ##__VA_ARGS__)
#else
#define warn(...)
#define dbg(...)
#endif

#define cgi_dbg(...)

static char str_null[] = "";
static char str_gatewayinterface[] = "CGI/1.1";
static char str_contenttype[] = "Content-Type";

#define ENV_NOTREQUIRED 0x01
typedef const char *(*httpenv_callback_t)(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path);
struct httpenv_s
{
	int id;
	char *target;
	int length;
	int options;
	httpenv_callback_t cb;
};
typedef struct httpenv_s httpenv_t;

const char *env_gatewayinterface(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return str_gatewayinterface;
}

const char *env_docroot(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return config->docroot;
}

const char *env_serversoftware(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_SERVER(request, "software");
}

const char *env_servername(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_SERVER(request, "name");
}

const char *env_serverprotocol(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_SERVER(request, "protocol");
}

const char *env_serveraddr(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_SERVER(request, "addr");
}

const char *env_serverport(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_SERVER(request, "port");
}

const char *env_requestmethod(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "method");
}

const char *env_requestscheme(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "scheme");
}

const char *env_requesturi(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "uri");
}

const char *env_requestcontentlength(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "Content-Length");
}

const char *env_requestcontenttype(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "Content-Type");
}

const char *env_requestquery(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "query");
}

const char *env_requestaccept(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "Accept");
}

const char *env_requestacceptencoding(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "Accept-Encoding");
}

const char *env_requestacceptlanguage(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "Accept-Language");
}

const char *env_requestcookie(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "Cookie");
}

const char *env_requestuseragent(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "User-Agent");
}

const char *env_requesthost(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "Host");
}

const char *env_requestreferer(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "Referer");
}

const char *env_requestorigin(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "Origin");
}

const char *env_remotehost(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	const char *value = httpmessage_REQUEST(request, "remote_host");
	if (value == NULL)
		value = httpmessage_REQUEST(request, "remote_addr");
	return value;
}

const char *env_remoteaddr(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "remote_addr");
}

const char *env_remoteport(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return httpmessage_REQUEST(request, "remote_port");
}

const char *env_authuser(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return auth_info(request, "user");
}

const char *env_authgroup(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return auth_info(request, "group");
}

const char *env_authtype(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path)
{
	return auth_info(request, "type");
}

enum cgi_env_e
{
	PATH_INFO =  1,
	PATH_TRANSLATED,
	SCRIPT_FILENAME,
	SCRIPT_NAME,
	HTTPS,
};
static const httpenv_t cgi_env[] =
{
	{
		.id = PATH_INFO,
		.target = "PATH_INFO=",
		.length = 512,
	},
	{
		.id = PATH_INFO,
		.target = "PATH_TRANSLATED=",
		.length = 512,
	},
	{
		.id = SCRIPT_FILENAME,
		.target = "SCRIPT_FILENAME=",
		.length = 512,
	},
	{
		. id = SCRIPT_NAME,
		.target = "SCRIPT_NAME=",
		.length = 64,
	},
	{
		.id = -1,
		.target = "DOCUMENT_ROOT=",
		.length = 512,
		.cb = &env_docroot,
	},
	{
		.id = -1,
		.target = "SERVER_SOFTWARE=",
		.length = 26,
		.cb = &env_serversoftware,

	},
	{
		.id = -1,
		.target = "SERVER_NAME=",
		.length = 26,
		.cb = &env_servername,
	},
	{
		.id = -1,
		.target = "GATEWAY_INTERFACE=",
		.length = 26,
		.cb = &env_gatewayinterface,
	},
	{
		.id = -1,
		.target = "SERVER_PROTOCOL=",
		.length = 10,
		.cb = &env_serverprotocol,
	},
	{
		.id = -1,
		.target = "SERVER_ADDR=",
		.length = 26,
		.cb = &env_serveraddr,
	},
	{
		.id = -1,
		.target = "SERVER_PORT=",
		.length = 6,
		.cb = &env_serverport,
	},
	{
		.id = -1,
		.target = "REQUEST_METHOD=",
		.length = 6,
		.cb = &env_requestmethod,
	},
	{
		.id = -1,
		.target = "REQUEST_SCHEME=",
		.length = 6,
		.cb = &env_requestscheme,
	},
	{
		.id = -1,
		.target = "REQUEST_URI=",
		.length = 512,
		.cb = &env_requesturi,
	},
	{
		.id = -1,
		.target = "CONTENT_LENGTH=",
		.length = 16,
		.cb = &env_requestcontentlength,
	},
	{
		.id = -1,
		.target = "CONTENT_TYPE=",
		.length = 128,
		.cb = &env_requestcontenttype,
	},
	{
		.id = -1,
		.target = "QUERY_STRING=",
		.length = 512,
		.cb = &env_requestquery,
	},
	{
		.id = -1,
		.target = "HTTP_ACCEPT=",
		.length = 128,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestaccept,
	},
	{
		.id = -1,
		.target = "HTTP_ACCEPT_ENCODING=",
		.length = 128,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestacceptencoding,
	},
	{
		.id = -1,
		.target = "HTTP_ACCEPT_LANGUAGE=",
		.length = 64,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestacceptlanguage,
	},
	{
		.id = -1,
		.target = "REMOTE_HOST=",
		.length = 26,
		.cb = &env_remotehost,
	},
	{
		.id = -1,
		.target = "REMOTE_ADDR=",
		.length = INET6_ADDRSTRLEN,
		.cb = &env_remoteaddr,
	},
	{
		.id = -1,
		.target = "REMOTE_PORT=",
		.length = 26,
		.cb = &env_remoteport,
	},
	{
		.id = -1,
		.target = "REMOTE_USER=",
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authuser,
	},
	{
		.id = -1,
		.target = "AUTH_TYPE=",
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authtype,
	},
	{
		.id = -1,
		.target = "HTTP_COOKIE=",
		.length = 512,
		.cb = &env_requestcookie,
	},
	{
		.id = -1,
		.target = "HTTP_HOST=",
		.length = 512,
		.cb = &env_requesthost,
	},
	{
		.id = -1,
		.target = "HTTP_USER_AGENT=",
		.length = 512,
		.cb = &env_requestuseragent,
	},
	{
		.id = -1,
		.target = "HTTP_REFERER=",
		.length = 512,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestreferer,
	},
	{
		.id = -1,
		.target = "HTTP_ORIGIN=",
		.length = 512,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestorigin,
	},
	{
		.id = -1,
		.target = "AUTH_USER=",
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authuser,
	},
	{
		.id = -1,
		.id = HTTPS,
		.target = "HTTPS=",
		.length = 1,
		.options = ENV_NOTREQUIRED,
	},
	{
		.id = -1,
		.target = "REMOTE_GROUP=",
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authgroup,
	},
	{
		.id = -1,
		.target = "AUTH_GROUP=",
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authgroup,
	}
};

char **cgi_buildenv(mod_cgi_config_t *config, http_message_t *request, const char *cgi_path, const char *path_info)
{
	char **env = NULL;
	int nbenvs = sizeof(cgi_env) / sizeof(*cgi_env);

	env = calloc(sizeof(char *), nbenvs + config->nbenvs + 1);

	int i = 0;
	int j = 0;
	for (i = 0; i < nbenvs; i++)
	{
		int options = cgi_env[i].options;
		int length = strlen(cgi_env[i].target) + cgi_env[i].length;
		env[i] = (char *)calloc(1, length + 1);
		const char *value = NULL;
		int valuelength = -1;
		switch (cgi_env[i].id)
		{
			case SCRIPT_NAME:
			{
				value = cgi_path;
				if (path_info != NULL)
					valuelength = strlen(path_info);
			}
			break;
			case SCRIPT_FILENAME:
				value = cgi_path;
			break;
			case PATH_INFO:
				value = path_info;
			break;
			case PATH_TRANSLATED:
				value = path_info;
			break;
			case HTTPS:
				if (config->options & CGI_OPTION_TLS)
					value = str_null;
			break;
			default:
				if (cgi_env[i].cb != NULL)
					value = cgi_env[i].cb(config, request, cgi_path);
		}
		if ((value == NULL) && (options & ENV_NOTREQUIRED) == 0)
			value = str_null;
		if (value != NULL)
		{
			if (valuelength == -1)
				valuelength = strlen(value);
			snprintf(env[j], length + 1, "%s%.*s", cgi_env[i].target, valuelength, value);
			j++;
		}
	}
	for (i = 0; i < config->nbenvs; i++)
	{
		env[j + i] = strdup(config->env[i]);
	}
	env[j + i] = NULL;
	return env;
}
