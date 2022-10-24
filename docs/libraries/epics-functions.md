---
layout: default
title: Included Libraries
nav_order: 6
has_children: true
---


# Included lua Library Functions

In addition to the standard lua libraries, the following are 
additional libraries built into the lua runtime interpreter to 
help with common epics tasks.

The 'asyn' Library - [Documentation](asyn-library) -
This library contains functions to allow users to get and set 
asyn parameters and communicate over an asyn octet port. It is 
primarily for use as an easy debugging tool for devices that 
have a command-response style control scheme, or a framework to 
allow control of said devices.

The 'db' Library - [Documentation](database-library) -
This library contains functions to allow users to generate 
database records like one would using the dbLoadDatabase function
to load a db file. Instead, all records can be generated entirely
within lua.

The 'epics' Library - [Documentation](epics-library) - 
This library contains functions to get and set pv values, as well 
as operating system independent tasks, like letting a thread sleep.

The 'iocsh' Library - [Documentation](iocsh-library) - 
This library provides the tools to interact with the existing epics 
framework for the ioc shell. It's major use is to allow the lua shell 
to be able to call all the same functions as the ioc shell without 
having to have to change the code for those functions.
