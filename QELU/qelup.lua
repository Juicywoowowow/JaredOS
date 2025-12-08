--[[
    QELUP - QELU Python Bridge
    Part of the QELU (Quality Enhanced Lua Utilities) library
    
    Features:
    - Call Python functions from Lua
    - Import Python modules
    - Execute Python code
    - Automatic type conversion
    - Support for Python 2.7 and Python 3.x
    - Bidirectional object access
    
    Dependencies:
    - Python 2.7+ or Python 3.x
    - qelup_core.so (C extension)
    
    @author QELU Contributors
    @license MIT
    @version 1.0.0
]]

local QELUP = {
    _VERSION = "1.0.0",
    _DESCRIPTION = "QELUP - QELU Python Bridge",
    _LICENSE = "MIT",
    _initialized = false,
    _python_version = nil
}

-- Try to load the C extension
local core_loaded, core = pcall(require, "qelup_core")

if not core_loaded then
    error([[
╔════════════════════════════════════════════════════════════════╗
║                    QELUP BUILD ERROR                           ║
╚════════════════════════════════════════════════════════════════╝

QELUP requires the C extension to be built.

Build instructions:

  1. Make sure you have Python development headers:
     macOS:    brew install python3
     Ubuntu:   sudo apt-get install python3-dev
     
  2. Build the extension:
     cd QELU
     make
     
  3. (Optional) Install:
     make install

For more information, see the QELUP documentation.

════════════════════════════════════════════════════════════════
]])
end

-- Module cache
local module_cache = {}

-- ============================================================================
-- Initialization
-- ============================================================================

--- Initialize Python interpreter
--- @param options table|nil {version: number}
--- @return boolean success
function QELUP.initialize(options)
    if QELUP._initialized then
        return true
    end
    
    options = options or {}
    
    local ok, err = core.initialize(options.version or 0)
    if not ok then
        error("Failed to initialize Python: " .. tostring(err))
    end
    
    -- Get Python version
    local version_str, major, minor = core.version()
    QELUP._python_version = {
        string = version_str,
        major = major,
        minor = minor
    }
    
    QELUP._initialized = true
    return true
end

--- Check if Python is initialized
--- @return boolean
function QELUP.isInitialized()
    return QELUP._initialized
end

--- Get Python version information
--- @return table|nil
function QELUP.version()
    if not QELUP._initialized then
        return nil
    end
    return QELUP._python_version
end

--- Finalize Python interpreter
function QELUP.finalize()
    if not QELUP._initialized then
        return
    end
    
    module_cache = {}
    core.finalize()
    QELUP._initialized = false
    QELUP._python_version = nil
end

-- ============================================================================
-- Module Management
-- ============================================================================

--- Import a Python module
--- @param name string Module name (e.g., "sys", "os", "numpy")
--- @return table Python module object
function QELUP.import(name)
    if not QELUP._initialized then
        QELUP.initialize()
    end
    
    -- Check cache
    if module_cache[name] then
        return module_cache[name]
    end
    
    -- Import module
    local module = core.import(name)
    module_cache[name] = module
    
    return module
end

--- Clear module cache
function QELUP.clearCache()
    module_cache = {}
end

-- ============================================================================
-- Code Execution
-- ============================================================================

--- Execute Python code
--- @param code string Python code to execute
--- @return boolean success
function QELUP.exec(code)
    if not QELUP._initialized then
        QELUP.initialize()
    end
    
    return core.exec(code)
end

--- Evaluate Python expression
--- @param expr string Python expression to evaluate
--- @return any Result of evaluation
function QELUP.eval(expr)
    if not QELUP._initialized then
        QELUP.initialize()
    end
    
    return core.eval(expr)
end

--- Execute Python code from file
--- @param filepath string Path to Python file
--- @return boolean success
function QELUP.execFile(filepath)
    local file = io.open(filepath, "r")
    if not file then
        error("Cannot open file: " .. filepath)
    end
    
    local code = file:read("*all")
    file:close()
    
    return QELUP.exec(code)
end

-- ============================================================================
-- Convenience Functions
-- ============================================================================

--- Protected call for Python operations
--- @param fn function Function to call
--- @param ... any Arguments
--- @return boolean success
--- @return any result or error
function QELUP.pcall(fn, ...)
    return pcall(fn, ...)
end

--- Create a Python object from Lua table
--- @param tbl table Lua table
--- @param as_list boolean|nil Force as list (default: auto-detect)
--- @return table Python object
function QELUP.table(tbl, as_list)
    if not QELUP._initialized then
        QELUP.initialize()
    end
    
    if as_list then
        -- Force as Python list
        local list_code = "["
        for i, v in ipairs(tbl) do
            if i > 1 then list_code = list_code .. ", " end
            if type(v) == "string" then
                list_code = list_code .. string.format("%q", v)
            else
                list_code = list_code .. tostring(v)
            end
        end
        list_code = list_code .. "]"
        return QELUP.eval(list_code)
    else
        -- Auto-detect (handled by C extension)
        return QELUP.eval("None")  -- Placeholder, actual conversion in C
    end
end

--- Get Python builtins
--- @return table Python builtins module
function QELUP.builtins()
    if QELUP._python_version and QELUP._python_version.major >= 3 then
        return QELUP.import("builtins")
    else
        return QELUP.import("__builtin__")
    end
end

-- ============================================================================
-- Common Python Modules (Shortcuts)
-- ============================================================================

--- Import sys module
--- @return table
function QELUP.sys()
    return QELUP.import("sys")
end

--- Import os module
--- @return table
function QELUP.os()
    return QELUP.import("os")
end

--- Import json module
--- @return table
function QELUP.json()
    return QELUP.import("json")
end

--- Import math module
--- @return table
function QELUP.math()
    return QELUP.import("math")
end

--- Import datetime module
--- @return table
function QELUP.datetime()
    return QELUP.import("datetime")
end

--- Import re (regex) module
--- @return table
function QELUP.re()
    return QELUP.import("re")
end

-- ============================================================================
-- Helper Functions
-- ============================================================================

--- Check if a Python module is available
--- @param name string Module name
--- @return boolean
function QELUP.hasModule(name)
    local ok = pcall(function()
        QELUP.import(name)
    end)
    return ok
end

--- Get list of available Python modules
--- @return table List of module names
function QELUP.listModules()
    if not QELUP._initialized then
        QELUP.initialize()
    end
    
    local code = [[
import sys
import pkgutil
modules = [name for _, name, _ in pkgutil.iter_modules()]
modules
]]
    
    return QELUP.eval(code)
end

--- Print Python object (for debugging)
--- @param obj any Python object
function QELUP.print(obj)
    if not QELUP._initialized then
        QELUP.initialize()
    end
    
    local builtins = QELUP.builtins()
    builtins.print(obj)
end

--- Get type of Python object
--- @param obj any Python object
--- @return string Type name
function QELUP.type(obj)
    if not QELUP._initialized then
        QELUP.initialize()
    end
    
    local builtins = QELUP.builtins()
    local py_type = builtins.type(obj)
    return tostring(py_type)
end

-- ============================================================================
-- Auto-initialization
-- ============================================================================

-- Auto-initialize on first use (can be disabled by calling finalize first)
local mt = {
    __index = function(t, k)
        -- Auto-initialize if accessing a function
        if not QELUP._initialized and type(QELUP[k]) == "function" then
            if k ~= "initialize" and k ~= "finalize" and k ~= "isInitialized" then
                QELUP.initialize()
            end
        end
        return rawget(QELUP, k)
    end
}

setmetatable(QELUP, mt)

-- ============================================================================
-- Module Export
-- ============================================================================

return QELUP
