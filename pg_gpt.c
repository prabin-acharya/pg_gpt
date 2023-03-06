#include <postgres.h>
#include <utils/builtins.h>
#include <curl/curl.h>
#include <executor/spi.h>
#include "secrets.h"
#include <string.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define API_URL "https://api.openai.com/v1/completions"
#define API_HEADER1 "Content-Type: application/json"
#define API_HEADER2 "Authorization: Bearer " SECRET_API_KEY

write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    char *response = (char *)userp;
    strncat(response, (char *)contents, realsize);
    return realsize;
}

char *remove_newline(char *src)
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

// request to OpenAI API
char *request_openAI(const char *req_body)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char *response = malloc(4096);

    curl = curl_easy_init();
    if (!curl)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("curl_easy_init failed")));
    }

    headers = curl_slist_append(headers, API_HEADER1);
    headers = curl_slist_append(headers, API_HEADER2);

    curl_easy_setopt(curl, CURLOPT_URL, API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("curl_easy_perform() failed: %s", curl_easy_strerror(res))));
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return response;
}

char *get_schema_of_db()
{
    int ret;
    int i;
    TupleDesc tupdesc;
    SPITupleTable *tuptable;
    HeapTuple tuple;

    char *db_schema = malloc(4096);

    // connect to database
    ret = SPI_connect();
    if (ret != SPI_OK_CONNECT)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("SPI_connect failed")));
    }

    // get formatted schema of the database  as  table_name(column_name, column_name, ...)
    ret = SPI_exec("SELECT CONCAT(table_name, '(', column_names, ')') as formatted_db_schema FROM (SELECT table_name, string_agg(column_name, ', ') as column_names FROM information_schema.columns WHERE table_schema NOT IN ('pg_catalog','information_schema') GROUP BY table_name ORDER BY table_name) as subquery;", 0);
    if (ret != SPI_OK_SELECT)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("SPI_exec failed")));
    }

    tupdesc = SPI_tuptable->tupdesc;
    tuptable = SPI_tuptable;
    strcpy(db_schema, "");

    for (i = 0; i < SPI_processed; i++)
    {
        tuple = tuptable->vals[i];
        snprintf(db_schema, 4096, "%s%s\\n", db_schema, remove_newline(SPI_getvalue(tuple, tupdesc, 1)));
    }

    SPI_finish();

    return db_schema;
}

// grabs the text from the API response
const char *get_text(const char *json)
{
    char *start, *end;

    start = strstr(json, "\"text\":\"");
    if (start == NULL)
    {
        return "Error! Something went Wrong. Make sure you are using a valid API key. You should add your API key to secrets.h.";
    }

    start += strlen("\"text\":\"");
    end = strstr(start, "\",\"index\"");
    if (end == NULL)
    {
        return NULL;
    }

    int length = end - start;
    char *result = malloc(4096);
    strncpy(result, start, length);
    result[length] = '\0';

    return result;
}

PG_FUNCTION_INFO_V1(gpt_query);
PG_FUNCTION_INFO_V1(gpt_explain);
PG_FUNCTION_INFO_V1(gpt_explain_plan);

Datum gpt_query(PG_FUNCTION_ARGS)
{
    char *natural_query = text_to_cstring(PG_GETARG_TEXT_P(0));

    char *db_schema;
    db_schema = get_schema_of_db();

    char *req_body[4096] = {0};
    char *json;
    char *text;

    sprintf(req_body, "{\r\n  \"model\": \"text-davinci-003\",  \r\n \"max_tokens\": 150,  \r\n \"prompt\": \"### Here are Postgres SQL tables with their properties:\\n %s . For the description below, write appropriate SQL query in a single line. Make sure the table exists in the database. Description:%s\\n SQL:\"}", db_schema, natural_query);
    json = request_openAI(req_body);
    text = get_text(json);

    PG_RETURN_TEXT_P(cstring_to_text(text));
}

Datum gpt_explain(PG_FUNCTION_ARGS)
{
    char *sql_query = text_to_cstring(PG_GETARG_TEXT_P(0));

    char *db_schema;
    db_schema = get_schema_of_db();

    char *req_body[4096] = {0};
    char *json;
    char *text;

    sprintf(req_body, "{\r\n  \"model\": \"text-davinci-003\",  \r\n \"max_tokens\": 150,  \r\n \"prompt\": \"### Here are Postgres SQL tables with their properties:\\n %s . Now, I will write a SQL query and you should write a clear and concise explanation of the query in natural language. SQL Query:%s\\n Explanation:\"}", db_schema, sql_query);
    json = request_openAI(req_body);
    text = get_text(json);

    PG_RETURN_TEXT_P(cstring_to_text(text));
}

Datum gpt_explain_plan(PG_FUNCTION_ARGS)
{
    char *sql_query = text_to_cstring(PG_GETARG_TEXT_P(0));

    char *db_schema;
    db_schema = get_schema_of_db();

    char *req_body[4096] = {0};
    char *json;
    char *text;

    snprintf(req_body, 4096, "{\r\n  \"model\": \"text-davinci-003\",  \r\n \"max_tokens\": 150,  \r\n \"prompt\": \"### Here are Postgres SQL tables with their properties:\\n %s .For the following SQL Query. How will the database engine execute the query? Explain the Query plan clearly and concisely. Query:%s\\n Query Plan:\"}", db_schema, sql_query);
    json = request_openAI(req_body);
    text = get_text(json);

    PG_RETURN_TEXT_P(cstring_to_text(text));
}
