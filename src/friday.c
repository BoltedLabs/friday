#include <kore/kore.h>
#include <kore/http.h>
#include <time.h>

#include "assets.h"

#define FRIDAY			5

int		serve_index(struct http_request *);

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