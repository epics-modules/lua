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
describing the detals of the language can be found `here <https://www.lua.org/manual/5.3/>`.

Release Notes
-------------
:doc:`lua Module Release Notes <luaReleaseNotes>`


.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Release Notes
   
   luaReleaseNotes


luascriptRecord
---------------
:doc:`luascriptRecord Documentation <luascriptRecord>`

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: luascript Record Type
   
   luascriptRecord

lua Shell
---------
:doc:`Using the lua Shell <using-lua-shell>`

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Shell Environment
   
   using-lua-shell


Available Libraries
-------------------
:doc:`Epics Library Functions <epics-functions>`

:doc:`Adding your own libraries <adding-libraries>`

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Support Libraries
   
   epics-functions
   adding-libraries
