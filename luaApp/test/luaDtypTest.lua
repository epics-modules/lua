-- Lua script for DTYP "lua" device support tests

function read_double(record, val)
    return val
end

function read_int(record, val)
    return math.tointeger(val)
end

function read_one(record)
    return 1
end

function read_string(record)
    return "test_value"
end

function read_on_string(record)
    return "On"
end

function write_noop(record)
    -- do nothing, just verify the call works
end

function read_error(record)
    error("intentional test error")
end
