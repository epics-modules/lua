out_val = START

param.int32.read "INCREMENTOR" [[
	out_val = out_val + 1
	return out_val
]]

param.int32.write "SET_VALUE" [[
	out_val = value
]]
