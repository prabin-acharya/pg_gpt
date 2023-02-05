MODULES = pg_gpt
EXTENSION = pg_gpt    # the extersion's name
DATA = pg_gpt--0.0.1.sql    # script file to install
REGRESS = pg_gpt_test      # the test script file

PG_LDFLAGS = -lcurl
# for posgres build
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
