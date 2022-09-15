# pg_uuid/Makefile
#
# Copyright (c) 2022 Tomas Vondra <tomas@pgaddict.com>
#

MODULE_big = pg_uuid

OBJS = pg_uuid.o

EXTENSION = pg_uuid
DATA = pg_uuid--1.0.0.sql

TESTS        = $(wildcard test/sql/*.sql)
REGRESS      = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
