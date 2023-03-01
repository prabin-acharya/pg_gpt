MODULES = pg_gpt
EXTENSION = pg_gpt    
DATA = pg_gpt--0.0.1.sql    
REGRESS = pg_gpt_test   

PG_LDFLAGS = -lcurl
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

# CFLAGS = -I/usr/include/postgresql

include $(PGXS)
