--complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_gpt" to load this file. \quit


CREATE OR REPLACE FUNCTION gpt_natural_to_sql(text) RETURNS  text
AS '$libdir/pg_gpt' , 'gpt_query'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gpt_query(natural_lang_query text, execute boolean DEFAULT false) RETURNS  text
AS $$
DECLARE 
    sql_query text;
BEGIN
    sql_query := gpt_natural_to_sql(natural_lang_query);

    IF execute THEN
        EXECUTE sql_query;
    END IF;
    
    RETURN sql_query;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION gpt_explain(text) RETURNS  text
AS '$libdir/pg_gpt' , 'gpt_explain'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gpt_explain_plan(text) RETURNS  text
AS '$libdir/pg_gpt' , 'gpt_explain_plan'
LANGUAGE C IMMUTABLE STRICT;

-- select gpt_query('list all passengers on flight no. PG0134');
-- select gpt_query('passenger on flight no. PG0134 with the most expensive ticket');
-- select gpt_query('list of all  passengers  with their ticket amount on flight no PG0134 ');
-- select gpt_query('update seat no 2A of aircraft 319 to Economy', true);