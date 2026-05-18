#include <dbAccess.h>
#include <dbAddr.h>
#include <iocsh.h>
#include <cadef.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <epicsExport.h>
#include "lepicslib.h"


/*
 * CA context management -- cached per Lua state.
 *
 * A sentinel userdata is stored in the Lua registry. Its __gc
 * destroys the CA context when the Lua state is closed. The
 * context is created once on first use and reused for all
 * subsequent epics.get/put/pv calls in that state.
 */
#define LEPICS_CA_CONTEXT_KEY "LEPICS_CA_CONTEXT"

static int l_ca_context_gc(lua_State* L)
{
	if (ca_current_context())    { ca_context_destroy(); }
	return 0;
}

static void ensure_ca_context(lua_State* L)
{
	if (ca_current_context())    { return; }

	ca_context_create(ca_enable_preemptive_callback);

	/* Create a sentinel userdata with __gc to destroy the context */
	lua_newuserdata(L, 1);

	if (luaL_newmetatable(L, "lepics_ca_gc"))
	{
		lua_pushcfunction(L, l_ca_context_gc);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);

	lua_setfield(L, LUA_REGISTRYINDEX, LEPICS_CA_CONTEXT_KEY);
}


/* ------------------------------------------------------------------ */
/*  Direct database access helpers for local PVs                       */
/* ------------------------------------------------------------------ */

/*
 * Lua type categories for database field mapping.
 *
 * The native field_type values from dbFldTypes.h conflict with the
 * DBF_* macros from db_access.h (included by cadef.h) -- they use
 * different numeric values for the same names. To avoid this, we
 * map the native field_type to our own category enum and switch on
 * that instead of using any DBF_* constants.
 */
enum db_lua_type {
	DB_LUA_STRING,
	DB_LUA_CHAR,
	DB_LUA_INTEGER,
	DB_LUA_DOUBLE,
	DB_LUA_ENUM,
	DB_LUA_UNKNOWN
};

/*
 * Database-side DBR request type values from dbFldTypes.h.
 * These must be used instead of the DBR_* macros from db_access.h
 * when calling dbGetField/dbPutField, because db_access.h uses a
 * different numbering scheme than the database functions expect.
 */
#define DB_DBR_STRING   0   /* dbFldTypes: DBF_STRING */
#define DB_DBR_CHAR     1   /* dbFldTypes: DBF_CHAR */
#define DB_DBR_LONG     5   /* dbFldTypes: DBF_LONG */
#define DB_DBR_DOUBLE  10   /* dbFldTypes: DBF_DOUBLE */

/*
 * Map the native database field_type (dbFldTypes.h enum values)
 * to a Lua type category. Uses numeric values directly to avoid
 * the db_access.h #define conflict.
 *
 * dbFldTypes.h enum:
 *   0=STRING, 1=CHAR, 2=UCHAR, 3=SHORT, 4=USHORT,
 *   5=LONG, 6=ULONG, 7=INT64, 8=UINT64, 9=FLOAT,
 *   10=DOUBLE, 11=ENUM, 12=MENU, 13=DEVICE
 */
static enum db_lua_type dbf_to_lua_type(short field_type)
{
	switch (field_type)
	{
		case 0:              return DB_LUA_STRING;   /* DBF_STRING */
		case 1:  case 2:     return DB_LUA_CHAR;     /* DBF_CHAR, DBF_UCHAR */
		case 3:  case 4:     return DB_LUA_INTEGER;  /* DBF_SHORT, DBF_USHORT */
		case 5:  case 6:     return DB_LUA_INTEGER;  /* DBF_LONG, DBF_ULONG */
		case 7:  case 8:     return DB_LUA_DOUBLE;   /* DBF_INT64, DBF_UINT64 */
		case 9:  case 10:    return DB_LUA_DOUBLE;   /* DBF_FLOAT, DBF_DOUBLE */
		case 11: case 12:
		case 13:             return DB_LUA_ENUM;     /* DBF_ENUM, DBF_MENU, DBF_DEVICE */
		default:             return DB_LUA_UNKNOWN;
	}
}

/*
 * db_get -- read a local PV via dbGetField.
 * Returns: number of Lua values pushed (1 on success, 2 on error).
 */
static int db_get(lua_State* L, DBADDR* paddr, int max_count, int as_string)
{
	enum db_lua_type lua_type = dbf_to_lua_type(paddr->field_type);
	long count = paddr->no_elements;

	if (max_count > 0 && max_count < count)
		count = max_count;

	if (count > 1)
	{
		/* ---- Array path ---- */
		long i;
		long options = 0;
		long nRequest = count;

		switch (lua_type)
		{
			case DB_LUA_CHAR:
			{
				if (as_string == 0)
				{
					epicsInt8* buf = (epicsInt8*) malloc(count * sizeof(epicsInt8));
					if (!buf) { break; }
					nRequest = count;
					if (dbGetField(paddr, DB_DBR_CHAR, buf, &options, &nRequest, NULL) == 0)
					{
						lua_createtable(L, nRequest, 0);
						for (i = 0; i < nRequest; i++)
						{
							lua_pushinteger(L, (unsigned char) buf[i]);
							lua_rawseti(L, -2, i + 1);
						}
						free(buf);
						return 1;
					}
					free(buf);
				}
				else
				{
					char* buf = (char*) malloc(count + 1);
					if (!buf) { break; }
					nRequest = count;
					if (dbGetField(paddr, DB_DBR_CHAR, buf, &options, &nRequest, NULL) == 0)
					{
						buf[nRequest] = '\0';
						lua_pushstring(L, buf);
						free(buf);
						return 1;
					}
					free(buf);
				}
				break;
			}

			case DB_LUA_STRING:
			{
				dbr_string_t* buf = (dbr_string_t*) malloc(count * sizeof(dbr_string_t));
				if (!buf) { break; }
				nRequest = count;
				if (dbGetField(paddr, DB_DBR_STRING, buf, &options, &nRequest, NULL) == 0)
				{
					lua_createtable(L, nRequest, 0);
					for (i = 0; i < nRequest; i++)
					{
						lua_pushstring(L, buf[i]);
						lua_rawseti(L, -2, i + 1);
					}
					free(buf);
					return 1;
				}
				free(buf);
				break;
			}

			case DB_LUA_ENUM:
			{
				if (as_string == 1)
				{
					dbr_string_t* buf = (dbr_string_t*) malloc(count * sizeof(dbr_string_t));
					if (!buf) { break; }
					nRequest = count;
					if (dbGetField(paddr, DB_DBR_STRING, buf, &options, &nRequest, NULL) == 0)
					{
						lua_createtable(L, nRequest, 0);
						for (i = 0; i < nRequest; i++)
						{
							lua_pushstring(L, buf[i]);
							lua_rawseti(L, -2, i + 1);
						}
						free(buf);
						return 1;
					}
					free(buf);
				}
				else
				{
					epicsInt32* buf = (epicsInt32*) malloc(count * sizeof(epicsInt32));
					if (!buf) { break; }
					nRequest = count;
					if (dbGetField(paddr, DB_DBR_LONG, buf, &options, &nRequest, NULL) == 0)
					{
						lua_createtable(L, nRequest, 0);
						for (i = 0; i < nRequest; i++)
						{
							lua_pushinteger(L, buf[i]);
							lua_rawseti(L, -2, i + 1);
						}
						free(buf);
						return 1;
					}
					free(buf);
				}
				break;
			}

			case DB_LUA_INTEGER:
			{
				epicsInt32* buf = (epicsInt32*) malloc(count * sizeof(epicsInt32));
				if (!buf) { break; }
				nRequest = count;
				if (dbGetField(paddr, DB_DBR_LONG, buf, &options, &nRequest, NULL) == 0)
				{
					lua_createtable(L, nRequest, 0);
					for (i = 0; i < nRequest; i++)
					{
						lua_pushinteger(L, buf[i]);
						lua_rawseti(L, -2, i + 1);
					}
					free(buf);
					return 1;
				}
				free(buf);
				break;
			}

			case DB_LUA_DOUBLE:
			{
				double* buf = (double*) malloc(count * sizeof(double));
				if (!buf) { break; }
				nRequest = count;
				if (dbGetField(paddr, DB_DBR_DOUBLE, buf, &options, &nRequest, NULL) == 0)
				{
					lua_createtable(L, nRequest, 0);
					for (i = 0; i < nRequest; i++)
					{
						lua_pushnumber(L, buf[i]);
						lua_rawseti(L, -2, i + 1);
					}
					free(buf);
					return 1;
				}
				free(buf);
				break;
			}

			default:
				break;
		}
	}
	else
	{
		/* ---- Scalar path ---- */
		long options = 0;
		long nRequest = 1;

		switch (lua_type)
		{
			case DB_LUA_STRING:
			{
				char buf[MAX_STRING_SIZE];
				if (dbGetField(paddr, DB_DBR_STRING, buf, &options, &nRequest, NULL) == 0)
				{
					lua_pushstring(L, buf);
					return 1;
				}
				break;
			}

			case DB_LUA_ENUM:
			{
				if (as_string == 1)
				{
					char buf[MAX_STRING_SIZE];
					if (dbGetField(paddr, DB_DBR_STRING, buf, &options, &nRequest, NULL) == 0)
					{
						lua_pushstring(L, buf);
						return 1;
					}
				}
				else
				{
					epicsInt32 val;
					if (dbGetField(paddr, DB_DBR_LONG, &val, &options, &nRequest, NULL) == 0)
					{
						lua_pushinteger(L, val);
						return 1;
					}
				}
				break;
			}

			case DB_LUA_CHAR:
			case DB_LUA_INTEGER:
			{
				epicsInt32 val;
				if (dbGetField(paddr, DB_DBR_LONG, &val, &options, &nRequest, NULL) == 0)
				{
					lua_pushinteger(L, val);
					return 1;
				}
				break;
			}

			case DB_LUA_DOUBLE:
			{
				double val;
				if (dbGetField(paddr, DB_DBR_DOUBLE, &val, &options, &nRequest, NULL) == 0)
				{
					lua_pushnumber(L, val);
					return 1;
				}
				break;
			}

			default:
				break;
		}
	}

	/* If we get here, the read failed */
	lua_pushnil(L);
	lua_pushfstring(L, "Failed to read local PV (field_type=%d, lua_type=%d)",
	                (int) paddr->field_type, (int) lua_type);
	return 2;
}

/*
 * db_put -- write a local PV via dbPutField.
 * Returns: number of Lua values pushed (1 on success, 2 on error).
 */
static int db_put(lua_State* L, DBADDR* paddr, int val_index)
{
	long status = -1;

	switch (lua_type(L, val_index))
	{
		case LUA_TNUMBER:
		{
			if (lua_isinteger(L, val_index))
			{
				epicsInt32 val = (epicsInt32) lua_tointeger(L, val_index);
				status = dbPutField(paddr, DB_DBR_LONG, &val, 1);
			}
			else
			{
				double val = lua_tonumber(L, val_index);
				status = dbPutField(paddr, DB_DBR_DOUBLE, &val, 1);
			}
			break;
		}

		case LUA_TBOOLEAN:
		{
			epicsInt32 val = lua_toboolean(L, val_index);
			status = dbPutField(paddr, DB_DBR_LONG, &val, 1);
			break;
		}

		case LUA_TSTRING:
		{
			const char* val = lua_tostring(L, val_index);
			status = dbPutField(paddr, DB_DBR_STRING, val, 1);
			break;
		}

		case LUA_TTABLE:
		{
			int tbl_len = (int) lua_rawlen(L, val_index);
			if (tbl_len == 0) { status = 0; break; }

			lua_rawgeti(L, val_index, 1);
			int elem_type = lua_type(L, -1);
			int elem_is_int = lua_isinteger(L, -1);
			lua_pop(L, 1);

			if (elem_type == LUA_TSTRING)
			{
				dbr_string_t* buf = (dbr_string_t*) calloc(tbl_len, sizeof(dbr_string_t));
				if (!buf) { break; }
				int i;
				for (i = 0; i < tbl_len; i++)
				{
					lua_rawgeti(L, val_index, i + 1);
					const char* s = lua_tostring(L, -1);
					if (s) { strncpy(buf[i], s, MAX_STRING_SIZE - 1); }
					lua_pop(L, 1);
				}
				status = dbPutField(paddr, DB_DBR_STRING, buf, tbl_len);
				free(buf);
			}
			else if (elem_type == LUA_TNUMBER && elem_is_int)
			{
				epicsInt32* buf = (epicsInt32*) malloc(tbl_len * sizeof(epicsInt32));
				if (!buf) { break; }
				int i;
				for (i = 0; i < tbl_len; i++)
				{
					lua_rawgeti(L, val_index, i + 1);
					buf[i] = (epicsInt32) lua_tointeger(L, -1);
					lua_pop(L, 1);
				}
				status = dbPutField(paddr, DB_DBR_LONG, buf, tbl_len);
				free(buf);
			}
			else if (elem_type == LUA_TNUMBER)
			{
				double* buf = (double*) malloc(tbl_len * sizeof(double));
				if (!buf) { break; }
				int i;
				for (i = 0; i < tbl_len; i++)
				{
					lua_rawgeti(L, val_index, i + 1);
					buf[i] = lua_tonumber(L, -1);
					lua_pop(L, 1);
				}
				status = dbPutField(paddr, DB_DBR_DOUBLE, buf, tbl_len);
				free(buf);
			}
			break;
		}

		default:
		{
			lua_pushstring(L, "Unsupported value type for put");
			return 1;
		}
	}

	if (status)
	{
		lua_pushstring(L, "Failed to write local PV");
		return 1;
	}

	return 0;
}


/*
 * epics_get -- core get function.
 *
 * Tries direct database access first (local PV). If the PV is not
 * found locally, falls through to Channel Access.
 *
 * max_count: 0 = fetch all elements, >0 = limit to this many
 * as_string: -1 = type-dependent default, 0 = force numeric, 1 = force string
 *   - DBF_ENUM scalar: default=numeric, string=1 returns label
 *   - DBF_CHAR array:  default=string, string=0 returns table of ints
 *   - DBF_CHAR scalar: string parameter ignored
 */
static int epics_get(lua_State* state, const char* pv_name,
                     double timeout, int max_count, int as_string)
{
	if (pv_name == NULL)
	{
		lua_pushnil(state);
		lua_pushstring(state, "PV name is nil");
		return 2;
	}

	/* Try direct database access first (local PV) */
	{
		DBADDR addr;
		if (iocshPpdbbase && *iocshPpdbbase && dbNameToAddr(pv_name, &addr) == 0)
		{
			return db_get(state, &addr, max_count, as_string);
		}
	}

	/* Remote PV -- use Channel Access */
	ensure_ca_context(state);

	chid id;

	int status = ca_create_channel(pv_name, NULL, NULL, 0, &id);

	if (status != ECA_NORMAL)
	{
		lua_pushnil(state);
		lua_pushfstring(state, "Failed to create channel for '%s'", pv_name);
		return 2;
	}

	status = ca_pend_io(timeout);

	if (status != ECA_NORMAL)
	{
		ca_clear_channel(id);
		lua_pushnil(state);
		lua_pushfstring(state, "Timeout connecting to '%s'", pv_name);
		return 2;
	}

	int result = 0;
	short field_type = ca_field_type(id);
	unsigned long count = ca_element_count(id);

	/* Apply count limit */
	if (max_count > 0 && (unsigned long) max_count < count)
		count = (unsigned long) max_count;

	if (count > 1)
	{
		/* ---- Array path ---- */
		unsigned long i;

		switch (field_type)
		{
			case DBF_CHAR:
			{
				if (as_string == 0)
				{
					/* string=false: return table of integers */
					epicsInt8* buf = (epicsInt8*) malloc(count * sizeof(epicsInt8));
					if (!buf) { break; }
					status = ca_array_get(DBR_CHAR, count, id, buf);
					status = ca_pend_io(timeout);
					if (status == ECA_NORMAL)
					{
						lua_createtable(state, count, 0);
						for (i = 0; i < count; i++)
						{
							lua_pushinteger(state, (unsigned char) buf[i]);
							lua_rawseti(state, -2, i + 1);
						}
						result = 1;
					}
					free(buf);
				}
				else
				{
					/* default or string=true: return as Lua string */
					char* buf = (char*) malloc(count + 1);
					if (!buf) { break; }
					status = ca_array_get(DBR_CHAR, count, id, buf);
					status = ca_pend_io(timeout);
					if (status == ECA_NORMAL)
					{
						buf[count] = '\0';
						lua_pushstring(state, buf);
						result = 1;
					}
					free(buf);
				}
				break;
			}

			case DBF_STRING:
			{
				dbr_string_t* buf = (dbr_string_t*) malloc(count * sizeof(dbr_string_t));
				if (!buf) { break; }
				status = ca_array_get(DBR_STRING, count, id, buf);
				status = ca_pend_io(timeout);
				if (status == ECA_NORMAL)
				{
					lua_createtable(state, count, 0);
					for (i = 0; i < count; i++)
					{
						lua_pushstring(state, buf[i]);
						lua_rawseti(state, -2, i + 1);
					}
					result = 1;
				}
				free(buf);
				break;
			}

			case DBF_ENUM:
			{
				if (as_string == 1)
				{
					/* string=true: return table of string labels */
					dbr_string_t* buf = (dbr_string_t*) malloc(count * sizeof(dbr_string_t));
					if (!buf) { break; }
					status = ca_array_get(DBR_STRING, count, id, buf);
					status = ca_pend_io(timeout);
					if (status == ECA_NORMAL)
					{
						lua_createtable(state, count, 0);
						for (i = 0; i < count; i++)
						{
							lua_pushstring(state, buf[i]);
							lua_rawseti(state, -2, i + 1);
						}
						result = 1;
					}
					free(buf);
				}
				else
				{
					/* default or string=false: return table of integers */
					epicsInt32* buf = (epicsInt32*) malloc(count * sizeof(epicsInt32));
					if (!buf) { break; }
					status = ca_array_get(DBR_LONG, count, id, buf);
					status = ca_pend_io(timeout);
					if (status == ECA_NORMAL)
					{
						lua_createtable(state, count, 0);
						for (i = 0; i < count; i++)
						{
							lua_pushinteger(state, buf[i]);
							lua_rawseti(state, -2, i + 1);
						}
						result = 1;
					}
					free(buf);
				}
				break;
			}

			case DBF_SHORT:
			case DBF_LONG:
			{
				/* Integer arrays */
				epicsInt32* buf = (epicsInt32*) malloc(count * sizeof(epicsInt32));
				if (!buf) { break; }
				status = ca_array_get(DBR_LONG, count, id, buf);
				status = ca_pend_io(timeout);
				if (status == ECA_NORMAL)
				{
					lua_createtable(state, count, 0);
					for (i = 0; i < count; i++)
					{
						lua_pushinteger(state, buf[i]);
						lua_rawseti(state, -2, i + 1);
					}
					result = 1;
				}
				free(buf);
				break;
			}

			case DBF_FLOAT:
			case DBF_DOUBLE:
			{
				/* Floating-point arrays */
				double* buf = (double*) malloc(count * sizeof(double));
				if (!buf) { break; }
				status = ca_array_get(DBR_DOUBLE, count, id, buf);
				status = ca_pend_io(timeout);
				if (status == ECA_NORMAL)
				{
					lua_createtable(state, count, 0);
					for (i = 0; i < count; i++)
					{
						lua_pushnumber(state, buf[i]);
						lua_rawseti(state, -2, i + 1);
					}
					result = 1;
				}
				free(buf);
				break;
			}

			default:
				break;
		}
	}
	else
	{
		/* ---- Scalar path ---- */
		switch (field_type)
		{
			case -1:
			case DBF_NO_ACCESS:
				break;

			case DBF_STRING:
			{
				struct dbr_time_string val;
				status = ca_get(DBR_TIME_STRING, id, &val);
				status = ca_pend_io(timeout);
				if (status == ECA_NORMAL) { lua_pushstring(state, val.value); result = 1; }
				break;
			}

			case DBF_ENUM:
			{
				if (as_string == 1)
				{
					/* string=true: return enum label */
					struct dbr_time_string val;
					status = ca_get(DBR_TIME_STRING, id, &val);
					status = ca_pend_io(timeout);
					if (status == ECA_NORMAL) { lua_pushstring(state, val.value); result = 1; }
				}
				else
				{
					/* default or string=false: return numeric index */
					struct dbr_time_enum val;
					status = ca_get(DBR_TIME_ENUM, id, &val);
					status = ca_pend_io(timeout);
					if (status == ECA_NORMAL) { lua_pushinteger(state, val.value); result = 1; }
				}
				break;
			}

			case DBF_CHAR:
			{
				struct dbr_time_char val;
				status = ca_get(DBR_TIME_CHAR, id, &val);
				status = ca_pend_io(timeout);
				if (status == ECA_NORMAL) { lua_pushinteger(state, val.value); result = 1; }
				break;
			}

			case DBF_SHORT:
			{
				struct dbr_time_short val;
				status = ca_get(DBR_TIME_SHORT, id, &val);
				status = ca_pend_io(timeout);
				if (status == ECA_NORMAL) { lua_pushinteger(state, val.value); result = 1; }
				break;
			}

			case DBF_LONG:
			{
				struct dbr_time_long val;
				status = ca_get(DBR_TIME_LONG, id, &val);
				status = ca_pend_io(timeout);
				if (status == ECA_NORMAL) { lua_pushinteger(state, val.value); result = 1; }
				break;
			}

			case DBF_FLOAT:
			{
				struct dbr_time_float val;
				status = ca_get(DBR_TIME_FLOAT, id, &val);
				status = ca_pend_io(timeout);
				if (status == ECA_NORMAL) { lua_pushnumber(state, val.value); result = 1; }
				break;
			}

			case DBF_DOUBLE:
			{
				struct dbr_time_double val;
				status = ca_get(DBR_TIME_DOUBLE, id, &val);
				status = ca_pend_io(timeout);
				if (status == ECA_NORMAL) { lua_pushnumber(state, val.value); result = 1; }
				break;
			}
		}
	}

	ca_clear_channel(id);

	if (result == 0)
	{
		lua_pushnil(state);
		lua_pushfstring(state, "Failed to read '%s'", pv_name);
		return 2;
	}

	return result;
}

static int epics_put(lua_State* state, const char* pv_name, int offset, double timeout)
{
	if (pv_name == NULL)
	{
		lua_pushstring(state, "PV name is nil");
		return 1;
	}

	/* Try direct database access first (local PV) */
	{
		DBADDR addr;
		if (iocshPpdbbase && *iocshPpdbbase && dbNameToAddr(pv_name, &addr) == 0)
		{
			return db_put(state, &addr, offset);
		}
	}

	/* Remote PV -- use Channel Access */
	ensure_ca_context(state);

	chid id;

	int status = ca_create_channel(pv_name, NULL, NULL, 0, &id);

	if (status != ECA_NORMAL)
	{
		lua_pushfstring(state, "Failed to create channel for '%s'", pv_name);
		return 1;
	}

	status = ca_pend_io(timeout);

	if (status != ECA_NORMAL)
	{
		ca_clear_channel(id);
		lua_pushfstring(state, "Timeout connecting to '%s'", pv_name);
		return 1;
	}

	switch (lua_type(state, offset))
	{
		case LUA_TNUMBER:
		{
			if (lua_isinteger(state, offset))
			{
				epicsInt32 data = (epicsInt32) lua_tointeger(state, offset);
				status = ca_put(DBR_LONG, id, &data);
			}
			else
			{
				double data = lua_tonumber(state, offset);
				status = ca_put(DBR_DOUBLE, id, &data);
			}
			break;
		}

		case LUA_TBOOLEAN:
		{
			short data = lua_toboolean(state, offset);
			status = ca_put(DBR_INT, id, &data);
			break;
		}

		case LUA_TSTRING:
		{
			const char* data = lua_tostring(state, offset);
			status = ca_put(DBR_STRING, id, data);
			break;
		}

		case LUA_TTABLE:
		{
			int tbl_len = (int) lua_rawlen(state, offset);

			if (tbl_len == 0)
			{
				status = ECA_NORMAL;
				break;
			}

			/* Determine element type from the first entry */
			lua_rawgeti(state, offset, 1);
			int elem_type = lua_type(state, -1);
			int elem_is_int = lua_isinteger(state, -1);
			lua_pop(state, 1);

			if (elem_type == LUA_TSTRING)
			{
				dbr_string_t* buf = (dbr_string_t*) calloc(tbl_len, sizeof(dbr_string_t));
				if (!buf) { status = ECA_ALLOCMEM; break; }

				int i;
				for (i = 0; i < tbl_len; i++)
				{
					lua_rawgeti(state, offset, i + 1);
					const char* s = lua_tostring(state, -1);
					if (s) { strncpy(buf[i], s, MAX_STRING_SIZE - 1); }
					lua_pop(state, 1);
				}

				status = ca_array_put(DBR_STRING, tbl_len, id, buf);
				free(buf);
			}
			else if (elem_type == LUA_TNUMBER && elem_is_int)
			{
				epicsInt32* buf = (epicsInt32*) malloc(tbl_len * sizeof(epicsInt32));
				if (!buf) { status = ECA_ALLOCMEM; break; }

				int i;
				for (i = 0; i < tbl_len; i++)
				{
					lua_rawgeti(state, offset, i + 1);
					buf[i] = (epicsInt32) lua_tointeger(state, -1);
					lua_pop(state, 1);
				}

				status = ca_array_put(DBR_LONG, tbl_len, id, buf);
				free(buf);
			}
			else if (elem_type == LUA_TNUMBER)
			{
				double* buf = (double*) malloc(tbl_len * sizeof(double));
				if (!buf) { status = ECA_ALLOCMEM; break; }

				int i;
				for (i = 0; i < tbl_len; i++)
				{
					lua_rawgeti(state, offset, i + 1);
					buf[i] = lua_tonumber(state, -1);
					lua_pop(state, 1);
				}

				status = ca_array_put(DBR_DOUBLE, tbl_len, id, buf);
				free(buf);
			}
			else
			{
				ca_clear_channel(id);
				lua_pushfstring(state, "Unsupported table element type for put to '%s'", pv_name);
				return 1;
			}
			break;
		}

		default:
		{
			ca_clear_channel(id);
			lua_pushfstring(state, "Unsupported value type for put to '%s'", pv_name);
			return 1;
		}
	}

	if (status != ECA_NORMAL)
	{
		ca_clear_channel(id);
		lua_pushfstring(state, "Failed to put value to '%s'", pv_name);
		return 1;
	}

	ca_pend_io(timeout);
	ca_clear_channel(id);

	return 0;
}


static int l_caget(lua_State* state)
{
	const char* pv_name = luaL_checkstring(state, 1);
	double timeout = 1.0;
	int max_count = 0;
	int as_string = -1;

	if (lua_isnumber(state, 2))
	{
		/* Old-style: epics.get("pv", timeout) */
		timeout = lua_tonumber(state, 2);
	}
	else if (lua_istable(state, 2))
	{
		/* New-style: epics.get("pv", {timeout=5, count=100, string=true}) */
		lua_getfield(state, 2, "timeout");
		if (!lua_isnil(state, -1))    { timeout = lua_tonumber(state, -1); }
		lua_pop(state, 1);

		lua_getfield(state, 2, "count");
		if (!lua_isnil(state, -1))    { max_count = (int) lua_tointeger(state, -1); }
		lua_pop(state, 1);

		lua_getfield(state, 2, "string");
		if (!lua_isnil(state, -1))    { as_string = lua_toboolean(state, -1); }
		lua_pop(state, 1);
	}

	return epics_get(state, pv_name, timeout, max_count, as_string);
}

static int l_caput(lua_State* state)
{
	const char* pv_name = luaL_checkstring(state, 1);
	double timeout = 1.0;

	if (lua_isnumber(state, 3))
	{
		/* Old-style: epics.put("pv", value, timeout) */
		timeout = lua_tonumber(state, 3);
	}
	else if (lua_istable(state, 3))
	{
		/* New-style: epics.put("pv", value, {timeout=5}) */
		lua_getfield(state, 3, "timeout");
		if (!lua_isnil(state, -1))    { timeout = lua_tonumber(state, -1); }
		lua_pop(state, 1);
	}

	return epics_put(state, pv_name, 2, timeout);
}

/*
 * lua_pv -- userdata representing an EPICS PV.
 * Field access via __index/__newindex dispatches to epics_get/epics_put,
 * which automatically use direct database access for local PVs or
 * Channel Access for remote PVs.
 */
typedef struct {
	char pv_name[128];
} lua_pv;

/*
 * pv:get(field [, options])
 *
 *   pv:get("VAL")
 *   pv:get("VAL", {timeout=5.0, string=true, count=100})
 */
static int l_pv_get(lua_State* state)
{
	lua_pv* pv = (lua_pv*) luaL_checkudata(state, 1, "lua_pv");
	const char* field = luaL_checkstring(state, 2);

	double timeout = 1.0;
	int max_count = 0;
	int as_string = -1;

	if (lua_istable(state, 3))
	{
		lua_getfield(state, 3, "timeout");
		if (!lua_isnil(state, -1))    { timeout = lua_tonumber(state, -1); }
		lua_pop(state, 1);

		lua_getfield(state, 3, "count");
		if (!lua_isnil(state, -1))    { max_count = (int) lua_tointeger(state, -1); }
		lua_pop(state, 1);

		lua_getfield(state, 3, "string");
		if (!lua_isnil(state, -1))    { as_string = lua_toboolean(state, -1); }
		lua_pop(state, 1);
	}
	else if (lua_isnumber(state, 3))
	{
		timeout = lua_tonumber(state, 3);
	}

	std::string full_name(pv->pv_name);
	full_name.append(".");
	full_name.append(field);

	return epics_get(state, full_name.c_str(), timeout, max_count, as_string);
}

/*
 * pv:put(field, value [, options])
 *
 *   pv:put("VAL", 42)
 *   pv:put("VAL", 42, {timeout=5.0})
 */
static int l_pv_put(lua_State* state)
{
	lua_pv* pv = (lua_pv*) luaL_checkudata(state, 1, "lua_pv");
	const char* field = luaL_checkstring(state, 2);

	double timeout = 1.0;

	if (lua_istable(state, 4))
	{
		lua_getfield(state, 4, "timeout");
		if (!lua_isnil(state, -1))    { timeout = lua_tonumber(state, -1); }
		lua_pop(state, 1);
	}
	else if (lua_isnumber(state, 4))
	{
		timeout = lua_tonumber(state, 4);
	}

	std::string full_name(pv->pv_name);
	full_name.append(".");
	full_name.append(field);

	return epics_put(state, full_name.c_str(), 3, timeout);
}

static int l_pv_index(lua_State* state)
{
	lua_pv* pv = (lua_pv*) luaL_checkudata(state, 1, "lua_pv");
	const char* key = luaL_checkstring(state, 2);

	/* Properties */
	if (strcmp(key, "name") == 0)
	{
		lua_pushstring(state, pv->pv_name);
		return 1;
	}

	/* Methods */
	if (strcmp(key, "get") == 0)    { lua_pushcfunction(state, l_pv_get); return 1; }
	if (strcmp(key, "put") == 0)    { lua_pushcfunction(state, l_pv_put); return 1; }

	/* Field access -- build pv_name.field and read */
	std::string full_name(pv->pv_name);
	full_name.append(".");
	full_name.append(key);

	return epics_get(state, full_name.c_str(), 1.0, 0, -1);
}

static int l_pv_newindex(lua_State* state)
{
	lua_pv* pv = (lua_pv*) luaL_checkudata(state, 1, "lua_pv");
	const char* key = luaL_checkstring(state, 2);

	std::string full_name(pv->pv_name);
	full_name.append(".");
	full_name.append(key);

	return epics_put(state, full_name.c_str(), 3, 1.0);
}

static int l_pv_gc(lua_State* state)
{
	/* No resources to free currently.
	 * Future: close cached CA channels here. */
	return 0;
}

static int l_pv_tostring(lua_State* state)
{
	lua_pv* pv = (lua_pv*) luaL_checkudata(state, 1, "lua_pv");
	lua_pushstring(state, pv->pv_name);
	return 1;
}

extern "C"
{
	void luaGeneratePV(lua_State* state, const char* pv_name)
	{
		lua_pv* pv = (lua_pv*) lua_newuserdata(state, sizeof(lua_pv));
		strncpy(pv->pv_name, pv_name, sizeof(pv->pv_name) - 1);
		pv->pv_name[sizeof(pv->pv_name) - 1] = '\0';
		luaL_setmetatable(state, "lua_pv");
	}
}

static int l_createpv(lua_State* state)
{
	if (! lua_isstring(state, 1))    { return 0; }

	const char* pv_name = lua_tostring(state, 1);

	luaGeneratePV(state, pv_name);

	return 1;
}


int luaopen_epics (lua_State *L)
{
	/* Register the lua_pv metatable */
	if (luaL_newmetatable(L, "lua_pv"))
	{
		lua_pushcfunction(L, l_pv_index);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, l_pv_newindex);
		lua_setfield(L, -2, "__newindex");
		lua_pushcfunction(L, l_pv_gc);
		lua_setfield(L, -2, "__gc");
		lua_pushcfunction(L, l_pv_tostring);
		lua_setfield(L, -2, "__tostring");
		lua_pushstring(L, "epics.pv");
		lua_setfield(L, -2, "__name");

		/* Documentation for info(pv_object) */
		lua_newtable(L);
		lua_pushstring(L, ".name                       -- PV name (property)"); lua_rawseti(L, -2, 1);
		lua_pushstring(L, ".FIELD                      -- read field value"); lua_rawseti(L, -2, 2);
		lua_pushstring(L, ".FIELD = value              -- write field value"); lua_rawseti(L, -2, 3);
		lua_pushstring(L, ":get(field [, {timeout, count, string}])"); lua_rawseti(L, -2, 4);
		lua_pushstring(L, ":put(field, value [, {timeout}])"); lua_rawseti(L, -2, 5);
		lua_setfield(L, -2, "_doc");
	}
	lua_pop(L, 1);

	static const luaL_Reg mylib[] = {
		{"get",   l_caget},
		{"put",   l_caput},
		{"pv",    l_createpv},
		{NULL, NULL}
	};

	luaL_newlib(L, mylib);

	/* Documentation for info(epics) */
	lua_newtable(L);
	lua_pushstring(L, ".get(PV [, timeout | {timeout, count, string}])"); lua_rawseti(L, -2, 1);
	lua_pushstring(L, ".put(PV, value [, timeout | {timeout}])"); lua_rawseti(L, -2, 2);
	lua_pushstring(L, ".pv(PV) -- create PV proxy object"); lua_rawseti(L, -2, 3);
	lua_setfield(L, -2, "_doc");

	return 1;
}

static void libepicsRegister(void)    { luaRegisterLibrary("epics", luaopen_epics); }

extern "C"
{
	epicsExportRegistrar(libepicsRegister);
}
