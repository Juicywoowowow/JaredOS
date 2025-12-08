--[[
    QELUT - QELU Table Utilities
    Part of the QELU (Quality Enhanced Lua Utilities) library
    
    Features:
    - Deep operations (copy, merge, equal)
    - Functional programming (map, filter, reduce, forEach)
    - Array operations (flatten, unique, reverse, shuffle, chunk, zip)
    - Object operations (keys, values, entries, pick, omit, invert)
    - Path-based access (get, set, has)
    - Utilities (isEmpty, size, clone, freeze)
    
    @author QELU Contributors
    @license MIT
    @version 1.0.0
]]

local QELUT = {
    _VERSION = "1.0.0",
    _DESCRIPTION = "QELUT - QELU Table Utilities",
    _LICENSE = "MIT"
}

-- ============================================================================
-- Deep Operations
-- ============================================================================

--- Deep copy a table
--- @param tbl table
--- @param seen table|nil
--- @return table
function QELUT.deepCopy(tbl, seen)
    if type(tbl) ~= "table" then
        return tbl
    end
    
    seen = seen or {}
    if seen[tbl] then
        return seen[tbl]
    end
    
    local copy = {}
    seen[tbl] = copy
    
    for k, v in pairs(tbl) do
        copy[QELUT.deepCopy(k, seen)] = QELUT.deepCopy(v, seen)
    end
    
    return setmetatable(copy, getmetatable(tbl))
end

--- Deep merge tables (later tables override earlier ones)
--- @param ... table
--- @return table
function QELUT.deepMerge(...)
    local result = {}
    
    for i = 1, select("#", ...) do
        local tbl = select(i, ...)
        if type(tbl) == "table" then
            for k, v in pairs(tbl) do
                if type(v) == "table" and type(result[k]) == "table" then
                    result[k] = QELUT.deepMerge(result[k], v)
                else
                    result[k] = QELUT.deepCopy(v)
                end
            end
        end
    end
    
    return result
end

--- Deep equality check
--- @param a table
--- @param b table
--- @param seen table|nil
--- @return boolean
function QELUT.deepEqual(a, b, seen)
    if type(a) ~= type(b) then
        return false
    end
    
    if type(a) ~= "table" then
        return a == b
    end
    
    seen = seen or {}
    if seen[a] and seen[a] == b then
        return true
    end
    seen[a] = b
    
    -- Check all keys in a
    for k, v in pairs(a) do
        if not QELUT.deepEqual(v, b[k], seen) then
            return false
        end
    end
    
    -- Check all keys in b
    for k in pairs(b) do
        if a[k] == nil then
            return false
        end
    end
    
    return true
end

-- ============================================================================
-- Functional Programming
-- ============================================================================

--- Map array elements through function
--- @param arr table
--- @param fn function
--- @return table
function QELUT.map(arr, fn)
    local result = {}
    for i, v in ipairs(arr) do
        result[i] = fn(v, i, arr)
    end
    return result
end

--- Filter array elements
--- @param arr table
--- @param fn function
--- @return table
function QELUT.filter(arr, fn)
    local result = {}
    for i, v in ipairs(arr) do
        if fn(v, i, arr) then
            result[#result + 1] = v
        end
    end
    return result
end

--- Reduce array to single value
--- @param arr table
--- @param fn function
--- @param initial any
--- @return any
function QELUT.reduce(arr, fn, initial)
    local acc = initial
    local startIndex = 1
    
    if initial == nil then
        acc = arr[1]
        startIndex = 2
    end
    
    for i = startIndex, #arr do
        acc = fn(acc, arr[i], i, arr)
    end
    
    return acc
end

--- Execute function for each element
--- @param arr table
--- @param fn function
function QELUT.forEach(arr, fn)
    for i, v in ipairs(arr) do
        fn(v, i, arr)
    end
end

--- Find first element matching predicate
--- @param arr table
--- @param fn function
--- @return any, number|nil
function QELUT.find(arr, fn)
    for i, v in ipairs(arr) do
        if fn(v, i, arr) then
            return v, i
        end
    end
    return nil, nil
end

--- Check if some elements match predicate
--- @param arr table
--- @param fn function
--- @return boolean
function QELUT.some(arr, fn)
    for i, v in ipairs(arr) do
        if fn(v, i, arr) then
            return true
        end
    end
    return false
end

--- Check if all elements match predicate
--- @param arr table
--- @param fn function
--- @return boolean
function QELUT.every(arr, fn)
    for i, v in ipairs(arr) do
        if not fn(v, i, arr) then
            return false
        end
    end
    return true
end

-- ============================================================================
-- Array Operations
-- ============================================================================

--- Flatten nested arrays
--- @param arr table
--- @param depth number|nil Maximum depth (default: infinite)
--- @return table
function QELUT.flatten(arr, depth)
    depth = depth or math.huge
    local result = {}
    
    local function flattenHelper(tbl, currentDepth)
        for _, v in ipairs(tbl) do
            if type(v) == "table" and currentDepth < depth then
                flattenHelper(v, currentDepth + 1)
            else
                result[#result + 1] = v
            end
        end
    end
    
    flattenHelper(arr, 0)
    return result
end

--- Get unique elements
--- @param arr table
--- @return table
function QELUT.unique(arr)
    local seen = {}
    local result = {}
    
    for _, v in ipairs(arr) do
        if not seen[v] then
            seen[v] = true
            result[#result + 1] = v
        end
    end
    
    return result
end

--- Reverse array
--- @param arr table
--- @return table
function QELUT.reverse(arr)
    local result = {}
    for i = #arr, 1, -1 do
        result[#result + 1] = arr[i]
    end
    return result
end

--- Shuffle array (Fisher-Yates)
--- @param arr table
--- @return table
function QELUT.shuffle(arr)
    local result = QELUT.deepCopy(arr)
    for i = #result, 2, -1 do
        local j = math.random(i)
        result[i], result[j] = result[j], result[i]
    end
    return result
end

--- Split array into chunks
--- @param arr table
--- @param size number
--- @return table
function QELUT.chunk(arr, size)
    local result = {}
    for i = 1, #arr, size do
        local chunk = {}
        for j = i, math.min(i + size - 1, #arr) do
            chunk[#chunk + 1] = arr[j]
        end
        result[#result + 1] = chunk
    end
    return result
end

--- Zip multiple arrays together
--- @param ... table
--- @return table
function QELUT.zip(...)
    local arrays = {...}
    local result = {}
    local maxLen = 0
    
    for _, arr in ipairs(arrays) do
        maxLen = math.max(maxLen, #arr)
    end
    
    for i = 1, maxLen do
        local tuple = {}
        for _, arr in ipairs(arrays) do
            tuple[#tuple + 1] = arr[i]
        end
        result[i] = tuple
    end
    
    return result
end

--- Partition array into two based on predicate
--- @param arr table
--- @param fn function
--- @return table, table
function QELUT.partition(arr, fn)
    local truthy = {}
    local falsy = {}
    
    for i, v in ipairs(arr) do
        if fn(v, i, arr) then
            truthy[#truthy + 1] = v
        else
            falsy[#falsy + 1] = v
        end
    end
    
    return truthy, falsy
end

--- Take first n elements
--- @param arr table
--- @param n number
--- @return table
function QELUT.take(arr, n)
    local result = {}
    for i = 1, math.min(n, #arr) do
        result[i] = arr[i]
    end
    return result
end

--- Drop first n elements
--- @param arr table
--- @param n number
--- @return table
function QELUT.drop(arr, n)
    local result = {}
    for i = n + 1, #arr do
        result[#result + 1] = arr[i]
    end
    return result
end

--- Slice array
--- @param arr table
--- @param startIdx number
--- @param endIdx number|nil
--- @return table
function QELUT.slice(arr, startIdx, endIdx)
    endIdx = endIdx or #arr
    local result = {}
    for i = startIdx, endIdx do
        result[#result + 1] = arr[i]
    end
    return result
end

-- ============================================================================
-- Object Operations
-- ============================================================================

--- Get all keys
--- @param tbl table
--- @return table
function QELUT.keys(tbl)
    local result = {}
    for k in pairs(tbl) do
        result[#result + 1] = k
    end
    return result
end

--- Get all values
--- @param tbl table
--- @return table
function QELUT.values(tbl)
    local result = {}
    for _, v in pairs(tbl) do
        result[#result + 1] = v
    end
    return result
end

--- Get key-value pairs as array of {key, value}
--- @param tbl table
--- @return table
function QELUT.entries(tbl)
    local result = {}
    for k, v in pairs(tbl) do
        result[#result + 1] = {k, v}
    end
    return result
end

--- Create table from entries
--- @param entries table
--- @return table
function QELUT.fromEntries(entries)
    local result = {}
    for _, entry in ipairs(entries) do
        result[entry[1]] = entry[2]
    end
    return result
end

--- Pick specific keys from table
--- @param tbl table
--- @param keys table
--- @return table
function QELUT.pick(tbl, keys)
    local result = {}
    for _, key in ipairs(keys) do
        if tbl[key] ~= nil then
            result[key] = tbl[key]
        end
    end
    return result
end

--- Omit specific keys from table
--- @param tbl table
--- @param keys table
--- @return table
function QELUT.omit(tbl, keys)
    local omitSet = {}
    for _, key in ipairs(keys) do
        omitSet[key] = true
    end
    
    local result = {}
    for k, v in pairs(tbl) do
        if not omitSet[k] then
            result[k] = v
        end
    end
    return result
end

--- Invert table (swap keys and values)
--- @param tbl table
--- @return table
function QELUT.invert(tbl)
    local result = {}
    for k, v in pairs(tbl) do
        result[v] = k
    end
    return result
end

--- Merge tables (shallow)
--- @param ... table
--- @return table
function QELUT.merge(...)
    local result = {}
    for i = 1, select("#", ...) do
        local tbl = select(i, ...)
        if type(tbl) == "table" then
            for k, v in pairs(tbl) do
                result[k] = v
            end
        end
    end
    return result
end

-- ============================================================================
-- Path-based Access
-- ============================================================================

--- Get value at path
--- @param tbl table
--- @param path string Dot-separated path like "a.b.c"
--- @param default any
--- @return any
function QELUT.get(tbl, path, default)
    local current = tbl
    
    for key in path:gmatch("[^.]+") do
        if type(current) ~= "table" then
            return default
        end
        current = current[key]
        if current == nil then
            return default
        end
    end
    
    return current
end

--- Set value at path
--- @param tbl table
--- @param path string
--- @param value any
function QELUT.set(tbl, path, value)
    local keys = {}
    for key in path:gmatch("[^.]+") do
        keys[#keys + 1] = key
    end
    
    local current = tbl
    for i = 1, #keys - 1 do
        local key = keys[i]
        if type(current[key]) ~= "table" then
            current[key] = {}
        end
        current = current[key]
    end
    
    current[keys[#keys]] = value
end

--- Check if path exists
--- @param tbl table
--- @param path string
--- @return boolean
function QELUT.has(tbl, path)
    local current = tbl
    
    for key in path:gmatch("[^.]+") do
        if type(current) ~= "table" or current[key] == nil then
            return false
        end
        current = current[key]
    end
    
    return true
end

-- ============================================================================
-- Utilities
-- ============================================================================

--- Check if table is empty
--- @param tbl table
--- @return boolean
function QELUT.isEmpty(tbl)
    return next(tbl) == nil
end

--- Get table size (number of key-value pairs)
--- @param tbl table
--- @return number
function QELUT.size(tbl)
    local count = 0
    for _ in pairs(tbl) do
        count = count + 1
    end
    return count
end

--- Shallow copy
--- @param tbl table
--- @return table
function QELUT.clone(tbl)
    local result = {}
    for k, v in pairs(tbl) do
        result[k] = v
    end
    return setmetatable(result, getmetatable(tbl))
end

--- Clear all entries
--- @param tbl table
function QELUT.clear(tbl)
    for k in pairs(tbl) do
        tbl[k] = nil
    end
end

--- Count elements matching predicate
--- @param tbl table
--- @param fn function
--- @return number
function QELUT.count(tbl, fn)
    local count = 0
    for k, v in pairs(tbl) do
        if fn(v, k, tbl) then
            count = count + 1
        end
    end
    return count
end

--- Group array elements by key function
--- @param arr table
--- @param fn function
--- @return table
function QELUT.groupBy(arr, fn)
    local result = {}
    for i, v in ipairs(arr) do
        local key = fn(v, i, arr)
        if not result[key] then
            result[key] = {}
        end
        result[key][#result[key] + 1] = v
    end
    return result
end

--- Sort array (returns new array)
--- @param arr table
--- @param compareFn function|nil
--- @return table
function QELUT.sort(arr, compareFn)
    local result = QELUT.clone(arr)
    table.sort(result, compareFn)
    return result
end

--- Compact array (remove nil, false values)
--- @param arr table
--- @return table
function QELUT.compact(arr)
    local result = {}
    for _, v in ipairs(arr) do
        if v then
            result[#result + 1] = v
        end
    end
    return result
end

--- Difference between arrays
--- @param arr1 table
--- @param arr2 table
--- @return table
function QELUT.difference(arr1, arr2)
    local set = {}
    for _, v in ipairs(arr2) do
        set[v] = true
    end
    
    local result = {}
    for _, v in ipairs(arr1) do
        if not set[v] then
            result[#result + 1] = v
        end
    end
    return result
end

--- Intersection of arrays
--- @param arr1 table
--- @param arr2 table
--- @return table
function QELUT.intersection(arr1, arr2)
    local set = {}
    for _, v in ipairs(arr2) do
        set[v] = true
    end
    
    local result = {}
    local seen = {}
    for _, v in ipairs(arr1) do
        if set[v] and not seen[v] then
            result[#result + 1] = v
            seen[v] = true
        end
    end
    return result
end

--- Union of arrays
--- @param ... table
--- @return table
function QELUT.union(...)
    local seen = {}
    local result = {}
    
    for i = 1, select("#", ...) do
        local arr = select(i, ...)
        for _, v in ipairs(arr) do
            if not seen[v] then
                seen[v] = true
                result[#result + 1] = v
            end
        end
    end
    
    return result
end

-- ============================================================================
-- Module Export
-- ============================================================================

return QELUT
