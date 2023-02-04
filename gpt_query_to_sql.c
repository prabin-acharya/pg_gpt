
#include <postgres.h>
// #include "json.h"
#include <fmgr.h>
#include <utils/builtins.h>
#include <curl/curl.h>
#include <catalog/pg_namespace.h>
#include <catalog/pg_class.h>
#include <catalog/pg_attribute.h>
#include <access/heapam.h>
#include <executor/spi.h>
#include <access/htup_details.h>
#include <access/tupdesc.h>
#include <utils/typcache.h>
#include "funcapi.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define API_URL "https://api.openai.com/v1/completions"
#define API_HEADER1 "Content-Type: application/json"
#define API_HEADER2 "Authorization: Bearer my-secret"

static size_t
write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    char *response = (char *)userp;
    strncat(response, (char *)contents, realsize);
    return realsize;
}

char *replace_newline(char *src)
{
    int i, j, len;
    char *dst = NULL;
    len = strlen(src);
    dst = malloc((len + 1) * sizeof(char));

    for (i = 0, j = 0; i < len; i++)
    {
        if (src[i] == '\n')
        {
            dst[j++] = ' ';
        }
        else
        {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';

    return dst;
}

const char *get_text(const char *json)
{
    const char *text_key = "\"text\":\"";
    const char *text_start = strstr(json, text_key);
    if (text_start == NULL)
    {
        return "text not found";
    }
    text_start += strlen(text_key);
    const char *text_end = strchr(text_start, '"');
    if (text_end == NULL)
    {
        return "text not found";
    }
    int text_len = text_end - text_start;
    char *text = (char *)malloc(text_len + 1);
    strncpy(text, text_start, text_len);
    text[text_len] = '\0';
    return text;
}

PG_FUNCTION_INFO_V1(gpt_query_to_sql);

Datum gpt_query_to_sql(PG_FUNCTION_ARGS)
{

    text *natural_query = PG_GETARG_TEXT_P(0);
    char *mattt = text_to_cstring(natural_query);

    CURL *curl;
    CURLcode res;

    struct curl_slist *headers = NULL;

    char response[2048] = {0};

    int ret;
    int i;
    TupleDesc tupdesc;
    SPITupleTable *tuptable;
    HeapTuple tuple;
    char *result = NULL;
    char *requestp = NULL;
    char *json_result = NULL;

    ret = SPI_connect();
    if (ret != SPI_OK_CONNECT)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("SPI_connect failed")));
    }

    ret = SPI_exec("SELECT CONCAT(table_name, '(', column_names, ')') as formatted_table FROM (SELECT table_name, string_agg(column_name, ', ') as column_names FROM information_schema.columns WHERE table_schema = 'public' GROUP BY table_name ORDER BY table_name) as subquery;", 0);
    if (ret != SPI_OK_SELECT)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("SPI_exec failed")));
    }

    tupdesc = SPI_tuptable->tupdesc;
    tuptable = SPI_tuptable;
    result = palloc(2048 * sizeof(char));
    strcpy(result, "");
    requestp = palloc(2048 * sizeof(char));
    strcpy(requestp, "");

    for (i = 0; i < SPI_processed; i++)
    {
        tuple = tuptable->vals[i];
        snprintf(result, 2048, "%s%s", result, replace_newline(SPI_getvalue(tuple, tupdesc, 1)));
    }

    // =======================================================

    curl = curl_easy_init();
    if (!curl)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("curl_easy_init failed")));
    }

    char *api_url2 = "http://localhost:3000/api/hello";

    // char datarr[] = "{\r\n  \"model\": \"code-davinci-002\",\r\n  \"prompt\": \"### Postgres SQL tables, with their properties:\\n#\\n#";
    // char dataMiddle[] = result;
    // char datarrend[] = "\\n #\\n## #Query : A query to list the names of the departments which employed more than 10 employees in the last 3 months\\nSQL :\",\r\n  \"temperature\": 0,\r\n  \"max_tokens\": 150,\r\n  \"top_p\": 1.0,\r\n  \"frequency_penalty\": 0.0,\r\n  \"presence_penalty\": 0.0,\r\n  \"stop\": [\"#\", \";\"]\r\n}";

    // strcat(datarr, result);
    // strcat(datarr, datarrend);
    headers = curl_slist_append(headers, API_HEADER1);
    headers = curl_slist_append(headers, API_HEADER2);
    char rit[5] = "pran";

    json_result = palloc(2048 * sizeof(char));
    snprintf(json_result, 2048, "{\r\n  \"model\": \"text-davinci-003\",\r\n \"prompt\": \"### Here are Postgres SQL tables with their properties:\\n# %s \\n. Now, I will write a  query to databse in natural language and you should translate them to appropriate SQL command for that.  Query:%s \\n SQL:\"}", replace_newline(result), mattt);

    // snprintf(requestp, 2048, "model=code-davinci-002&prompt=### Postgres SQL tables, with their properties:\n#\n%s#\n#\n### Query: A query to list the names of the departments which employed more than 10 employees in the last 3 months\nSQL:&temperature=0", result);
    // snprintf(requestp, 2048, "{\r\n  \"model\": \"code-davinci-002\",\r\n  \"prompt\": \"### Postgres SQL tables, with their properties:\\n#\\n# %s\\n#\\n### Query: A query to list the names of the departments which employed more than 10 employees in the last 3 months\\nSQL:\",\r\n  \"temperature\": 0,\r\n  \"max_tokens\": 150,\r\n  \"top_p\": 1.0,\r\n  \"frequency_penalty\": 0.0,\r\n  \"presence_penalty\": 0.0,\r\n  \"stop\": [\"#\", \";\"]\r\n}", result);
    // snprintf(requestp, 2048, "{\r\n  \"model\": \"code-davinci-002\",\r\n\"prompt\":\"asdff%sfds\"}", rit);
    curl_easy_setopt(curl, CURLOPT_URL, API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\r\n  \"model\": \"code-davinci-002\"}");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_result);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("curl_easy_perform failed: %s", curl_easy_strerror(res))));
    }

    curl_easy_cleanup(curl);

    SPI_finish();

    // struct json_object *parsed_json = json_tokener_parse(response);
    // struct json_object *result2, *text;
    // bool success;

    // success = json_object_object_get_ex(parsed_json, "result", &result2);
    // success = json_object_array_get_idx(result2, 0, &text);
    // success = json_object_object_get_ex(text, "text", &text);

    PG_RETURN_TEXT_P(cstring_to_text(get_text(response)));
    // PG_RETURN_TEXT_P(cstring_to_text(mattt));
}
