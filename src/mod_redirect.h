/*****************************************************************************
 * mod_redirect.h: Redirect the request on 404 error
 * this file is part of https://github.com/ouistiti-project/ouistiti
 *****************************************************************************
 * Copyright (C) 2016-2017
 *
 * Authors: Marc Chalain <marc.chalain@gmail.com
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

#ifndef __MOD_REDIRECT_H__
#define __MOD_REDIRECT_H__

#include "ouistiti.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct mod_redirect_link_s mod_redirect_link_t;
struct mod_redirect_link_s
{
	const char *origin;
	int result;
	const char *destination;
	int options;
	const char *defaultpage;
	mod_redirect_link_t *next;
};
#define REDIRECT_HSTS			0x0001
#define REDIRECT_LINK			0x0002
#define REDIRECT_GENERATE204	0x0004
#define REDIRECT_PERMANENTLY	0x0008
#define REDIRECT_TEMPORARY		0x0010
#define REDIRECT_ERROR			0x0020
#define REDIRECT_QUERY			0x0040
typedef struct mod_redirect_s
{
	int options;
	mod_redirect_link_t *links;
} mod_redirect_t;


extern const module_t mod_redirect;

#ifdef __cplusplus
}
#endif

#endif
