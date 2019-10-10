# Dynamic Libraries

If you are on a system that supports dynamic libraries, you can add new
functions into lua using the *require* function. Compile a dynamic library
with the function int luaopen_xxxx(lua_State* L), where xxxx matches the
name of the library file.

Then, you'll need to tell lua the location of the library in question.
This is accomplished by either setting the environment variable LUA_CPATH
or LUA_CPATH_5_3 before lua is invoked, or the global variable package.cpath
if you are already in lua.

These variables are not a set of search directories, like a normal path, however.
Instead, they are a set of templates that just replace a wildcard character with
the library name you are looking for and check to see if that file exists. So,
a package.cpath that is set as

    "./?.so;/usr/local/?/init.so"

would try to search for the files ./foo.so and /usr/local/foo/init.so when you call
require("foo"). The first such file that is found gets dynamically loaded and then
the function luaopen_xxxx is attempted to be called, in this instance luaopen_foo
(**NOTE:** If you compile the library with epics, the resulting file will be called
libxxxx.so or libxxxx.dll, so you should include the lib part in the search path).

That luaopen function is then where you would bind your functions into the lua_state.
Lua provides the luaL_newlib function to make this easy. You just provide it with a
list of pairs of function names and function pointers like so:

    static int l_bar( lua_State *L )
    {
        lua_pushstring(L, "Hello, World");
        return 1;
    }

    int luaopen_foo( lua_State *L )
    {
        static const luaL_Reg foo[] = {
            { "bar", l_bar },
            { NULL, NULL } /* Sentinel item */
        };

        luaL_newlib( L, foo );
        return 1;
    }

Then you can use the above library like so:

    foo = require("foo")

    print(foo.bar())

Note that luaopen_foo just returns a table with all the functions indexed to their
correct names, which is why the return value of the require function then needs to
be assigned to a variable name.

# Static Libraries

Using static libraries is very similar to using dynamic ones. The major difference
is that you will have to tell lua what the name of the function that opens your
library is rather than giving it a path to find the library. In the luaEpics.h
file, there is a function, *luaRegisterLibrary*. The function takes in a name for
the library and a function with the same signature as the luaopen_xxxx that we used
before. It would be good practice to just continue to use the same luaopen_xxxx
naming convention.

The recommended way to get the libraries to be registered correctly for the shell is to
use the dbd's registrar function.

**foo.cpp**

    static void fooRegister(void)
    {
        luaRegisterLibrary("foo", luaopen_foo);
    }

    extern "C"
    {
        epicsExportRegistrar(fooRegister);
    }

**foo.dbd**

    registrar(fooRegister)

Then when you load the dbd file into your IOC, lua is given the link between "foo" and
the luaopen_foo function, so when you use require to try to load the "foo" library,
it will call the open function registered to that name.

Lua will use the built-in means of searching for libraries first, before looking through
the libraries registered with *luaRegisterLibrary* so if you have a lua file or shared
library in your path or cpath, it will load that rather than the static library.


# Adding Individual Functions

You are also able to register individual functions into the global scope,
though it isn't necessarily recommended due to possible collisions with
existing functions.

To do this, use the *luaRegisterFunction* command pretty much just like you
would use the *luaRegisterLibrary* call. Instead of a library name, it takes
the name you want the function to have. And instead of taking a function that
registers other functions, it just takes the functions directly.

**Example:**

    static int l_bar( lua_State *L )
    {
        lua_pushstring(L, "Hello, World");
        return 1;
    }

    static void testRegister(void)
    {
        luaRegisterFunction("bar", l_bar);
    }

Then, you can call the function bar in the lua shell. Since these functions are
not part of any library, you don't need to use *require* to load them.
