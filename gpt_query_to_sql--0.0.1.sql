--complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION gpt_query_to_sql" to load this file. \quit

CREATE OR REPLACE FUNCTION gpt_query_to_sql(text) RETURNS  text
AS '$libdir/gpt_query_to_sql'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION concat_numbers(a INTEGER, b INTEGER)
RETURNS TEXT AS $$
BEGIN
    RETURN CAST(a AS TEXT) || CAST(b AS TEXT);
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION execute_concat_numbers(a INTEGER, b INTEGER)
RETURNS TEXT AS $$
BEGIN
    RETURN concat_numbers(a, b);
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION execute_gpt_sql(a text)
RETURNS refcursor AS $$
DECLARE
    result refcursor;
BEGIN
    -- EXECUTE gpt_query_to_sql(a);
    -- EXECUTE 'SELECT * FROM actor LIMIT 10';
     OPEN result FOR EXECUTE 'SELECT * FROM actor LIMIT 10';
    RETURN result;
END;
$$ LANGUAGE plpgsql;

