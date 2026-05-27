local asyn = require("asyn")
local db = require("db")

local Int32, Float64 = asyn.Int32, asyn.Float64

luaRegisterState(PORT)

local drv = asyn.driver.new(PORT, {
    Float64 "READBACK" (0.0),
    Float64 "SETPOINT" (0.0),
}, function(self)
    self.scale = 2.0
end)

drv.READBACK.read = function(self)
    return self.SETPOINT.value * self.scale
end

drv.SETPOINT.write = function(value, self)
    self.SETPOINT.value = value
end


db.record("ai", P .. "readback") {
    DTYP = "asynFloat64",
    INP  = "@asyn(" .. PORT .. ",0) READBACK",
    SCAN = "Passive",
}

db.record("ao", P .. "setpoint") {
    DTYP = "asynFloat64",
    OUT  = "@asyn(" .. PORT .. ",0) SETPOINT",
    SCAN = "Passive",
    FLNK = P .. "readback",
}
