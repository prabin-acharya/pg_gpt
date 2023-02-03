--complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION my_get_sum" to load this file. \quit

CREATE OR REPLACE FUNCTION my_get_sum() RETURNS  text
AS '$libdir/my_get_sum'
LANGUAGE C IMMUTABLE STRICT;