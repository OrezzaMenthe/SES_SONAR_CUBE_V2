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

#include "ouistiti/httpserver.h"
#include "ouistiti/log.h"
#include "ouistiti.h"
#include "mod_cgi.h"
#include "mod_auth.h"

#define cgi_dbg(...)

static char str_null[] = "";
static char str_gatewayinterface[] = "CGI/1.1";

#define ENV_NOTREQUIRED 0x01
typedef size_t (*httpenv_callback_t)(const mod_cgi_config_t *config, http_message_t *request, const char *cgi_path, const char **value);
struct httpenv_s
{
	int id;
	string_t target;
	int length;
	int options;
	httpenv_callback_t cb;
};
typedef struct httpenv_s httpenv_t;

size_t env_gatewayinterface(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	*value = str_gatewayinterface;
	return sizeof(str_gatewayinterface) - 1;
}

size_t env_docroot(const mod_cgi_config_t *config, http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	*value = config->docroot;
	return -1;
}

size_t env_serversoftware(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "software", value);
}

size_t env_servername(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "name", value);
}

size_t env_serverprotocol(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "protocol", value);
}

size_t env_serveraddr(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "addr", value);
}

size_t env_serverport(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "port", value);
}

size_t env_serverservice(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "service", value);
}

size_t env_requestmethod(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "method", value);
}

size_t env_requestscheme(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "scheme", value);
}

size_t env_requesturi(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "uri", value);
}

size_t env_requestcontentlength(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "Content-Length", value);
}

size_t env_requestcontenttype(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "Content-Type", value);
}

size_t env_requestquery(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "query", value);
}

size_t env_requestaccept(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "Accept", value);
}

size_t env_requestacceptencoding(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "Accept-Encoding", value);
}

size_t env_requestacceptlanguage(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "Accept-Language", value);
}

size_t env_requestcookie(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "Cookie", value);
}

size_t env_requestuseragent(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "User-Agent", value);
}

size_t env_requesthost(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "Host", value);
}

size_t env_requestreferer(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "Referer", value);
}

size_t env_requestorigin(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "Origin", value);
}

size_t env_remotehost(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	size_t valuelen = httpmessage_REQUEST2(request, "remote_host", value);
	if (valuelen == 0)
		valuelen = httpmessage_REQUEST2(request, "remote_addr", value);
	return valuelen;
}

size_t env_remoteaddr(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "remote_addr", value);
}

size_t env_remoteport(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return httpmessage_REQUEST2(request, "remote_port", value);
}

size_t env_authuser(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return auth_info2(request, ("user"), value);
}

size_t env_authgroup(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return auth_info2(request, ("group"), value);
}

size_t env_authtype(const mod_cgi_config_t *UNUSED(config), http_message_t *request, const char *UNUSED(cgi_path), const char **value)
{
	return auth_info2(request, ("authtype"), value);
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
		.id = -1,
		.target = STRING_DCL("CONTENT_LENGTH="),
		.length = 16,
		.cb = &env_requestcontentlength,
	},
	{
		.id = -1,
		.target = STRING_DCL("GATEWAY_INTERFACE="),
		.length = 26,
		.cb = &env_gatewayinterface,
	},
	{
		.id = PATH_INFO,
		.target = STRING_DCL("PATH_INFO="),
		.length = 512,
	},
	{
		.id = PATH_INFO,
		.target = STRING_DCL("PATH_TRANSLATED="),
		.length = 512,
	},
	{
		.id = SCRIPT_FILENAME,
		.target = STRING_DCL("SCRIPT_FILENAME="),
		.length = 512,
	},
	{
		. id = SCRIPT_NAME,
		.target = STRING_DCL("SCRIPT_NAME="),
		.length = 64,
	},
	{
		.id = -1,
		.target = STRING_DCL("DOCUMENT_ROOT="),
		.length = 512,
		.cb = &env_docroot,
	},
	{
		.id = -1,
		.target = STRING_DCL("SERVER_SOFTWARE="),
		.length = 26,
		.cb = &env_serversoftware,

	},
	{
		.id = -1,
		.target = STRING_DCL("SERVER_NAME="),
		.length = 26,
		.cb = &env_servername,
	},
	{
		.id = -1,
		.target = STRING_DCL("SERVER_PROTOCOL="),
		.length = 10,
		.cb = &env_serverprotocol,
	},
	{
		.id = -1,
		.target = STRING_DCL("SERVER_ADDR="),
		.length = 26,
		.cb = &env_serveraddr,
	},
	{
		.id = -1,
		.target = STRING_DCL("SERVER_PORT="),
		.length = 6,
		.cb = &env_serverport,
	},
	{
		.id = -1,
		.target = STRING_DCL("SERVER_SERVICE="),
		.length = 26,
		.cb = &env_serverservice,
	},
	{
		.id = -1,
		.target = STRING_DCL("REQUEST_METHOD="),
		.length = 6,
		.cb = &env_requestmethod,
	},
	{
		.id = -1,
		.target = STRING_DCL("REQUEST_SCHEME="),
		.length = 6,
		.cb = &env_requestscheme,
	},
	{
		.id = -1,
		.target = STRING_DCL("REQUEST_URI="),
		.length = 512,
		.cb = &env_requesturi,
	},
	{
		.id = -1,
		.target = STRING_DCL("CONTENT_TYPE="),
		.length = 128,
		.cb = &env_requestcontenttype,
	},
	{
		.id = -1,
		.target = STRING_DCL("QUERY_STRING="),
		.length = 512,
		.cb = &env_requestquery,
	},
	{
		.id = -1,
		.target = STRING_DCL("HTTP_ACCEPT="),
		.length = 128,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestaccept,
	},
	{
		.id = -1,
		.target = STRING_DCL("HTTP_ACCEPT_ENCODING="),
		.length = 128,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestacceptencoding,
	},
	{
		.id = -1,
		.target = STRING_DCL("HTTP_ACCEPT_LANGUAGE="),
		.length = 64,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestacceptlanguage,
	},
	{
		.id = -1,
		.target = STRING_DCL("REMOTE_HOST="),
		.length = 26,
		.cb = &env_remotehost,
	},
	{
		.id = -1,
		.target = STRING_DCL("REMOTE_ADDR="),
		.length = INET6_ADDRSTRLEN,
		.cb = &env_remoteaddr,
	},
	{
		.id = -1,
		.target = STRING_DCL("REMOTE_PORT="),
		.length = 26,
		.cb = &env_remoteport,
	},
	{
		.id = -1,
		.target = STRING_DCL("REMOTE_USER="),
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authuser,
	},
	{
		.id = -1,
		.target = STRING_DCL("AUTH_TYPE="),
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authtype,
	},
	{
		.id = -1,
		.target = STRING_DCL("HTTP_COOKIE="),
		.length = 512,
		.cb = &env_requestcookie,
	},
	{
		.id = -1,
		.target = STRING_DCL("HTTP_HOST="),
		.length = 512,
		.cb = &env_requesthost,
	},
	{
		.id = -1,
		.target = STRING_DCL("HTTP_USER_AGENT="),
		.length = 512,
		.cb = &env_requestuseragent,
	},
	{
		.id = -1,
		.target = STRING_DCL("HTTP_REFERER="),
		.length = 512,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestreferer,
	},
	{
		.id = -1,
		.target = STRING_DCL("HTTP_ORIGIN="),
		.length = 512,
		.options = ENV_NOTREQUIRED,
		.cb = &env_requestorigin,
	},
	{
		.id = -1,
		.target = STRING_DCL("AUTH_USER="),
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authuser,
	},
	{
		.id = HTTPS,
		.target = STRING_DCL("HTTPS="),
		.length = 1,
		.options = ENV_NOTREQUIRED,
	},
	{
		.id = -1,
		.target = STRING_DCL("REMOTE_GROUP="),
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authgroup,
	},
	{
		.id = -1,
		.target = STRING_DCL("AUTH_GROUP="),
		.length = 26,
		.options = ENV_NOTREQUIRED,
		.cb = &env_authgroup,
	}
};

char **cgi_buildenv(const mod_cgi_config_t *config, http_message_t *request, const char *cgi_path, const char *path_info)
{
	char **env = NULL;
	int nbenvs = sizeof(cgi_env) / sizeof(*cgi_env);

	env = calloc(sizeof(char *), nbenvs + config->nbenvs + 1);

	int i = 0;
	int j = 0;
	for (i = 0; i < nbenvs; i++)
	{
		int options = cgi_env[i].options;
		size_t length = cgi_env[i].target.length + cgi_env[i].length;
		env[i] = (char *)calloc(1, length + 1);
		const char *value = NULL;
		int valuelength = -1;
		switch (cgi_env[i].id)
		{
			case SCRIPT_NAME:
			case SCRIPT_FILENAME:
				value = cgi_path;
				if (cgi_path != NULL)
					valuelength = (int)strlen(cgi_path);
			break;
			case PATH_INFO:
			case PATH_TRANSLATED:
				value = path_info;
				if (path_info != NULL)
					valuelength = (int)strlen(path_info);
			break;
			case HTTPS:
				if (config->options & CGI_OPTION_TLS)
					value = str_null;
			break;
			default:
				if (cgi_env[i].cb != NULL)
					valuelength = cgi_env[i].cb(config, request, cgi_path, &value);
		}
		if ((value == NULL) && (options & ENV_NOTREQUIRED) == 0)
		{
			value = str_null;
			valuelength = sizeof(str_null) - 1;
		}
		if (value != NULL)
		{
			snprintf(env[j], length + 1, "%s%.*s", cgi_env[i].target.data, valuelength, value);
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
