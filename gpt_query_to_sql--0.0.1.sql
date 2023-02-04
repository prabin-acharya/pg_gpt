--complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION gpt_query_to_sql" to load this file. \quit

CREATE OR REPLACE FUNCTION gpt_query_to_sql(text) RETURNS  text
AS '$libdir/gpt_query_to_sql'
LANGUAGE C IMMUTABLE STRICT;