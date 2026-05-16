-- Test script for the new asyn.driver.new API

local asyn = require("asyn")
local Int32, Float64 = asyn.Int32, asyn.Float64

local drv = asyn.driver.new(PORT, {
    Float64 "READBACK" (0.0),
    Int32   "SETPOINT" (0),
    Float64 "COMPUTED" (3.14159),
}, function(self)
    self.scale = 2.0
end)

drv.READBACK.read = function(self)
    return drv.SETPOINT.value * self.scale
end

drv.SETPOINT.write = function(value, self)
    drv.SETPOINT.value = value
end

drv.COMPUTED.read = function(self)
    return drv.COMPUTED.value
end
