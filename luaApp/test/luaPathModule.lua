-- Require-able module fixture for Stage 4 path-unification tests.
-- Placed on LUA_SCRIPT_PATH; the tests verify that a directory on
-- LUA_SCRIPT_PATH is reachable by require() (not just @file), proving
-- the unified path registry feeds both resolvers.

local M = {}

function M.marker()
    return "luaPathModule-loaded"
end

return M
