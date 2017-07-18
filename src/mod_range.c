/*****************************************************************************
 * mod_range.c: Range request support RFC 7233
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
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <errno.h>

#include "httpserver/httpserver.h"
#include "httpserver/uri.h"
#include "mod_static_file.h"

#define err(format, ...) fprintf(stderr, "\x1B[31m"format"\x1B[0m\n",  ##__VA_ARGS__)
#define warn(format, ...) fprintf(stderr, "\x1B[35m"format"\x1B[0m\n",  ##__VA_ARGS__)
#ifdef DEBUG
#define dbg(format, ...) fprintf(stderr, "\x1B[32m"format"\x1B[0m\n",  ##__VA_ARGS__)
#else
#define dbg(...)
#endif


int range_connector(void *arg, http_message_t *request, http_message_t *response)
{
	static_file_connector_t *private = (static_file_connector_t *)arg;
	if (private->type & STATIC_FILE_DIRLISTING || private->filepath == NULL)
		return EREJECT;
	int filesize = private->size;

	char *range = strstr(httpmessage_REQUEST(request,"Range"), "bytes=");
	if (range)
	{
		range += 6;
		private->offset = atoi(range);
		if (private->offset > filesize)
		{
			goto notsatisfiable;
		}
		char *end = strchr(range, '-');
		if (end != NULL)
		{
			int offset = filesize;
			if (*(end+1) != '*')
				offset = atoi(end+1);
			if (offset > filesize || offset < private->offset)
			{
				goto notsatisfiable;
			}
			private->size = offset - private->offset;
		}
		char buffer[256];
		snprintf(buffer, 256, "bytes %d-%d/%d", private->offset, private->offset + private->size, filesize);
		httpmessage_addheader(response, "Content-Range", buffer);
		httpmessage_result(response, RESULT_206);
	}
	return EREJECT;
notsatisfiable:
	free(private->filepath);
	private->filepath = NULL;
	free(private->path_info);
	private->path_info = NULL;
	char buffer[256];
	snprintf(buffer, 256, "bytes */%d", filesize);
	httpmessage_addheader(response, "Content-Range", buffer);
	httpmessage_result(response, RESULT_416);
	return ESUCCESS;
}
