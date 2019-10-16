==============================
Included lua Library Functions
==============================

In addition to the standard lua libraries, the following are 
additional libraries built into the lua runtime interpreter to 
help with common epics tasks.

The 'asyn' Library - This library contains functions to allow
users to get and set asyn parameters and communicate over an
asyn octet port. It is primarily for use as an easy debugging 
tool for devices that have a command-response style control
scheme, or a framework to allow control of said devices.

:doc:`Full Documentation <asyn-library>`

The 'epics' Library - This library contains functions to get
and set pv values, as well as operating system independent
tasks, like letting a thread sleep.

:doc:`Full Documentation <epics-library>`

The 'iocsh' Library - This library provides the tools to
interact with the existing epics framework for the ioc
shell. It's major use is to allow the lua shell to be
able to call all the same functions as the ioc shell
without having to have to change the code for those
functions.

:doc:`Full Documentation <iocsh-library>`
