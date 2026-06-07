-- db_examples.lua
--
-- Demonstrates the db library for creating EPICS records from Lua.
-- Loaded via luaLoadFile from st.lua before iocInit.

local db = require("db")
local P = P or "lua:"


-- === db.record: Create records with chained field syntax ===
-- The table-call syntax sets multiple fields at once.

db.record("ai", P .. "created_ai") {
	DESC = "Created from Lua",
	EGU  = "degC",
	PREC = "2",
	VAL  = "25.0",
	PINI = "YES",
}

db.record("stringin", P .. "created_si") {
	DESC = "A string record",
	VAL  = "created by db.record",
	PINI = "YES",
}


-- === db.loadRecords: Load a .db file with table macros ===
-- Table macros are more readable than the comma-separated string form.

db.loadRecords("template.db", {P=P, N="1", DESC="First sensor",  EGU="degC", PREC="2"})
db.loadRecords("template.db", {P=P, N="2", DESC="Second sensor", EGU="V",    PREC="3"})


-- === db.loadTemplate: Load a template with multiple substitutions ===
-- The pattern style names the macro columns; each row provides values.
-- The global table provides macros shared by all rows.

db.loadTemplate("template.db", {
	global  = {P=P, PREC="1"},
	pattern = {"N", "DESC", "EGU"},
	{"10", "Channel 10", "mA"},
	{"11", "Channel 11", "mA"},
	{"12", "Channel 12", "mA"},
})
