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

-- Returns a 64-bit value larger than 2^32 to prove int64 precision is
-- preserved end-to-end (would be truncated/rounded through 32-bit or double).
function read_bignum(record)
    return math.tointeger(5000000000)   -- 5e9, > 2^32
end

-- Bitfield value for mbbiDirect (bits 0 and 2 set => 5).
function read_bits(record)
    return math.tointeger(5)
end

-- A string longer than the 40-char stringin/stringout limit, to exercise
-- the lsi/lso long-string path.
long_string = string.rep("abcdefghij", 10)   -- 100 chars

function read_longstring(record)
    return long_string
end

-- Array (waveform/aai) read callbacks.
function read_double_array(record)
    return {1.5, 2.5, 3.5, 4.5}
end

function read_int_array(record)
    return {math.tointeger(10), math.tointeger(20), math.tointeger(30)}
end

function read_string_array(record)
    return {"alpha", "beta", "gamma"}
end

-- aao write callback: read the outgoing array via the PV object and stash
-- its element count so the test can confirm the array was received.
aao_sum = 0

function capture_array(record)
    local arr = record.VAL
    aao_sum = 0
    if type(arr) == "table" then
        for _, v in ipairs(arr) do aao_sum = aao_sum + v end
    end
end
