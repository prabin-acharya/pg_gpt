MODULES = gpt_query_to_sql
EXTENSION = gpt_query_to_sql     # the extersion's name
DATA = gpt_query_to_sql--0.0.1.sql    # script file to install
REGRESS = gpt_query_to_sql_test      # the test script file

PG_LDFLAGS = -lcurl
# for posgres build
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
