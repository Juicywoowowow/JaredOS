--[[
    QELS - QELU String Utilities
    Part of the QELU (Quality Enhanced Lua Utilities) library
    
    Features:
    - String splitting and joining
    - Trimming and padding
    - Case conversion (camelCase, snake_case, kebab-case, etc.)
    - String checking (startsWith, endsWith, contains, isEmpty)
    - Template strings with placeholders
    - String escaping (HTML, regex, shell)
    - Truncation and wrapping
    - Character utilities
    - Unicode-aware operations (basic)
    
    @author QELU Contributors
    @license MIT
    @version 1.0.0
]]

local QELS = {
    _VERSION = "1.0.0",
    _DESCRIPTION = "QELS - QELU String Utilities",
    _LICENSE = "MIT"
}

-- ============================================================================
-- Splitting & Joining
-- ============================================================================

--- Split a string by delimiter
--- @param str string
--- @param delimiter string
--- @param plain boolean Use plain text matching (default: true)
--- @return table
function QELS.split(str, delimiter, plain)
    if plain == nil then plain = true end
    
    local result = {}
    local pattern = plain and ("([^" .. delimiter .. "]+)") or delimiter
    
    if plain then
        local pos = 1
        while true do
            local startPos, endPos = str:find(delimiter, pos, true)
            if not startPos then
                table.insert(result, str:sub(pos))
                break
            end
            table.insert(result, str:sub(pos, startPos - 1))
            pos = endPos + 1
        end
    else
        for match in str:gmatch(pattern) do
            table.insert(result, match)
        end
    end
    
    return result
end

--- Split string into lines
--- @param str string
--- @return table
function QELS.lines(str)
    local result = {}
    for line in str:gmatch("([^\r\n]*)[\r\n]?") do
        if line ~= "" or #result > 0 then
            table.insert(result, line)
        end
    end
    -- Remove last empty line if present
    if #result > 0 and result[#result] == "" then
        table.remove(result)
    end
    return result
end

--- Join array of strings with delimiter
--- @param arr table
--- @param delimiter string
--- @return string
function QELS.join(arr, delimiter)
    return table.concat(arr, delimiter or "")
end

--- Split string into characters
--- @param str string
--- @return table
function QELS.chars(str)
    local result = {}
    for i = 1, #str do
        result[i] = str:sub(i, i)
    end
    return result
end

-- ============================================================================
-- Trimming
-- ============================================================================

--- Trim whitespace from both ends
--- @param str string
--- @return string
function QELS.trim(str)
    return str:match("^%s*(.-)%s*$")
end

--- Trim whitespace from left
--- @param str string
--- @return string
function QELS.ltrim(str)
    return str:match("^%s*(.*)$")
end

--- Trim whitespace from right
--- @param str string
--- @return string
function QELS.rtrim(str)
    return str:match("^(.-)%s*$")
end

--- Trim specific characters from both ends
--- @param str string
--- @param chars string Characters to trim
--- @return string
function QELS.trimChars(str, chars)
    local pattern = "^[" .. chars .. "]*(.-)[ " .. chars .. "]*$"
    return str:match(pattern)
end

-- ============================================================================
-- Case Conversion
-- ============================================================================

--- Capitalize first letter
--- @param str string
--- @return string
function QELS.capitalize(str)
    return str:sub(1, 1):upper() .. str:sub(2):lower()
end

--- Capitalize first letter of each word
--- @param str string
--- @return string
function QELS.titleCase(str)
    return str:gsub("(%a)([%w_']*)", function(first, rest)
        return first:upper() .. rest:lower()
    end)
end

--- Convert to camelCase
--- @param str string
--- @return string
function QELS.camelCase(str)
    str = str:gsub("[%s_-]+(%w)", function(c) return c:upper() end)
    return str:sub(1, 1):lower() .. str:sub(2)
end

--- Convert to PascalCase
--- @param str string
--- @return string
function QELS.pascalCase(str)
    str = str:gsub("[%s_-]+(%w)", function(c) return c:upper() end)
    return str:sub(1, 1):upper() .. str:sub(2)
end

--- Convert to snake_case
--- @param str string
--- @return string
function QELS.snakeCase(str)
    str = str:gsub("([a-z])([A-Z])", "%1_%2")
    str = str:gsub("[%s-]+", "_")
    return str:lower()
end

--- Convert to kebab-case
--- @param str string
--- @return string
function QELS.kebabCase(str)
    str = str:gsub("([a-z])([A-Z])", "%1-%2")
    str = str:gsub("[%s_]+", "-")
    return str:lower()
end

--- Convert to CONSTANT_CASE
--- @param str string
--- @return string
function QELS.constantCase(str)
    return QELS.snakeCase(str):upper()
end

-- ============================================================================
-- Checking
-- ============================================================================

--- Check if string starts with prefix
--- @param str string
--- @param prefix string
--- @return boolean
function QELS.startsWith(str, prefix)
    return str:sub(1, #prefix) == prefix
end

--- Check if string ends with suffix
--- @param str string
--- @param suffix string
--- @return boolean
function QELS.endsWith(str, suffix)
    return suffix == "" or str:sub(-#suffix) == suffix
end

--- Check if string contains substring
--- @param str string
--- @param substr string
--- @param plain boolean Use plain text search (default: true)
--- @return boolean
function QELS.contains(str, substr, plain)
    if plain == nil then plain = true end
    return str:find(substr, 1, plain) ~= nil
end

--- Check if string is empty or only whitespace
--- @param str string
--- @return boolean
function QELS.isEmpty(str)
    return str:match("^%s*$") ~= nil
end

--- Check if string is blank (nil, empty, or whitespace)
--- @param str string|nil
--- @return boolean
function QELS.isBlank(str)
    return not str or QELS.isEmpty(str)
end

--- Check if string contains only alphabetic characters
--- @param str string
--- @return boolean
function QELS.isAlpha(str)
    return str:match("^%a+$") ~= nil
end

--- Check if string contains only numeric characters
--- @param str string
--- @return boolean
function QELS.isNumeric(str)
    return str:match("^%d+$") ~= nil
end

--- Check if string contains only alphanumeric characters
--- @param str string
--- @return boolean
function QELS.isAlphanumeric(str)
    return str:match("^%w+$") ~= nil
end

--- Check if string is lowercase
--- @param str string
--- @return boolean
function QELS.isLower(str)
    return str == str:lower() and str ~= str:upper()
end

--- Check if string is uppercase
--- @param str string
--- @return boolean
function QELS.isUpper(str)
    return str == str:upper() and str ~= str:lower()
end

-- ============================================================================
-- Padding & Alignment
-- ============================================================================

--- Pad string on the left
--- @param str string
--- @param length number
--- @param char string Padding character (default: space)
--- @return string
function QELS.padLeft(str, length, char)
    char = char or " "
    local padding = string.rep(char, math.max(0, length - #str))
    return padding .. str
end

--- Pad string on the right
--- @param str string
--- @param length number
--- @param char string Padding character (default: space)
--- @return string
function QELS.padRight(str, length, char)
    char = char or " "
    local padding = string.rep(char, math.max(0, length - #str))
    return str .. padding
end

--- Center string with padding
--- @param str string
--- @param length number
--- @param char string Padding character (default: space)
--- @return string
function QELS.center(str, length, char)
    char = char or " "
    local totalPad = math.max(0, length - #str)
    local leftPad = math.floor(totalPad / 2)
    local rightPad = totalPad - leftPad
    return string.rep(char, leftPad) .. str .. string.rep(char, rightPad)
end

-- ============================================================================
-- Templates
-- ============================================================================

--- Simple template replacement with {key} placeholders
--- @param template string
--- @param values table
--- @return string
function QELS.template(template, values)
    return template:gsub("{([%w_]+)}", function(key)
        return tostring(values[key] or "{" .. key .. "}")
    end)
end

--- Format string with %{key} placeholders
--- @param template string
--- @param values table
--- @return string
function QELS.format(template, values)
    return template:gsub("%%{([%w_]+)}", function(key)
        return tostring(values[key] or "%{" .. key .. "}")
    end)
end

--- Interpolate string with ${key} placeholders
--- @param template string
--- @param values table
--- @return string
function QELS.interpolate(template, values)
    return template:gsub("%$%{([%w_]+)%}", function(key)
        return tostring(values[key] or "${" .. key .. "}")
    end)
end

-- ============================================================================
-- Escaping
-- ============================================================================

--- Escape HTML special characters
--- @param str string
--- @return string
function QELS.escapeHtml(str)
    local replacements = {
        ["&"] = "&amp;",
        ["<"] = "&lt;",
        [">"] = "&gt;",
        ['"'] = "&quot;",
        ["'"] = "&#39;"
    }
    return str:gsub("[&<>\"']", replacements)
end

--- Unescape HTML entities
--- @param str string
--- @return string
function QELS.unescapeHtml(str)
    local replacements = {
        ["&amp;"] = "&",
        ["&lt;"] = "<",
        ["&gt;"] = ">",
        ["&quot;"] = '"',
        ["&#39;"] = "'"
    }
    return str:gsub("(&[%w#]+;)", function(entity)
        return replacements[entity] or entity
    end)
end

--- Escape Lua pattern special characters
--- @param str string
--- @return string
function QELS.escapePattern(str)
    return str:gsub("([%^%$%(%)%%%.%[%]%*%+%-%?])", "%%%1")
end

--- Escape shell command arguments
--- @param str string
--- @return string
function QELS.escapeShell(str)
    return "'" .. str:gsub("'", "'\\''") .. "'"
end

--- Escape quotes in string
--- @param str string
--- @param quoteChar string Quote character to escape (default: ")
--- @return string
function QELS.escapeQuotes(str, quoteChar)
    quoteChar = quoteChar or '"'
    return str:gsub(quoteChar, "\\" .. quoteChar)
end

-- ============================================================================
-- Truncation & Wrapping
-- ============================================================================

--- Truncate string to maximum length
--- @param str string
--- @param maxLength number
--- @param suffix string Suffix to add (default: "...")
--- @return string
function QELS.truncate(str, maxLength, suffix)
    suffix = suffix or "..."
    if #str <= maxLength then
        return str
    end
    return str:sub(1, maxLength - #suffix) .. suffix
end

--- Truncate string at word boundary
--- @param str string
--- @param maxLength number
--- @param suffix string Suffix to add (default: "...")
--- @return string
function QELS.truncateWords(str, maxLength, suffix)
    suffix = suffix or "..."
    if #str <= maxLength then
        return str
    end
    
    local truncated = str:sub(1, maxLength - #suffix)
    local lastSpace = truncated:match("^.*()%s")
    
    if lastSpace then
        truncated = truncated:sub(1, lastSpace - 1)
    end
    
    return truncated .. suffix
end

--- Wrap text to specified width
--- @param str string
--- @param width number
--- @return string
function QELS.wrap(str, width)
    local lines = {}
    local currentLine = ""
    
    for word in str:gmatch("%S+") do
        if #currentLine + #word + 1 > width then
            if #currentLine > 0 then
                table.insert(lines, currentLine)
                currentLine = word
            else
                table.insert(lines, word)
            end
        else
            if #currentLine > 0 then
                currentLine = currentLine .. " " .. word
            else
                currentLine = word
            end
        end
    end
    
    if #currentLine > 0 then
        table.insert(lines, currentLine)
    end
    
    return table.concat(lines, "\n")
end

-- ============================================================================
-- Utilities
-- ============================================================================

--- Reverse a string
--- @param str string
--- @return string
function QELS.reverse(str)
    return str:reverse()
end

--- Repeat string n times
--- @param str string
--- @param count number
--- @return string
function QELS.repeat_(str, count)
    return string.rep(str, count)
end

--- Count occurrences of substring
--- @param str string
--- @param substr string
--- @param plain boolean Use plain text search (default: true)
--- @return number
function QELS.count(str, substr, plain)
    if plain == nil then plain = true end
    
    local count = 0
    local pos = 1
    
    while true do
        local startPos = str:find(substr, pos, plain)
        if not startPos then break end
        count = count + 1
        pos = startPos + 1
    end
    
    return count
end

--- Replace all occurrences
--- @param str string
--- @param search string
--- @param replace string
--- @param plain boolean Use plain text search (default: true)
--- @return string
function QELS.replaceAll(str, search, replace, plain)
    if plain == nil then plain = true end
    
    if plain then
        search = QELS.escapePattern(search)
    end
    
    return str:gsub(search, replace)
end

--- Create a slug from string (URL-friendly)
--- @param str string
--- @return string
function QELS.slugify(str)
    str = str:lower()
    str = str:gsub("[àáâãäå]", "a")
    str = str:gsub("[èéêë]", "e")
    str = str:gsub("[ìíîï]", "i")
    str = str:gsub("[òóôõö]", "o")
    str = str:gsub("[ùúûü]", "u")
    str = str:gsub("[ñ]", "n")
    str = str:gsub("[ç]", "c")
    str = str:gsub("[^%w%s-]", "")
    str = str:gsub("%s+", "-")
    str = str:gsub("^-+", "")
    str = str:gsub("-+$", "")
    return str
end

--- Remove all whitespace
--- @param str string
--- @return string
function QELS.removeWhitespace(str)
    return str:gsub("%s+", "")
end

--- Collapse multiple whitespace into single space
--- @param str string
--- @return string
function QELS.collapseWhitespace(str)
    return str:gsub("%s+", " ")
end

--- Extract numbers from string
--- @param str string
--- @return table
function QELS.extractNumbers(str)
    local numbers = {}
    for num in str:gmatch("%-?%d+%.?%d*") do
        table.insert(numbers, tonumber(num))
    end
    return numbers
end

--- Extract words from string
--- @param str string
--- @return table
function QELS.extractWords(str)
    local words = {}
    for word in str:gmatch("%a+") do
        table.insert(words, word)
    end
    return words
end

--- Levenshtein distance (edit distance between two strings)
--- @param str1 string
--- @param str2 string
--- @return number
function QELS.levenshtein(str1, str2)
    local len1, len2 = #str1, #str2
    local matrix = {}
    
    for i = 0, len1 do
        matrix[i] = {[0] = i}
    end
    
    for j = 0, len2 do
        matrix[0][j] = j
    end
    
    for i = 1, len1 do
        for j = 1, len2 do
            local cost = str1:sub(i, i) == str2:sub(j, j) and 0 or 1
            matrix[i][j] = math.min(
                matrix[i-1][j] + 1,      -- deletion
                matrix[i][j-1] + 1,      -- insertion
                matrix[i-1][j-1] + cost  -- substitution
            )
        end
    end
    
    return matrix[len1][len2]
end

--- Calculate string similarity (0-1, based on Levenshtein)
--- @param str1 string
--- @param str2 string
--- @return number
function QELS.similarity(str1, str2)
    local maxLen = math.max(#str1, #str2)
    if maxLen == 0 then return 1 end
    return 1 - (QELS.levenshtein(str1, str2) / maxLen)
end

-- ============================================================================
-- Random & Generation
-- ============================================================================

--- Generate random string
--- @param length number
--- @param charset string Character set (default: alphanumeric)
--- @return string
function QELS.random(length, charset)
    charset = charset or "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    local result = {}
    
    for i = 1, length do
        local randIndex = math.random(1, #charset)
        result[i] = charset:sub(randIndex, randIndex)
    end
    
    return table.concat(result)
end

--- Generate UUID v4
--- @return string
function QELS.uuid()
    local template = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx"
    return template:gsub("[xy]", function(c)
        local v = (c == "x") and math.random(0, 15) or math.random(8, 11)
        return string.format("%x", v)
    end)
end

-- ============================================================================
-- Aliases (for convenience)
-- ============================================================================

QELS.len = string.len
QELS.upper = string.upper
QELS.lower = string.lower
QELS.sub = string.sub
QELS.find = string.find
QELS.match = string.match
QELS.gmatch = string.gmatch
QELS.gsub = string.gsub
QELS.byte = string.byte
QELS.char = string.char
QELS.rep = string.rep

-- ============================================================================
-- Module Export
-- ============================================================================

return QELS
