#include <kore/kore.h>
#include <kore/http.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "assets.h"

#define FRIDAY			5

int		serve_index(struct http_request *);
int		serve_css(struct http_request *);
char*	read_file(char [], long *, time_t *);

int
serve_css(struct http_request *req)
{
	char				*static_path = getenv("static_path");
	char				filename[255];
	long				asset_len_css;
	time_t				asset_mtime_css;

	// Read requested file, if not found return 404.
	sprintf(filename, "%s%s", static_path, req->path);
	char *asset_css = read_file(filename, &asset_len_css, &asset_mtime_css);
	
	if (asset_css == NULL) {
		http_response(req, 404, NULL, 0);
	} else {
		char		*date;
		time_t		tstamp = 0;
		
		if (http_request_header(req, "if-modified-since", &date)) {
			tstamp = kore_date_to_time(date);
		}

		// Make sure not to return if the modified since header was received.
		if (tstamp != 0 && tstamp <= asset_mtime_css) {
			http_response(req, 304, NULL, 0);
		} else {
			date = kore_time_to_date(asset_mtime_css);
			if (date != NULL)
				http_response_header(req, "last-modified", date);

			// Serve CSS.
			http_response_header(req, "content-type", "text/css");
			http_response(req, 200, asset_css, asset_len_css);
		}
	}

	return (KORE_RESULT_OK);
}

int
serve_index(struct http_request *req)
{
	u_int8_t				*d;
	struct kore_buf			*b;
	u_int32_t				len;

	b = kore_buf_create(asset_len_index_html);
	kore_buf_append(b, asset_index_html, asset_len_index_html);

	time_t rt = time(NULL);
	struct tm *lt = localtime(&rt);

	// Check if it's friday today!
	if (lt->tm_wday == FRIDAY) {
		kore_buf_replace_string(b, "$content$", asset_friday_html, asset_len_friday_html);
	} else {
		u_int32_t nlen;
		struct kore_buf *nb = kore_buf_create(asset_len_notday_html);
		kore_buf_append(nb, asset_notday_html, asset_len_notday_html);

		// Calculate number of days left.
		int days;
		if (lt->tm_wday > FRIDAY) {
			days = 6; // This would be Saturday.
		} else {
			days = FRIDAY - lt->tm_wday;
		}

		char strdays[10];
		if (days > 1) {
			(void)sprintf(strdays, "%d days", days);
		} else {
			(void)sprintf(strdays, "%d day", days);
		}

		kore_buf_replace_string(nb, "$days$", strdays, strlen(strdays));
		u_int8_t *nd = kore_buf_release(nb,  &nlen);

		kore_buf_replace_string(b, "$content$", nd, nlen);
		kore_mem_free(nd);
	}

	d = kore_buf_release(b, &len);

	http_response_header(req, "content-type", "text/html");
	http_response(req, 200, d, len);
	kore_mem_free(d);

	return (KORE_RESULT_OK);
}

char*
read_file(char filename[255], long *filesize, time_t *lmd)
{
	FILE	*fp = fopen(filename, "r");
	char	*contents = NULL;
	long	size;
	struct stat attr;

	if (fp == NULL) {
		printf("File `%s` not found.\n", filename);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);
	contents = malloc(size * (sizeof(char)));
	fread(contents, sizeof(char), size, fp);
	
	fclose(fp);

	stat(filename, &attr);
	
	// Set stat pointers.
	*filesize = size;
	*lmd = attr.st_mtime;

	return contents;
}
