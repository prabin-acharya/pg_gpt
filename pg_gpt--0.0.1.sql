--complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_gpt" to load this file. \quit


CREATE OR REPLACE FUNCTION gpt_query(text) RETURNS  text
AS '$libdir/pg_gpt' , 'gpt_query'
LANGUAGE C IMMUTABLE STRICT;


