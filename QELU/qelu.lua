--[[
    QELU - Quality Enhanced Lua Utilities
    A comprehensive Object-Oriented Programming library for Lua/LuaJIT
    
    Features:
    - Class definition with inheritance
    - Multiple inheritance support
    - Mixins and interfaces
    - Private/public/protected members
    - Static members and methods
    - Abstract classes and methods
    - Method chaining
    - Super calls
    - Metamethod inheritance
    - Type checking and reflection
    
    @author QELU Contributors
    @license MIT
    @version 1.0.0
]]

local QELU = {
    _VERSION = "1.0.0",
    _DESCRIPTION = "Quality Enhanced Lua Utilities - OOP for Lua/LuaJIT",
    _LICENSE = "MIT"
}

-- ============================================================================
-- Internal Utilities
-- ============================================================================

local rawget, rawset = rawget, rawset
local setmetatable, getmetatable = setmetatable, getmetatable
local type, pairs, ipairs = type, pairs, ipairs
local error, assert = error, assert
local format = string.format
local unpack = unpack or table.unpack

--- Deep copy a table
--- @param tbl table
--- @param seen table|nil
--- @return table
local function deepcopy(tbl, seen)
    if type(tbl) ~= "table" then return tbl end
    seen = seen or {}
    if seen[tbl] then return seen[tbl] end
    
    local copy = {}
    seen[tbl] = copy
    
    for k, v in pairs(tbl) do
        copy[deepcopy(k, seen)] = deepcopy(v, seen)
    end
    
    return setmetatable(copy, getmetatable(tbl))
end

--- Shallow merge tables
--- @param ... table
--- @return table
local function merge(...)
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

--- Check if a value is callable
--- @param v any
--- @return boolean
local function isCallable(v)
    if type(v) == "function" then return true end
    local mt = getmetatable(v)
    return mt and type(mt.__call) == "function"
end

-- ============================================================================
-- Class Registry (for type checking and reflection)
-- ============================================================================

local classRegistry = setmetatable({}, { __mode = "k" })
local instanceRegistry = setmetatable({}, { __mode = "k" })

-- Store the class for each instance for super calls during construction
local instanceClassMap = setmetatable({}, { __mode = "k" })

-- ============================================================================
-- Private Member Storage
-- ============================================================================

local privateStorage = setmetatable({}, { __mode = "k" })

--- Get private storage for an instance
--- @param instance table
--- @return table
local function getPrivate(instance)
    if not privateStorage[instance] then
        privateStorage[instance] = {}
    end
    return privateStorage[instance]
end

-- ============================================================================
-- Abstract Method Marker
-- ============================================================================

local ABSTRACT_MARKER = {}

--- Mark a method as abstract
--- @param name string
--- @return function
function QELU.abstract(name)
    return setmetatable({
        _abstract = true,
        _name = name or "unnamed"
    }, {
        __call = function(self)
            error(format("Abstract method '%s' must be implemented", self._name), 2)
        end
    })
end

-- ============================================================================
-- Interface Definition
-- ============================================================================

local interfaceRegistry = {}

--- Define an interface
--- @param name string
--- @param methods table
--- @return table
function QELU.interface(name, methods)
    assert(type(name) == "string", "Interface name must be a string")
    assert(type(methods) == "table", "Interface methods must be a table")
    
    local iface = {
        _isInterface = true,
        _name = name,
        _methods = {}
    }
    
    for methodName, signature in pairs(methods) do
        iface._methods[methodName] = signature or true
    end
    
    interfaceRegistry[name] = iface
    
    return iface
end

--- Check if value is an interface
--- @param value any
--- @return boolean
local function isInterface(value)
    return type(value) == "table" and rawget(value, "_isInterface") == true
end

--- Check if a class implements an interface
--- @param cls table
--- @param iface table
--- @return boolean, string|nil
local function implementsInterface(cls, iface)
    for methodName, _ in pairs(iface._methods) do
        local method = rawget(cls, methodName)
        if not method or not isCallable(method) then
            return false, format("Missing method '%s' from interface '%s'", methodName, iface._name)
        end
    end
    return true
end

-- ============================================================================
-- Mixin Support
-- ============================================================================

--- Create a mixin
--- @param members table
--- @return table
function QELU.mixin(members)
    return {
        _isMixin = true,
        _members = members or {}
    }
end

--- Apply a mixin to a class
--- @param cls table
--- @param mixin table
local function applyMixin(cls, mixin)
    for k, v in pairs(mixin._members) do
        if rawget(cls, k) == nil then
            rawset(cls, k, v)
        end
    end
end

-- ============================================================================
-- Core Class System
-- ============================================================================

--- Create the instance metatable
--- @param cls table
--- @return table
local function createInstanceMeta(cls)
    local meta = {
        __index = function(self, key)
            -- Check for private access attempt
            if type(key) == "string" and key:sub(1, 2) == "__" and key:sub(-2) ~= "__" then
                local priv = getPrivate(self)
                return priv[key]
            end
            
            -- Check class directly
            local value = rawget(cls, key)
            if value ~= nil then
                return value
            end
            
            -- Check parent classes recursively using rawget
            local function findInParents(c, visited)
                visited = visited or {}
                if visited[c] then return nil end
                visited[c] = true
                
                local parents = rawget(c, "_parents")
                if parents then
                    for _, parent in ipairs(parents) do
                        -- First check parent directly
                        local val = rawget(parent, key)
                        if val ~= nil then
                            return val
                        end
                        -- Then check parent's parents
                        val = findInParents(parent, visited)
                        if val ~= nil then
                            return val
                        end
                    end
                end
                return nil
            end
            
            return findInParents(cls)
        end,
        
        __newindex = function(self, key, value)
            -- Private members (double underscore prefix)
            if type(key) == "string" and key:sub(1, 2) == "__" and key:sub(-2) ~= "__" then
                local priv = getPrivate(self)
                priv[key] = value
            else
                rawset(self, key, value)
            end
        end,
        
        __tostring = function(self)
            local className = rawget(cls, "_name") or "Object"
            if rawget(cls, "toString") and isCallable(rawget(cls, "toString")) then
                return cls.toString(self)
            end
            -- Use debug.getinfo or string.format to get address without triggering __tostring
            local addr = string.format("%p", self):match("0x(.+)") or "?"
            return format("<%s: %s>", className, addr)
        end
    }
    
    -- Inherit metamethods from class
    local metamethods = {
        "__add", "__sub", "__mul", "__div", "__mod", "__pow",
        "__unm", "__concat", "__len", "__eq", "__lt", "__le",
        "__call", "__pairs", "__ipairs"
    }
    
    for _, mm in ipairs(metamethods) do
        local method = rawget(cls, mm)
        if method then
            meta[mm] = method
        end
    end
    
    return meta
end

--- Create a new class
--- @param name string|nil
--- @param options table|nil
--- @return table
function QELU.class(name, options)
    options = options or {}
    
    local cls = {
        _isClass = true,
        _name = name or "AnonymousClass",
        _parents = {},
        _interfaces = {},
        _static = {},
        _abstract = options.abstract or false
    }
    
    -- Static members container
    local static = {}
    
    -- Class metatable
    local classMeta = {
        __call = function(self, ...)
            -- Prevent instantiation of abstract classes
            if rawget(cls, "_abstract") then
                error(format("Cannot instantiate abstract class '%s'", rawget(cls, "_name")), 2)
            end
            
            -- Check all abstract methods are implemented
            for k, v in pairs(cls) do
                if type(v) == "table" and rawget(v, "_abstract") then
                    error(format("Cannot instantiate class '%s': abstract method '%s' not implemented", 
                        rawget(cls, "_name"), rawget(v, "_name")), 2)
                end
            end
            
            -- Create instance
            local instance = setmetatable({}, createInstanceMeta(cls))
            instanceRegistry[instance] = cls
            instanceClassMap[instance] = cls
            
            -- Initialize private storage
            privateStorage[instance] = {}
            
            -- Call constructor
            local init = rawget(cls, "init") or rawget(cls, "new") or rawget(cls, "constructor")
            if init then
                init(instance, ...)
            end
            
            return instance
        end,
        
        __index = function(self, key)
            -- Check static members first
            if static[key] ~= nil then
                return static[key]
            end
            return rawget(cls, key)
        end,
        
        __newindex = function(self, key, value)
            rawset(cls, key, value)
        end,
        
        __tostring = function()
            return format("<class '%s'>", rawget(cls, "_name"))
        end
    }
    
    setmetatable(cls, classMeta)
    classRegistry[cls] = true
    
    -- Static member accessor
    function cls.static(members)
        for k, v in pairs(members) do
            static[k] = v
            rawset(cls, "_static", rawget(cls, "_static") or {})
            cls._static[k] = v
        end
        return cls
    end
    
    -- Inheritance (handles both cls.extends(...) and cls:extends(...))
    function cls.extends(...)
        local args = {...}
        for i, parent in ipairs(args) do
            -- Skip if parent is self (when called with colon syntax)
            if parent ~= cls then
                assert(type(parent) == "table", "Parent must be a class or table")
                table.insert(rawget(cls, "_parents"), parent)
                
                -- Copy methods from parent (respecting existing) using rawget/rawset
                for k, v in pairs(parent) do
                    if rawget(cls, k) == nil and type(k) == "string" and k:sub(1, 1) ~= "_" then
                        rawset(cls, k, v)
                    end
                end
                
                -- Copy static members
                local parentStatic = rawget(parent, "_static")
                if parentStatic then
                    for k, v in pairs(parentStatic) do
                        if static[k] == nil then
                            static[k] = v
                            cls._static[k] = v
                        end
                    end
                end
            end
        end
        return cls
    end
    
    -- Implement interfaces (handles both cls.implements(...) and cls:implements(...))
    function cls.implements(...)
        local args = {...}
        for _, iface in ipairs(args) do
            -- Skip if iface is self (when called with colon syntax)
            if iface ~= cls and isInterface(iface) then
                table.insert(rawget(cls, "_interfaces"), iface)
            elseif iface ~= cls then
                assert(false, "Argument must be an interface")
            end
        end
        return cls
    end
    
    -- Include mixins (handles both cls.include(...) and cls:include(...))
    function cls.include(...)
        local args = {...}
        for _, mixin in ipairs(args) do
            -- Skip if mixin is self (when called with colon syntax)
            if mixin ~= cls and type(mixin) == "table" then
                if rawget(mixin, "_isMixin") then
                    applyMixin(cls, mixin)
                else
                    for k, v in pairs(mixin) do
                        if rawget(cls, k) == nil then
                            rawset(cls, k, v)
                        end
                    end
                end
            end
        end
        return cls
    end
    
    -- Validate interfaces before first instantiation
    local validated = false
    local origCall = classMeta.__call
    classMeta.__call = function(self, ...)
        if not validated then
            local interfaces = rawget(cls, "_interfaces")
            for _, iface in ipairs(interfaces) do
                local ok, err = implementsInterface(cls, iface)
                if not ok then
                    error(format("Class '%s' does not implement interface: %s", rawget(cls, "_name"), err), 2)
                end
            end
            validated = true
        end
        return origCall(self, ...)
    end
    
    return cls
end

-- ============================================================================
-- Super Call Support
-- ============================================================================

--- Create a super reference for calling parent methods
--- @param instance table
--- @param targetClass table|nil
--- @return table
function QELU.super(instance, targetClass)
    local cls = targetClass or instanceClassMap[instance]
    
    if not cls then
        error("Cannot create super reference: instance not from a QELU class", 2)
    end
    
    local parents = rawget(cls, "_parents")
    if not parents or #parents == 0 then
        error(format("Class '%s' has no parent class", rawget(cls, "_name")), 2)
    end
    
    local superProxy = {}
    
    setmetatable(superProxy, {
        __index = function(_, key)
            -- Find method in parents using rawget only
            local function findMethod(parentList, visited)
                visited = visited or {}
                for _, parent in ipairs(parentList) do
                    if not visited[parent] then
                        visited[parent] = true
                        
                        local value = rawget(parent, key)
                        if value ~= nil then
                            return value, parent
                        end
                        
                        -- Check grandparents
                        local grandparents = rawget(parent, "_parents")
                        if grandparents then
                            value, parent = findMethod(grandparents, visited)
                            if value ~= nil then
                                return value, parent
                            end
                        end
                    end
                end
                return nil, nil
            end
            
            local value, foundInParent = findMethod(parents, {})
            
            if value ~= nil then
                if isCallable(value) then
                    return function(_, ...)
                        return value(instance, ...)
                    end
                end
                return value
            end
            return nil
        end
    })
    
    return superProxy
end

-- ============================================================================
-- Type Checking & Reflection
-- ============================================================================

--- Check if value is a QELU class
--- @param value any
--- @return boolean
function QELU.isClass(value)
    return type(value) == "table" and classRegistry[value] == true
end

--- Check if value is an instance of a QELU class
--- @param value any
--- @return boolean
function QELU.isInstance(value)
    return type(value) == "table" and instanceRegistry[value] ~= nil
end

--- Check if an instance is of a specific class (or subclass)
--- @param instance any
--- @param cls table
--- @return boolean
function QELU.instanceOf(instance, cls)
    if not QELU.isInstance(instance) then
        return false
    end
    
    local instanceClass = instanceRegistry[instance]
    
    -- Direct match
    if instanceClass == cls then
        return true
    end
    
    -- Check parent chain using rawget
    local function checkParents(c, visited)
        visited = visited or {}
        if visited[c] then return false end
        visited[c] = true
        
        if c == cls then return true end
        
        local parents = rawget(c, "_parents")
        if parents then
            for _, parent in ipairs(parents) do
                if checkParents(parent, visited) then
                    return true
                end
            end
        end
        return false
    end
    
    return checkParents(instanceClass, {})
end

--- Get the class of an instance
--- @param instance any
--- @return table|nil
function QELU.classOf(instance)
    return instanceRegistry[instance]
end

--- Get class name
--- @param classOrInstance any
--- @return string|nil
function QELU.className(classOrInstance)
    if QELU.isClass(classOrInstance) then
        return rawget(classOrInstance, "_name")
    elseif QELU.isInstance(classOrInstance) then
        local cls = instanceRegistry[classOrInstance]
        return cls and rawget(cls, "_name")
    end
    return nil
end

--- Get all methods of a class or instance
--- @param classOrInstance table
--- @return table
function QELU.getMethods(classOrInstance)
    local cls = QELU.isClass(classOrInstance) and classOrInstance 
                or instanceRegistry[classOrInstance]
    
    if not cls then return {} end
    
    local methods = {}
    
    local function collectMethods(c, visited)
        visited = visited or {}
        if visited[c] then return end
        visited[c] = true
        
        for k, v in pairs(c) do
            if isCallable(v) and type(k) == "string" and k:sub(1, 1) ~= "_" then
                methods[k] = v
            end
        end
        local parents = rawget(c, "_parents")
        if parents then
            for _, parent in ipairs(parents) do
                collectMethods(parent, visited)
            end
        end
    end
    
    collectMethods(cls, {})
    return methods
end

-- ============================================================================
-- Property System (Getters/Setters)
-- ============================================================================

--- Define properties with getters and setters
--- @param cls table
--- @param properties table
function QELU.properties(cls, properties)
    assert(QELU.isClass(cls), "First argument must be a QELU class")
    
    local propStorage = setmetatable({}, { __mode = "k" })
    
    for propName, propDef in pairs(properties) do
        local getter = propDef.get
        local setter = propDef.set
        local default = propDef.default
        
        -- Create getter method
        if getter then
            rawset(cls, "get" .. propName:sub(1,1):upper() .. propName:sub(2), function(self)
                return getter(self, propStorage[self] and propStorage[self][propName])
            end)
        end
        
        -- Create setter method
        if setter then
            rawset(cls, "set" .. propName:sub(1,1):upper() .. propName:sub(2), function(self, value)
                if not propStorage[self] then
                    propStorage[self] = {}
                end
                propStorage[self][propName] = setter(self, value)
                return self  -- Allow chaining
            end)
        end
        
        -- Initialize with default in constructor wrapper
        local origInit = rawget(cls, "init")
        rawset(cls, "init", function(self, ...)
            if not propStorage[self] then
                propStorage[self] = {}
            end
            if default ~= nil then
                propStorage[self][propName] = default
            end
            if origInit then
                return origInit(self, ...)
            end
        end)
    end
    
    return cls
end

-- ============================================================================
-- Event System (Observer Pattern)
-- ============================================================================

local eventMixin = QELU.mixin({
    on = function(self, event, callback)
        self._events = self._events or {}
        self._events[event] = self._events[event] or {}
        table.insert(self._events[event], callback)
        return self
    end,
    
    off = function(self, event, callback)
        if not self._events or not self._events[event] then
            return self
        end
        
        if callback then
            for i, cb in ipairs(self._events[event]) do
                if cb == callback then
                    table.remove(self._events[event], i)
                    break
                end
            end
        else
            self._events[event] = nil
        end
        return self
    end,
    
    emit = function(self, event, ...)
        if not self._events or not self._events[event] then
            return self
        end
        
        for _, callback in ipairs(self._events[event]) do
            callback(self, ...)
        end
        return self
    end,
    
    once = function(self, event, callback)
        local wrapper
        wrapper = function(...)
            self:off(event, wrapper)
            callback(...)
        end
        return self:on(event, wrapper)
    end
})

QELU.EventEmitter = eventMixin

-- ============================================================================
-- Singleton Pattern
-- ============================================================================

--- Create a singleton class
--- @param name string
--- @return table
function QELU.singleton(name)
    local instance = nil
    local cls = QELU.class(name)
    
    local mt = getmetatable(cls)
    local origCall = mt.__call
    
    mt.__call = function(self, ...)
        if instance == nil then
            instance = origCall(self, ...)
        end
        return instance
    end
    
    rawset(cls, "getInstance", function(...)
        return cls(...)
    end)
    
    rawset(cls, "resetInstance", function()
        instance = nil
    end)
    
    return cls
end

-- ============================================================================
-- Enum Support
-- ============================================================================

--- Create an enum
--- @param name string
--- @param values table
--- @return table
function QELU.enum(name, values)
    local enum = {
        _isEnum = true,
        _name = name,
        _values = {}
    }
    
    if type(values) == "table" then
        if #values > 0 then
            -- Array-style: {"A", "B", "C"}
            for i, v in ipairs(values) do
                enum[v] = i
                enum._values[i] = v
            end
        else
            -- Table-style: {A = 1, B = 2}
            for k, v in pairs(values) do
                enum[k] = v
                enum._values[v] = k
            end
        end
    end
    
    -- Get name from value
    function enum.nameOf(value)
        return enum._values[value]
    end
    
    -- Check if value is valid
    function enum.isValid(value)
        return enum._values[value] ~= nil
    end
    
    return setmetatable(enum, {
        __index = function(_, key)
            error(format("Invalid enum value '%s' for enum '%s'", tostring(key), name), 2)
        end,
        __newindex = function()
            error(format("Cannot modify enum '%s'", name), 2)
        end,
        __tostring = function()
            return format("<enum '%s'>", name)
        end
    })
end

-- ============================================================================
-- Utility Exports
-- ============================================================================

QELU.utils = {
    deepcopy = deepcopy,
    merge = merge,
    isCallable = isCallable
}

-- ============================================================================
-- Module Export
-- ============================================================================

return QELU
