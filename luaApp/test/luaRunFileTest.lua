-- Fixture for Stage 2 luaRunFile tests.
--
-- Uses locals defined on one line and used on a later line: this only
-- works because luaRunFile compiles the whole file as a SINGLE chunk.
-- A line-by-line REPL would put each line in its own chunk and 'base'
-- would be nil on the second line.
--
-- The result is written to the file named by the LUA_RUNFILE_OUT
-- environment variable so the C test can observe it after the
-- (separate) run-state has been closed. The macro P, if supplied, is
-- appended so macro delivery can be verified too.

local base = 21
local doubled = base * 2

-- Optional startup delay (seconds) so async execution can be
-- distinguished from synchronous: with a delay, a synchronous run
-- blocks the caller for the whole delay, while an async run returns
-- immediately and the output appears only after the delay.
local delay = os.getenv("LUA_RUNFILE_DELAY")
if delay ~= nil then
    local secs = tonumber(delay) or 0
    local t0 = os.clock()
    while os.clock() - t0 < secs do end   -- busy-wait (portable, no posix dep)
end

local outpath = os.getenv("LUA_RUNFILE_OUT")

if outpath ~= nil then
    local f = io.open(outpath, "w")
    if f ~= nil then
        f:write(tostring(doubled))
        if P ~= nil then
            f:write(",")
            f:write(tostring(P))
        end
        f:close()
    end
end
