-- Test script for the new asyn.driver.new API

local asyn = require("asyn")
local Int32, Float64, Octet = asyn.Int32, asyn.Float64, asyn.Octet

local drv = asyn.driver.new(PORT, {
    Float64 "READBACK" (0.0),
    Int32   "SETPOINT" (0),
    Float64 "COMPUTED" (3.14159),
    Octet   "STR_RB",
    Octet   "STR_SP",
}, function(self)
    self.scale = 2.0
    self.stored = ""
end)

drv.READBACK.read = function(self)
    return drv.SETPOINT.value * self.scale
end

drv.SETPOINT.write = function(self, value)
    drv.SETPOINT.value = value
end

drv.COMPUTED.read = function(self)
    return drv.COMPUTED.value
end

-- Octet read: returns a string LONGER than a stringin VAL field (40),
-- to exercise readOctet truncation/NUL-termination safety.
drv.STR_RB.read = function(self)
    return string.rep("A", 100)
end

-- Octet write: store what was written so it can be read back.
drv.STR_SP.write = function(self, value)
    self.stored = value
end

drv.STR_SP.read = function(self)
    return self.stored
end
