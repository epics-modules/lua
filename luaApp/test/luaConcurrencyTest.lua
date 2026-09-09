-- Shared-state script for the concurrency stress test.
--
-- These functions deliberately do stack-intensive work (build and sum
-- a table, string work) and mutate shared globals so that unsynchronized
-- concurrent access to a single lua_State corrupts the stack/heap.

shared_counter = 0

-- ai/longin style: returns a number, bumps the shared counter.
function stress_read(record)
    local t = {}
    for i = 1, 32 do t[i] = i end

    local sum = 0
    for i = 1, #t do sum = sum + t[i] end

    shared_counter = shared_counter + 1

    return sum       -- always 528 (= 32*33/2)
end

-- stringin style: returns a string, also bumps the counter.
function stress_read_string(record)
    local parts = {}
    for i = 1, 16 do parts[i] = tostring(i) end

    shared_counter = shared_counter + 1

    return table.concat(parts, ",")
end

-- output style: consumes the record, bumps the counter, returns nil.
function stress_write(record)
    local t = {}
    for i = 1, 32 do t[i] = i * 2 end

    shared_counter = shared_counter + 1
end
