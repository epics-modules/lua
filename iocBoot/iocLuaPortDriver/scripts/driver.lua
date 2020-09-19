db = require("db")

out_val = START

param.int32.read "CURR_VAL" [[
	return out_val
]]

db.record("ai", P .. R .. "Value") {
	DTYP = "asynInt32",
	SCAN = "I/O Intr",
	INP = "@asyn(" .. PORT .. ",0,0)CURR_VAL",
	PINI = 1
}


param.int32.write "INCREMENTOR" [[
	out_val = out_val + value
	self:writeParam("CURR_VAL", out_val)
]]

db.record("ao", P .. R .. "Increment") {
	DTYP = "asynInt32",
	OUT  = "@asyn(" .. PORT .. ",0,0)INCREMENTOR"
}

param.int32.write "SET_VALUE" [[
	out_val = value
	self:writeParam("CURR_VAL", out_val)
]]

db.record("ao", P .. R .. "Set") {
	DTYP = "asynInt32",
	OUT  = "@asyn(" .. PORT .. ",0,0)SET_VALUE"
}
