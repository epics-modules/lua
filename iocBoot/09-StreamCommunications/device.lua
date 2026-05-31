-- device.lua
--
-- Demonstrates bytestream.client for DTYP "lua" device support.
-- Creates records and defines their callback functions in one file.
--
-- The bytestream.client wraps asyn.client, adding:
--   :write(fmt, ...) - format and send data, returns self for chaining
--   :read(fmt)       - read and parse response, returns multiple values
--
-- Each load of this file creates a separate Lua state registered
-- under the PORT name. This allows multiple instances for different
-- devices without state collisions:
--
--   luaLoadFile("device.lua", {P="dev1:", PORT="SENSOR1", IP="192.168.1.100:5025"})
--   luaLoadFile("device.lua", {P="dev2:", PORT="SENSOR2", IP="192.168.1.101:5025"})

local db = require("db")
local bs = require("bytestream")

local P    = P    or "dev:"
local PORT = PORT or "SENSOR"

-- Register lua_State to be able to reference by records
luaRegisterState(PORT)

-- Create the network port to communicate
drvAsynIPPortConfigure(PORT, IP)

-- Create client for this state's port
local client = bs.client(PORT)
client.OutTerminator = "\n"
client.InTerminator  = "\n"


---------------------------------------------------------------------------
-- Records and callback functions
---------------------------------------------------------------------------

-- Read temperature: send "MEAS:TEMP?" and parse float response
db.record("ai", P .. "temperature") {
	DTYP = "lua",
	INP  = "@" .. PORT .. " read_temperature()",
	SCAN = "1 second",
	EGU  = "degC",
	PREC = "2",
}

function read_temperature()
	return client:write("MEAS:TEMP?"):read("%f")
end


-- Read voltage
db.record("ai", P .. "voltage") {
	DTYP = "lua",
	INP  = "@" .. PORT .. " read_voltage()",
	SCAN = "1 second",
	EGU  = "V",
	PREC = "3",
}

function read_voltage()
	return client:write("MEAS:VOLT?"):read("%f")
end


-- Read device status as enum index (0=off, 1=on, 2=standby)
db.record("longin", P .. "status") {
	DTYP = "lua",
	INP  = "@" .. PORT .. " read_status()",
	SCAN = "1 second",
}

function read_status()
	return client:write("STAT?"):read("%{off|on|standby}")
end


-- Set output voltage with formatted command
db.record("ao", P .. "setVoltage") {
	DTYP = "lua",
	OUT  = "@" .. PORT .. " set_voltage()",
	EGU  = "V",
	PREC = "3",
}

function set_voltage(record)
	client:write("SET:VOLT %.3f", record.VAL)
end


-- Set operating mode via enum
db.record("mbbo", P .. "setMode") {
	DTYP = "lua",
	OUT  = "@" .. PORT .. " set_mode()",
	ZRST = "off",
	ONST = "on",
	TWST = "standby",
}

function set_mode(record)
	client:write("MODE %{off|on|standby}", record.RVAL)
end
