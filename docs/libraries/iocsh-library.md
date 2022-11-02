---
layout: default
title: iocsh
parent: Included Libraries
nav_order: 4
---


# IOC Shell Functions

### iocsh.<item>
---

```
iocsh. <item> [(arguments…)]
```

Performs an environment check for the given item. First, the user's environment is
checked to see if there are any environment variables that match up with the item's
name. If there aren't, the search then drops back to trying to find a matching
ioc shell function. Only functions that are registered in the iocsh function database
will be found. If there are no matching elements in either of these two locations,
a nil will be returned.

When using the lua shell interpreter, this functionality is embedded into the global
environment. Any attempt to reference a name that hasn't been set as a lua variable
will attempt a search to see if the name references an environment variable or iocsh
function. These are, however, read-only accesses. If you attempt to set a given
item name to a value, all you will do is create a new lua variable with the given
value.

| Parameter | Type | Description |
| - | - | - |
| arguments |  varies | If you are referencing an ioc shell function, these are the arguments that will get sent to the function. Since function references can be passed around, parentheses are necessary to actually invoke the function. |
