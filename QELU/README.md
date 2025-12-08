# QELU - Quality Enhanced Lua Utilities

> A comprehensive Lua standard library with OOP, Testing, HTTP, String, Table, JSON, and Python bridge

[![Lua](https://img.shields.io/badge/Lua-5.1%2B-blue.svg)](https://www.lua.org/)
[![LuaJIT](https://img.shields.io/badge/LuaJIT-2.0%2B-orange.svg)](https://luajit.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

QELU is a comprehensive Lua library suite that brings modern programming patterns to Lua:

- **qelu.lua** - Full-featured Object-Oriented Programming system
- **qelutest.lua** - Professional testing framework with rich matchers
- **qeluhttp.lua** - HTTP client for making REST API requests (curl-like)
- **qels.lua** - Advanced string utilities (split, trim, case conversion, templates, etc.)
- **qelut.lua** - Table utilities (map, filter, reduce, deep operations, functional programming)
- **qeluj.lua** - Robust JSON encoding/decoding with pretty printing and file I/O
- **qelup.lua** - Python bridge for calling Python from Lua (requires C extension)

All modules are written in mostly Lua with dependencies, optimized for LuaJIT.


---

## Table of Contents

- [Installation](#installation)
- [Quick Start](#quick-start)
- [QELU OOP Library](#qelu-oop-library)
- [QELUTest Framework](#qelutest-framework)
- [QELUHttp Client](#qeluhttp-client)
- [QELS String Utilities](#qels-string-utilities)
- [QELUT Table Utilities](#qelut-table-utilities)
- [QELUJ JSON Library](#qeluj-json-library)
- [QELUP Python Bridge](#qelup-python-bridge)
- [API Reference](#api-reference)
- [License](#license)

---

## Installation

### Core Libraries (No Dependencies)

```bash
# Copy qelu.lua and qelutest.lua to your project
cp qelu.lua qelutest.lua /path/to/your/project/
```

### HTTP Client (Requires LuaSocket + LuaSec)

```bash
# Install dependencies
luarocks install luasocket
luarocks install luasec

# Copy qeluhttp.lua
cp qeluhttp.lua /path/to/your/project/
```

---

## Quick Start

### OOP Quick Start

```lua
local QELU = require("qelu")

-- Define a class
local Animal = QELU.class("Animal")
function Animal:init(name)
    self.name = name
end

-- Inheritance with super
local Dog = QELU.class("Dog"):extends(Animal)
function Dog:init(name, breed)
    QELU.super(self):init(name)
    self.breed = breed
end

local rex = Dog("Rex", "German Shepherd")
```

### Testing Quick Start

```lua
local QELUTest = require("qelutest")
QELUTest.globalize()

describe("My Module", function()
    it("should work", function()
        expect(1 + 1):toBe(2)
        expect({a = 1}):toContainKey("a")
    end)
end)

QELUTest.run()
```

### HTTP Quick Start

```lua
local http = require("qeluhttp")

-- Simple GET request
local response = http.get("https://api.example.com/users")
print(response.status)  -- 200
print(response.body)

-- POST with JSON
local response = http.post("https://api.example.com/users", {
    json = {
        name = "John Doe",
        email = "john@example.com"
    }
})

-- Access parsed JSON
local data = response:json()
print(data.id)
```

### String Utilities Quick Start

```lua
local str = require("qels")

-- Split and join
local parts = str.split("a,b,c", ",")  -- {"a", "b", "c"}
local joined = str.join(parts, "-")    -- "a-b-c"

-- Case conversion
str.camelCase("hello_world")           -- "helloWorld"
str.snakeCase("helloWorld")            -- "hello_world"
str.kebabCase("HelloWorld")            -- "hello-world"

-- Templates
str.template("Hello {name}!", {name = "Lua"})  -- "Hello Lua!"

-- Trimming
str.trim("  hello  ")                  -- "hello"

-- Slugify
str.slugify("Hello World! 123")        -- "hello-world-123"
```

---

## QELU OOP Library

Full-featured object-oriented programming for Lua.

### Classes

```lua
local Person = QELU.class("Person")

function Person:init(name, age)
    self.name = name
    self.age = age
end

function Person:greet()
    return "Hello, I'm " .. self.name
end

local john = Person("John", 30)
```

### Inheritance & Super Calls

```lua
local Employee = QELU.class("Employee"):extends(Person)

function Employee:init(name, age, position)
    QELU.super(self):init(name, age)
    self.position = position
end

function Employee:greet()
    return QELU.super(self):greet() .. ", I work as a " .. self.position
end
```

### Private Members

```lua
local BankAccount = QELU.class("BankAccount")

function BankAccount:init(balance)
    self.__balance = balance  -- Private (double underscore)
end

function BankAccount:getBalance()
    return self.__balance
end
```

### Static Members

```lua
local Counter = QELU.class("Counter")

Counter.static({
    count = 0,
    increment = function()
        Counter.count = Counter.count + 1
    end
})
```

### Interfaces

```lua
local Drawable = QELU.interface("Drawable", {
    draw = true,
    getPosition = true
})

local Sprite = QELU.class("Sprite"):implements(Drawable)
function Sprite:draw() end
function Sprite:getPosition() return 0, 0 end
```

### Mixins

```lua
local Loggable = QELU.mixin({
    log = function(self, msg)
        print("[LOG] " .. msg)
    end
})

local Service = QELU.class("Service"):include(Loggable)

-- Built-in EventEmitter
local Button = QELU.class("Button"):include(QELU.EventEmitter)
btn:on("click", function() print("Clicked!") end)
btn:emit("click")
```

### Abstract Classes

```lua
local Shape = QELU.class("Shape", { abstract = true })
Shape.area = QELU.abstract("area")

local Rectangle = QELU.class("Rectangle"):extends(Shape)
function Rectangle:area()
    return self.width * self.height
end
```

### Singletons

```lua
local Database = QELU.singleton("Database")
local db1 = Database()
local db2 = Database()
print(db1 == db2)  -- true
```

### Enums

```lua
local Color = QELU.enum("Color", { "RED", "GREEN", "BLUE" })
print(Color.RED)         -- 1
print(Color.nameOf(1))   -- "RED"
```

---

## QELUTest Framework

Professional testing framework with rich matchers.

### Writing Tests

```lua
local QELUTest = require("qelutest")
QELUTest.globalize()  -- Makes describe, it, expect global

describe("Calculator", function()
    it("should add numbers", function()
        expect(2 + 2):toBe(4)
    end)
    
    it("should handle tables", function()
        expect({a = 1, b = 2}):toContainKey("a")
    end)
end)

QELUTest.run()
```

### Matchers

#### Basic Matchers

```lua
expect(value):toBe(expected)           -- Strict equality
expect(value):toEqual(expected)        -- Deep equality
expect(value):toBeTruthy()
expect(value):toBeFalsy()
expect(value):toBeNil()
expect(value):toBeDefined()
```

#### Type Matchers

```lua
expect(value):toBeString()
expect(value):toBeNumber()
expect(value):toBeBoolean()
expect(value):toBeTable()
expect(value):toBeFunction()
expect(value):toBeThread()             -- Coroutine
```

#### Number Matchers

```lua
expect(10):toBeGreaterThan(5)
expect(10):toBeGreaterThanOrEqual(10)
expect(5):toBeLessThan(10)
expect(0.3):toBeCloseTo(0.1 + 0.2, 5)
expect(5):toBeInRange(1, 10)
expect(val):toBeNaN()
expect(val):toBeInfinite()
```

#### String Matchers

```lua
expect("hello world"):toContain("world")
expect("hello world"):toMatch("^hello")
expect("hello world"):toStartWith("hello")
expect("hello world"):toEndWith("world")
expect("hello"):toHaveLength(5)
```

#### Table Matchers

```lua
expect(tbl):toContainKey("key")
expect(tbl):toContainKeys("a", "b", "c")
expect(tbl):toContainValue(value)
expect(tbl):toContainSubset({a = 1})
expect({}):toBeEmpty()
expect({1, 2, 3}):toBeArray()
expect(tbl):toHaveMetatable(mt)
```

#### Error Matchers

```lua
expect(fn):toThrow()
expect(fn):toThrow("error message")
expect(fn):toNotThrow()
expect(fn):toReturnValue(expected)
```

#### Negation

```lua
expect(5):never():toBe(10)
expect(value):Not():toContain("x")
```

### Hooks

```lua
describe("Suite", function()
    beforeAll(function() end)    -- Run once before all tests
    afterAll(function() end)     -- Run once after all tests
    beforeEach(function() end)   -- Run before each test
    afterEach(function() end)    -- Run after each test
    
    it("test", function() end)
end)
```

### Mocking & Spying

```lua
local mySpy = spy(originalFunction)
mySpy.fn(1, 2, 3)

mySpy:wasCalled()              -- true
mySpy:wasCalledTimes(1)        -- true
mySpy:wasCalledWith(1, 2, 3)   -- true
```

---

## QELUHttp Client

curl-like HTTP client for making REST API requests.

### Dependencies

```bash
luarocks install luasocket  # Required
luarocks install luasec     # Required for HTTPS
```

### HTTP Methods

```lua
local http = require("qeluhttp")

-- GET request
local response = http.get("https://api.example.com/users")

-- POST request
local response = http.post("https://api.example.com/users", {
    json = { name = "John", email = "john@example.com" }
})

-- PUT, DELETE, PATCH, HEAD, OPTIONS
http.put(url, options)
http.delete(url, options)
http.patch(url, options)
http.head(url, options)
http.options(url, options)
```

### Request Options

```lua
local response = http.get("https://api.example.com/data", {
    -- Query parameters
    params = {
        page = 1,
        limit = 10
    },
    
    -- Custom headers
    headers = {
        ["Authorization"] = "Bearer token123",
        ["X-Custom-Header"] = "value"
    },
    
    -- JSON body (auto-encoded)
    json = {
        key = "value"
    },
    
    -- Form data
    form = {
        username = "user",
        password = "pass"
    },
    
    -- Raw body
    body = "raw string data",
    
    -- Timeout in seconds
    timeout = 30,
    
    -- Follow redirects
    followRedirects = true
})
```

### Response Object

```lua
local response = http.get("https://api.example.com/users")

-- Status
print(response.status)        -- 200
print(response.statusText)    -- "OK"
print(response:isSuccess())   -- true (2xx)
print(response:isClientError()) -- false (4xx)
print(response:isServerError()) -- false (5xx)

-- Headers
print(response.headers["content-type"])

-- Body
print(response.body)          -- Raw string
local data = response:json()  -- Parsed JSON
```

### JSON Support

```lua
-- POST JSON
local response = http.post("https://api.example.com/users", {
    json = {
        name = "John Doe",
        age = 30,
        tags = {"developer", "lua"}
    }
})

-- Parse JSON response
local data = response:json()
print(data.id)
print(data.name)
```

### Convenience Methods

```lua
-- Simple GET returning just body
local body = http.fetch("https://api.example.com/data")

-- Simple JSON GET
local data = http.getJSON("https://api.example.com/users")

-- Simple JSON POST
local result = http.postJSON("https://api.example.com/users", {
    name = "John"
})

-- Download file
http.download("https://example.com/file.zip", "/path/to/save.zip")
```

### Error Handling

```lua
local response = http.get("https://api.example.com/users")

if response:isSuccess() then
    local data = response:json()
    print("Success:", data)
else
    print("Error:", response.status, response.statusText)
    print("Body:", response.body)
end
```

### Configuration

```lua
-- Global configuration
http.config.timeout = 60
http.config.followRedirects = true
http.config.userAgent = "MyApp/1.0"

-- Per-request override
http.get(url, { timeout = 10 })
```

### Utilities

```lua
-- URL encoding
local encoded = http.urlEncode("Hello World!")

-- Build query string
local qs = http.buildQueryString({
    search = "lua programming",
    page = 1
})  -- "search=lua+programming&page=1"

-- JSON encoding/decoding
local json = http.JSON.encode({a = 1, b = 2})
local data = http.JSON.decode('{"a":1,"b":2}')
```

---

## QELS String Utilities

Advanced string manipulation utilities for Lua.

### Splitting & Joining

```lua
local str = require("qels")

-- Split string
str.split("a,b,c", ",")                -- {"a", "b", "c"}
str.lines("line1\nline2\nline3")       -- {"line1", "line2", "line3"}
str.chars("hello")                     -- {"h", "e", "l", "l", "o"}

-- Join array
str.join({"a", "b", "c"}, ",")         -- "a,b,c"
```

### Case Conversion

```lua
str.capitalize("hello")                -- "Hello"
str.titleCase("hello world")           -- "Hello World"
str.camelCase("hello_world")           -- "helloWorld"
str.pascalCase("hello_world")          -- "HelloWorld"
str.snakeCase("helloWorld")            -- "hello_world"
str.kebabCase("helloWorld")            -- "hello-world"
str.constantCase("helloWorld")         -- "HELLO_WORLD"
```

### Trimming & Padding

```lua
str.trim("  hello  ")                  -- "hello"
str.ltrim("  hello")                   -- "hello"
str.rtrim("hello  ")                   -- "hello"
str.trimChars("--hello--", "-")        -- "hello"

str.padLeft("5", 3, "0")               -- "005"
str.padRight("5", 3, "0")              -- "500"
str.center("hi", 5)                    -- " hi  "
```

### String Checking

```lua
str.startsWith("hello", "he")          -- true
str.endsWith("hello", "lo")            -- true
str.contains("hello", "ell")           -- true
str.isEmpty("   ")                     -- true
str.isBlank(nil)                       -- true
str.isAlpha("hello")                   -- true
str.isNumeric("123")                   -- true
str.isAlphanumeric("hello123")         -- true
```

### Templates

```lua
-- {key} placeholders
str.template("Hello {name}!", {name = "World"})

-- %{key} placeholders
str.format("User: %{user}, Age: %{age}", {user = "John", age = 30})

-- ${key} placeholders
str.interpolate("Path: ${path}", {path = "/home/user"})
```

### Escaping

```lua
str.escapeHtml("<div>")                -- "&lt;div&gt;"
str.unescapeHtml("&lt;div&gt;")        -- "<div>"
str.escapePattern("a.b")               -- "a%.b"
str.escapeShell("rm -rf /")            -- Safely escaped
str.escapeQuotes('say "hello"')        -- 'say \"hello\"'
```

### Truncation & Wrapping

```lua
str.truncate("long text here", 10)     -- "long te..."
str.truncateWords("long text here", 10) -- "long..."
str.wrap("very long text that needs wrapping", 20)
```

### Utilities

```lua
str.reverse("hello")                   -- "olleh"
str.repeat_("ab", 3)                   -- "ababab"
str.count("hello", "l")                -- 2
str.replaceAll("hello", "l", "L")      -- "heLLo"
str.slugify("Hello World!")            -- "hello-world"
str.removeWhitespace("h e l l o")      -- "hello"
str.collapseWhitespace("a    b")       -- "a b"
str.extractNumbers("a1b2c3")           -- {1, 2, 3}
str.extractWords("hello123world")      -- {"hello", "world"}
```

### String Similarity

```lua
-- Levenshtein distance
str.levenshtein("hello", "hallo")      -- 1

-- Similarity score (0-1)
str.similarity("hello", "hallo")       -- 0.8
```

### Random Generation

```lua
-- Random string
str.random(10)                         -- Random 10-char alphanumeric
str.random(8, "0123456789")            -- Random 8-digit number

-- UUID
str.uuid()                             -- "550e8400-e29b-41d4-a716-446655440000"
```

```

---

## QELUT Table Utilities

Functional programming and advanced table operations for Lua.

### Deep Operations

```lua
local t = require("qelut")

-- Deep copy
local copy = t.deepCopy(originalTable)

-- Deep merge
local merged = t.deepMerge(table1, table2, table3)

-- Deep equality
t.deepEqual(table1, table2)  -- true/false
```

### Functional Programming

```lua
-- Map
local doubled = t.map({1, 2, 3}, function(x) return x * 2 end)  -- {2, 4, 6}

-- Filter
local evens = t.filter({1, 2, 3, 4}, function(x) return x % 2 == 0 end)  -- {2, 4}

-- Reduce
local sum = t.reduce({1, 2, 3, 4}, function(acc, x) return acc + x end, 0)  -- 10

-- Find
local value, index = t.find({1, 2, 3}, function(x) return x > 1 end)  -- 2, 2

-- Some/Every
t.some({1, 2, 3}, function(x) return x > 2 end)   -- true
t.every({1, 2, 3}, function(x) return x > 0 end)  -- true
```

### Array Operations

```lua
t.flatten({{1, 2}, {3, 4}})              -- {1, 2, 3, 4}
t.unique({1, 2, 2, 3, 3})                -- {1, 2, 3}
t.reverse({1, 2, 3})                     -- {3, 2, 1}
t.shuffle({1, 2, 3, 4, 5})               -- Random order
t.chunk({1, 2, 3, 4, 5}, 2)              -- {{1, 2}, {3, 4}, {5}}
t.zip({1, 2}, {"a", "b"})                -- {{1, "a"}, {2, "b"}}
t.slice({1, 2, 3, 4, 5}, 2, 4)           -- {2, 3, 4}
t.take({1, 2, 3, 4, 5}, 3)               -- {1, 2, 3}
t.drop({1, 2, 3, 4, 5}, 2)               -- {3, 4, 5}
```

### Object Operations

```lua
t.keys({a = 1, b = 2})                   -- {"a", "b"}
t.values({a = 1, b = 2})                 -- {1, 2}
t.entries({a = 1, b = 2})                -- {{"a", 1}, {"b", 2}}
t.pick({a = 1, b = 2, c = 3}, {"a", "c"}) -- {a = 1, c = 3}
t.omit({a = 1, b = 2, c = 3}, {"b"})     -- {a = 1, c = 3}
t.invert({a = 1, b = 2})                 -- {[1] = "a", [2] = "b"}
t.merge({a = 1}, {b = 2}, {c = 3})       -- {a = 1, b = 2, c = 3}
```

### Path-based Access

```lua
local data = {user = {name = "John", age = 30}}

t.get(data, "user.name")                 -- "John"
t.get(data, "user.email", "N/A")         -- "N/A" (default)
t.set(data, "user.email", "john@example.com")
t.has(data, "user.name")                 -- true
```

### Set Operations

```lua
t.difference({1, 2, 3}, {2, 3, 4})       -- {1}
t.intersection({1, 2, 3}, {2, 3, 4})     -- {2, 3}
t.union({1, 2}, {2, 3}, {3, 4})          -- {1, 2, 3, 4}
```

---

## QELUJ JSON Library

Robust JSON encoding and decoding for Lua.

### Encoding

```lua
local json = require("qeluj")

-- Basic encoding
local str = json.encode({name = "John", age = 30})
-- {"name":"John","age":30}

-- Pretty printing
local pretty = json.encodePretty({
    name = "John",
    tags = {"developer", "lua"}
})
--[[
{
  "name": "John",
  "tags": [
    "developer",
    "lua"
  ]
}
]]

-- Custom options
json.encode(data, {
    pretty = true,
    indent = "    ",  -- 4 spaces
    strict = true
})
```

### Decoding

```lua
local data = json.decode('{"name":"John","age":30}')
print(data.name)  -- John
print(data.age)   -- 30

-- With options
json.decode(str, {
    strict = true,
    nullValue = false  -- Use false instead of nil for null
})
```

### File I/O

```lua
-- Write JSON to file
json.encodeFile(data, "output.json")
json.encodeFile(data, "pretty.json", {pretty = true})

-- Read JSON from file
local data = json.decodeFile("input.json")
```

### Utilities

```lua
-- Validate JSON
json.isValid('{"valid": true}')          -- true
json.isValid('{invalid}')                -- false

-- Minify JSON
json.minify('{ "a" : 1 , "b" : 2 }')     -- {"a":1,"b":2}

-- Prettify JSON
json.prettify('{"a":1,"b":2}')
--[[
{
  "a": 1,
  "b": 2
}
]]
```

### Features

- ✅ Handles nested structures
- ✅ Proper array vs object detection
- ✅ Escape sequences (\\n, \\t, \\", etc.)
- ✅ Unicode support (basic)
- ✅ NaN/Infinity handling
- ✅ Configurable null values
- ✅ Strict mode for validation
- ✅ Maximum depth protection


---

## QELUP Python Bridge

Call Python code from Lua and access the entire Python ecosystem.

### Installation

QELUP requires building a C extension:

```bash
# Install dependencies
# macOS:
brew install python3

# Ubuntu/Debian:
sudo apt-get install python3-dev lua5.4-dev build-essential

# Build
cd QELU
make

# (Optional) Install
make install
```

### Basic Usage

```lua
local py = require("qelup")

-- Initialize Python (auto-detects Python 2 or 3)
py.initialize()

-- Import modules
local sys = py.import("sys")
local os = py.import("os")

print("Python version:", sys.version)
print("Current directory:", os.getcwd())

-- Execute Python code
py.exec([[
def greet(name):
    return f"Hello, {name}!"
]])

-- Call Python functions
local greet = py.eval("greet")
print(greet("Lua"))  -- "Hello, Lua!"

-- Cleanup (optional, happens automatically)
py.finalize()
```

### Importing Modules

```lua
-- Standard library
local json = py.import("json")
local math = py.import("math")
local datetime = py.import("datetime")

-- Third-party (if installed)
local numpy = py.import("numpy")
local pandas = py.import("pandas")
local requests = py.import("requests")
```

### Type Conversion

Automatic conversion between Lua and Python types:

| Lua Type | Python Type |
|----------|-------------|
| `nil` | `None` |
| `boolean` | `bool` |
| `number` | `int` or `float` |
| `string` | `str` |
| `table` (array) | `list` |
| `table` (dict) | `dict` |

```lua
-- Lua to Python
py.exec("data = None")  -- nil → None
local result = py.eval("data")  -- None → nil

-- Arrays
local arr = {1, 2, 3, 4, 5}
py.exec("arr = " .. py.JSON.encode(arr))

-- Dictionaries
local dict = {name = "John", age = 30}
-- Converted automatically when passed to Python
```

### Calling Python Functions

```lua
-- Define Python function
py.exec([[
def add(a, b):
    return a + b

def multiply(a, b):
    return a * b
]])

-- Call from Lua
local add = py.eval("add")
local result = add(5, 3)  -- 8

-- Chain calls
local math = py.import("math")
local sqrt = math.sqrt(16)  -- 4.0
```

### Using Python Classes

```lua
py.exec([[
class Calculator:
    def __init__(self, initial=0):
        self.value = initial
    
    def add(self, x):
        self.value += x
        return self.value
    
    def get(self):
        return self.value
]])

local Calculator = py.eval("Calculator")
local calc = Calculator(10)
print(calc:add(5))   -- 15
print(calc:add(3))   -- 18
print(calc:get())    -- 18
```

### Working with NumPy

```lua
local np = py.import("numpy")

-- Create array
local arr = np.array({1, 2, 3, 4, 5})

-- Operations
local mean = np.mean(arr)
local std = np.std(arr)
local sum = np.sum(arr)

print("Mean:", mean)
print("Std:", std)
print("Sum:", sum)

-- Matrix operations
local matrix = np.array({{1, 2}, {3, 4}})
local det = np.linalg.det(matrix)
print("Determinant:", det)
```

### Working with pandas

```lua
local pd = py.import("pandas")

-- Create DataFrame
py.exec([[
import pandas as pd
df = pd.DataFrame({
    'name': ['Alice', 'Bob', 'Charlie'],
    'age': [25, 30, 35],
    'city': ['NYC', 'LA', 'Chicago']
})
]])

local df = py.eval("df")

-- Access data
print(df:head())
print("Mean age:", df.age:mean())
```

### Error Handling

```lua
-- Use pcall for error handling
local ok, result = py.pcall(function()
    return py.eval("1 / 0")  -- Division by zero
end)

if not ok then
    print("Python error:", result)
end

-- Or use Lua's pcall
local ok, result = pcall(function()
    local bad_module = py.import("nonexistent_module")
end)

if not ok then
    print("Import failed:", result)
end
```

### Convenience Functions

```lua
-- Quick module access
local sys = py.sys()
local os = py.os()
local json = py.json()
local math = py.math()
local datetime = py.datetime()

-- Check if module exists
if py.hasModule("numpy") then
    local np = py.import("numpy")
end

-- Get Python version
local version = py.version()
print("Python", version.major .. "." .. version.minor)

-- List available modules
local modules = py.listModules()
for _, name in ipairs(modules) do
    print(name)
end
```

### Known Issues

- **Segfault on finalize**: Calling `py.finalize()` may cause a segmentation fault on some systems. This is a known issue with Python's `Py_Finalize()`. The Python interpreter is automatically cleaned up when the Lua process exits, so calling `finalize()` is optional.

- **Thread safety**: QELUP is not thread-safe. Use only from the main Lua thread.

- **Memory management**: Python objects are automatically garbage collected when no longer referenced from Lua.

### Dependencies

- **Python 2.7+** or **Python 3.x**
- **Python development headers** (`python-dev` or `python3-dev`)
- **Lua 5.1+** or **LuaJIT**
- **C compiler** (gcc, clang)

---

## API Reference

### QELU OOP

| Function | Description |
|----------|-------------|
| `QELU.class(name, options)` | Create a new class |
| `QELU.interface(name, methods)` | Define an interface |
| `QELU.mixin(members)` | Create a mixin |
| `QELU.abstract(name)` | Mark method as abstract |
| `QELU.super(instance)` | Get super reference |
| `QELU.singleton(name)` | Create singleton class |
| `QELU.enum(name, values)` | Create an enum |
| `QELU.isClass(value)` | Check if value is a class |
| `QELU.isInstance(value)` | Check if value is an instance |
| `QELU.instanceOf(inst, cls)` | Check instance type |
| `QELU.classOf(instance)` | Get instance's class |
| `QELU.className(value)` | Get class name |
| `QELU.getMethods(value)` | Get all methods |

### QELUTest

| Function | Description |
|----------|-------------|
| `QELUTest.describe(name, fn)` | Create test suite |
| `QELUTest.it(name, fn)` | Create test case |
| `QELUTest.expect(value)` | Create expectation |
| `QELUTest.beforeAll(fn)` | Suite setup hook |
| `QELUTest.afterAll(fn)` | Suite teardown hook |
| `QELUTest.beforeEach(fn)` | Test setup hook |
| `QELUTest.afterEach(fn)` | Test teardown hook |
| `QELUTest.spy(fn)` | Create function spy |
| `QELUTest.mock(obj)` | Create mock object |
| `QELUTest.run(options)` | Run all tests |
| `QELUTest.globalize()` | Export to global scope |

### QELUHttp

| Function | Description |
|----------|-------------|
| `http.get(url, options)` | GET request |
| `http.post(url, options)` | POST request |
| `http.put(url, options)` | PUT request |
| `http.delete(url, options)` | DELETE request |
| `http.patch(url, options)` | PATCH request |
| `http.head(url, options)` | HEAD request |
| `http.options(url, options)` | OPTIONS request |
| `http.fetch(url, options)` | Simple GET (returns body) |
| `http.getJSON(url, options)` | GET and parse JSON |
| `http.postJSON(url, data, options)` | POST JSON |
| `http.download(url, filepath, options)` | Download file |
| `http.urlEncode(str)` | URL encode string |
| `http.buildQueryString(params)` | Build query string |
| `http.JSON.encode(value)` | Encode to JSON |
| `http.JSON.decode(str)` | Decode from JSON |

---

## Examples

See the included example files:
- `test.lua` - Comprehensive test suite for QELU and QELUTest
- `http_examples.lua` - HTTP client examples

---

## License

MIT License - see [LICENSE](LICENSE) for details.

---

**QELU Suite:**
- ✅ **qelu.lua** (25KB) - OOP System
- ✅ **qelutest.lua** (31KB) - Testing Framework  
- ✅ **qeluhttp.lua** (15KB) - HTTP Client
- ✅ **qels.lua** (18KB) - String Utilities
- ✅ **qelut.lua** (15KB) - Table Utilities
- ✅ **qeluj.lua** (13KB) - JSON Library
- ✅ **qelup.lua** + **bindings/qelup.c** - Python Bridge (C extension)

Made with ❤️ for the Lua community
