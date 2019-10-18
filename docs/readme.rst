================
Lua EPICS Module
================

.. contents:: Contents

The lua EPICS Module is an embedding of the lua language interpreter
into an EPICS IOC. From there, the interpreter is exposed to the user
in the form of the luascript record, which uses the interpreter to
allow for scriptable record support; the lua shell, an addition to
or replacement of the ioc shell; and functions to allow easy use of
lua in other modules.

Currently, the lua module uses lua version 5.3.5. A reference manual
describing the details of the language can be `found here`_.

.. _found here: https://www.lua.org/manual/5.3/

:doc:`lua Module Release Notes <luaReleaseNotes>`

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: lua Module
   
   luaReleaseNotes


luascriptRecord
---------------

The luascript record type provides customizeable record behavior in
much the same way as the calcout or scalcout record. Every time the
record is processeed, lua code is executed and any returned variables
are outputted to an OUT_LINK. The record has both double and string
input links that get exposed as global variables for the code to use.

The record has access to a set of :doc:`library functions <epics-functions>`
that allow it to get and put values from other PV's or asyn port
parameters, sleep, call iocsh functions, send commands to a device,
or you can :doc:`bind your own functions <adding-libraries>` to extend
its functionality.

Full documentation can be found :doc:`here <luascriptRecord>`

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: luascript Record Type
   
   luascriptRecord

lua Shell
---------

The lua shell is an alternate shell to either the vxWorks or ioc
shell environment. It has all the same functionality of those
shells, while also providing the ability to conditionally
execute code, calculate necessary values, construct loops over
code, and define functions within startup scripts.

The lua Shell is able to access all the same :doc:`epics lua libraries <epics-functions>`
as the luascriptRecord, including any functions or libraries that
you :doc:`build yourself <adding-libraries>` into your IOC. The shell
even implicitly loads the iocsh library, which means you can call
iocsh-registered functions in the lua shell exactly like you would 
in the ioc shell. 

:doc:`Further Information <using-lua-shell>`

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Shell Environment
   
   using-lua-shell
   epics-functions
   adding-libraries
