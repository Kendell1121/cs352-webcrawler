#ifndef HTML_UTILS_H
#define HTML_UTILS_H

char **extract_urls(const char *html, const char *base_url, int *count_out);

char *fetch_html(const char *url);

char *resolve_url(const char *base, const char *href);

#endif