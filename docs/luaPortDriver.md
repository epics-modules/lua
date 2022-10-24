---
layout: default
title: luaPortDriver
nav_order: 4
---


# luaPortDriver

Within the lua module support is the lua and IOC shell function luaPortDriver.
This function takes in the name of an asyn port, a lua script, and a string of
macro definitions. An example of a luaPortDriver function call is shown below:

```
   luaPortDriver("EXAMPLE", "exampleDriver.lua", "VAL=10")
```

An asynPortDriver is created with the given asyn port name and the lua script
is run with the defined macro values. Within the script, parameters can be
implemented using the following convention:


```
   param.<param_type> "<NAME>"
```

The parameter type can be any of Int32, Float64, or Octet, each corresponding
to the equivalent asynParamType that shares their name. You can also use
String as a alternative for an Octet definition. None of these definitions
are case sensitive. Name defines the name that the parameter is created with.

The above, basic definition of a parameter only creates a correctly typed
parameter in the port driver. Values can be written or read from the parameter,
but nothing else is actually done. Instead, a slightly more advanced form is
used to bind lua code to reading and writing callbacks.


```
   param.<param_type>.<read/write> "NAME" [[ 
      CODE 
   ]]
```

The same parameter type and name conventions remain, but the definition now
also includes a specifier on whether you are providing a function for the 
reading of a parameter or the writing of a parameter. The code is then lua
code as a multi-line string. 

Implicitly defined within the code is a variable named "self". This is a
lua driver object as defined in the asyn library and represents the luaPortDriver
you are creating. This is useful for being able to read from and write to
other parameters in the driver during execution. As well, for code that 
implements a write callback, you also have the variable "value" that contains
the value to write coming from the asyn callback. For read callbacks, a value 
that is returned by the code will be written out to the value parameter in the 
asyn callback.

Put together, here is a small example of a functional luaPortDriver for a simple
calculation of the length of a hypotenuse:


```
   param.int32 "BASE"
   param.int32 "SIDE"

   param.float64.read "HYPOTENUSE" [[
      return math.sqrt(self["BASE"]^2 + self["SIDE"]^2)
   ]]
```


















