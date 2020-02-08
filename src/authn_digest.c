/*****************************************************************************
 * authn_digest.c: Basic Authentication mode
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
#include <time.h>

#include "httpserver/httpserver.h"
#include "httpserver/hash.h"
#include "mod_auth.h"
#include "authn_digest.h"

#define err(format, ...) fprintf(stderr, "\x1B[31m"format"\x1B[0m\n",  ##__VA_ARGS__)
#define warn(format, ...) fprintf(stderr, "\x1B[35m"format"\x1B[0m\n",  ##__VA_ARGS__)
#ifdef DEBUG
#define dbg(format, ...) fprintf(stderr, "\x1B[32m"format"\x1B[0m\n",  ##__VA_ARGS__)
#else
#define dbg(...)
#endif

#define auth_dbg(...)

#define MAXNONCE 64

static const char *str_digest;

typedef struct authn_digest_s authn_digest_t;
struct authn_digest_s
{
	authn_digest_config_t *config;
	authz_t *authz;
	const hash_t *hash;
	const char *opaque;
	char nonce[MAXNONCE];
	int stale;
	int encode;
};

static void utils_searchstring(char **result, const char *haystack, const char *needle, int length)
{
	if ((*result == NULL || *result[0] == '\0') &&
		!strncmp(haystack, needle, length) && haystack[length] == '=')
	{
		char *end = NULL;
		*result = strchr(haystack, '=');
		if (*result == NULL)
			return 0;
		*result += 1;
		if (**result == '"')
		{
			*result = *result + 1;
			end = strchr(*result, '"');
		}
		if (end == NULL)
		{
			end = *result;
			while(*end != 0 && *end != ' ' && *end != ',')
			{
				end++;
			}
		}
		*end = 0;
		return end - *result;
	}
	return 0;
}

static char *utils_stringify(const unsigned char *data, int len)
{
	int i;
	char *result = calloc(2, len + 1);
	for (i = 0; i < len; i++)
	{
		snprintf(result + i * 2, 3, "%02x", data[i]);
	}
	return result;
}

static char str_opaque[] = "FQhe/qaU925kfnzjCev0ciny7QMkPqMAFRtzCUYo5tdS";

static void *authn_digest_create(const authn_t *authn, authz_t *authz, void *config)
{
	if (authn->hash == NULL)
		return NULL;
	if (authz->rules->passwd == NULL)
	{
		err("authn Digest is not compatible with authz %s", authz->name);
		return NULL;
	}
	str_digest = authn->auth->authn.name;

	authn_digest_t *mod = calloc(1, sizeof(*mod));
	mod->config = (authn_digest_config_t *)config;
	mod->authz = authz;
	mod->hash = authn->hash;
	if (mod->config->opaque == NULL)
		mod->opaque = str_opaque;
	else
		mod->opaque = mod->config->opaque;
	if (mod->config->realm == NULL)
		mod->config->realm = httpserver_INFO(authn->server, "host");
	return mod;
}

static void authn_digest_nonce(void *arg, char *nonce, int noncelen)
{
	const authn_digest_t *mod = (authn_digest_t *)arg;
/**
 *  nonce and opaque may be B64 encoded data
 * or hexa encoded data
 */
#ifndef DEBUG
	(void) arg;
	char _nonce[24];
	int i;

	srandom(time(NULL));
	int usedate = random() % 5;
	if (usedate)
	{
		struct tm nowtm;
		time_t now = time(NULL) / (60 * 30); // 30 minutes of validity
		strftime(_nonce, 24, "%M%H%Y%A%B", localtime_r(&now, &nowtm));
		const char *key = mod->config->secret;
		if (hash_macsha256 != NULL && key != NULL)
		{
			void *ctx = hash_macsha256->initkey(key, strlen(key));
			if (ctx)
			{
				hash_macsha256->update(ctx, _nonce, 24);
				char signature[HASH_MAX_SIZE];
				hash_macsha256->finish(ctx, signature);
				memcpy(_nonce, signature, 24);
			}
		}
	}
	else
	{
		for (i = 0; i < 6; i++)
			*(int *)(_nonce + i * 4) = random();
	}
	base64->encode(_nonce, 24, nonce, noncelen);
#else
	err("Auth DIGEST is not secure in DEBUG mode, rebuild!!!");
	if (!strcmp(mod->opaque, str_opaque))
		strncpy(nonce, "7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v", noncelen); //RFC7616
	else
		strncpy(nonce, "dcd98b7102dd2f0e8b11d0f600bfb0c093", noncelen);  //RFC2617
#endif
}

static int authn_digest_setup(void *arg, http_client_t *ctl, struct sockaddr *addr, int addrsize)
{
	authn_digest_t *mod = (authn_digest_t *)arg;
	(void) ctl;
	(void) addr;
	(void) addrsize;

	mod->stale = 0;
	authn_digest_nonce(arg, mod->nonce, MAXNONCE);
}

static void authn_digest_www_authenticate(authn_digest_t *mod, http_message_t * response)
{
	httpmessage_addheader(response, str_authenticate, "Digest ");
	if (mod->config->realm != NULL && mod->config->realm[0] != 0)
	{
		httpmessage_appendheader(response, str_authenticate,
				"realm=\"", mod->config->realm, "\"", NULL);
	}
	httpmessage_appendheader(response, str_authenticate,
				"\",qop=\"auth\",nonce=\"", mod->nonce,
				"\",opaque=\"", mod->opaque,
				"\",stale=", (mod->stale)?"true":"false", NULL);
}

static int authn_digest_challenge(void *arg, http_message_t *request, http_message_t *response)
{
	int ret;
	authn_digest_t *mod = (authn_digest_t *)arg;

	/**
	 * WWW-AUTHENTICATE header without algorithm is mandatory
	 * Firefox and Chrome doesn't support other algorithm than MD5
	 */
	authn_digest_www_authenticate(mod, response);

	if (mod->hash != hash_md5)
	{
		/**
		 * this adds a second WWW-AUTHENTICATE header with algorithm
		 */
		authn_digest_www_authenticate(mod, response);
		httpmessage_appendheader(response, str_authenticate,
				",algorithm=", mod->hash->name, "", NULL);
	}
	httpmessage_keepalive(response);
	ret = ECONTINUE;
	return ret;
}

struct authn_digest_computing_s
{
	char *(*digest)(const hash_t * hash, const char *a1, const char *nonce, const char *nc, const char *cnonce, const char *qop, const char *a2);
	char *(*a1)(const hash_t * hash, const char *username, const char *realm, const char *passwd);
	char *(*a2)(const hash_t * hash, const char *method, const char *uri, const char *entity);
};

static char *authn_digest_digest(const hash_t * hash, const char *a1, const char *nonce, const char *nc, const char *cnonce, const char *qop, const char *a2)
{
	if (a1 && a2)
	{
		char digest[32];
		void *ctx;

		ctx = hash->init();
		hash->update(ctx, a1, strlen(a1));
		hash->update(ctx, ":", 1);
		hash->update(ctx, nonce, strlen(nonce));
		if (qop && !strcmp(qop, "auth"))
		{
			if (nc)
			{
				hash->update(ctx, ":", 1);
				hash->update(ctx, nc, strlen(nc));
			}
			if (cnonce)
			{
				hash->update(ctx, ":", 1);
				hash->update(ctx, cnonce, strlen(cnonce));
			}
			hash->update(ctx, ":", 1);
			hash->update(ctx, qop, strlen(qop));
		}
		hash->update(ctx, ":", 1);
		hash->update(ctx, a2, strlen(a2));
		hash->finish(ctx, digest);
		return utils_stringify(digest, hash->size);
	}
	return NULL;
}

static char *authn_digest_a1(const hash_t * hash, const char *username, const char *realm, const char *passwd)
{
	if (passwd[0] != '$')
	{
		char A1[32];
		void *ctx;

		ctx = hash->init();
		hash->update(ctx, username, strlen(username));
		hash->update(ctx, ":", 1);
		hash->update(ctx, realm, strlen(realm));
		hash->update(ctx, ":", 1);
		hash->update(ctx, passwd, strlen(passwd));
		hash->finish(ctx, A1);
		return utils_stringify(A1, hash->size);
	}
	else if (passwd[0] == '$')
	{
		int i = 1;
		int decode = 0;
		if (passwd[i] == 'a')
		{
			decode = 1;
			i++;
		}
		if (passwd[i] == 'd')
		{
			i++;
		}
		int decrypt = 0;
		if ( passwd[i] == hash->nameid)
			decrypt = 1;
		if (decrypt)
		{
			passwd = strrchr(passwd + 1, '$');
			if (passwd)
			{
				passwd += 1;
				char *a1 = NULL;
				if (decode)
				{
					char b64passwd[64];
					int len = base64->decode(passwd, strlen(passwd), b64passwd, 64);
					a1 = utils_stringify(b64passwd, len);
				}
				else
					a1 = strdup(passwd);
				return a1;
			}
		}
	}
	return NULL;
}

static char *authn_digest_a2(const hash_t * hash, const char *method, const char *uri, const char *entity)
{
	char A2[32];
	void *ctx;

	ctx = hash->init();
	hash->update(ctx, method, strlen(method));
	hash->update(ctx, ":", 1);
	hash->update(ctx, uri, strlen(uri));
	if (entity)
	{
		hash->update(ctx, ":", 1);
		hash->update(ctx, entity, strlen(entity));
	}
	hash->finish(ctx, A2);
	return utils_stringify(A2, hash->size);
}
static struct authn_digest_computing_s authn_digest_md5_computing =
{
	.digest = authn_digest_digest,
	.a1 = authn_digest_a1,
	.a2 = authn_digest_a2,
};

static struct authn_digest_computing_s *authn_digest_computing = &authn_digest_md5_computing;

static char *str_empty = "";
static const char *authn_digest_check(void *arg, const char *method, const char *url, const char *string)
{
	authn_digest_t *mod = (authn_digest_t *)arg;
	const char *passwd = NULL;
	char *user = str_empty;
	const char *user_ret = NULL;
	char *uri = NULL;
	char *realm = str_empty;
	char *qop = NULL;
	char *nonce = NULL;
	char *cnonce = str_empty;
	char *nc = NULL;
	char *opaque = str_empty;
	char *algorithm = NULL;
	char *response = NULL;
	int length, i;

	length = strlen(string);
	for (i = 0; i < length; i++)
	{
		switch (string[i])
		{
		case 'a':
			utils_searchstring(&algorithm, string + i, "algorithm", sizeof("algorithm") - 1);
		break;
		case 'r':
			utils_searchstring(&realm, string + i, "realm", sizeof("realm") - 1);
			utils_searchstring(&response, string + i, "response", sizeof("response") - 1);
		break;
		case 'c':
			utils_searchstring(&cnonce, string + i, "cnonce", sizeof("cnonce") - 1);
		break;
		case 'n':
			utils_searchstring(&nonce, string + i, "nonce", sizeof("nonce") - 1);
			utils_searchstring(&nc, string + i, "nc", sizeof("nc") - 1);
		break;
		case 'o':
			utils_searchstring(&opaque, string + i, "opaque", sizeof("opaque") - 1);
		break;
		case 'q':
			utils_searchstring(&qop, string + i, "qop", sizeof("qop") - 1);
		break;
		case 'u':
			utils_searchstring(&user, string + i, "username", sizeof("username") - 1);
			utils_searchstring(&uri, string + i, "uri", sizeof("uri") - 1);
		break;
		default:
		break;
		}
	}

	int check = 1;
	if (nonce == NULL)
	{
		warn("auth: nonce is unset");
		check = 0;
	}
	else if (strcmp(nonce, mod->nonce))
	{
		mod->stale++;
		mod->stale %= 5;
		warn("auth: nonce is corrupted");
		check = 0;
	}
	else if (algorithm == NULL || strcmp(algorithm, mod->hash->name))
	{
		/**
		 * Firefox and Chrome doesn't support other algorithm than MD5
		 * https://bugzilla.mozilla.org/show_bug.cgi?id=472823
		 */
		warn("auth: algorithm is bad %s/%s", algorithm, mod->hash->name);
		mod->hash = hash_md5;
		check = 1;
	}
	else if (strcmp(opaque, mod->opaque) || strcmp(realm, mod->config->realm))
	{
		dbg("opaque %s", opaque);
		dbg("realm %s", realm);
		warn("auth: opaque or realm is bad");
		check = 0;
	}
	else if (uri == NULL)
	{
		warn("auth: uri is unset");
		check = 0;
	}
	else if (strcmp(url, uri))
	{
		warn("try connection on %s with authorization on %s", url, uri);
		check = 0;
	}
#ifndef DEBUG
	else
#endif
	{
		passwd = mod->authz->rules->passwd(mod->authz->ctx, user);
	}
	if (response && passwd && authn_digest_computing)
	{
		char *a1 = authn_digest_computing->a1(mod->hash, user, realm, passwd);
		char *a2 = authn_digest_computing->a2(mod->hash, method, uri, NULL);
		char *digest = authn_digest_computing->digest(mod->hash, a1, nonce, nc, cnonce, qop, a2);

		auth_dbg("Digest %s", digest);
		if (digest && !strcmp(digest, response) && check)
		{
			user_ret = user;
		}
		free (a1);
		free (a2);
		free (digest);
	}
	else
	{
		user_ret = mod->authz->rules->check(mod->authz->ctx, NULL, NULL, string);
	}
	return user_ret;
}

static void authn_digest_destroy(void *arg)
{
	authn_digest_t *mod = (authn_digest_t *)arg;
	free(mod);
}

authn_rules_t authn_digest_rules =
{
	.create = &authn_digest_create,
	.setup = &authn_digest_setup,
	.challenge = &authn_digest_challenge,
	.check = &authn_digest_check,
	.destroy = &authn_digest_destroy,
};
