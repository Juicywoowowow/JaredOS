#!/usr/bin/env luajit
--[[
    QELU Library Test Suite
    Uses QELUTest framework to test QELU OOP library
    
    Run with: luajit test.lua
]]

-- Load both libraries
local QELU = require("qelu")
local QELUTest = require("qelutest")

-- Globalize test functions for cleaner syntax
QELUTest.globalize()

-- ============================================================================
-- QELU OOP Library Tests
-- ============================================================================

describe("QELU Library", function()
    
    describe("Version Info", function()
        it("should have version information", function()
            expect(QELU._VERSION):toBeString()
            expect(QELU._DESCRIPTION):toBeString()
            expect(QELU._LICENSE):toBe("MIT")
        end)
    end)
    
    -- ========================================================================
    -- Basic Classes
    -- ========================================================================
    
    describe("Basic Classes", function()
        
        it("should create a simple class", function()
            local MyClass = QELU.class("MyClass")
            expect(QELU.isClass(MyClass)):toBeTruthy()
            expect(QELU.className(MyClass)):toBe("MyClass")
        end)
        
        it("should instantiate with constructor", function()
            local Person = QELU.class("Person")
            function Person:init(name, age)
                self.name = name
                self.age = age
            end
            
            local john = Person("John", 30)
            
            expect(QELU.isInstance(john)):toBeTruthy()
            expect(john.name):toBe("John")
            expect(john.age):toBe(30)
        end)
        
        it("should support instance methods with chaining", function()
            local Calculator = QELU.class("Calculator")
            
            function Calculator:init(value)
                self.value = value or 0
            end
            
            function Calculator:add(n)
                self.value = self.value + n
                return self
            end
            
            function Calculator:multiply(n)
                self.value = self.value * n
                return self
            end
            
            function Calculator:getResult()
                return self.value
            end
            
            local calc = Calculator(10)
            local result = calc:add(5):multiply(2):getResult()
            
            expect(result):toBe(30)
        end)
    end)
    
    -- ========================================================================
    -- Inheritance
    -- ========================================================================
    
    describe("Inheritance", function()
        
        it("should support single inheritance", function()
            local Animal = QELU.class("Animal")
            function Animal:init(name)
                self.name = name
            end
            function Animal:speak()
                return "..."
            end
            
            local Dog = QELU.class("Dog"):extends(Animal)
            function Dog:speak()
                return "Woof!"
            end
            
            local rex = Dog("Rex")
            
            expect(rex.name):toBe("Rex")
            expect(rex:speak()):toBe("Woof!")
        end)
        
        it("should support super calls", function()
            local Parent = QELU.class("Parent")
            function Parent:init(x)
                self.x = x
            end
            function Parent:getValue()
                return self.x
            end
            
            local Child = QELU.class("Child"):extends(Parent)
            function Child:init(x, y)
                QELU.super(self):init(x)
                self.y = y
            end
            function Child:getValue()
                return QELU.super(self):getValue() + self.y
            end
            
            local c = Child(10, 5)
            expect(c:getValue()):toBe(15)
        end)
        
        it("should support multiple inheritance", function()
            local A = QELU.class("A")
            function A:methodA() return "A" end
            
            local B = QELU.class("B")
            function B:methodB() return "B" end
            
            local C = QELU.class("C"):extends(A, B)
            
            local obj = C()
            expect(obj:methodA()):toBe("A")
            expect(obj:methodB()):toBe("B")
        end)
        
        it("should correctly check instanceOf", function()
            local Animal = QELU.class("Animal")
            local Dog = QELU.class("Dog"):extends(Animal)
            local Cat = QELU.class("Cat"):extends(Animal)
            
            local dog = Dog()
            
            expect(QELU.instanceOf(dog, Dog)):toBeTruthy()
            expect(QELU.instanceOf(dog, Animal)):toBeTruthy()
            expect(QELU.instanceOf(dog, Cat)):toBeFalsy()
        end)
    end)
    
    -- ========================================================================
    -- Private Members
    -- ========================================================================
    
    describe("Private Members", function()
        
        it("should keep private members private", function()
            local Secret = QELU.class("Secret")
            
            function Secret:init(value)
                self.__secret = value
            end
            
            function Secret:getSecret()
                return self.__secret
            end
            
            local s = Secret(42)
            
            expect(s:getSecret()):toBe(42)
            expect(rawget(s, "__secret")):toBeNil()
        end)
    end)
    
    -- ========================================================================
    -- Static Members
    -- ========================================================================
    
    describe("Static Members", function()
        
        it("should support static properties and methods", function()
            local Counter = QELU.class("Counter")
            
            Counter.static({
                count = 0,
                increment = function()
                    Counter.count = Counter.count + 1
                end
            })
            
            function Counter:init()
                Counter.increment()
            end
            
            Counter()
            Counter()
            Counter()
            
            expect(Counter.count):toBe(3)
        end)
    end)
    
    -- ========================================================================
    -- Interfaces
    -- ========================================================================
    
    describe("Interfaces", function()
        
        it("should validate interface implementation", function()
            local IDrawable = QELU.interface("IDrawable", {
                draw = true,
                getPosition = true
            })
            
            local Sprite = QELU.class("Sprite"):implements(IDrawable)
            function Sprite:draw() end
            function Sprite:getPosition() return 0, 0 end
            
            expect(function()
                local s = Sprite()
            end):toNotThrow()
        end)
        
        it("should reject incomplete interface implementation", function()
            local ISerializable = QELU.interface("ISerializable", {
                serialize = true,
                deserialize = true
            })
            
            local BadClass = QELU.class("BadClass"):implements(ISerializable)
            function BadClass:serialize() end
            -- Missing deserialize!
            
            expect(function()
                BadClass()
            end):toThrow()
        end)
    end)
    
    -- ========================================================================
    -- Mixins
    -- ========================================================================
    
    describe("Mixins", function()
        
        it("should include mixin methods", function()
            local Loggable = QELU.mixin({
                log = function(self, msg)
                    return "[LOG] " .. msg
                end
            })
            
            local Service = QELU.class("Service"):include(Loggable)
            local svc = Service()
            
            expect(svc:log("test")):toBe("[LOG] test")
        end)
        
        it("should support EventEmitter mixin", function()
            local Button = QELU.class("Button"):include(QELU.EventEmitter)
            
            local clicked = false
            local btn = Button()
            
            btn:on("click", function()
                clicked = true
            end)
            
            btn:emit("click")
            expect(clicked):toBeTruthy()
        end)
    end)
    
    -- ========================================================================
    -- Abstract Classes
    -- ========================================================================
    
    describe("Abstract Classes", function()
        
        it("should prevent instantiation of abstract classes", function()
            local AbstractShape = QELU.class("AbstractShape", { abstract = true })
            
            expect(function()
                AbstractShape()
            end):toThrow("abstract")
        end)
        
        it("should require abstract method implementation", function()
            local Shape = QELU.class("Shape")
            Shape.area = QELU.abstract("area")
            
            local Circle = QELU.class("Circle"):extends(Shape)
            -- Not implementing area!
            
            expect(function()
                Circle()
            end):toThrow("abstract")
        end)
        
        it("should allow properly implemented abstract methods", function()
            local Shape = QELU.class("Shape")
            Shape.area = QELU.abstract("area")
            
            local Square = QELU.class("Square"):extends(Shape)
            function Square:init(side)
                self.side = side
            end
            function Square:area()
                return self.side * self.side
            end
            
            local sq = Square(5)
            expect(sq:area()):toBe(25)
        end)
    end)
    
    -- ========================================================================
    -- Singletons
    -- ========================================================================
    
    describe("Singletons", function()
        
        it("should return the same instance", function()
            local Config = QELU.singleton("Config")
            function Config:init()
                self.value = math.random(10000)
            end
            
            local a = Config()
            local b = Config()
            
            expect(a):toBe(b)
            expect(a.value):toBe(b.value)
        end)
    end)
    
    -- ========================================================================
    -- Enums
    -- ========================================================================
    
    describe("Enums", function()
        
        it("should support array-style enums", function()
            local Color = QELU.enum("Color", { "RED", "GREEN", "BLUE" })
            
            expect(Color.RED):toBe(1)
            expect(Color.GREEN):toBe(2)
            expect(Color.BLUE):toBe(3)
            expect(Color.nameOf(1)):toBe("RED")
            expect(Color.isValid(2)):toBeTruthy()
        end)
        
        it("should support table-style enums", function()
            local Status = QELU.enum("Status", {
                PENDING = 0,
                ACTIVE = 1,
                DONE = 2
            })
            
            expect(Status.PENDING):toBe(0)
            expect(Status.ACTIVE):toBe(1)
            expect(Status.nameOf(2)):toBe("DONE")
        end)
    end)
    
    -- ========================================================================
    -- Utilities
    -- ========================================================================
    
    describe("Utilities", function()
        
        it("should deep copy tables", function()
            local original = { a = 1, b = { c = 2 } }
            local copy = QELU.utils.deepcopy(original)
            
            copy.b.c = 99
            
            expect(original.b.c):toBe(2)
            expect(copy.b.c):toBe(99)
        end)
        
        it("should get methods of a class", function()
            local Foo = QELU.class("Foo")
            function Foo:bar() end
            function Foo:baz() end
            
            local methods = QELU.getMethods(Foo)
            
            expect(methods.bar):toBeDefined()
            expect(methods.baz):toBeDefined()
        end)
    end)
end)

-- ============================================================================
-- QELUTest Framework Self-Tests
-- ============================================================================

describe("QELUTest Framework", function()
    
    describe("Type Matchers", function()
        
        it("should match string types", function()
            expect("hello"):toBeString()
        end)
        
        it("should match number types", function()
            expect(42):toBeNumber()
        end)
        
        it("should match boolean types", function()
            expect(true):toBeBoolean()
            expect(false):toBeBoolean()
        end)
        
        it("should match table types", function()
            expect({}):toBeTable()
        end)
        
        it("should match function types", function()
            expect(function() end):toBeFunction()
        end)
    end)
    
    describe("Number Matchers", function()
        
        it("should compare numbers correctly", function()
            expect(10):toBeGreaterThan(5)
            expect(10):toBeGreaterThanOrEqual(10)
            expect(5):toBeLessThan(10)
            expect(5):toBeLessThanOrEqual(5)
        end)
        
        it("should check number ranges", function()
            expect(5):toBeInRange(1, 10)
        end)
        
        it("should check close values", function()
            expect(0.1 + 0.2):toBeCloseTo(0.3, 5)
        end)
    end)
    
    describe("String Matchers", function()
        
        it("should check string contents", function()
            expect("hello world"):toContain("world")
            expect("hello world"):toMatch("^hello")
            expect("hello world"):toStartWith("hello")
            expect("hello world"):toEndWith("world")
        end)
        
        it("should check string length", function()
            expect("hello"):toHaveLength(5)
        end)
    end)
    
    describe("Table Matchers", function()
        
        it("should check table contents", function()
            local t = { a = 1, b = 2, c = 3 }
            
            expect(t):toContainKey("a")
            expect(t):toContainKeys("a", "b")
            expect(t):toContainValue(2)
        end)
        
        it("should check table subsets", function()
            local t = { a = 1, b = { c = 2, d = 3 } }
            
            expect(t):toContainSubset({ a = 1 })
            expect(t):toContainSubset({ b = { c = 2 } })
        end)
        
        it("should check empty tables", function()
            expect({}):toBeEmpty()
            expect(""):toBeEmpty()
        end)
        
        it("should identify arrays", function()
            expect({1, 2, 3}):toBeArray()
            expect({a = 1}):never():toBeArray()
        end)
    end)
    
    describe("Error Matchers", function()
        
        it("should catch thrown errors", function()
            expect(function()
                error("test error")
            end):toThrow()
        end)
        
        it("should match error messages", function()
            expect(function()
                error("specific error message")
            end):toThrow("specific")
        end)
        
        it("should verify no error thrown", function()
            expect(function()
                local x = 1 + 1
            end):toNotThrow()
        end)
    end)
    
    describe("Negation", function()
        
        it("should support negated matchers", function()
            expect(5):never():toBe(10)
            expect("hello"):Not():toContain("xyz")
            expect(nil):never():toBeDefined()
        end)
    end)
    
    describe("Spy System", function()
        
        it("should track function calls", function()
            local s = spy(function(x) return x * 2 end)
            
            s.fn(5)
            s.fn(10)
            
            expect(s:wasCalled()):toBeTruthy()
            expect(s:wasCalledTimes(2)):toBeTruthy()
            expect(s:wasCalledWith(5)):toBeTruthy()
        end)
        
        it("should record call arguments", function()
            local s = spy()
            
            s.fn("a", "b", "c")
            
            local call = s:getLastCall()
            expect(call.args):toEqual({"a", "b", "c"})
        end)
    end)
    
    describe("Hooks", function()
        local hookOrder = {}
        
        beforeAll(function()
            table.insert(hookOrder, "beforeAll")
        end)
        
        beforeEach(function()
            table.insert(hookOrder, "beforeEach")
        end)
        
        afterEach(function()
            table.insert(hookOrder, "afterEach")
        end)
        
        afterAll(function()
            table.insert(hookOrder, "afterAll")
        end)
        
        it("should run hooks in order", function()
            expect(hookOrder):toContain("beforeAll")
            expect(hookOrder):toContain("beforeEach")
        end)
    end)
end)

-- ============================================================================
-- Run All Tests
-- ============================================================================

local results = QELUTest.run({
    colorOutput = true,
    verbose = true,
    showTraceback = false
})

-- Exit with appropriate code
os.exit(results.failed > 0 and 1 or 0)
