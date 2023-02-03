#include <postgres.h>
#include <fmgr.h>
#include <utils/builtins.h>
#include <curl/curl.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define API_URL "https://jsonplaceholder.typicode.com/posts/5"

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    char *response = (char *)userp;
    strncat(response, (char *)contents, realsize);
    return realsize;
}

PG_FUNCTION_INFO_V1(my_get_sum);

Datum my_get_sum(PG_FUNCTION_ARGS)
{
    CURL *curl;
    CURLcode res;
    char response[1024] = {0};

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, API_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    PG_RETURN_TEXT_P(cstring_to_text(response));
}