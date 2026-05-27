-- Lua Shell Example IOC
--
-- Demonstrates using the Lua shell as a replacement for the EPICS
-- ioc shell.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

-- You can enable hash comments, if you want better compatibility with iocShell scripts
#ENABLE_HASH_COMMENTS

#print("This won't get run")

-- Also allows for silent comments
#- print("This wont' get shown")

-- Get the version of Epics we're running under

-- Convert strings to numbers
VERSION      = 0 + EPICS_VERSION_MAJOR
REVISION     = 0 + EPICS_VERSION_MIDDLE
MODIFICATION = 0 + EPICS_VERSION_MINOR

-- Create a comparable number
VERSION_INT   = VERSION << 16 | REVISION << 8 | MODIFICATION;
VERSION_CHECK = 3 << 16 | 15 << 8 | 6;

-- Example to check if user is using a specific version of EPICS
if (VERSION_INT < VERSION_CHECK) then
	print("You are using a version below base-3.15.6")
end

-- LUA_SCRIPT_PATH is an environment variable that provides a search path to 
-- find scripts for luash, luaSpawn, and luaRecord 
epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

-- The '<' command is equivalent to calling luash
< load_userscripts.lua

---------------
iocInit()
---------------

-- Runs a script in the background
luaSpawn("tick.lua")

dbl()

