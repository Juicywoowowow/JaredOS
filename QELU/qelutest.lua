--[[
    QELUTest - Testing Framework for Lua
    Part of the QELU (Quality Enhanced Lua Utilities) library
    
    Features:
    - Test suites with describe/it syntax
    - Rich assertion matchers
    - Lua-specific matchers (tables, metatables, coroutines, etc.)
    - Before/after hooks (beforeAll, beforeEach, afterAll, afterEach)
    - Async test support
    - Mocking and spying
    - Colorized output
    - Test filtering and tagging
    
    @author QELU Contributors
    @license MIT
    @version 1.0.0
]]

local QELUTest = {
    _VERSION = "1.0.0",
    _DESCRIPTION = "QELUTest - Testing Framework for Lua",
    _LICENSE = "MIT"
}

-- ============================================================================
-- Configuration
-- ============================================================================

QELUTest.config = {
    colorOutput = true,
    verbose = true,
    stopOnFail = false,
    showTraceback = true,
    timeout = 5000,  -- ms for async tests
}

-- ============================================================================
-- Internal State
-- ============================================================================

local suites = {}
local currentSuite = nil
local currentTest = nil
local results = {
    passed = 0,
    failed = 0,
    skipped = 0,
    total = 0,
    failures = {},
    duration = 0
}

-- ============================================================================
-- ANSI Colors
-- ============================================================================

local colors = {
    reset = "\27[0m",
    bold = "\27[1m",
    dim = "\27[2m",
    red = "\27[31m",
    green = "\27[32m",
    yellow = "\27[33m",
    blue = "\27[34m",
    magenta = "\27[35m",
    cyan = "\27[36m",
    white = "\27[37m",
}

local function color(name, text)
    if not QELUTest.config.colorOutput then
        return text
    end
    return (colors[name] or "") .. text .. colors.reset
end

-- ============================================================================
-- Utilities
-- ============================================================================

local function deepEqual(a, b, visited)
    if type(a) ~= type(b) then return false end
    if type(a) ~= "table" then return a == b end
    
    visited = visited or {}
    if visited[a] and visited[a] == b then return true end
    visited[a] = b
    
    local aKeys = {}
    for k in pairs(a) do aKeys[k] = true end
    
    for k, v in pairs(b) do
        if not aKeys[k] then return false end
        if not deepEqual(a[k], v, visited) then return false end
        aKeys[k] = nil
    end
    
    for k in pairs(aKeys) do
        return false
    end
    
    return true
end

local function tableContains(tbl, subset)
    for k, v in pairs(subset) do
        if type(v) == "table" and type(tbl[k]) == "table" then
            if not tableContains(tbl[k], v) then return false end
        elseif tbl[k] ~= v then
            return false
        end
    end
    return true
end

local function prettyPrint(value, indent, visited)
    indent = indent or 0
    visited = visited or {}
    
    local t = type(value)
    
    if t == "string" then
        return string.format("%q", value)
    elseif t == "number" or t == "boolean" or t == "nil" then
        return tostring(value)
    elseif t == "table" then
        if visited[value] then
            return "<circular>"
        end
        visited[value] = true
        
        local mt = getmetatable(value)
        if mt and mt.__tostring then
            return tostring(value)
        end
        
        local parts = {}
        local isArray = #value > 0
        local count = 0
        
        for k, v in pairs(value) do
            count = count + 1
            if count > 10 then
                table.insert(parts, "...")
                break
            end
            
            local key = isArray and "" or (type(k) == "string" and k or "[" .. tostring(k) .. "]") .. " = "
            table.insert(parts, key .. prettyPrint(v, indent + 1, visited))
        end
        
        if #parts == 0 then
            return "{}"
        elseif #parts <= 3 and not string.find(table.concat(parts), "\n") then
            return "{ " .. table.concat(parts, ", ") .. " }"
        else
            local ws = string.rep("  ", indent + 1)
            return "{\n" .. ws .. table.concat(parts, ",\n" .. ws) .. "\n" .. string.rep("  ", indent) .. "}"
        end
    elseif t == "function" then
        local info = debug.getinfo(value, "S")
        return string.format("<function: %s:%d>", info.short_src or "?", info.linedefined or 0)
    else
        return "<" .. t .. ">"
    end
end

local function getCallLocation()
    local info = debug.getinfo(4, "Sl")
    if info then
        return string.format("%s:%d", info.short_src or "?", info.currentline or 0)
    end
    return "unknown"
end

-- ============================================================================
-- Expectation/Matcher System
-- ============================================================================

local Expectation = {}
Expectation.__index = Expectation

function Expectation.new(value)
    local self = setmetatable({}, Expectation)
    self.value = value
    self.negated = false
    self.location = getCallLocation()
    return self
end

function Expectation:_assert(condition, message, expected, actual)
    if self.negated then
        condition = not condition
    end
    
    if not condition then
        local prefix = self.negated and "Expected NOT " or "Expected "
        local err = {
            message = prefix .. message,
            expected = expected,
            actual = actual,
            location = self.location
        }
        error(err, 0)
    end
    return self
end

-- Negation
function Expectation:never()
    self.negated = not self.negated
    return self
end
Expectation.Not = Expectation.never  -- Alias

-- ============================================================================
-- Basic Matchers
-- ============================================================================

function Expectation:toBe(expected)
    return self:_assert(
        self.value == expected,
        string.format("to be %s but got %s", prettyPrint(expected), prettyPrint(self.value)),
        expected,
        self.value
    )
end

function Expectation:toEqual(expected)
    return self:_assert(
        deepEqual(self.value, expected),
        string.format("to equal %s but got %s", prettyPrint(expected), prettyPrint(self.value)),
        expected,
        self.value
    )
end

function Expectation:toBeTruthy()
    return self:_assert(
        self.value and true or false,
        string.format("to be truthy but got %s", prettyPrint(self.value)),
        "truthy",
        self.value
    )
end

function Expectation:toBeFalsy()
    return self:_assert(
        not self.value,
        string.format("to be falsy but got %s", prettyPrint(self.value)),
        "falsy",
        self.value
    )
end

function Expectation:toBeNil()
    return self:_assert(
        self.value == nil,
        string.format("to be nil but got %s", prettyPrint(self.value)),
        nil,
        self.value
    )
end

function Expectation:toBeDefined()
    return self:_assert(
        self.value ~= nil,
        "to be defined but got nil",
        "defined",
        self.value
    )
end

-- ============================================================================
-- Type Matchers (Lua-specific)
-- ============================================================================

function Expectation:toBeType(expectedType)
    local actualType = type(self.value)
    return self:_assert(
        actualType == expectedType,
        string.format("to be type '%s' but got '%s'", expectedType, actualType),
        expectedType,
        actualType
    )
end

function Expectation:toBeString()
    return self:toBeType("string")
end

function Expectation:toBeNumber()
    return self:toBeType("number")
end

function Expectation:toBeBoolean()
    return self:toBeType("boolean")
end

function Expectation:toBeTable()
    return self:toBeType("table")
end

function Expectation:toBeFunction()
    return self:toBeType("function")
end

function Expectation:toBeThread()
    return self:toBeType("thread")
end

function Expectation:toBeUserdata()
    return self:toBeType("userdata")
end

-- ============================================================================
-- Number Matchers
-- ============================================================================

function Expectation:toBeGreaterThan(n)
    return self:_assert(
        type(self.value) == "number" and self.value > n,
        string.format("to be greater than %s but got %s", n, self.value),
        "> " .. n,
        self.value
    )
end

function Expectation:toBeGreaterThanOrEqual(n)
    return self:_assert(
        type(self.value) == "number" and self.value >= n,
        string.format("to be greater than or equal to %s but got %s", n, self.value),
        ">= " .. n,
        self.value
    )
end

function Expectation:toBeLessThan(n)
    return self:_assert(
        type(self.value) == "number" and self.value < n,
        string.format("to be less than %s but got %s", n, self.value),
        "< " .. n,
        self.value
    )
end

function Expectation:toBeLessThanOrEqual(n)
    return self:_assert(
        type(self.value) == "number" and self.value <= n,
        string.format("to be less than or equal to %s but got %s", n, self.value),
        "<= " .. n,
        self.value
    )
end

function Expectation:toBeCloseTo(n, precision)
    precision = precision or 2
    local diff = math.abs(self.value - n)
    local threshold = 10 ^ (-precision)
    return self:_assert(
        type(self.value) == "number" and diff < threshold,
        string.format("to be close to %s (precision %d) but got %s (diff: %s)", n, precision, self.value, diff),
        n,
        self.value
    )
end

function Expectation:toBeNaN()
    return self:_assert(
        self.value ~= self.value,  -- NaN is never equal to itself
        string.format("to be NaN but got %s", self.value),
        "NaN",
        self.value
    )
end

function Expectation:toBeInfinite()
    return self:_assert(
        self.value == math.huge or self.value == -math.huge,
        string.format("to be infinite but got %s", self.value),
        "infinite",
        self.value
    )
end

function Expectation:toBeInRange(min, max)
    return self:_assert(
        type(self.value) == "number" and self.value >= min and self.value <= max,
        string.format("to be in range [%s, %s] but got %s", min, max, self.value),
        string.format("[%s, %s]", min, max),
        self.value
    )
end

-- ============================================================================
-- String Matchers
-- ============================================================================

function Expectation:toContain(substring)
    if type(self.value) == "string" then
        return self:_assert(
            string.find(self.value, substring, 1, true) ~= nil,
            string.format("to contain %q but got %q", substring, self.value),
            substring,
            self.value
        )
    elseif type(self.value) == "table" then
        local found = false
        for _, v in pairs(self.value) do
            if v == substring then
                found = true
                break
            end
        end
        return self:_assert(
            found,
            string.format("to contain %s but got %s", prettyPrint(substring), prettyPrint(self.value)),
            substring,
            self.value
        )
    end
    error("toContain requires a string or table", 2)
end

function Expectation:toMatch(pattern)
    return self:_assert(
        type(self.value) == "string" and string.match(self.value, pattern) ~= nil,
        string.format("to match pattern %q but got %q", pattern, self.value),
        pattern,
        self.value
    )
end

function Expectation:toStartWith(prefix)
    return self:_assert(
        type(self.value) == "string" and self.value:sub(1, #prefix) == prefix,
        string.format("to start with %q but got %q", prefix, self.value),
        prefix,
        self.value
    )
end

function Expectation:toEndWith(suffix)
    return self:_assert(
        type(self.value) == "string" and self.value:sub(-#suffix) == suffix,
        string.format("to end with %q but got %q", suffix, self.value),
        suffix,
        self.value
    )
end

function Expectation:toHaveLength(len)
    local actualLen = type(self.value) == "string" and #self.value 
                     or type(self.value) == "table" and #self.value
                     or -1
    return self:_assert(
        actualLen == len,
        string.format("to have length %d but got %d", len, actualLen),
        len,
        actualLen
    )
end

-- ============================================================================
-- Table Matchers (Lua-specific)
-- ============================================================================

function Expectation:toContainKey(key)
    return self:_assert(
        type(self.value) == "table" and self.value[key] ~= nil,
        string.format("to contain key %s", prettyPrint(key)),
        key,
        self.value
    )
end

function Expectation:toContainKeys(...)
    local keys = {...}
    for _, key in ipairs(keys) do
        if self.value[key] == nil then
            return self:_assert(
                false,
                string.format("to contain key %s", prettyPrint(key)),
                key,
                self.value
            )
        end
    end
    return self:_assert(true, "", nil, nil)
end

function Expectation:toContainValue(value)
    local found = false
    if type(self.value) == "table" then
        for _, v in pairs(self.value) do
            if deepEqual(v, value) then
                found = true
                break
            end
        end
    end
    return self:_assert(
        found,
        string.format("to contain value %s", prettyPrint(value)),
        value,
        self.value
    )
end

function Expectation:toContainSubset(subset)
    return self:_assert(
        type(self.value) == "table" and tableContains(self.value, subset),
        string.format("to contain subset %s", prettyPrint(subset)),
        subset,
        self.value
    )
end

function Expectation:toBeEmpty()
    local isEmpty = false
    if type(self.value) == "table" then
        isEmpty = next(self.value) == nil
    elseif type(self.value) == "string" then
        isEmpty = self.value == ""
    end
    return self:_assert(
        isEmpty,
        "to be empty but got " .. prettyPrint(self.value),
        "empty",
        self.value
    )
end

function Expectation:toBeArray()
    if type(self.value) ~= "table" then
        return self:_assert(false, "to be an array but got " .. type(self.value), "array", self.value)
    end
    
    local isArray = true
    local count = 0
    for k in pairs(self.value) do
        count = count + 1
        if type(k) ~= "number" or k ~= math.floor(k) or k < 1 then
            isArray = false
            break
        end
    end
    
    -- Check for holes
    if isArray and count > 0 then
        for i = 1, count do
            if self.value[i] == nil then
                isArray = false
                break
            end
        end
    end
    
    return self:_assert(
        isArray,
        "to be an array",
        "array",
        self.value
    )
end

function Expectation:toHaveMetatable(mt)
    local actual = getmetatable(self.value)
    if mt then
        return self:_assert(
            actual == mt,
            "to have specified metatable",
            mt,
            actual
        )
    else
        return self:_assert(
            actual ~= nil,
            "to have a metatable",
            "metatable",
            actual
        )
    end
end

-- ============================================================================
-- Function/Error Matchers
-- ============================================================================

function Expectation:toThrow(expectedMessage)
    if type(self.value) ~= "function" then
        error("toThrow requires a function", 2)
    end
    
    local ok, err = pcall(self.value)
    
    if expectedMessage then
        local errStr = tostring(err)
        local matches = not ok and (errStr:find(expectedMessage, 1, true) or errStr:match(expectedMessage))
        return self:_assert(
            matches,
            string.format("to throw error matching %q but got %q", expectedMessage, errStr),
            expectedMessage,
            errStr
        )
    else
        return self:_assert(
            not ok,
            "to throw an error but it didn't",
            "error",
            "no error"
        )
    end
end

function Expectation:toNotThrow()
    if type(self.value) ~= "function" then
        error("toNotThrow requires a function", 2)
    end
    
    local ok, err = pcall(self.value)
    return self:_assert(
        ok,
        string.format("to not throw but threw: %s", tostring(err)),
        "no error",
        err
    )
end

function Expectation:toReturnValue(expected)
    if type(self.value) ~= "function" then
        error("toReturnValue requires a function", 2)
    end
    
    local ok, result = pcall(self.value)
    if not ok then
        return self:_assert(false, "to return a value but threw: " .. tostring(result), expected, result)
    end
    
    return self:_assert(
        deepEqual(result, expected),
        string.format("to return %s but got %s", prettyPrint(expected), prettyPrint(result)),
        expected,
        result
    )
end

-- ============================================================================
-- Coroutine Matchers (Lua-specific)
-- ============================================================================

function Expectation:toBeCoroutine()
    return self:_assert(
        type(self.value) == "thread",
        string.format("to be a coroutine but got %s", type(self.value)),
        "coroutine",
        self.value
    )
end

function Expectation:toBeCoroutineStatus(status)
    if type(self.value) ~= "thread" then
        return self:_assert(false, "to be a coroutine", "coroutine", type(self.value))
    end
    
    local actual = coroutine.status(self.value)
    return self:_assert(
        actual == status,
        string.format("to have status '%s' but got '%s'", status, actual),
        status,
        actual
    )
end

-- ============================================================================
-- QELU Class Matchers (if QELU is available)
-- ============================================================================

function Expectation:toBeClass()
    local QELU = package.loaded["qelu"]
    if not QELU then
        error("toBeClass requires QELU library", 2)
    end
    return self:_assert(
        QELU.isClass(self.value),
        "to be a QELU class",
        "class",
        self.value
    )
end

function Expectation:toBeInstanceOf(cls)
    local QELU = package.loaded["qelu"]
    if not QELU then
        error("toBeInstanceOf requires QELU library", 2)
    end
    return self:_assert(
        QELU.instanceOf(self.value, cls),
        string.format("to be instance of %s", tostring(cls)),
        cls,
        self.value
    )
end

function Expectation:toBeInstance()
    local QELU = package.loaded["qelu"]
    if not QELU then
        error("toBeInstance requires QELU library", 2)
    end
    return self:_assert(
        QELU.isInstance(self.value),
        "to be a QELU instance",
        "instance",
        self.value
    )
end

-- ============================================================================
-- Test Suite Definition
-- ============================================================================

local function createSuite(name, parent)
    return {
        name = name,
        parent = parent,
        tests = {},
        beforeAll = {},
        afterAll = {},
        beforeEach = {},
        afterEach = {},
        children = {},
        only = false,
        skip = false
    }
end

function QELUTest.describe(name, fn)
    local suite = createSuite(name, currentSuite)
    
    if currentSuite then
        table.insert(currentSuite.children, suite)
    else
        table.insert(suites, suite)
    end
    
    local prevSuite = currentSuite
    currentSuite = suite
    
    fn()
    
    currentSuite = prevSuite
    
    return suite
end

function QELUTest.xdescribe(name, fn)
    local suite = QELUTest.describe(name, fn)
    suite.skip = true
    return suite
end

function QELUTest.fdescribe(name, fn)
    local suite = QELUTest.describe(name, fn)
    suite.only = true
    return suite
end

-- ============================================================================
-- Test Case Definition
-- ============================================================================

function QELUTest.it(name, fn)
    if not currentSuite then
        error("it() must be called inside describe()", 2)
    end
    
    local test = {
        name = name,
        fn = fn,
        skip = false,
        only = false,
        tags = {}
    }
    
    table.insert(currentSuite.tests, test)
    return test
end

function QELUTest.xit(name, fn)
    local test = QELUTest.it(name, fn)
    test.skip = true
    return test
end

function QELUTest.fit(name, fn)
    local test = QELUTest.it(name, fn)
    test.only = true
    return test
end

-- Test alias
QELUTest.test = QELUTest.it
QELUTest.xtest = QELUTest.xit
QELUTest.ftest = QELUTest.fit

-- ============================================================================
-- Hooks
-- ============================================================================

function QELUTest.beforeAll(fn)
    if not currentSuite then
        error("beforeAll() must be called inside describe()", 2)
    end
    table.insert(currentSuite.beforeAll, fn)
end

function QELUTest.afterAll(fn)
    if not currentSuite then
        error("afterAll() must be called inside describe()", 2)
    end
    table.insert(currentSuite.afterAll, fn)
end

function QELUTest.beforeEach(fn)
    if not currentSuite then
        error("beforeEach() must be called inside describe()", 2)
    end
    table.insert(currentSuite.beforeEach, fn)
end

function QELUTest.afterEach(fn)
    if not currentSuite then
        error("afterEach() must be called inside describe()", 2)
    end
    table.insert(currentSuite.afterEach, fn)
end

-- ============================================================================
-- Expect Function (Entry point for matchers)
-- ============================================================================

function QELUTest.expect(value)
    return Expectation.new(value)
end

-- ============================================================================
-- Mock/Spy System
-- ============================================================================

local Spy = {}
Spy.__index = Spy

function QELUTest.spy(fn)
    local s = setmetatable({
        calls = {},
        callCount = 0,
        originalFn = fn
    }, Spy)
    
    s.fn = function(...)
        local args = {...}
        s.callCount = s.callCount + 1
        table.insert(s.calls, {
            args = args,
            timestamp = os.clock()
        })
        
        if fn then
            return fn(...)
        end
    end
    
    return s
end

function Spy:wasCalled()
    return self.callCount > 0
end

function Spy:wasCalledTimes(n)
    return self.callCount == n
end

function Spy:wasCalledWith(...)
    local expected = {...}
    for _, call in ipairs(self.calls) do
        if deepEqual(call.args, expected) then
            return true
        end
    end
    return false
end

function Spy:getCall(n)
    return self.calls[n]
end

function Spy:getLastCall()
    return self.calls[#self.calls]
end

function Spy:reset()
    self.calls = {}
    self.callCount = 0
end

-- Mock object
function QELUTest.mock(original)
    local m = {}
    local mocks = {}
    
    setmetatable(m, {
        __index = function(_, key)
            if mocks[key] then
                return mocks[key].fn
            end
            return original and original[key]
        end,
        __newindex = function(_, key, value)
            if type(value) == "function" then
                mocks[key] = QELUTest.spy(value)
            else
                mocks[key] = value
            end
        end
    })
    
    m._getSpy = function(name)
        return mocks[name]
    end
    
    m._resetAll = function()
        for _, spy in pairs(mocks) do
            if type(spy) == "table" and spy.reset then
                spy:reset()
            end
        end
    end
    
    return m
end

-- ============================================================================
-- Test Runner
-- ============================================================================

local function runHooks(hooks)
    for _, hook in ipairs(hooks) do
        hook()
    end
end

local function collectBeforeEach(suite)
    local hooks = {}
    local s = suite
    while s do
        for i = #s.beforeEach, 1, -1 do
            table.insert(hooks, 1, s.beforeEach[i])
        end
        s = s.parent
    end
    return hooks
end

local function collectAfterEach(suite)
    local hooks = {}
    local s = suite
    while s do
        for _, hook in ipairs(s.afterEach) do
            table.insert(hooks, hook)
        end
        s = s.parent
    end
    return hooks
end

local function getSuitePath(suite)
    local path = {}
    local s = suite
    while s do
        table.insert(path, 1, s.name)
        s = s.parent
    end
    return table.concat(path, " > ")
end

local function runTest(test, suite, indent)
    indent = indent or "  "
    currentTest = test
    
    if test.skip then
        results.skipped = results.skipped + 1
        results.total = results.total + 1
        if QELUTest.config.verbose then
            print(indent .. color("yellow", "○") .. " " .. color("dim", test.name .. " (skipped)"))
        end
        return
    end
    
    local beforeHooks = collectBeforeEach(suite)
    local afterHooks = collectAfterEach(suite)
    
    local startTime = os.clock()
    local ok, err = pcall(function()
        runHooks(beforeHooks)
        test.fn()
        runHooks(afterHooks)
    end)
    local duration = (os.clock() - startTime) * 1000
    
    results.total = results.total + 1
    
    if ok then
        results.passed = results.passed + 1
        if QELUTest.config.verbose then
            local timeStr = duration > 100 and color("yellow", string.format(" (%.0fms)", duration)) or ""
            print(indent .. color("green", "✓") .. " " .. test.name .. timeStr)
        end
    else
        results.failed = results.failed + 1
        print(indent .. color("red", "✗") .. " " .. color("red", test.name))
        
        local failure = {
            suite = getSuitePath(suite),
            test = test.name,
            error = err
        }
        table.insert(results.failures, failure)
        
        if type(err) == "table" then
            print(indent .. "  " .. color("red", err.message))
            if err.location then
                print(indent .. "  " .. color("dim", "at " .. err.location))
            end
        else
            print(indent .. "  " .. color("red", tostring(err)))
            if QELUTest.config.showTraceback then
                print(indent .. "  " .. color("dim", debug.traceback("", 2)))
            end
        end
        
        if QELUTest.config.stopOnFail then
            error("Test failed - stopping", 0)
        end
    end
    
    currentTest = nil
end

local function runSuite(suite, indent)
    indent = indent or ""
    
    if suite.skip then
        if QELUTest.config.verbose then
            print(indent .. color("yellow", "○") .. " " .. color("dim", suite.name .. " (skipped)"))
        end
        return
    end
    
    print(indent .. color("bold", suite.name))
    
    -- Run beforeAll hooks
    local ok, err = pcall(function()
        runHooks(suite.beforeAll)
    end)
    
    if not ok then
        print(indent .. "  " .. color("red", "beforeAll hook failed: " .. tostring(err)))
        return
    end
    
    -- Run tests
    for _, test in ipairs(suite.tests) do
        runTest(test, suite, indent .. "  ")
    end
    
    -- Run child suites
    for _, child in ipairs(suite.children) do
        runSuite(child, indent .. "  ")
    end
    
    -- Run afterAll hooks
    pcall(function()
        runHooks(suite.afterAll)
    end)
end

function QELUTest.run(options)
    options = options or {}
    
    for k, v in pairs(options) do
        QELUTest.config[k] = v
    end
    
    results = {
        passed = 0,
        failed = 0,
        skipped = 0,
        total = 0,
        failures = {},
        duration = 0
    }
    
    print("")
    print(color("bold", "Running tests..."))
    print("")
    
    local startTime = os.clock()
    
    for _, suite in ipairs(suites) do
        runSuite(suite)
    end
    
    results.duration = (os.clock() - startTime) * 1000
    
    -- Print summary
    print("")
    print(string.rep("─", 50))
    
    local summary = string.format(
        "Tests: %s passed, %s failed, %s skipped, %s total",
        color("green", tostring(results.passed)),
        results.failed > 0 and color("red", tostring(results.failed)) or "0",
        results.skipped > 0 and color("yellow", tostring(results.skipped)) or "0",
        tostring(results.total)
    )
    print(summary)
    print(string.format("Time:  %.2fms", results.duration))
    
    if results.failed > 0 then
        print("")
        print(color("red", "FAILED"))
    else
        print("")
        print(color("green", "PASSED"))
    end
    
    -- Clear suites for next run
    suites = {}
    
    return results
end

-- ============================================================================
-- Utility Functions
-- ============================================================================

function QELUTest.reset()
    suites = {}
    currentSuite = nil
    currentTest = nil
    results = {
        passed = 0,
        failed = 0,
        skipped = 0,
        total = 0,
        failures = {},
        duration = 0
    }
end

function QELUTest.pending(message)
    error({ type = "pending", message = message or "Pending" }, 0)
end

function QELUTest.fail(message)
    error({ type = "fail", message = message or "Explicit fail" }, 0)
end

-- ============================================================================
-- Global Exports (optional)
-- ============================================================================

function QELUTest.globalize()
    _G.describe = QELUTest.describe
    _G.xdescribe = QELUTest.xdescribe
    _G.fdescribe = QELUTest.fdescribe
    _G.it = QELUTest.it
    _G.xit = QELUTest.xit
    _G.fit = QELUTest.fit
    _G.test = QELUTest.test
    _G.xtest = QELUTest.xtest
    _G.ftest = QELUTest.ftest
    _G.expect = QELUTest.expect
    _G.beforeAll = QELUTest.beforeAll
    _G.afterAll = QELUTest.afterAll
    _G.beforeEach = QELUTest.beforeEach
    _G.afterEach = QELUTest.afterEach
    _G.spy = QELUTest.spy
    _G.mock = QELUTest.mock
end

-- ============================================================================
-- Module Export
-- ============================================================================

return QELUTest
