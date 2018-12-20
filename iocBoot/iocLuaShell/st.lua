-- Get the version of Epics we're running under

-- Convert strings to numbers
VERSION      = 0 + os.getenv("EPICS_VERSION_MAJOR")
REVISION     = 0 + os.getenv("EPICS_VERSION_MIDDLE")
MODIFICATION = 0 + os.getenv("EPICS_VERSION_MINOR")

-- Create a comparable number
VERSION_INT   = VERSION << 16 | REVISION << 8 | MODIFICATION;
VERSION_CHECK = 3 << 16 | 15 << 8 | 6;

-- Require 3.15.6 or higher for the iocsh library
if (VERSION_INT <= VERSION_CHECK) then
	print("Requires Epics Base version 3.15.6 or higher")
	os.exit()
end

iocsh.epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

iocsh.dbLoadDatabase("../../dbd/testLuaShell.dbd")
iocsh.testLuaShell_registerRecordDeviceDriver(pdbbase)

-- If the database registered any new libraries, load them
shell.loadRegistered()

< load_userscripts.lua

---------------
iocsh.iocInit()
---------------

-- Runs a script in the background
iocsh.luaSpawn("tick.lua")

iocsh.dbl()

