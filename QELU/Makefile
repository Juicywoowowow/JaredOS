# QELUP - Python Bridge Makefile
# Builds the C extension for Lua-Python interop

# Detect OS
UNAME_S := $(shell uname -s)

# Python detection
PYTHON := $(shell which python3 2>/dev/null || which python 2>/dev/null)
PYTHON_VERSION := $(shell $(PYTHON) -c "import sys; print(sys.version_info[0])" 2>/dev/null)
PYTHON_CONFIG := python$(PYTHON_VERSION)-config

# Check if Python is available
ifeq ($(PYTHON),)
$(error Python not found. Please install Python 2.7+ or Python 3.x)
endif

# Compiler
CC := gcc
CFLAGS := -O2 -fPIC -Wall -Wextra

# Python flags
PYTHON_CFLAGS := $(shell $(PYTHON_CONFIG) --cflags 2>/dev/null)
PYTHON_LDFLAGS := $(shell $(PYTHON_CONFIG) --ldflags --embed 2>/dev/null || $(PYTHON_CONFIG) --ldflags 2>/dev/null)

# Lua flags (use system Lua)
LUA_CFLAGS := $(shell pkg-config --cflags lua5.4 2>/dev/null || pkg-config --cflags lua 2>/dev/null || echo "-I/opt/homebrew/include/lua")
LUA_LDFLAGS := $(shell pkg-config --libs lua5.4 2>/dev/null || pkg-config --libs lua 2>/dev/null || echo "-llua")

# Platform-specific settings
ifeq ($(UNAME_S),Darwin)
    # macOS
    SHARED_FLAG := -bundle -undefined dynamic_lookup
    SO_EXT := so
else ifeq ($(UNAME_S),Linux)
    # Linux
    SHARED_FLAG := -shared
    SO_EXT := so
    CFLAGS += -DLUA_USE_LINUX
else
    # Windows (MinGW)
    SHARED_FLAG := -shared
    SO_EXT := dll
endif

# Output
TARGET := bindings/qelup_core.$(SO_EXT)
SRC := bindings/qelup.c

# Build target
all: check-python $(TARGET)

check-python:
	@echo "Checking Python installation..."
	@echo "Python: $(PYTHON)"
	@echo "Python version: $(PYTHON_VERSION)"
	@$(PYTHON) -c "import sys; print('Python {}.{}.{}'.format(*sys.version_info[:3]))"
	@echo ""

$(TARGET): $(SRC)
	@echo "Building QELUP C extension..."
	@echo "CFLAGS: $(CFLAGS)"
	@echo "PYTHON_CFLAGS: $(PYTHON_CFLAGS)"
	@echo "LUA_CFLAGS: $(LUA_CFLAGS)"
	@echo ""
	$(CC) $(CFLAGS) $(PYTHON_CFLAGS) $(LUA_CFLAGS) \
		$(SHARED_FLAG) -o $@ $< \
		$(PYTHON_LDFLAGS)
	@echo ""
	@echo "✓ Build successful: $(TARGET)"
	@echo ""

clean:
	rm -f $(TARGET)
	@echo "Cleaned build artifacts"

test: $(TARGET)
	@echo "Testing QELUP..."
	lua -e "local core = require('qelup_core'); print('✓ Module loads'); core.initialize(); print('✓ Python initialized'); print(core.version())"

install: $(TARGET)
	@echo "Installing QELUP..."
	@mkdir -p ~/.luarocks/lib/lua/5.4/
	@mkdir -p ~/.luarocks/share/lua/5.4/
	cp $(TARGET) ~/.luarocks/lib/lua/5.4/
	cp qelup.lua ~/.luarocks/share/lua/5.4/
	@echo "✓ Installed to ~/.luarocks/"

.PHONY: all clean test install check-python
