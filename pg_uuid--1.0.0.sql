/* pg_uuid.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_uuid" to load this file. \quit

CREATE FUNCTION uuid_generate_v6() RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_generate_v6'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION uuid_generate_v7() RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_generate_v7'
LANGUAGE C STRICT PARALLEL SAFE;
