/*****************************************************************************
 * ouistiti_modules.c: modules initialisation
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>

#include "httpserver/httpserver.h"
#include "ouistiti.h"

#define err(format, ...) fprintf(stderr, "\x1B[31m"format"\x1B[0m\n",  ##__VA_ARGS__)
#define warn(format, ...) fprintf(stderr, "\x1B[35m"format"\x1B[0m\n",  ##__VA_ARGS__)
#ifdef DEBUG
#define dbg(format, ...) fprintf(stderr, "\x1B[32m"format"\x1B[0m\n",  ##__VA_ARGS__)
#else
#define dbg(...)
#endif

static int modulefilter(const struct dirent *entry)
{
	return !strncmp(entry->d_name, "mod_", 4);
}

int ouistiti_initmodules()
{
	int i;
	int cwdfd = open(".", O_DIRECTORY);
	int pkglibfd = open(PKGLIBDIR, O_DIRECTORY);
	if (pkglibfd == -1)
	{
		return EREJECT;
	}
	if (fchdir(pkglibfd) == -1)
	{
		err("Package linbrary dir "PKGLIBDIR" notfound");
		return EREJECT;
	}

	int ret;
	struct dirent **namelist = NULL;
	ret = scandir(".", &namelist, &modulefilter, alphasort);
	for (i = 0; i < ret; i++)
	{
		const char *name = namelist[i]->d_name;

		if (access(name, X_OK) == -1)
			continue;

		void *dh = dlopen(name, RTLD_LAZY | RTLD_GLOBAL);

		if (dh != NULL)
		{
			module_t *module = dlsym(dh, "mod_info");
			if (module)
				ouistiti_registermodule(module);
		}
		else
		{
			err("module %s loading error: %s", name, dlerror());
		}
	}
	if (fchdir(cwdfd) == -1)
	{
		err("Package linbrary dir "PKGLIBDIR" notfound");
		return EREJECT;
	}
	return ESUCCESS;
}
