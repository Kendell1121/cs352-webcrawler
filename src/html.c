#include <stdio.h>
#include <stdlib.h>
#include "html.h"
#include <ctype.h>
#include <curl/curl.h>
#include <string.h>


typedef struct{
    char *data;
    size_t size;

} curl_buf_t;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t real = size * nmemb;
    curl_buf_t *buf = (curl_buf_t *)userp;

    char *tmp = realloc(buf->data, buf->size + real + 1);
    if (!tmp) return 0;

    buf->data = tmp;
    memcpy(buf->data + buf->size, contents, real);
    buf->size += real;
    buf->data[buf->size] = '\0';
    return real;
}

char *fetch_html(const char *url)
{
    CURL *curl;
    CURLcode res;
    curl_buf_t buf = { NULL, 0};

    buf.data = malloc(1);
    if (!buf.data) return NULL;
    buf.data[0] = '\0';

    curl = curl_easy_init();
    if (!curl) { free(buf.data); return NULL; }

    curl_easy_setopt(curl, CURLOPT_URL,            url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,      "my-webcrawler/1.0");

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "fetch_html: curl error for %s: %s\n",
                url, curl_easy_strerror(res));
        free(buf.data);
        return NULL;
    }
    return buf.data;
}

char *resolve_url(const char *base, const char *href)
{
    if (!href || href[0] == '\0') return NULL;

    if (strncmp(href, "http://", 7) == 0 ||
        strncmp(href, "https://", 8) == 0)
        return strdup(href);
    
    if (href[0] == '#')                         return NULL;
    if (strncmp(href, "mailto:", 7) == 0)       return NULL;
    if (strncmp(href, "javascript:", 11) == 0)  return NULL;

    const char *scheme_end = strstr(base, "://");
    if (!scheme_end) return NULL;
    const char *host_start = scheme_end + 3;
    const char *host_end = strchr(host_start, '/');

    char origin[1024];
    if (host_end) {
        size_t len = (size_t)(host_end - base);
        if (len >= sizeof(origin)) return NULL;
        strncpy(origin, base, len);
        origin[len] = '\0';
    } else {
        snprintf(origin, sizeof(origin), "%s", base);
    }

    char result[4096];

    if (href[0] == '/'){
        snprintf(result, sizeof(result), "%s%s", origin, href);
    } else {
        const char *last_slash = strrchr(base, '/');
        if(!last_slash || last_slash < host_start){
            snprintf(result, sizeof(result), "%s/%s", origin, href);
        } else {
            size_t prefix_len = (size_t)(last_slash - base) + 1;
            if (prefix_len >= sizeof(result)) return NULL;
            strncpy(result, base, prefix_len);
            result[prefix_len] = '\0';
            strncat(result, href, sizeof(result) - prefix_len - 1);
        }
    }
    return strdup(result);
}


char **extract_urls(const char *html, const char *base_url, int *count_out)
{
    *count_out = 0;
    if (!html || !base_url) return NULL;

    int capacity = 64;
    int count = 0;
    char **urls = malloc((size_t)capacity * sizeof(char *));
    if (!urls) return NULL;
    const char *p = html;

    while (*p){
        const char *found = NULL;
        for (const char *q = p; *q; q++){
            if (tolower((unsigned char)q[0]) == 'h' &&
                tolower((unsigned char)q[1]) == 'r' &&
                tolower((unsigned char)q[2]) == 'e' &&
                tolower((unsigned char)q[3]) == 'f')
            {
                found = q;
                break;
            }
        }
        if (!found) break;

        p = found + 4;

        while (*p && isspace ((unsigned char)*p)) p++;
        if (*p != '=') continue;
        p++;
        while (*p && isspace((unsigned char)*p)) p++;

        char delim = 0;
        if (*p == '"' || *p == '\'') {
            delim = *p;
            p++;
        }

        char href[2048];
        int i = 0;
        if (delim) {
            while (*p && *p != delim && i < (int)sizeof(href) - 1)
                href[i++] = *p++;
        } else {
            while (*p && !isspace((unsigned char)*p) && *p != '>' && i < (int)sizeof(href) - 1)
                href[i++] = *p++;
        }
        href[i] = '\0';
        if (i == 0) continue;

        char *abs = resolve_url(base_url, href);
        if (!abs) continue;

        if (count == capacity){
            capacity *= 2;
            char **tmp = realloc(urls, (size_t)capacity * sizeof(char *));
            if (!tmp) { free(abs); break; }
            urls = tmp;
        }
        urls[count++] = abs;
    }
    
    if (count == 0) { free(urls); return NULL;}
    *count_out = count;
    return urls;
}
