/*****************************************************************************
 * mod_userfilter.c: callbacks and management of request method
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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <sqlite3.h>

#ifdef FILE_CONFIG
#include <libconfig.h>
#endif

#include "httpserver/httpserver.h"
#include "httpserver/utils.h"
#include "httpserver/log.h"
#include "mod_auth.h"
#include "mod_userfilter.h"

#define userfilter_dbg(...)

#define SQLITE3_CHECK(ret, value, sql) \
	do { \
		if (ret != SQLITE_OK) {\
			err("%s(%d) %d: %s\n%s", __FUNCTION__, __LINE__, ret, sql, sqlite3_errmsg(ctx->db)); \
			sqlite3_finalize(statement); \
			return value; \
		} \
	} while (0)

static const char str_userfilter[] = "userfilter";
static const char str_annonymous[] = "annonymous";
static const char str_userfilterpath[] = SYSCONFDIR"/userfilter.db";

typedef struct _mod_userfilter_s _mod_userfilter_t;

typedef int (*cmp_t)(_mod_userfilter_t *mod, const char *value,
				const char *user, const char *group, const char *home,
				const char *uri);

struct _mod_userfilter_s
{
	sqlite3 *db;
	cmp_t cmp;
	mod_userfilter_t *config;
	int line;
};

int _exp_cmp(_mod_userfilter_t *UNUSED(ctx), const char *value,
				const char *user, const char *group, const char *home,
				const char *uri)
{
	int ret = EREJECT;
	const char *entries[3] = {0};
	int nbentries = 0;

	char *p, *valuefree = strdup(value);
	p = strchr(valuefree, '%');
	while (p != NULL)
	{
		p++;
		switch (*p)
		{
			case 'u':
				*p = 's';
				entries[nbentries] = user;
				nbentries++;
			break;
			case 'g':
				*p = 's';
				entries[nbentries] = group;
				nbentries++;
			break;
			case 'h':
				*p = 's';
				entries[nbentries] = home;
				nbentries++;
			break;
			default:
				free(valuefree);
				return ret;
		}
		p = strchr(p, '%');
		if (nbentries >= 3)
			break;
	}
	char *checking;
	if (asprintf(&checking, valuefree, entries[0], entries[1], entries[2]) < 0)
	{
		free(valuefree);
		return EREJECT;
	}
	userfilter_dbg("userfilter: check %s %s", uri, checking);
	if (utils_searchexp(uri, checking, NULL) == ESUCCESS)
		ret = ESUCCESS;
	free(valuefree);
	free(checking);
	return ret;
}

static int _search_field(_mod_userfilter_t *ctx, int ifield, const char *value, int length)
{
	int ret = EREJECT;
	int step;
	sqlite3_stmt *statement;
	const char *sql[] = {
		"select id from methods where name=@VALUE;",
		"select id from roles where name=@VALUE;",
	};
	step = sqlite3_prepare_v2(ctx->db, sql[ifield], -1, &statement, NULL);
	SQLITE3_CHECK(step, EREJECT, sql[ifield]);

	int index;
	index = sqlite3_bind_parameter_index(statement, "@VALUE");
	step = sqlite3_bind_text(statement, index, value, length, SQLITE_STATIC);
	SQLITE3_CHECK(step, EREJECT, sql[ifield]);

	step = sqlite3_step(statement);
	if (step == SQLITE_ROW)
	{
		ret = sqlite3_column_int(statement,0);
	}
	sqlite3_finalize(statement);
	return ret;
}

static int _search_method(_mod_userfilter_t *ctx, const char *method, int length)
{
	return _search_field(ctx, 0, method, length);
}

static int _search_role(_mod_userfilter_t *ctx, const char *role, int length)
{
	return _search_field(ctx, 1, role, length);
}

static int _request(_mod_userfilter_t *ctx, const char *method,
				const char *user, const char *group, const char *home,
				const char *uri)
{
	int ret = EREJECT;
	int methodid = _search_method(ctx, method, -1);
	int userid = _search_role(ctx, user, -1);
	int groupid = _search_role(ctx, group, -1);
	sqlite3_stmt *statement;
	const char *sql = "select exp from rules " \
		"where methodid=@METHODID and " \
		"(roleid=@USERID or roleid=@GROUPID or roleid=2);";
	ret = sqlite3_prepare_v2(ctx->db, sql, -1, &statement, NULL);
	SQLITE3_CHECK(ret, EREJECT, sql);

	int index;
	index = sqlite3_bind_parameter_index(statement, "@METHODID");
	ret = sqlite3_bind_int(statement, index, methodid);
	SQLITE3_CHECK(ret, EREJECT, sql);

	index = sqlite3_bind_parameter_index(statement, "@USERID");
	ret = sqlite3_bind_int(statement, index, userid);
	SQLITE3_CHECK(ret, EREJECT, sql);

	index = sqlite3_bind_parameter_index(statement, "@GROUPID");
	ret = sqlite3_bind_int(statement, index, groupid);
	SQLITE3_CHECK(ret, EREJECT, sql);

	userfilter_dbg("select exp from rules " \
			"where methodid=%d and " \
			"(roleid=%d or roleid=%d or roleid=2);",
			methodid, userid, groupid);
	ret = EREJECT;
	int step = sqlite3_step(statement);
	while (step == SQLITE_ROW)
	{
		int i = 0;
		if (sqlite3_column_type(statement, i) == SQLITE_TEXT)
		{
			const char *value = NULL;
			value = sqlite3_column_text(statement, i);
			userfilter_dbg("=> %s", value);
			if (!ctx->cmp(ctx, value, user, group, home, uri))
			{
				ret = ESUCCESS;
				break;
			}
		}
		step = sqlite3_step(statement);
	}
	if (step != SQLITE_DONE && ret != ESUCCESS)
		err("request %d %s", ret, sqlite3_errmsg(ctx->db));
	sqlite3_finalize(statement);
	return ret;
}

static int _insert_field(_mod_userfilter_t *ctx, int table, const char *value, int length)
{
	int ret = EREJECT;
	sqlite3_stmt *statement;
	const char *sql[] = {
		"insert into methods (name) values(@VALUE);",
		"insert into roles (name) values(@VALUE);"
	};
	ret = sqlite3_prepare_v2(ctx->db, sql[table], -1, &statement, NULL);
	SQLITE3_CHECK(ret, EREJECT, sql[table]);

	int index;
	index = sqlite3_bind_parameter_index(statement, "@VALUE");
	ret = sqlite3_bind_text(statement, index, value, length, SQLITE_STATIC);
	SQLITE3_CHECK(ret, EREJECT, sql[table]);

	int step = sqlite3_step(statement);
	if (step == SQLITE_DONE)
	{
		ret = sqlite3_last_insert_rowid(ctx->db);
	}
	sqlite3_finalize(statement);
	return ret;
}

static int _insert_method(_mod_userfilter_t *ctx, const char *method, int length)
{
	return _insert_field(ctx, 0, method, length);
}

static int _insert_role(_mod_userfilter_t *ctx, const char *role, int length)
{
	return _insert_field(ctx, 1, role, length);
}

static int _insert_rules(_mod_userfilter_t *ctx, int methodid, int roleid, const char *exp, int length)
{
	int ret = EREJECT;
	sqlite3_stmt *statement;
	const char *sql = "insert into rules (exp,methodid,roleid) values(@EXP,@METHODID,@ROLEID);";
	ret = sqlite3_prepare_v2(ctx->db, sql, -1, &statement, NULL);
	SQLITE3_CHECK(ret, EREJECT, sql);

	int index;
	index = sqlite3_bind_parameter_index(statement, "@METHODID");
	ret = sqlite3_bind_int(statement, index, methodid);
	SQLITE3_CHECK(ret, EREJECT, sql);

	index = sqlite3_bind_parameter_index(statement, "@ROLEID");
	ret = sqlite3_bind_int(statement, index, roleid);
	SQLITE3_CHECK(ret, EREJECT, sql);

	index = sqlite3_bind_parameter_index(statement, "@EXP");
	ret = sqlite3_bind_text(statement, index, exp, length, SQLITE_STATIC);
	SQLITE3_CHECK(ret, EREJECT, sql);

	userfilter_dbg("insert into rules (exp,methodid,roleid) values(%s,%d,%d);",
		exp, methodid, roleid);
	int step = sqlite3_step(statement);
	if (step == SQLITE_DONE)
	{
		ret = ESUCCESS;
	}
	sqlite3_finalize(statement);
	return ret;
}

static int _jsonifyrule(_mod_userfilter_t *ctx, int id, http_message_t *response)
{
	
	int ret = EREJECT;
	sqlite3_stmt *statement;
	const char *sql = "select methods.name as \"mehtod\", roles.name as \"role\", exp as \"pathexp\", rules.rowid as \"id\" " \
				"from rules " \
				"inner join methods on methods.id = rules.methodid " \
				"inner join roles on roles.id = rules.roleid ;";
	ret = sqlite3_prepare_v2(ctx->db, sql, -1, &statement, NULL);
	SQLITE3_CHECK(ret, EREJECT, sql);

	int step = sqlite3_step(statement);
	int j = 1;
	while (step == SQLITE_ROW)
	{
		if (j != id)
		{
			step = sqlite3_step(statement);
			j++;
			continue;
		}
		int i = 0;
		const char *field;
		httpmessage_appendcontent(response, "{\"method\":\"", -1);
		field = sqlite3_column_text(statement, i);
		if (field)
			httpmessage_appendcontent(response, field, -1);
		httpmessage_appendcontent(response, "\",\"role\":\"", -1);
		i++;
		field = sqlite3_column_text(statement, i);
		if (field)
			httpmessage_appendcontent(response, field, -1);
		httpmessage_appendcontent(response, "\",\"pathexp\":\"", -1);
		i++;
		field = sqlite3_column_text(statement, i);
		if (field)
			httpmessage_appendcontent(response, field, -1);
		httpmessage_appendcontent(response, "\",\"id\":\"", -1);
		i++;
		field = sqlite3_column_text(statement, i);
		if (field)
			httpmessage_appendcontent(response, field, -1);
		httpmessage_appendcontent(response, "\"}", -1);
		ret = ECONTINUE;
		break;
	}
	if (step == SQLITE_DONE)
		ret = ESUCCESS;
	sqlite3_finalize(statement);
	return ret;
}

static int userfilter_connector(void *arg, http_message_t *request, http_message_t *response)
{
	_mod_userfilter_t *ctx = (_mod_userfilter_t *)arg;
	const mod_userfilter_t *config = ctx->config;
	int ret = ESUCCESS;
	const char *uri = httpmessage_REQUEST(request,"uri");
	const char *method = httpmessage_REQUEST(request, "method");
	const char *user = auth_info(request, "user");
	if (user == NULL)
		user = str_annonymous;

	if ((utils_searchexp(uri, config->allow, NULL) == ESUCCESS) &&
		(strcmp(uri, config->configuri) != 0))
	{
		/**
		 * this path is always allowed
		 */
		userfilter_dbg("userfilter: forward to allowed path %s", config->allow);
		ret = EREJECT;
	}
	else if (_request(ctx, method, user,
				auth_info(request, "group"),
				auth_info(request, "home"),
				uri) == 0)
	{
		ret = EREJECT;
	}
	else
	{
		warn("userfilter: role %s forbidden for %s", user, uri);
		if (user == str_annonymous)
			httpmessage_result(response, RESULT_401);
		else
#if defined RESULT_403
			httpmessage_result(response, RESULT_403);
#else
			httpmessage_result(response, RESULT_400);
#endif
		ret = ESUCCESS;
	}
	return ret;
}

static int _parsequery(_mod_userfilter_t *ctx, const char *query, int ifield)
{
	const char *field[] = {
		"method=",
		"role=",
	};
	const char *value = strstr(query, field[ifield]);
	if (value == NULL)
		return EREJECT;
	else
		value += strlen(field[ifield]);
	int length = -1;
	const char *end = strchr(value, '&');
	if (end != NULL)
		length = end - value;
	int id = _search_field(ctx, ifield, value, length);
	if (id == EREJECT)
		id = _insert_field(ctx, ifield, value, length);
	return id;
}

static int rootgenerator_connector(void *arg, http_message_t *request, http_message_t *response)
{
	_mod_userfilter_t *ctx = (_mod_userfilter_t *)arg;
	mod_userfilter_t *config = ctx->config;
	int ret = EREJECT;
	const char *uri = httpmessage_REQUEST(request,"uri");
	if (!strcmp(uri, config->configuri))
	{
		const char *method = httpmessage_REQUEST(request, "method");
		const char *query = httpmessage_REQUEST(request, "query");
		if (!strcmp(method, str_post) && query != NULL)
		{
			ret = EREJECT;
			int methodid = _parsequery(ctx, query, 0);
			int roleid = _parsequery(ctx, query, 1);
			const char *exp = strstr(query, "pathexp=");
			if (exp != NULL && methodid != EREJECT && roleid != EREJECT)
			{
				exp += 8;
				int elength = -1;
				const char *end = strchr(exp, '&');
				if (end != NULL)
					elength = end - exp;
				ret = _insert_rules(ctx, methodid, roleid, exp, elength);
			}
			if (ret != ESUCCESS)
				httpmessage_result(response, RESULT_400);
			else
			{
				warn("userfilter: insert %s", query);
#if defined RESULT_204
				httpmessage_result(response, RESULT_204);
#endif
			}
			ret = ESUCCESS;
		}
		else if (!strcmp(method, str_get) && ctx->line > -1)
		{
			if (ctx->line == 0)
			{
				httpmessage_addcontent(response, "text/json", NULL, -1);
				httpmessage_appendcontent(response, "[", -1);
			}
			else
				httpmessage_addcontent(response, NULL, "", -1);
			ctx->line++;
			ret = _jsonifyrule(ctx, ctx->line, response);
			if (ret != ECONTINUE)
			{
				httpmessage_appendcontent(response, "{}]", -1);
				ctx->line = -1;
				ret = ECONTINUE;
			}
			else
				httpmessage_appendcontent(response, ",", -1);
		}
		else if (ctx->line == -1)
		{
			httpclient_shutdown(httpmessage_client(request));
			return ESUCCESS;
		}
		else
		{
#if defined RESULT_405
			httpmessage_result(response, RESULT_405);
#else
			httpmessage_result(response, RESULT_400);
#endif
			ret = ESUCCESS;
		}
	}

	return ret;
}

#ifdef FILE_CONFIG
static void *userfilter_config(config_setting_t *iterator, server_t *UNUSED(server))
{
	mod_userfilter_t *modconfig = NULL;
#if LIBCONFIG_VER_MINOR < 5
	const config_setting_t *config = config_setting_get_member(iterator, "userfilter");
#else
	const config_setting_t *config = config_setting_lookup(iterator, "userfilter");
#endif
	if (config)
	{
		modconfig = calloc(1, sizeof(*modconfig));
		config_setting_lookup_string(config, "superuser", &modconfig->superuser);
		config_setting_lookup_string(config, "allow", &modconfig->allow);
		config_setting_lookup_string(config, "configuri", &modconfig->configuri);
		config_setting_lookup_string(config, "dbname", &modconfig->dbname);
		if (modconfig->dbname == NULL || modconfig->dbname[0] == '\0')
			modconfig->dbname = str_userfilterpath;
	}
	return modconfig;
}
#else
mod_userfilter_t g_userfilter_config =
{
	.superuser = "root",
	.configuri = "auth/filter",
	.dbname = str_userfilterpath,
};

static void *userfilter_config(void *iterator, server_t *server)
{
	return &g_userfilter_config;
}
#endif

void *mod_userfilter_create(http_server_t *server, void *arg)
{
	mod_userfilter_t *config = (mod_userfilter_t *)arg;
	if (config == NULL)
		return NULL;

	sqlite3 *db;
	int ret;

	if (access(config->dbname, R_OK))
	{
		ret = sqlite3_open_v2(config->dbname, &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
		if (ret != SQLITE_OK)
			return NULL;

		const char *query[] = {
			"create table methods (\"id\" INTEGER PRIMARY KEY, \"name\" TEXT UNIQUE NOT NULL);",
			"insert into methods (name) values(\"GET\");",
			"insert into methods (name) values(\"POST\");",
			"insert into methods (name) values(\"PUT\");",
			"insert into methods (name) values(\"DELETE\");",
			"insert into methods (name) values(\"HEAD\");",
			"create table roles (\"id\" INTEGER PRIMARY KEY, \"name\" TEXT UNIQUE NOT NULL);",
			"insert into roles (id, name) values(0, @SUPERUSER);",
			"insert into roles (id, name) values(1, \"annonymous\");",
			"insert into roles (id, name) values(2, \"*\");",
#ifdef DEBUG
			"insert into roles (id, name) values(3, \"users\");",
#endif
			"create table rules (\"exp\" TEXT NOT NULL, \"methodid\" INTEGER NOT NULL,\"roleid\" INTEGER NOT NULL, FOREIGN KEY (methodid) REFERENCES methods(id) ON UPDATE SET NULL, FOREIGN KEY (roleid) REFERENCES roles(id) ON UPDATE SET NULL);",
			"insert into rules (exp,methodid,roleid) values(@CONFIGURI,(select id from methods where name=\"POST\"),0);",
			"insert into rules (exp,methodid,roleid) values(\"^/auth/*\",(select id from methods where name=\"GET\"),0);",
			"insert into rules (exp,methodid,roleid) values(\"^/auth/*\",(select id from methods where name=\"PUT\"),0);",
			"insert into rules (exp,methodid,roleid) values(\"^/auth/*\",(select id from methods where name=\"POST\"),0);",
			"insert into rules (exp,methodid,roleid) values(\"^/auth/*\",(select id from methods where name=\"DELETE\"),0);",
			"insert into rules (exp,methodid,roleid) values(\"^/auth/mngt/%u/*\",(select id from methods where name=\"GET\"),2);",
			"insert into rules (exp,methodid,roleid) values(\"^/auth/mngt/%u/*\",(select id from methods where name=\"POST\"),2);",
			"insert into rules (exp,methodid,roleid) values(\"^/auth/mngt/%u/*\",(select id from methods where name=\"DELETE\"),2);",
#ifdef DEBUG
			"insert into rules (exp,methodid,roleid) values(\"^/%g/%u/*\",(select id from methods where name=\"GET\"),3);",
			"insert into rules (exp,methodid,roleid) values(\"^/trust/*\",(select id from methods where name=\"GET\"),1);",
#endif
			NULL,
		};
		char *error = NULL;
		char *configuriexp = calloc(1, strlen(config->configuri) + 2 + 1);
		sprintf(configuriexp, "^%s$", config->configuri);
		int i = 0;
		while (query[i] != NULL)
		{
			sqlite3_stmt *statement;
			ret = sqlite3_prepare_v2(db, query[i], -1, &statement, NULL);

			int index;
			index = sqlite3_bind_parameter_index(statement, "@SUPERUSER");
			ret = sqlite3_bind_text(statement, index, config->superuser, -1, SQLITE_STATIC);

			index = sqlite3_bind_parameter_index(statement, "@CONFIGURI");
			ret = sqlite3_bind_text(statement, index, configuriexp, -1, SQLITE_STATIC);

			ret = sqlite3_step(statement);
			sqlite3_finalize(statement);
			if (ret != SQLITE_OK && ret != SQLITE_DONE)
			{
				err("%s(%d) %d: %s\n%s", __FUNCTION__, __LINE__, ret, query[i], sqlite3_errmsg(db)); \
				break;
			}
			i++;
		}
		free(configuriexp);
		sqlite3_close(db);
		chmod(config->dbname, S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP);
	}
	ret = sqlite3_open_v2(config->dbname, &db, SQLITE_OPEN_READWRITE, NULL);
	if (ret != SQLITE_OK)
	{
		err("userfilter: database not found %s", config->dbname);
		return NULL;
	}
	dbg("userfilter: DB storage on %s", config->dbname);

	_mod_userfilter_t *mod = calloc(1, sizeof(*mod));
	mod->config = config;
	mod->cmp = &_exp_cmp;
	mod->db = db;
	httpserver_addconnector(server, userfilter_connector, mod, CONNECTOR_DOCFILTER, str_userfilter);
	httpserver_addconnector(server, rootgenerator_connector, mod, CONNECTOR_DOCUMENT, str_userfilter);

	return mod;
}

void mod_userfilter_destroy(void *arg)
{
	_mod_userfilter_t *mod = (_mod_userfilter_t *)arg;
	sqlite3_close(mod->db);
#ifdef FILE_CONFIG
	free(mod->config);
#endif
	free(arg);
}

const module_t mod_userfilter =
{
	.name = str_userfilter,
	.configure = (module_configure_t)&userfilter_config,
	.create = (module_create_t)&mod_userfilter_create,
	.destroy = &mod_userfilter_destroy
};
#ifdef MODULES
extern module_t mod_info __attribute__ ((weak, alias ("mod_userfilter")));
#endif
