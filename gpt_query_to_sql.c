#include <postgres.h>
#include <fmgr.h>
#include <utils/builtins.h>
#include <curl/curl.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define API_URL "https://api.openai.com/v1/completions"
#define API_HEADER1 "Content-Type: application/json"
#define API_HEADER2 "Authorization: Bearer sk-my-secret-code"

static size_t
write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    char *response = (char *)userp;
    strncat(response, (char *)contents, realsize);
    return realsize;
}

PG_FUNCTION_INFO_V1(gpt_query_to_sql);

Datum gpt_query_to_sql(PG_FUNCTION_ARGS)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;

    char response[1024] = {0};
    char data[] = "{\r\n  \"model\": \"code-davinci-002\",\r\n  \"prompt\": \"### Postgres SQL tables, with their properties:\\n#\\n# Employee(id, name, department_id)\\n# Department(id, name, address)\\n# Salary_Payments(id, employee_id, amount, date)\\n#\\n### Query: A query to list the names of the departments which employed more than 10 employees in the last 3 months\\nSQL:\",\r\n  \"temperature\": 0,\r\n  \"max_tokens\": 150,\r\n  \"top_p\": 1.0,\r\n  \"frequency_penalty\": 0.0,\r\n  \"presence_penalty\": 0.0,\r\n  \"stop\": [\"#\", \";\"]\r\n}";

    curl = curl_easy_init();
    if (curl)
    {
        headers = curl_slist_append(headers, API_HEADER1);
        headers = curl_slist_append(headers, API_HEADER2);

        curl_easy_setopt(curl, CURLOPT_URL, API_URL);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    PG_RETURN_TEXT_P(cstring_to_text(response));
}