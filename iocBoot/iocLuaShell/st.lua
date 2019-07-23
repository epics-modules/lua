-- Get the version of Epics we're running under

-- Convert strings to numbers
VERSION      = 0 + os.getenv("EPICS_VERSION_MAJOR")
REVISION     = 0 + os.getenv("EPICS_VERSION_MIDDLE")
MODIFICATION = 0 + os.getenv("EPICS_VERSION_MINOR")

-- Create a comparable number
VERSION_INT   = VERSION << 16 | REVISION << 8 | MODIFICATION;
VERSION_CHECK = 3 << 16 | 15 << 8 | 6;

-- Example to check if user is using a specific version of EPICS
if (VERSION_INT < VERSION_CHECK) then
	print("You are using a version below base-3.15.6")
end

epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

< load_userscripts.lua

---------------
iocInit()
---------------

-- Runs a script in the background
luaSpawn("tick.lua")

dbl()

