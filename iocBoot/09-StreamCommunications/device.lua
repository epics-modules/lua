-- device.lua
--
-- Demonstrates bytestream.client for DTYP "lua" device support.
-- Creates records and defines their callback functions in one file.
--
-- The bytestream.client wraps asyn.client, adding:
--   :write(fmt, ...) - format and send data, returns self for chaining
--   :read(fmt)       - read and parse response, returns multiple values
--
-- To use this example, configure an asyn port in st.lua before loading:
--
--   drvAsynIPPortConfigure("SENSOR", "192.168.1.100:5025")
--   luaLoadFile("device.lua", {P="bs:", PORT="SENSOR"})

local db = require("db")
local bs = require("bytestream")

local P    = P    or "dev:"
local PORT = PORT or "SENSOR"

luaRegisterState("bytestream_device")


---------------------------------------------------------------------------
-- Client cache (one client per port, reused across calls)
---------------------------------------------------------------------------

local clients = {}

local function dev(port)
	if not clients[port] then
		local c = bs.client(port)
		c.OutTerminator = "\n"
		c.InTerminator  = "\n"
		clients[port] = c
	end
	return clients[port]
end


---------------------------------------------------------------------------
-- Records and callback functions
---------------------------------------------------------------------------

-- Read temperature: send "MEAS:TEMP?" and parse float response
db.record("ai", P .. "temperature") {
	DTYP = "lua",
	INP  = "@bytestream_device read_temperature('" .. PORT .. "')",
	SCAN = "1 second",
	EGU  = "degC",
	PREC = "2",
}

function read_temperature(port)
	return dev(port):write("MEAS:TEMP?"):read("%f")
end


-- Read voltage
db.record("ai", P .. "voltage") {
	DTYP = "lua",
	INP  = "@bytestream_device read_voltage('" .. PORT .. "')",
	SCAN = "1 second",
	EGU  = "V",
	PREC = "3",
}

function read_voltage(port)
	return dev(port):write("MEAS:VOLT?"):read("%f")
end


-- Read device status as enum index (0=off, 1=on, 2=standby)
db.record("longin", P .. "status") {
	DTYP = "lua",
	INP  = "@bytestream_device read_status('" .. PORT .. "')",
	SCAN = "1 second",
}

function read_status(port)
	return dev(port):write("STAT?"):read("%{off|on|standby}")
end


-- Set output voltage with formatted command
db.record("ao", P .. "setVoltage") {
	DTYP = "lua",
	OUT  = "@bytestream_device set_voltage('" .. PORT .. "')",
	EGU  = "V",
	PREC = "3",
}

function set_voltage(port, pv)
	dev(port):write("SET:VOLT %.3f", pv.VAL)
end


-- Set operating mode via enum
db.record("mbbo", P .. "setMode") {
	DTYP = "lua",
	OUT  = "@bytestream_device set_mode('" .. PORT .. "')",
	ZRST = "off",
	ONST = "on",
	TWST = "standby",
}

function set_mode(port, pv)
	dev(port):write("MODE %{off|on|standby}", pv.RVAL)
end
