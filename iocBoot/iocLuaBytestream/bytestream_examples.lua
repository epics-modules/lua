-- bytestream_examples.lua
--
-- Demonstrates bytestream.match and bytestream.format in a single file
-- that both creates records (via the db library) and defines the Lua
-- callback functions they reference.
--
-- Loaded via luaLoadFile from st.lua. The luaRegisterState call makes
-- the functions defined here available to luascript CODE fields via
-- the "@bytestream_demo function()" named-state pattern.

local db = require("db")
local bs = require("bytestream")

local P = P or "bs:"

luaRegisterState("bytestream_demo")


---------------------------------------------------------------------------
-- Input records (test data sources)
---------------------------------------------------------------------------

db.record("stringout", P .. "raw_response") {
	VAL = "VOLTS 3.14,AMPS 1.27",
}

db.record("stringout", P .. "raw_status") {
	VAL = "STATUS:on TEMP:25.3",
}

db.record("ao", P .. "channel") {
	VAL = "1",
}

db.record("ao", P .. "voltage") {
	VAL  = "3.3",
	PREC = "3",
}

db.record("ao", P .. "mode_index") {
	VAL = "2",
}

db.record("ao", P .. "binary_input") {
	VAL = "42",
}

db.record("stringout", P .. "binary_string") {
	VAL = "00101010",
}


---------------------------------------------------------------------------
-- Parsing examples
---------------------------------------------------------------------------

-- Parse voltage from "VOLTS 3.14,AMPS 1.27"
db.record("luascript", P .. "parse_volts") {
	INAA = P .. "raw_response",
	CODE = "@bytestream_demo parse_volts()",
	PREC = "3",
}

function parse_volts()
	local volts, amps = bs.match("VOLTS %f,AMPS %f", AA)
	return volts
end


-- Parse current using * flag to skip voltage
db.record("luascript", P .. "parse_amps") {
	INAA = P .. "raw_response",
	CODE = "@bytestream_demo parse_amps()",
	PREC = "3",
}

function parse_amps()
	return bs.match("VOLTS %*f,AMPS %f", AA)
end


-- Parse status enum (0=off, 1=on, 2=standby)
db.record("luascript", P .. "parse_status") {
	INAA = P .. "raw_status",
	CODE = "@bytestream_demo parse_status()",
}

function parse_status()
	return bs.match("STATUS:%{off|on|standby} TEMP:%*f", AA)
end


-- Parse temperature, ignoring the enum
db.record("luascript", P .. "parse_temp") {
	INAA = P .. "raw_status",
	CODE = "@bytestream_demo parse_temp()",
	PREC = "1",
}

function parse_temp()
	return bs.match("STATUS:%*{off|on|standby} TEMP:%f", AA)
end


---------------------------------------------------------------------------
-- Formatting examples
---------------------------------------------------------------------------

-- Format a command string from channel and voltage
db.record("luascript", P .. "format_command") {
	INPA = P .. "channel",
	INPB = P .. "voltage",
	CODE = "@bytestream_demo format_command()",
}

function format_command()
	return bs.format("SET:CHAN %d VOLT %.3f", A, B)
end


-- Format an enum value
db.record("luascript", P .. "format_enum") {
	INPA = P .. "mode_index",
	CODE = "@bytestream_demo format_enum()",
}

function format_enum()
	return bs.format("MODE:%{off|on|standby}", A)
end


---------------------------------------------------------------------------
-- Binary examples
---------------------------------------------------------------------------

-- Format a number as 8-digit binary string
db.record("luascript", P .. "to_binary") {
	INPA = P .. "binary_input",
	CODE = "@bytestream_demo to_binary()",
}

function to_binary()
	return bs.format("%08b", A)
end


-- Parse a binary string back to a number
db.record("luascript", P .. "from_binary") {
	INAA = P .. "binary_string",
	CODE = "@bytestream_demo from_binary()",
}

function from_binary()
	return bs.match("%b", AA)
end
