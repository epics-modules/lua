-- Load a set of userscript records
for i = 1, 3 do
	dbLoadRecords("../../luaApp/Db/luascripts10.db", "P=lua:,R=set" .. i .. ":")
end
