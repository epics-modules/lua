---
layout: default
title: epics
parent: Included Libraries
nav_order: 3
---

# Epics Library Documentation
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

### epics.get
---

```
epics.get (PV[, timeout])
```

Calls ca_get to retrieve the value of a PV accessible by the host.

Returns the value of the PV given or Nil if the PV cannot be reached.

| Parameter | Type | Description |
| - | - | - |
| PV       |   string | The name of the PV to request.
| timeout  |   number | Amount of seconds to search for pv before giving a timeout, default is 1.0 (can be fractional).

<br>

### epics.put
---

```
epics.put (PV, value)
```

Calls ca_put to set the value of a PV accessible by the host.

| Parameter | Type | Description |
| - | - | - |
| PV       |   string | The name of the PV to request.
| value    |   varies | The new value you want to set the PV to. The type of this parameter should match with the dbtype of the PV requested.

<br>

### epics.sleep
---

```
epics.sleep (seconds)
```

Tells the epics thread running the lua script to sleep for a given time.

| Parameter | Type | Description |
| - | - | - |
| seconds   |  number | Amount of seconds to sleep for (can be fractional). |

<br>

### epics.pv
---

```
epics.pv (PV)
```

Returns a table representing a PV object. Index accesses can be used to retrive or
change record fields. These changes are completed through ca_get or ca_put.

| Parameter | Type | Description |
| - | - | - |
| PV   |  string | The name of the PV to request. |
