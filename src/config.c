/*****************************************************************************
 * config.c: configuration file parser
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <libconfig.h>

#include "httpserver/httpserver.h"
#include "httpserver/utils.h"

#include "httpserver/mod_tls.h"
#include "httpserver/mod_websocket.h"
#include "mod_document.h"
#include "mod_cgi.h"
#include "mod_auth.h"
#include "mod_vhosts.h"
#include "mod_cors.h"

#include "config.h"

#define err(format, ...) fprintf(stderr, "\x1B[31m"format"\x1B[0m\n",  ##__VA_ARGS__)
#define warn(format, ...) fprintf(stderr, "\x1B[35m"format"\x1B[0m\n",  ##__VA_ARGS__)
#ifdef DEBUG
#define dbg(format, ...) fprintf(stderr, "\x1B[32m"format"\x1B[0m\n",  ##__VA_ARGS__)
#else
# define dbg(...)
#endif

static config_t configfile;
static char *logfile = NULL;
static int logfd = 0;

#ifdef DOCUMENT
static const char *str_index = "index.html";
static mod_document_t *document_config(config_setting_t *iterator, int tls, char *entry)
{
	mod_document_t * static_file = NULL;
#if LIBCONFIG_VER_MINOR < 5
	config_setting_t *configstaticfile = config_setting_get_member(iterator, entry);
#else
	config_setting_t *configstaticfile = config_setting_lookup(iterator, entry);
#endif
	if (configstaticfile)
	{
		int length;
		char *transfertype = NULL;
		static_file = calloc(1, sizeof(*static_file));
		config_setting_lookup_string(configstaticfile, "docroot", (const char **)&static_file->docroot);
		config_setting_lookup_string(configstaticfile, "dochome", (const char **)&static_file->dochome);
		config_setting_lookup_string(configstaticfile, "allow", (const char **)&static_file->allow);
		config_setting_lookup_string(configstaticfile, "deny", (const char **)&static_file->deny);
		config_setting_lookup_string(configstaticfile, "defaultpage", (const char **)&static_file->defaultpage);
		if (static_file->defaultpage == NULL)
			static_file->defaultpage = str_index;
		config_setting_lookup_string(configstaticfile, "options", (const char **)&transfertype);
		char *ext = transfertype;

		while (ext != NULL)
		{
			length = strlen(ext);
			char *ext_end = strchr(ext, ',');
			if (ext_end)
			{
				length -= strlen(ext_end + 1) + 1;
				ext_end++;
			}
#ifdef DIRLISTING
			if (!strncmp(ext, "dirlisting", length))
				static_file->options |= DOCUMENT_DIRLISTING;
#endif
#ifdef SENDFILE
			if (!strncmp(ext, "sendfile", length))
			{
				if(!tls)
					static_file->options |= DOCUMENT_SENDFILE;
				else
					warn("sendfile configuration is not allowed with tls");
			}
#endif
#ifdef RANGEREQUEST
			if (!strncmp(ext, "range", length))
			{
				static_file->options |= DOCUMENT_RANGE;
			}
#endif
#ifdef DOCUMENTREST
			if (!strncmp(ext, "rest", length))
			{
				static_file->options |= DOCUMENT_REST;
			}
#endif
#ifdef DOCUMENTHOME
			if (!strncmp(ext, "home", length))
			{
				static_file->options |= DOCUMENT_HOME;
			}
#endif
			ext = ext_end;
		}
	}
	return static_file;
}
#else
#define document_config(...) NULL
#endif


#if defined(TLS)
static mod_tls_t *tls_config(config_setting_t *iterator)
{
	mod_tls_t *tls = NULL;
#if LIBCONFIG_VER_MINOR < 5
	config_setting_t *configtls = config_setting_get_member(iterator, "tls");
#else
	config_setting_t *configtls = config_setting_lookup(iterator, "tls");
#endif
	if (configtls)
	{
		tls = calloc(1, sizeof(*tls));
		config_setting_lookup_string(configtls, "crtfile", (const char **)&tls->crtfile);
		config_setting_lookup_string(configtls, "pemfile",(const char **) &tls->pemfile);
		config_setting_lookup_string(configtls, "cachain", (const char **)&tls->cachain);
		config_setting_lookup_string(configtls, "dhmfile", (const char **)&tls->dhmfile);
	}
	return tls;
}
#else
#define tls_config(...) NULL
#endif

#ifdef CLIENTFILTER
static mod_clientfilter_t *clientfilter_config(config_setting_t *iterator, int tls)
{
	mod_clientfilter_t *clientfilter = NULL;
#if LIBCONFIG_VER_MINOR < 5
	config_setting_t *config = config_setting_get_member(iterator, "clientfilter");
#else
	config_setting_t *config = config_setting_lookup(iterator, "clientfilter");
#endif
	if (config)
	{
		clientfilter = calloc(1, sizeof(*clientfilter));
		config_setting_lookup_string(config, "allow", (const char **)&clientfilter->accept);
		config_setting_lookup_string(config, "deny", (const char **)&clientfilter->deny);
	}
	return clientfilter;
}
#else
#define clientfilter_config(...) NULL
#endif

#ifdef AUTH
#ifdef MODULES
const char *str_authenticate_types[] =
{
	"None",
	"Basic",
	"Digest",
	"Bearer",
	"oAuth2",
};
#endif
static const char *str_realm = "ouistiti";
static mod_auth_t *auth_config(config_setting_t *iterator, int tls)
{
	mod_auth_t *auth = NULL;
#if LIBCONFIG_VER_MINOR < 5
	config_setting_t *configauth = config_setting_get_member(iterator, "auth");
#else
	config_setting_t *configauth = config_setting_lookup(iterator, "auth");
#endif
	if (configauth)
	{
		auth = calloc(1, sizeof(*auth));
		config_setting_lookup_string(configauth, "signin", &auth->redirect);
		config_setting_lookup_string(configauth, "protect", &auth->protect);
		config_setting_lookup_string(configauth, "unprotect", &auth->unprotect);
		/**
		 * algorithm allow to change secret algorithm used during authentication default is md5. (see authn_digest.c)
		 */
		config_setting_lookup_string(configauth, "algorithm", (const char **)&auth->algo);
		/**
		 * secret is the secret used during the token generation. (see authz_jwt.c)
		 */
		config_setting_lookup_string(configauth, "secret", (const char **)&auth->secret);

		char *mode = NULL;
		config_setting_lookup_string(configauth, "options", (const char **)&mode);
		if (mode && strstr(mode, "home") != NULL)
			auth->authz_type |= AUTHZ_HOME_E;
		if (mode && strstr(mode, "cookie") != NULL)
			auth->authz_type |= AUTHZ_COOKIE_E;
		if (mode && strstr(mode, "header") != NULL)
			auth->authz_type |= AUTHZ_HEADER_E;
		if (mode && strstr(mode, "token") != NULL)
			auth->authz_type |= AUTHZ_TOKEN_E;
		config_setting_lookup_int(configauth, "expire", &auth->expire);

#ifdef AUTHZ_UNIX
		if (auth->authz_config == NULL)
		{
			char *path = NULL;

			config_setting_lookup_string(configauth, "file", (const char **)&path);
			if (path != NULL && path[0] != '0' && strstr(path, "shadow"))
			{
				authz_file_config_t *authz_config = calloc(1, sizeof(*authz_config));
				authz_config->path = path;
				auth->authz_type |= AUTHZ_UNIX_E;
				auth->authz_config = authz_config;
			}
		}
#endif
#ifdef AUTHZ_FILE
		if (auth->authz_config == NULL)
		{
			char *path = NULL;

			config_setting_lookup_string(configauth, "file", (const char **)&path);
			if (path != NULL && path[0] != '0')
			{
				authz_file_config_t *authz_config = calloc(1, sizeof(*authz_config));
				authz_config->path = path;
				auth->authz_type |= AUTHZ_FILE_E;
				auth->authz_config = authz_config;
			}
		}
#endif
#ifdef AUTHZ_SQLITE
		if (auth->authz_config == NULL)
		{
			char *path = NULL;

			config_setting_lookup_string(configauth, "dbname", (const char **)&path);
			if (path != NULL && path[0] != '0')
			{
				authz_sqlite_config_t *authz_config = calloc(1, sizeof(*authz_config));
				authz_config->dbname = path;
				auth->authz_type |= AUTHZ_SQLITE_E;
				auth->authz_config = authz_config;
			}
		}
#endif
#ifdef AUTHZ_SIMPLE
		if (auth->authz_config == NULL)
		{
			char *user = NULL;
			config_setting_lookup_string(configauth, "user", (const char **)&user);
			if (user != NULL && user[0] != '0')
			{
				char *passwd = NULL;
				char *group = NULL;
				char *home = NULL;
				config_setting_lookup_string(configauth, "passwd", (const char **)&passwd);
				config_setting_lookup_string(configauth, "group", (const char **)&group);
				config_setting_lookup_string(configauth, "home", (const char **)&home);
				authz_simple_config_t *authz_config = calloc(1, sizeof(*authz_config));
				authz_config->user = user;
				authz_config->group = group;
				authz_config->home = home;
				authz_config->passwd = passwd;
				auth->authz_type |= AUTHZ_SIMPLE_E;
				auth->authz_config = authz_config;
			}
		}
#endif

		char *type = NULL;
		config_setting_lookup_string(configauth, "type", (const char **)&type);
#ifdef AUTHN_NONE
		if (type != NULL && !strncasecmp(type, str_authenticate_types[AUTHN_NONE_E], 4))
		{
			authn_none_config_t *authn_config = calloc(1, sizeof(*authn_config));
			auth->authn_type = AUTHN_NONE_E;
			config_setting_lookup_string(configauth, "user", (const char **)&authn_config->user);
			auth->authn_config = authn_config;
		}
#endif
#ifdef AUTHN_BASIC
		if (type != NULL && !strncasecmp(type, str_authenticate_types[AUTHN_BASIC_E], 5))
		{
			authn_basic_config_t *authn_config = calloc(1, sizeof(*authn_config));
			auth->authn_type = AUTHN_BASIC_E;
			config_setting_lookup_string(configauth, "realm", (const char **)&authn_config->realm);
			if (authn_config->realm == NULL)
				authn_config->realm = (char *)str_realm;
			auth->authn_config = authn_config;
		}
#endif
#ifdef AUTHN_DIGEST
		if (type != NULL && !strncasecmp(type, str_authenticate_types[AUTHN_DIGEST_E], 5))
		{
			authn_digest_config_t *authn_config = calloc(1, sizeof(*authn_config));
			auth->authn_type = AUTHN_DIGEST_E;
			config_setting_lookup_string(configauth, "realm", (const char **)&authn_config->realm);
			if (authn_config->realm == NULL)
				authn_config->realm = (char *)str_realm;
			config_setting_lookup_string(configauth, "opaque", (const char **)&authn_config->opaque);
			auth->authn_config = authn_config;
		}
#endif
#ifdef AUTHN_BEARER
		if (type != NULL && !strncasecmp(type, str_authenticate_types[AUTHN_BEARER_E], 6))
		{
			authn_bearer_config_t *authn_config = calloc(1, sizeof(*authn_config));
			auth->authn_type = AUTHN_BEARER_E;
			config_setting_lookup_string(configauth, "realm", (const char **)&authn_config->realm);
			if (authn_config->realm == NULL)
				authn_config->realm = (char *)str_realm;
			/**
			 * token_ep and signin are not compatible
			 */
			auth->redirect = NULL;
			config_setting_lookup_string(configauth, "token_ep", (const char **)&authn_config->token_ep);
			if (authn_config->token_ep == NULL)
				config_setting_lookup_string(configauth, "signin", (const char **)&authn_config->token_ep);
			auth->authn_config = authn_config;
#ifdef AUTHZ_JWT
			/**
			 * defautl configuration
			 */
			if (auth->authz_config == NULL)
			{
				authz_jwt_config_t *authz_config = calloc(1, sizeof(*authz_config));
				authz_config->key = auth->secret;
				auth->authz_type |= AUTHZ_JWT_E;
				auth->authz_config = authz_config;
			}
#endif
		}

#endif
#ifdef AUTHN_OAUTH2
		if (type != NULL && !strncasecmp(type, str_authenticate_types[AUTHN_OAUTH2_E], 6))
		{
			authn_oauth2_config_t *authn_config = calloc(1, sizeof(*authn_config));
			auth->authn_type = AUTHN_OAUTH2_E;
			if (authn_config->iss == NULL)
				authn_config->iss = authn_config->realm;
			config_setting_lookup_string(configauth, "realm", (const char **)&authn_config->realm);
			if (authn_config->realm == NULL)
				authn_config->realm = (char *)str_realm;
			config_setting_lookup_string(configauth, "auth_ep", (const char **)&authn_config->auth_ep);
			config_setting_lookup_string(configauth, "token_ep", (const char **)&authn_config->token_ep);
			authn_config->client_passwd = auth->secret;
			authn_config->client_id = authn_config->realm;
			config_setting_lookup_string(configauth, "discovery", (const char **)&authn_config->discovery);
			auth->authn_config = authn_config;
		}
#endif
	}
	return auth;
}
#else
#define auth_config(...) NULL
#endif

#ifdef CGI
static mod_cgi_config_t *cgi_config(config_setting_t *iterator, int tls)
{
	mod_cgi_config_t *cgi = NULL;
#if LIBCONFIG_VER_MINOR < 5
	config_setting_t *configcgi = config_setting_get_member(iterator, "cgi");
#else
	config_setting_t *configcgi = config_setting_lookup(iterator, "cgi");
#endif
	if (configcgi)
	{
		cgi = calloc(1, sizeof(*cgi));
		config_setting_lookup_string(configcgi, "docroot", (const char **)&cgi->docroot);
		config_setting_lookup_string(configcgi, "allow", (const char **)&cgi->allow);
		config_setting_lookup_string(configcgi, "deny", (const char **)&cgi->deny);
		cgi->nbenvs = 0;
		cgi->chunksize = 64;
		config_setting_lookup_int(iterator, "chunksize", &cgi->chunksize);
#if LIBCONFIG_VER_MINOR < 5
		config_setting_t *cgienv = config_setting_get_member(configcgi, "env");
#else
		config_setting_t *cgienv = config_setting_lookup(configcgi, "env");
#endif
		if (cgienv)
		{
			int count = config_setting_length(cgienv);
			int i;
			cgi->env = calloc(sizeof(char *), count);
			for (i = 0; i < count; i++)
			{
				config_setting_t *iterator = config_setting_get_elem(cgienv, i);
				cgi->env[i] = config_setting_get_string(iterator);
			}
			cgi->nbenvs = count;
		}
	}
	return cgi;
}
#else
#define cgi_config(...) NULL
#endif

#ifdef WEBSOCKET
static mod_websocket_t *websocket_config(config_setting_t *iterator, int tls)
{
	mod_websocket_t *ws = NULL;
#if LIBCONFIG_VER_MINOR < 5
	config_setting_t *configws = config_setting_get_member(iterator, "websocket");
#else
	config_setting_t *configws = config_setting_lookup(iterator, "websocket");
#endif
	if (configws)
	{
		char *mode = NULL;
		ws = calloc(1, sizeof(*ws));
		config_setting_lookup_string(configws, "docroot", (const char **)&ws->docroot);
		config_setting_lookup_string(configws, "allow", (const char **)&ws->allow);
		config_setting_lookup_string(configws, "deny", (const char **)&ws->deny);
		config_setting_lookup_string(configws, "options", (const char **)&mode);
		char *ext = mode;

		while (ext != NULL)
		{
			int length;
			length = strlen(ext);
			char *ext_end = strchr(ext, ',');
			if (ext_end)
			{
				length -= strlen(ext_end + 1) + 1;
				ext_end++;
			}
#ifdef WEBSOCKET_RT
			if (!strncmp(ext, "direct", length))
			{
				if (!tls)
					ws->options |= WEBSOCKET_REALTIME;
				else
					warn("realtime configuration is not allowed with tls");
			}
#endif
			ext = ext_end;
		}
	}
	return ws;
}
#else
#define websocket_config(...) NULL
#endif

#ifdef WEBSTREAM
static mod_webstream_t *webstream_config(config_setting_t *iterator, int tls)
{
	mod_webstream_t *ws = NULL;
#if LIBCONFIG_VER_MINOR < 5
	config_setting_t *configws = config_setting_get_member(iterator, "webstream");
#else
	config_setting_t *configws = config_setting_lookup(iterator, "webstream");
#endif
	if (configws)
	{
		char *url = NULL;
		char *mode = NULL;
		ws = calloc(1, sizeof(*ws));
		config_setting_lookup_string(configws, "docroot", (const char **)&ws->docroot);
		config_setting_lookup_string(configws, "deny", (const char **)&ws->deny);
		config_setting_lookup_string(configws, "allow", (const char **)&ws->allow);
		config_setting_lookup_string(configws, "options", (const char **)&mode);
		char *ext = mode;

		while (ext != NULL)
		{
			int length;
			length = strlen(ext);
			char *ext_end = strchr(ext, ',');
			if (ext_end)
			{
				length -= strlen(ext_end + 1) + 1;
				ext_end++;
			}
			if (!strncmp(ext, "direct", length))
			{
				if (!tls)
					ws->options |= WEBSOCKET_REALTIME;
				else
					warn("realtime configuration is not allowed with tls");
			}
			ext = ext_end;
		}
	}
	return ws;
}
#else
#define webstream_config(...) NULL
#endif

#ifdef REDIRECT
static int redirect_mode(const char *mode)
{
	int options = 0;
	const char *ext = mode;

	while (ext != NULL)
	{
		int length;
		length = strlen(ext);
		char *ext_end = strchr(ext, ',');
		if (ext_end)
		{
			length -= strlen(ext_end + 1) + 1;
			ext_end++;
		}
		if (!strncmp(ext, "generate_204", length))
		{
			options |= REDIRECT_GENERATE204;
		}
		else if (!strncmp(ext, "hsts", length))
		{
			options |= REDIRECT_HSTS;
		}
		else if (!strncmp(ext, "temporary", length))
		{
			options |= REDIRECT_TEMPORARY;
		}
		else if (!strncmp(ext, "permanently", length))
		{
			options |= REDIRECT_PERMANENTLY;
		}
		else if (!strncmp(ext, "error", length))
		{
			options |= REDIRECT_ERROR;
		}
		ext = ext_end;
	}
	return options;
}
static mod_redirect_t *redirect_config(config_setting_t *iterator, int tls)
{
	mod_redirect_t *conf = NULL;
#if LIBCONFIG_VER_MINOR < 5
	config_setting_t *config = config_setting_get_member(iterator, "redirect");
#else
	config_setting_t *config = config_setting_lookup(iterator, "redirect");
#endif
	if (config)
	{
		conf = calloc(1, sizeof(*conf));
		char *mode = NULL;
		config_setting_lookup_string(config, "options", (const char **)&mode);
		conf->options = redirect_mode(mode);

		config_setting_t *configlinks = config_setting_lookup(config, "links");
		if (configlinks)
		{
			conf->options |= REDIRECT_LINK;
			int count = config_setting_length(configlinks);
			int i;
			for (i = 0; i < count; i++)
			{
				config_setting_t *iterator = config_setting_get_elem(configlinks, i);
				if (iterator)
				{
					char *destination = NULL;
					const char *origin = NULL;

					config_setting_lookup_string(iterator, "options", (const char **)&mode);
					int options = redirect_mode(mode);

					static char origin_error[4];
					config_setting_t *originset = config_setting_lookup(iterator, "origin");
					if (config_setting_is_number(originset))
					{
						int value;
						value = config_setting_get_int(originset);
						snprintf(origin_error, 4, "%.3d", value);
						origin = origin_error;
						config_setting_set_string(originset, origin_error);
						//originset = config_setting_lookup(iterator, "origin");
						if (value == 204)
							options |= REDIRECT_GENERATE204;
						else
							options |= REDIRECT_ERROR;
					}
					else
						origin = config_setting_get_string(originset);
					config_setting_lookup_string(iterator, "destination", (const char **)&destination);
					if (origin != NULL)
					{
						mod_redirect_link_t *link = calloc(1, sizeof(*link));
						link->origin = strdup(origin);
						link->options = options;
						link->destination = destination;
						link->next = conf->links;
						conf->links = link;
					}
				}
			}
		}
	}
	return conf;
}
#else
#define redirect_config(...) NULL
#endif

#ifdef CORS
static mod_cors_t *cors_config(config_setting_t *iterator, int tls)
{
	mod_cors_t *config = NULL;
#if LIBCONFIG_VER_MINOR < 5
	config_setting_t *config_set = config_setting_get_member(iterator, "cors");
#else
	config_setting_t *config_set = config_setting_lookup(iterator, "cors");
#endif
	if (config_set)
	{
		config = calloc(1, sizeof(*config));
		config_setting_lookup_string(config_set, "origin", (const char **)&config->origin);
	}
	return config;
}
#else
#define cors_config(...) NULL
#endif

#ifdef VHOSTS
#warning VHOSTS is deprecated
static mod_vhost_t *vhost_config(config_setting_t *iterator, int tls)
{
	mod_vhost_t *vhost = NULL;
	char *hostname = NULL;

	config_setting_lookup_string(iterator, "hostname", (const char **)&hostname);
	if (hostname && hostname[0] != '0')
	{
		vhost = calloc(1, sizeof(*vhost));
		vhost->hostname = hostname;
		vhost->modules.document = document_config(iterator, tls, "document");
		if (vhost->modules.document == NULL)
			vhost->modules.document = document_config(iterator, tls, "static_file");
		if (vhost->modules.document == NULL)
		{
			vhost->modules.document = document_config(iterator, tls, "filestorage");
			if (vhost->modules.document != NULL)
				vhost->modules.document->options |= DOCUMENT_REST;
		}
		vhost->modules.auth = auth_config(iterator, tls);
		vhost->modules.cgi = cgi_config(iterator, tls);
		vhost->modules.websocket = websocket_config(iterator, tls);
		vhost->modules.redirect = redirect_config(iterator,tls);
		vhost->modules.cors = cors_config(iterator, tls);
	}
	else
	{
		warn("vhost configuration without hostname");
	}

	return vhost;
}
#else
#define vhost_config(...) NULL
#endif

ouistiticonfig_t *ouistiticonfig_create(char *filepath)
{
	int ret;
	ouistiticonfig_t *ouistiticonfig = NULL;

	config_init(&configfile);
	dbg("config file: %s", filepath);
	ret = config_read_file(&configfile, filepath);
	if (ret == CONFIG_TRUE)
	{
		ouistiticonfig = calloc(1, sizeof(*ouistiticonfig));

		config_lookup_string(&configfile, "user", (const char **)&ouistiticonfig->user);
		config_lookup_string(&configfile, "log-file", (const char **)&logfile);
		if (logfile != NULL && logfile[0] != '\0')
		{
			logfd = open(logfile, O_WRONLY | O_CREAT | O_TRUNC, 00644);
			if (logfd > 0)
			{
				dup2(logfd, 1);
				dup2(logfd, 2);
				close(logfd);
			}
			else
				err("log file error %s", strerror(errno));
		}
		config_lookup_string(&configfile, "pid-file", (const char **)&ouistiticonfig->pidfile);
		config_setting_t *configmimes = config_lookup(&configfile, "mimetypes");
		if (configmimes)
		{
			int count = config_setting_length(configmimes);
			int i;
			for (i = 0; i < count && i < MAXSERVERS; i++)
			{
				char *ext = NULL;
				char *mime = NULL;
				config_setting_t *iterator = config_setting_get_elem(configmimes, i);
				if (iterator)
				{
					config_setting_lookup_string(iterator, "ext", (const char **)&ext);
					config_setting_lookup_string(iterator, "mime", (const char **)&mime);
					if (mime != NULL && ext != NULL)
					{
						utils_addmime(ext, mime);
					}
				}
			}
		}
		config_setting_t *configservers = config_lookup(&configfile, "servers");
		if (configservers)
		{
			int count = config_setting_length(configservers);
			int i;

			for (i = 0; i < count && i < MAX_SERVERS; i++)
			{
				config_setting_t *iterator = config_setting_get_elem(configservers, i);
				if (iterator)
				{
					ouistiticonfig->servers[i] = calloc(1, sizeof(*ouistiticonfig->servers[i]));
					serverconfig_t *config = ouistiticonfig->servers[i];

					config->server = calloc(1, sizeof(*config->server));

					config_setting_lookup_string(iterator, "hostname", (const char **)&config->server->hostname);
					if (config->server->hostname && strchr(config->server->hostname, '.') == NULL)
					{
						err("hostname must contain the domain");
					}
					config->server->port = 80;
					config_setting_lookup_int(iterator, "port", &config->server->port);
					config_setting_lookup_string(iterator, "addr", (const char **)&config->server->addr);
					config_setting_lookup_int(iterator, "keepalivetimeout", &config->server->keepalive);
					config->server->chunksize = DEFAULT_CHUNKSIZE;
					config_setting_lookup_int(iterator, "chunksize", &config->server->chunksize);
					config->server->maxclients = DEFAULT_MAXCLIENTS;
					config_setting_lookup_int(iterator, "maxclients", &config->server->maxclients);
					config->server->version = HTTP11;
					const char *version = NULL;
					config_setting_lookup_string(iterator, "version", &version);
					if (version)
					{
						int i = 0;
						for (i = 0; httpversion[i] != NULL; i++)
						{
							if (!strcmp(version,  httpversion[i]))
							{
								config->server->version = i;
								break;
							}
						}
					}
					config->server->versionstr = httpversion[config->server->version];

					config_setting_lookup_string(iterator, "unlock_groups", (const char **)&config->unlock_groups);
					config->tls = tls_config(iterator);
					config->modules.document = document_config(iterator,(config->tls!=NULL), "document");
					if (config->modules.document == NULL)
						config->modules.document = document_config(iterator,(config->tls!=NULL), "static_file");
					if (config->modules.document == NULL)
					{
						config->modules.document = document_config(iterator,(config->tls!=NULL), "filestorage");
						if (config->modules.document != NULL)
							config->modules.document->options |= DOCUMENT_REST;
					}
					config->modules.auth = auth_config(iterator,(config->tls!=NULL));
					config->modules.clientfilter = clientfilter_config(iterator,(config->tls!=NULL));
					config->modules.cgi = cgi_config(iterator,(config->tls!=NULL));
					config->modules.websocket = websocket_config(iterator,(config->tls!=NULL));
					config->modules.redirect = redirect_config(iterator,(config->tls!=NULL));
					config->modules.cors = cors_config(iterator,(config->tls!=NULL));
					config->modules.webstream = webstream_config(iterator,(config->tls!=NULL));
#ifdef VHOSTS
#if LIBCONFIG_VER_MINOR < 5
					config_setting_t *configvhosts = config_setting_get_member(iterator, "vhosts");
#else
					config_setting_t *configvhosts = config_setting_lookup(iterator, "vhosts");
#endif
					if (configvhosts)
					{
						int count = config_setting_length(configvhosts);
						int j;

						for (j = 0; j < count && (j + i) < MAX_SERVERS; j++)
						{
							config_setting_t *iterator = config_setting_get_elem(configvhosts, j);
							config->vhosts[j] = vhost_config(iterator,(config->tls!=NULL));
						}
					}
#endif
				}
			}
			ouistiticonfig->servers[i] = NULL;
		}

	}
	else
		printf("%s\n", config_error_text(&configfile));
	return ouistiticonfig;
}

static void _modulesconfig_destroy(modulesconfig_t *config)
{
	if (config->document)
		free(config->document);
	if (config->websocket)
		free(config->websocket);
	if (config->redirect)
	{
		mod_redirect_link_t *link = config->redirect->links;
		while (link != NULL)
		{
			mod_redirect_link_t *old = link->next;
			free(link);
			link = old;
		}
		free(config->redirect);
	}
	if (config->auth)
	{
		if (config->auth->authn_config)
			free(config->auth->authn_config);
		if (config->auth->authz_config)
			free(config->auth->authz_config);
		free(config->auth);
	}
	if (config->cgi)
	{
		if (config->cgi->env)
			free(config->cgi->env);
		free(config->cgi);
	}
}

void ouistiticonfig_destroy(ouistiticonfig_t *ouistiticonfig)
{
	int i;

	if (logfd > 0)
		close(logfd);
	config_destroy(&configfile);

	for (i = 0; i < MAX_SERVERS; i++)
	{
		serverconfig_t *config = ouistiticonfig->servers[i];
		if (config)
		{
			_modulesconfig_destroy(&config->modules);
			int j;
			for (j = 0; j < MAX_SERVERS; j++)
			{
				if (config->vhosts[j])
				{
					_modulesconfig_destroy(&config->vhosts[j]->modules);
					free(config->vhosts[j]);
				}
				else
					break;
			}
			if (config->tls)
				free(config->tls);
			free(config->server);
			free(config);
			ouistiticonfig->servers[i] = NULL;
		}
	}

	free(ouistiticonfig);
}
