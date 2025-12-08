--[[
    QELUJ - QELU JSON Library
    Part of the QELU (Quality Enhanced Lua Utilities) library
    
    Features:
    - JSON encoding/decoding
    - Pretty printing with indentation
    - Strict mode for validation
    - File I/O support
    - Handles nested structures, arrays, objects
    - Proper escape sequences
    - Number precision handling
    - Null handling
    
    @author QELU Contributors
    @license MIT
    @version 1.0.0
]]

local QELUJ = {
    _VERSION = "1.0.0",
    _DESCRIPTION = "QELUJ - QELU JSON Library",
    _LICENSE = "MIT"
}

-- ============================================================================
-- Configuration
-- ============================================================================

QELUJ.config = {
    strictMode = false,      -- Strict JSON validation
    nullValue = nil,         -- Value to use for JSON null
    maxDepth = 100,          -- Maximum nesting depth
    prettyIndent = "  ",     -- Indentation for pretty printing
}

-- ============================================================================
-- Encoding
-- ============================================================================

local function encodeString(str)
    local replacements = {
        ["\\"] = "\\\\",
        ['"'] = '\\"',
        ["\n"] = "\\n",
        ["\r"] = "\\r",
        ["\t"] = "\\t",
        ["\b"] = "\\b",
        ["\f"] = "\\f",
    }
    
    return '"' .. str:gsub('[\\"\n\r\t\b\f]', replacements) .. '"'
end

local function encodeValue(value, options, depth)
    depth = depth or 0
    
    if depth > (options.maxDepth or QELUJ.config.maxDepth) then
        error("Maximum depth exceeded")
    end
    
    local t = type(value)
    
    if t == "nil" then
        return "null"
    elseif t == "boolean" then
        return value and "true" or "false"
    elseif t == "number" then
        if value ~= value then
            return "null"  -- NaN
        elseif value == math.huge then
            return "null"  -- Infinity
        elseif value == -math.huge then
            return "null"  -- -Infinity
        else
            return tostring(value)
        end
    elseif t == "string" then
        return encodeString(value)
    elseif t == "table" then
        -- Check if it's an array
        local isArray = true
        local maxIndex = 0
        local count = 0
        
        for k, _ in pairs(value) do
            count = count + 1
            if type(k) ~= "number" or k ~= math.floor(k) or k < 1 then
                isArray = false
                break
            end
            if k > maxIndex then
                maxIndex = k
            end
        end
        
        -- Check for holes in array
        if isArray and maxIndex ~= count then
            isArray = false
        end
        
        if isArray and count > 0 then
            -- Encode as array
            local parts = {}
            for i = 1, maxIndex do
                parts[#parts + 1] = encodeValue(value[i], options, depth + 1)
            end
            
            if options.pretty then
                local indent = string.rep(options.indent or QELUJ.config.prettyIndent, depth)
                local innerIndent = string.rep(options.indent or QELUJ.config.prettyIndent, depth + 1)
                return "[\n" .. innerIndent .. table.concat(parts, ",\n" .. innerIndent) .. "\n" .. indent .. "]"
            else
                return "[" .. table.concat(parts, ",") .. "]"
            end
        else
            -- Encode as object
            local parts = {}
            for k, v in pairs(value) do
                if type(k) == "string" then
                    parts[#parts + 1] = encodeString(k) .. ":" .. (options.pretty and " " or "") .. encodeValue(v, options, depth + 1)
                elseif not options.strict then
                    -- In non-strict mode, convert non-string keys to strings
                    parts[#parts + 1] = encodeString(tostring(k)) .. ":" .. (options.pretty and " " or "") .. encodeValue(v, options, depth + 1)
                end
            end
            
            if #parts == 0 then
                return "{}"
            end
            
            if options.pretty then
                local indent = string.rep(options.indent or QELUJ.config.prettyIndent, depth)
                local innerIndent = string.rep(options.indent or QELUJ.config.prettyIndent, depth + 1)
                return "{\n" .. innerIndent .. table.concat(parts, ",\n" .. innerIndent) .. "\n" .. indent .. "}"
            else
                return "{" .. table.concat(parts, ",") .. "}"
            end
        end
    else
        if options.strict or QELUJ.config.strictMode then
            error("Cannot encode type: " .. t)
        else
            return "null"
        end
    end
end

--- Encode a Lua value to JSON string
--- @param value any
--- @param options table|nil {pretty: boolean, indent: string, strict: boolean, maxDepth: number}
--- @return string
function QELUJ.encode(value, options)
    options = options or {}
    return encodeValue(value, options, 0)
end

--- Encode to pretty-printed JSON
--- @param value any
--- @param indent string|nil
--- @return string
function QELUJ.encodePretty(value, indent)
    return QELUJ.encode(value, {pretty = true, indent = indent})
end

-- ============================================================================
-- Decoding
-- ============================================================================

local function skipWhitespace(str, pos)
    while pos <= #str do
        local c = str:sub(pos, pos)
        if c ~= " " and c ~= "\t" and c ~= "\n" and c ~= "\r" then
            break
        end
        pos = pos + 1
    end
    return pos
end

local function decodeString(str, pos)
    local result = {}
    pos = pos + 1  -- Skip opening quote
    
    while pos <= #str do
        local c = str:sub(pos, pos)
        
        if c == '"' then
            return table.concat(result), pos + 1
        elseif c == "\\" then
            pos = pos + 1
            local escape = str:sub(pos, pos)
            
            if escape == '"' then
                result[#result + 1] = '"'
            elseif escape == "\\" then
                result[#result + 1] = "\\"
            elseif escape == "/" then
                result[#result + 1] = "/"
            elseif escape == "n" then
                result[#result + 1] = "\n"
            elseif escape == "r" then
                result[#result + 1] = "\r"
            elseif escape == "t" then
                result[#result + 1] = "\t"
            elseif escape == "b" then
                result[#result + 1] = "\b"
            elseif escape == "f" then
                result[#result + 1] = "\f"
            elseif escape == "u" then
                -- Unicode escape (basic support)
                local hex = str:sub(pos + 1, pos + 4)
                local codepoint = tonumber(hex, 16)
                if codepoint then
                    result[#result + 1] = string.char(codepoint)
                    pos = pos + 4
                end
            else
                error("Invalid escape sequence: \\" .. escape)
            end
            pos = pos + 1
        else
            result[#result + 1] = c
            pos = pos + 1
        end
    end
    
    error("Unterminated string")
end

local function decodeNumber(str, pos)
    local numStr = str:match("^-?%d+%.?%d*[eE]?[+-]?%d*", pos)
    if not numStr then
        error("Invalid number at position " .. pos)
    end
    
    return tonumber(numStr), pos + #numStr
end

local decodeValue  -- Forward declaration

local function decodeArray(str, pos, options)
    local result = {}
    pos = pos + 1  -- Skip opening bracket
    pos = skipWhitespace(str, pos)
    
    -- Empty array
    if str:sub(pos, pos) == "]" then
        return result, pos + 1
    end
    
    while pos <= #str do
        local value
        value, pos = decodeValue(str, pos, options)
        result[#result + 1] = value
        
        pos = skipWhitespace(str, pos)
        local c = str:sub(pos, pos)
        
        if c == "]" then
            return result, pos + 1
        elseif c == "," then
            pos = pos + 1
            pos = skipWhitespace(str, pos)
        else
            error("Expected ',' or ']' at position " .. pos)
        end
    end
    
    error("Unterminated array")
end

local function decodeObject(str, pos, options)
    local result = {}
    pos = pos + 1  -- Skip opening brace
    pos = skipWhitespace(str, pos)
    
    -- Empty object
    if str:sub(pos, pos) == "}" then
        return result, pos + 1
    end
    
    while pos <= #str do
        pos = skipWhitespace(str, pos)
        
        -- Parse key
        if str:sub(pos, pos) ~= '"' then
            error("Expected string key at position " .. pos)
        end
        
        local key
        key, pos = decodeString(str, pos)
        
        pos = skipWhitespace(str, pos)
        
        -- Expect colon
        if str:sub(pos, pos) ~= ":" then
            error("Expected ':' at position " .. pos)
        end
        pos = pos + 1
        
        pos = skipWhitespace(str, pos)
        
        -- Parse value
        local value
        value, pos = decodeValue(str, pos, options)
        result[key] = value
        
        pos = skipWhitespace(str, pos)
        local c = str:sub(pos, pos)
        
        if c == "}" then
            return result, pos + 1
        elseif c == "," then
            pos = pos + 1
        else
            error("Expected ',' or '}' at position " .. pos)
        end
    end
    
    error("Unterminated object")
end

function decodeValue(str, pos, options)
    options = options or {}
    pos = skipWhitespace(str, pos)
    
    local c = str:sub(pos, pos)
    
    if c == '"' then
        return decodeString(str, pos)
    elseif c == "{" then
        return decodeObject(str, pos, options)
    elseif c == "[" then
        return decodeArray(str, pos, options)
    elseif c == "t" then
        if str:sub(pos, pos + 3) == "true" then
            return true, pos + 4
        end
    elseif c == "f" then
        if str:sub(pos, pos + 4) == "false" then
            return false, pos + 5
        end
    elseif c == "n" then
        if str:sub(pos, pos + 3) == "null" then
            return options.nullValue or QELUJ.config.nullValue, pos + 4
        end
    elseif c == "-" or (c >= "0" and c <= "9") then
        return decodeNumber(str, pos)
    end
    
    error("Unexpected character '" .. c .. "' at position " .. pos)
end

--- Decode JSON string to Lua value
--- @param str string
--- @param options table|nil {strict: boolean, nullValue: any}
--- @return any
function QELUJ.decode(str, options)
    options = options or {}
    
    if type(str) ~= "string" then
        error("Expected string, got " .. type(str))
    end
    
    local value, pos = decodeValue(str, 1, options)
    
    -- Check for trailing content
    pos = skipWhitespace(str, pos)
    if pos <= #str and (options.strict or QELUJ.config.strictMode) then
        error("Unexpected content after JSON at position " .. pos)
    end
    
    return value
end

-- ============================================================================
-- File I/O
-- ============================================================================

--- Encode value and write to file
--- @param value any
--- @param filepath string
--- @param options table|nil
function QELUJ.encodeFile(value, filepath, options)
    local json = QELUJ.encode(value, options)
    local file = io.open(filepath, "w")
    if not file then
        error("Cannot open file for writing: " .. filepath)
    end
    file:write(json)
    file:close()
end

--- Read file and decode JSON
--- @param filepath string
--- @param options table|nil
--- @return any
function QELUJ.decodeFile(filepath, options)
    local file = io.open(filepath, "r")
    if not file then
        error("Cannot open file for reading: " .. filepath)
    end
    local content = file:read("*all")
    file:close()
    return QELUJ.decode(content, options)
end

-- ============================================================================
-- Utilities
-- ============================================================================

--- Check if string is valid JSON
--- @param str string
--- @return boolean
function QELUJ.isValid(str)
    local ok = pcall(QELUJ.decode, str)
    return ok
end

--- Minify JSON string (remove whitespace)
--- @param str string
--- @return string
function QELUJ.minify(str)
    local value = QELUJ.decode(str)
    return QELUJ.encode(value)
end

--- Prettify JSON string
--- @param str string
--- @param indent string|nil
--- @return string
function QELUJ.prettify(str, indent)
    local value = QELUJ.decode(str)
    return QELUJ.encodePretty(value, indent)
end

-- ============================================================================
-- Module Export
-- ============================================================================

return QELUJ
