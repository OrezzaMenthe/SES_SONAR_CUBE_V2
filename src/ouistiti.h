/*****************************************************************************
 * config.h: configuration file parser
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

#ifndef __OUISTITI_CONFIG_H__
#define __OUISTITI_CONFIG_H__

#ifndef MAX_SERVERS
#define MAX_SERVERS 4
#endif

typedef struct server_s server_t;

typedef struct module_s module_t;
typedef struct modulesconfig_s modulesconfig_t;

#define WEBSOCKET_REALTIME 0x01

#define MODULE_VERSION_CURRENT 0x0000
#define MODULE_VERSION_DEPRECATED 0x8000
typedef void *(*module_configure_t)(void *parser, server_t *server);
typedef void *(*module_create_t)(http_server_t *server, void *config);
struct module_s
{
	const char *name;
	void *(*configure)(void *parser, server_t *server);
	void *(*create)(http_server_t *server, void *config);
	void (*destroy)(void*);
	unsigned short version;
};

typedef struct serverconfig_s
{
	http_server_config_t *server;
} serverconfig_t;

typedef struct ouistiticonfig_s
{
	char *user;
	char *pidfile;
	serverconfig_t *servers[MAX_SERVERS];
} ouistiticonfig_t;

ouistiticonfig_t *ouistiticonfig_create(const char *filepath, int serverid);
void ouistiticonfig_destroy(ouistiticonfig_t *ouistiticonfig);

int ouistiti_initmodules(void);
server_t *ouistiti_loadserver(serverconfig_t *config);
typedef void *(*configure_t)(void *data, const char *name, server_t *server);
int ouistiti_setmodules(server_t *server, configure_t configure, void *parser);
void ouistiti_registermodule(const module_t *module);
int ouistiti_issecure(server_t *server);
http_server_t *ouistiti_httpserver(server_t *server);
#endif
