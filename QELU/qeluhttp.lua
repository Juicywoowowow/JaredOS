--[[
    QELUHttp - HTTP Client for Lua
    Part of the QELU (Quality Enhanced Lua Utilities) library
    
    Features:
    - Simple HTTP requests (GET, POST, PUT, DELETE, PATCH, HEAD)
    - JSON support (automatic encoding/decoding)
    - Custom headers
    - Query parameters
    - Request/response bodies
    - Timeout configuration
    - Follow redirects
    - Basic authentication
    - Form data (application/x-www-form-urlencoded)
    - Multipart form data
    - Error handling
    
    Dependencies:
    - LuaSocket (http.request, ltn12, url)
    
    @author QELU Contributors
    @license MIT
    @version 1.0.0
]]

local QELUHttp = {
    _VERSION = "1.0.0",
    _DESCRIPTION = "QELUHttp - HTTP Client for Lua",
    _LICENSE = "MIT"
}

-- ============================================================================
-- Dependency Check
-- ============================================================================

local socket_available, socket = pcall(require, "socket")
local http_available, http = pcall(require, "socket.http")
local ltn12_available, ltn12 = pcall(require, "ltn12")
local url_available, url_module = pcall(require, "socket.url")

local function checkDependencies()
    local missing = {}
    
    if not socket_available then
        table.insert(missing, "socket")
    end
    if not http_available then
        table.insert(missing, "socket.http")
    end
    if not ltn12_available then
        table.insert(missing, "ltn12")
    end
    if not url_available then
        table.insert(missing, "socket.url")
    end
    
    if #missing > 0 then
        local errorMsg = [[
╔════════════════════════════════════════════════════════════════╗
║                    DEPENDENCY ERROR                            ║
╚════════════════════════════════════════════════════════════════╝

QELUHttp requires LuaSocket to be installed.

Missing modules: ]] .. table.concat(missing, ", ") .. [[


Installation instructions:

  macOS (Homebrew):
    brew install luarocks
    luarocks install luasocket

  Ubuntu/Debian:
    sudo apt-get install lua-socket
    # or
    sudo luarocks install luasocket

  LuaRocks (any platform):
    luarocks install luasocket

  Manual installation:
    git clone https://github.com/diegonehab/luasocket.git
    cd luasocket
    make && make install

After installation, verify with:
    lua -e "require('socket')"

════════════════════════════════════════════════════════════════
]]
        error(errorMsg, 0)
    end
end

-- Check dependencies on load
checkDependencies()

-- ============================================================================
-- Configuration
-- ============================================================================

QELUHttp.config = {
    timeout = 30,           -- Default timeout in seconds
    followRedirects = true, -- Follow HTTP redirects
    maxRedirects = 5,       -- Maximum number of redirects
    userAgent = "QELUHttp/" .. QELUHttp._VERSION,
}

-- ============================================================================
-- JSON Encoding/Decoding (Simple implementation)
-- ============================================================================

local JSON = {}

function JSON.encode(value)
    local t = type(value)
    
    if t == "nil" then
        return "null"
    elseif t == "boolean" then
        return value and "true" or "false"
    elseif t == "number" then
        return tostring(value)
    elseif t == "string" then
        -- Escape special characters
        local escaped = value:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n"):gsub("\r", "\\r"):gsub("\t", "\\t")
        return '"' .. escaped .. '"'
    elseif t == "table" then
        -- Detect if table is an array
        local isArray = true
        local n = 0
        for k, _ in pairs(value) do
            n = n + 1
            if type(k) ~= "number" or k ~= math.floor(k) or k < 1 or k > n then
                isArray = false
                break
            end
        end
        
        if isArray and n > 0 then
            -- Encode as array
            local parts = {}
            for i = 1, n do
                parts[#parts + 1] = JSON.encode(value[i])
            end
            return "[" .. table.concat(parts, ",") .. "]"
        else
            -- Encode as object
            local parts = {}
            for k, v in pairs(value) do
                local key = '"' .. tostring(k) .. '"'
                parts[#parts + 1] = key .. ":" .. JSON.encode(v)
            end
            return "{" .. table.concat(parts, ",") .. "}"
        end
    else
        error("Cannot encode type: " .. t)
    end
end

function JSON.decode(str)
    str = str:gsub("^%s*(.-)%s*$", "%1") -- trim
    
    -- null
    if str == "null" then return nil end
    
    -- boolean
    if str == "true" then return true end
    if str == "false" then return false end
    
    -- number
    local num = tonumber(str)
    if num then return num end
    
    -- string
    if str:sub(1, 1) == '"' then
        return str:sub(2, -2):gsub("\\(.)", {
            n = "\n", t = "\t", r = "\r", ["\\"] = "\\", ['"'] = '"'
        })
    end
    
    -- array
    if str:sub(1, 1) == "[" then
        local arr = {}
        local content = str:sub(2, -2)
        local depth = 0
        local current = ""
        
        for i = 1, #content do
            local c = content:sub(i, i)
            if c == "[" or c == "{" then
                depth = depth + 1
                current = current .. c
            elseif c == "]" or c == "}" then
                depth = depth - 1
                current = current .. c
            elseif c == "," and depth == 0 then
                table.insert(arr, JSON.decode(current))
                current = ""
            else
                current = current .. c
            end
        end
        
        if current ~= "" then
            table.insert(arr, JSON.decode(current))
        end
        
        return arr
    end
    
    -- object
    if str:sub(1, 1) == "{" then
        local obj = {}
        local content = str:sub(2, -2)
        local depth = 0
        local current = ""
        
        for i = 1, #content do
            local c = content:sub(i, i)
            if c == "[" or c == "{" then
                depth = depth + 1
                current = current .. c
            elseif c == "]" or c == "}" then
                depth = depth - 1
                current = current .. c
            elseif c == "," and depth == 0 then
                local key, value = current:match('^%s*"(.-)"%s*:%s*(.+)%s*$')
                if key and value then
                    obj[key] = JSON.decode(value)
                end
                current = ""
            else
                current = current .. c
            end
        end
        
        if current ~= "" then
            local key, value = current:match('^%s*"(.-)"%s*:%s*(.+)%s*$')
            if key and value then
                obj[key] = JSON.decode(value)
            end
        end
        
        return obj
    end
    
    error("Invalid JSON: " .. str)
end

-- ============================================================================
-- URL Encoding
-- ============================================================================

local function urlEncode(str)
    if not str then return "" end
    str = tostring(str)
    str = str:gsub("\n", "\r\n")
    str = str:gsub("([^%w%-%.%_%~ ])", function(c)
        return string.format("%%%02X", string.byte(c))
    end)
    str = str:gsub(" ", "+")
    return str
end

local function buildQueryString(params)
    if not params then return "" end
    
    local parts = {}
    for k, v in pairs(params) do
        table.insert(parts, urlEncode(k) .. "=" .. urlEncode(v))
    end
    
    return table.concat(parts, "&")
end

-- ============================================================================
-- Request Builder
-- ============================================================================

local Request = {}
Request.__index = Request

function Request.new(method, url, options)
    local self = setmetatable({}, Request)
    
    self.method = method:upper()
    self.url = url
    self.headers = options.headers or {}
    self.params = options.params or {}
    self.body = options.body
    self.json = options.json
    self.form = options.form
    self.timeout = options.timeout or QELUHttp.config.timeout
    self.followRedirects = options.followRedirects ~= nil and options.followRedirects or QELUHttp.config.followRedirects
    self.auth = options.auth
    
    -- Set default headers
    if not self.headers["User-Agent"] then
        self.headers["User-Agent"] = QELUHttp.config.userAgent
    end
    
    return self
end

function Request:buildUrl()
    local fullUrl = self.url
    
    -- Add query parameters
    if self.params and next(self.params) then
        local separator = fullUrl:find("?") and "&" or "?"
        fullUrl = fullUrl .. separator .. buildQueryString(self.params)
    end
    
    return fullUrl
end

function Request:buildHeaders()
    local headers = {}
    
    -- Copy user headers
    for k, v in pairs(self.headers) do
        headers[k] = v
    end
    
    -- Add authentication
    if self.auth then
        local credentials = self.auth.username .. ":" .. self.auth.password
        local encoded = mime.b64(credentials)
        headers["Authorization"] = "Basic " .. encoded
    end
    
    -- Handle JSON body
    if self.json then
        self.body = JSON.encode(self.json)
        headers["Content-Type"] = "application/json"
        headers["Content-Length"] = #self.body
    end
    
    -- Handle form data
    if self.form then
        self.body = buildQueryString(self.form)
        headers["Content-Type"] = "application/x-www-form-urlencoded"
        headers["Content-Length"] = #self.body
    end
    
    -- Set content length for body
    if self.body and not headers["Content-Length"] then
        headers["Content-Length"] = #self.body
    end
    
    return headers
end

function Request:execute()
    local fullUrl = self:buildUrl()
    local headers = self:buildHeaders()
    
    -- Prepare response
    local responseBody = {}
    local responseHeaders = {}
    
    -- Build request table
    local requestTable = {
        url = fullUrl,
        method = self.method,
        headers = headers,
        sink = ltn12.sink.table(responseBody),
        redirect = self.followRedirects,
    }
    
    -- Add source for body
    if self.body then
        requestTable.source = ltn12.source.string(self.body)
    end
    
    -- Set timeout
    http.TIMEOUT = self.timeout
    
    -- Execute request
    local ok, statusCode, responseHeadersTable, statusLine = http.request(requestTable)
    
    -- Build response object
    local response = {
        ok = ok == 1,
        status = statusCode,
        statusText = statusLine,
        headers = responseHeadersTable or {},
        body = table.concat(responseBody),
        request = {
            method = self.method,
            url = fullUrl,
            headers = headers,
        }
    }
    
    -- Try to parse JSON response
    local contentType = response.headers["content-type"] or ""
    if contentType:find("application/json") then
        local success, decoded = pcall(JSON.decode, response.body)
        if success then
            response.data = decoded
        end
    end
    
    return response
end

-- ============================================================================
-- Response Object
-- ============================================================================

local Response = {}
Response.__index = Response

function Response.new(data)
    local self = setmetatable(data, Response)
    return self
end

function Response:isSuccess()
    return self.status >= 200 and self.status < 300
end

function Response:isRedirect()
    return self.status >= 300 and self.status < 400
end

function Response:isClientError()
    return self.status >= 400 and self.status < 500
end

function Response:isServerError()
    return self.status >= 500 and self.status < 600
end

function Response:text()
    return self.body
end

function Response:json()
    if not self.data then
        self.data = JSON.decode(self.body)
    end
    return self.data
end

-- ============================================================================
-- HTTP Methods
-- ============================================================================

function QELUHttp.request(method, url, options)
    options = options or {}
    local req = Request.new(method, url, options)
    local response = req:execute()
    return Response.new(response)
end

function QELUHttp.get(url, options)
    return QELUHttp.request("GET", url, options)
end

function QELUHttp.post(url, options)
    return QELUHttp.request("POST", url, options)
end

function QELUHttp.put(url, options)
    return QELUHttp.request("PUT", url, options)
end

function QELUHttp.delete(url, options)
    return QELUHttp.request("DELETE", url, options)
end

function QELUHttp.patch(url, options)
    return QELUHttp.request("PATCH", url, options)
end

function QELUHttp.head(url, options)
    return QELUHttp.request("HEAD", url, options)
end

function QELUHttp.options(url, options)
    return QELUHttp.request("OPTIONS", url, options)
end

-- ============================================================================
-- Convenience Methods
-- ============================================================================

--- Download a file from URL
function QELUHttp.download(url, filepath, options)
    options = options or {}
    
    local file = io.open(filepath, "wb")
    if not file then
        error("Cannot open file for writing: " .. filepath)
    end
    
    local requestTable = {
        url = url,
        sink = ltn12.sink.file(file),
        headers = options.headers or {},
    }
    
    http.TIMEOUT = options.timeout or QELUHttp.config.timeout
    
    local ok, statusCode = http.request(requestTable)
    file:close()
    
    return {
        ok = ok == 1,
        status = statusCode,
        filepath = filepath
    }
end

--- Simple GET request returning just the body
function QELUHttp.fetch(url, options)
    local response = QELUHttp.get(url, options)
    if response:isSuccess() then
        return response.body
    else
        error(string.format("HTTP %d: %s", response.status, response.statusText))
    end
end

--- Simple JSON GET request
function QELUHttp.getJSON(url, options)
    options = options or {}
    options.headers = options.headers or {}
    options.headers["Accept"] = "application/json"
    
    local response = QELUHttp.get(url, options)
    if response:isSuccess() then
        return response:json()
    else
        error(string.format("HTTP %d: %s", response.status, response.statusText))
    end
end

--- Simple JSON POST request
function QELUHttp.postJSON(url, data, options)
    options = options or {}
    options.json = data
    
    local response = QELUHttp.post(url, options)
    if response:isSuccess() then
        local contentType = response.headers["content-type"] or ""
        if contentType:find("application/json") then
            return response:json()
        else
            return response.body
        end
    else
        error(string.format("HTTP %d: %s", response.status, response.statusText))
    end
end

-- ============================================================================
-- Utilities
-- ============================================================================

QELUHttp.JSON = JSON
QELUHttp.urlEncode = urlEncode
QELUHttp.buildQueryString = buildQueryString

-- ============================================================================
-- Module Export
-- ============================================================================

return QELUHttp
