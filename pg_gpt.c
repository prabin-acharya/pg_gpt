#include <postgres.h>
#include <fmgr.h>
#include <utils/builtins.h>
#include <curl/curl.h>
#include <catalog/pg_namespace.h>
#include <catalog/pg_class.h>
#include <executor/spi.h>
#include <utils/typcache.h>
#include <libpq-fe.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define API_URL "https://api.openai.com/v1/completions"
#define API_HEADER1 "Content-Type: application/json"
#define API_HEADER2 "Authorization: Bearer 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"

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
    char *start, *end;

    start = strstr(json, "\"text\":\"");
    if (start == NULL)
    {
        return NULL;
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

// function to make the request to OpenAI API
void request_openAI(const char *req_body, char *response)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;

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
}

// function to get the schema of the database
void get_schema_of_db(char *db_schema)
{
    int ret;
    int i;
    TupleDesc tupdesc;
    SPITupleTable *tuptable;
    HeapTuple tuple;

    // connect to database
    ret = SPI_connect();
    if (ret != SPI_OK_CONNECT)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("SPI_connect failed")));
    }

    // get formatted schema fo the database  as  table_name(column_name, column_name, ...)
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
        snprintf(db_schema, 4096, "%s%s\\n", db_schema, replace_newline(SPI_getvalue(tuple, tupdesc, 1)));
    }

    SPI_finish();
}

PG_FUNCTION_INFO_V1(gpt_query);
PG_FUNCTION_INFO_V1(gpt_explain);
PG_FUNCTION_INFO_V1(gpt_explain_plan);

Datum gpt_query(PG_FUNCTION_ARGS)
{
    text *natural_query_text = PG_GETARG_TEXT_P(0);
    char *natural_query = text_to_cstring(natural_query_text);

    char *db_schema[4096] = {0};
    get_schema_of_db(db_schema);

    char response[4096] = {0};
    char *req_body[4096] = {0};

    sprintf(req_body, "{\r\n  \"model\": \"text-davinci-003\",  \r\n \"max_tokens\": 150,  \r\n \"prompt\": \"### Here are Postgres SQL tables with their properties:\\n %s . Now, I will write a  query to databse in natural language and you should translate them to appropriate SQL command according to the Postgres Tables for that. Also make sure the table exists in the given data and if it does not exist match the closest. Also write SQL command in a single line.  Query:%s\\n SQL:\"}", replace_newline(db_schema), natural_query);
    request_openAI(req_body, response);

    PG_RETURN_TEXT_P(cstring_to_text(get_text(response)));
}

Datum gpt_explain(PG_FUNCTION_ARGS)
{
    text *natural_query_text = PG_GETARG_TEXT_P(0);
    char *natural_query = text_to_cstring(natural_query_text);

    char *db_schema[4096] = {0};
    get_schema_of_db(db_schema);

    char response[4096] = {0};
    char *req_body[4096] = {0};

    sprintf(req_body, "{\r\n  \"model\": \"text-davinci-003\",  \r\n \"max_tokens\": 150,  \r\n \"prompt\": \"### Here are Postgres SQL tables with their properties:\\n %s . Now, For a SQL Query expain the query in natural language clearly in a single line.  Query:%s\\n Explanation:\"}", replace_newline(db_schema), natural_query);
    request_openAI(req_body, response);

    PG_RETURN_TEXT_P(cstring_to_text(get_text(response)));
}

Datum gpt_explain_plan(PG_FUNCTION_ARGS)
{

    text *natural_query_text = PG_GETARG_TEXT_P(0);
    char *natural_query = text_to_cstring(natural_query_text);

    char *db_schema[4096] = {0};
    get_schema_of_db(db_schema);

    char response[4096] = {0};
    char *req_body[4096] = {0};

    snprintf(req_body, 4096, "{\r\n  \"model\": \"text-davinci-003\",  \r\n \"max_tokens\": 150,  \r\n \"prompt\": \"### Here are Postgres SQL tables with their properties:\\n %s .For the following SQL Query write a Query plan.  Query:%s\\n Query Plan:\"}", replace_newline(db_schema), natural_query);
    request_openAI(req_body, response);

    PG_RETURN_TEXT_P(cstring_to_text(get_text(response)));
}
