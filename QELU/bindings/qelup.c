/*
    QELUP - QELU Python Bridge (C Extension)
    
    Provides Lua bindings to Python interpreter using Python C API.
    Supports Python 2.7+ and Python 3.x
    
    @author QELU Contributors
    @license MIT
    @version 1.0.0
*/

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <string.h>
#include <stdlib.h>

/* Compatibility macros for Python 2/3 */
#if PY_MAJOR_VERSION >= 3
    #define QELUP_PY3
    #define PyInt_Check PyLong_Check
    #define PyInt_AsLong PyLong_AsLong
    #define PyInt_FromLong PyLong_FromLong
    #define PyString_Check PyUnicode_Check
    #define PyString_AsString PyUnicode_AsUTF8
    #define PyString_FromString PyUnicode_FromString
#endif

/* Metatable names */
#define QELUP_PYOBJECT_MT "qelup.pyobject"

/* Python object wrapper for Lua */
typedef struct {
    PyObject *obj;
} qelup_PyObject;

/* Forward declarations */
static PyObject* lua_to_python(lua_State *L, int index);
static void python_to_lua(lua_State *L, PyObject *obj);
static int handle_python_exception(lua_State *L);

/* ========================================================================== */
/* Helper Functions */
/* ========================================================================== */

/* Check if Python is initialized */
static int check_initialized(lua_State *L) {
    if (!Py_IsInitialized()) {
        return luaL_error(L, "Python interpreter not initialized");
    }
    return 0;
}

/* Create a new Python object wrapper */
static qelup_PyObject* qelup_newpyobject(lua_State *L, PyObject *obj) {
    qelup_PyObject *udata = (qelup_PyObject*)lua_newuserdata(L, sizeof(qelup_PyObject));
    udata->obj = obj;
    Py_XINCREF(obj);
    
    luaL_getmetatable(L, QELUP_PYOBJECT_MT);
    lua_setmetatable(L, -2);
    
    return udata;
}

/* Get Python object from Lua userdata */
static PyObject* qelup_checkpyobject(lua_State *L, int index) {
    qelup_PyObject *udata = (qelup_PyObject*)luaL_checkudata(L, index, QELUP_PYOBJECT_MT);
    return udata->obj;
}

/* ========================================================================== */
/* Type Conversion: Lua -> Python */
/* ========================================================================== */

static PyObject* lua_to_python(lua_State *L, int index) {
    int type = lua_type(L, index);
    
    switch (type) {
        case LUA_TNIL:
            Py_RETURN_NONE;
        
        case LUA_TBOOLEAN:
            return PyBool_FromLong(lua_toboolean(L, index));
        
        case LUA_TNUMBER:
            #if LUA_VERSION_NUM >= 503
            if (lua_isinteger(L, index)) {
                return PyInt_FromLong(lua_tointeger(L, index));
            }
            #endif
            return PyFloat_FromDouble(lua_tonumber(L, index));
        
        case LUA_TSTRING: {
            const char *str = lua_tostring(L, index);
            return PyString_FromString(str);
        }
        
        case LUA_TTABLE: {
            /* Check if it's an array or dictionary */
            int is_array = 1;
            int max_index = 0;
            int count = 0;
            
            lua_pushnil(L);
            while (lua_next(L, index < 0 ? index - 1 : index) != 0) {
                if (lua_type(L, -2) != LUA_TNUMBER) {
                    is_array = 0;
                    lua_pop(L, 2);
                    break;
                }
                int idx = lua_tointeger(L, -2);
                if (idx > max_index) max_index = idx;
                count++;
                lua_pop(L, 1);
            }
            
            if (is_array && max_index == count) {
                /* Convert to Python list */
                PyObject *list = PyList_New(count);
                for (int i = 1; i <= count; i++) {
                    lua_rawgeti(L, index, i);
                    PyObject *item = lua_to_python(L, -1);
                    PyList_SetItem(list, i - 1, item);
                    lua_pop(L, 1);
                }
                return list;
            } else {
                /* Convert to Python dict */
                PyObject *dict = PyDict_New();
                lua_pushnil(L);
                while (lua_next(L, index < 0 ? index - 1 : index) != 0) {
                    PyObject *key = lua_to_python(L, -2);
                    PyObject *value = lua_to_python(L, -1);
                    PyDict_SetItem(dict, key, value);
                    Py_DECREF(key);
                    Py_DECREF(value);
                    lua_pop(L, 1);
                }
                return dict;
            }
        }
        
        case LUA_TUSERDATA: {
            /* Check if it's a Python object wrapper */
            if (luaL_testudata(L, index, QELUP_PYOBJECT_MT)) {
                PyObject *obj = qelup_checkpyobject(L, index);
                Py_INCREF(obj);
                return obj;
            }
            /* Fall through to default */
        }
        
        default:
            Py_RETURN_NONE;
    }
}

/* ========================================================================== */
/* Type Conversion: Python -> Lua */
/* ========================================================================== */

static void python_to_lua(lua_State *L, PyObject *obj) {
    if (obj == NULL || obj == Py_None) {
        lua_pushnil(L);
    }
    else if (PyBool_Check(obj)) {
        lua_pushboolean(L, obj == Py_True);
    }
    else if (PyInt_Check(obj)) {
        lua_pushinteger(L, PyInt_AsLong(obj));
    }
    else if (PyFloat_Check(obj)) {
        lua_pushnumber(L, PyFloat_AsDouble(obj));
    }
    else if (PyString_Check(obj)) {
        const char *str = PyString_AsString(obj);
        lua_pushstring(L, str);
    }
    else if (PyList_Check(obj)) {
        Py_ssize_t size = PyList_Size(obj);
        lua_createtable(L, size, 0);
        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject *item = PyList_GetItem(obj, i);
            python_to_lua(L, item);
            lua_rawseti(L, -2, i + 1);
        }
    }
    else if (PyTuple_Check(obj)) {
        Py_ssize_t size = PyTuple_Size(obj);
        lua_createtable(L, size, 0);
        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject *item = PyTuple_GetItem(obj, i);
            python_to_lua(L, item);
            lua_rawseti(L, -2, i + 1);
        }
    }
    else if (PyDict_Check(obj)) {
        lua_newtable(L);
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        
        while (PyDict_Next(obj, &pos, &key, &value)) {
            python_to_lua(L, key);
            python_to_lua(L, value);
            lua_settable(L, -3);
        }
    }
    else {
        /* Wrap as Python object */
        qelup_newpyobject(L, obj);
    }
}

/* ========================================================================== */
/* Error Handling */
/* ========================================================================== */

static int handle_python_exception(lua_State *L) {
    if (!PyErr_Occurred()) {
        return 0;
    }
    
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
    
    const char *error_msg = "Unknown Python error";
    
    if (pvalue != NULL) {
        PyObject *str = PyObject_Str(pvalue);
        if (str != NULL) {
            error_msg = PyString_AsString(str);
            lua_pushstring(L, error_msg);
            Py_DECREF(str);
        }
    }
    
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
    
    return lua_error(L);
}

/* ========================================================================== */
/* Core Functions */
/* ========================================================================== */

/* Initialize Python interpreter */
static int qelup_initialize(lua_State *L) {
    if (Py_IsInitialized()) {
        lua_pushboolean(L, 1);
        return 1;
    }
    
    Py_Initialize();
    
    if (!Py_IsInitialized()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Failed to initialize Python interpreter");
        return 2;
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

/* Finalize Python interpreter */
static int qelup_finalize(lua_State *L) {
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
    return 0;
}

/* Import Python module */
static int qelup_import(lua_State *L) {
    check_initialized(L);
    
    const char *module_name = luaL_checkstring(L, 1);
    
    PyObject *module = PyImport_ImportModule(module_name);
    
    if (module == NULL) {
        return handle_python_exception(L);
    }
    
    qelup_newpyobject(L, module);
    Py_DECREF(module);
    
    return 1;
}

/* Execute Python code */
static int qelup_exec(lua_State *L) {
    check_initialized(L);
    
    const char *code = luaL_checkstring(L, 1);
    
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *global_dict = PyModule_GetDict(main_module);
    
    PyObject *result = PyRun_String(code, Py_file_input, global_dict, global_dict);
    
    if (result == NULL) {
        return handle_python_exception(L);
    }
    
    Py_DECREF(result);
    lua_pushboolean(L, 1);
    return 1;
}

/* Evaluate Python expression */
static int qelup_eval(lua_State *L) {
    check_initialized(L);
    
    const char *expr = luaL_checkstring(L, 1);
    
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *global_dict = PyModule_GetDict(main_module);
    
    PyObject *result = PyRun_String(expr, Py_eval_input, global_dict, global_dict);
    
    if (result == NULL) {
        return handle_python_exception(L);
    }
    
    python_to_lua(L, result);
    Py_DECREF(result);
    
    return 1;
}

/* Get Python version */
static int qelup_version(lua_State *L) {
    lua_pushstring(L, Py_GetVersion());
    lua_pushinteger(L, PY_MAJOR_VERSION);
    lua_pushinteger(L, PY_MINOR_VERSION);
    return 3;
}

/* ========================================================================== */
/* Python Object Methods */
/* ========================================================================== */

/* Call Python object */
static int pyobject_call(lua_State *L) {
    PyObject *obj = qelup_checkpyobject(L, 1);
    int nargs = lua_gettop(L) - 1;
    
    PyObject *args = PyTuple_New(nargs);
    for (int i = 0; i < nargs; i++) {
        PyObject *arg = lua_to_python(L, i + 2);
        PyTuple_SetItem(args, i, arg);
    }
    
    PyObject *result = PyObject_CallObject(obj, args);
    Py_DECREF(args);
    
    if (result == NULL) {
        return handle_python_exception(L);
    }
    
    python_to_lua(L, result);
    Py_DECREF(result);
    
    return 1;
}

/* Get attribute */
static int pyobject_index(lua_State *L) {
    PyObject *obj = qelup_checkpyobject(L, 1);
    const char *key = luaL_checkstring(L, 2);
    
    PyObject *attr = PyObject_GetAttrString(obj, key);
    
    if (attr == NULL) {
        PyErr_Clear();
        lua_pushnil(L);
        return 1;
    }
    
    python_to_lua(L, attr);
    Py_DECREF(attr);
    
    return 1;
}

/* Set attribute */
static int pyobject_newindex(lua_State *L) {
    PyObject *obj = qelup_checkpyobject(L, 1);
    const char *key = luaL_checkstring(L, 2);
    PyObject *value = lua_to_python(L, 3);
    
    int result = PyObject_SetAttrString(obj, key, value);
    Py_DECREF(value);
    
    if (result == -1) {
        return handle_python_exception(L);
    }
    
    return 0;
}

/* Garbage collection */
static int pyobject_gc(lua_State *L) {
    qelup_PyObject *udata = (qelup_PyObject*)luaL_checkudata(L, 1, QELUP_PYOBJECT_MT);
    if (udata->obj != NULL) {
        Py_DECREF(udata->obj);
        udata->obj = NULL;
    }
    return 0;
}

/* String representation */
static int pyobject_tostring(lua_State *L) {
    PyObject *obj = qelup_checkpyobject(L, 1);
    
    PyObject *str = PyObject_Str(obj);
    if (str == NULL) {
        lua_pushstring(L, "<Python object>");
        return 1;
    }
    
    const char *cstr = PyString_AsString(str);
    lua_pushstring(L, cstr);
    Py_DECREF(str);
    
    return 1;
}

/* ========================================================================== */
/* Module Registration */
/* ========================================================================== */

static const luaL_Reg qelup_funcs[] = {
    {"initialize", qelup_initialize},
    {"finalize", qelup_finalize},
    {"import", qelup_import},
    {"exec", qelup_exec},
    {"eval", qelup_eval},
    {"version", qelup_version},
    {NULL, NULL}
};

static const luaL_Reg pyobject_methods[] = {
    {"__call", pyobject_call},
    {"__index", pyobject_index},
    {"__newindex", pyobject_newindex},
    {"__gc", pyobject_gc},
    {"__tostring", pyobject_tostring},
    {NULL, NULL}
};

int luaopen_qelup_core(lua_State *L) {
    /* Create metatable for Python objects */
    luaL_newmetatable(L, QELUP_PYOBJECT_MT);
    
    #if LUA_VERSION_NUM >= 502
    luaL_setfuncs(L, pyobject_methods, 0);
    #else
    luaL_register(L, NULL, pyobject_methods);
    #endif
    
    lua_pop(L, 1);
    
    /* Create module table */
    #if LUA_VERSION_NUM >= 502
    luaL_newlib(L, qelup_funcs);
    #else
    luaL_register(L, "qelup.core", qelup_funcs);
    #endif
    
    return 1;
}
