db = require("db")


-- START is a value given as a parameter 
-- when the port driver is created.

out_val = START


--[[
	CURR_VAL / $(P)$(R)Value
 
	Returns the current value saved in the
	state.
--]]


param.int32.read "CURR_VAL" [[
	return out_val
]]

db.record("ai", P .. R .. "Value") {
	DTYP = "asynInt32",
	SCAN = "I/O Intr",
	INP = "@asyn(" .. PORT .. ",0,0)CURR_VAL",
	PINI = 1
}


--[[
	INCREMENTOR / $(P)$(R)Increment
   
	Increments the current value by the
	value of $(P)$(R)Increment
--]]

--[[ 
	self and value are implicit parameters that the port driver sets. 
	self is a driver object (see the asyn library documentation) 
	representing the port driver, and value is the value taken from
	the asyn callback.
--]]
param.int32.write "INCREMENTOR" [[
	out_val = out_val + value
	self:writeParam("CURR_VAL", out_val)
	self:callParamCallbacks()
]]

db.record("ao", P .. R .. "Increment") {
	DTYP = "asynInt32",
	OUT  = "@asyn(" .. PORT .. ",0,0)INCREMENTOR"
}


--------------------------------------------
-- SET_VALUE / $(P)$(R)Set
--
-- Sets the current value to the value
-- of $(P)$(R)Set
--------------------------------------------


param.int32.write "SET_VALUE" [[
	out_val = value
	self:writeParam("CURR_VAL", out_val)
	self:callParamCallbacks()
]]

db.record("ao", P .. R .. "Set") {
	DTYP = "asynInt32",
	OUT  = "@asyn(" .. PORT .. ",0,0)SET_VALUE"
}
