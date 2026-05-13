-- Lua port driver definition for tests

local current_value = 0

param.int32.read "READBACK" [[
    return current_value
]]

param.int32.write "SETPOINT" [[
    current_value = value
]]

param.float64.read "FLOAT_RB" [[
    return 3.14159
]]

param.int32 "BASIC_PARAM"
