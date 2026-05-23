-- Lua script for luascriptRecord file-based tests

function test_add()
    return A + B
end

-- Array I/O test functions

function arr_sum()
    if type(AA) ~= "table" then return -1 end
    local s = 0
    for _, v in ipairs(AA) do s = s + v end
    return s
end

function arr_count()
    if type(AA) ~= "table" then return -1 end
    return #AA
end

function str_len()
    if type(AA) ~= "string" then return -1 end
    return #AA
end

function str_first()
    if type(AA) ~= "table" then return "" end
    return AA[1] or ""
end
