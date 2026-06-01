/*
 * lseqlib.cpp -- Sequencer library C support
 *
 * Provides thread spawning and initHook integration for the seq
 * Lua library. The state machine engine itself is pure Lua (seq.lua).
 *
 * Lua states that have registered sequencer programs are tracked
 * in a global list. After iocInit, a background thread is spawned
 * for each state to run the Lua scheduler (which drives all programs
 * in that state via coroutines).
 *
 * When all programs in a state exit, the thread closes the Lua state
 * unless it was registered as a named state via luaRegisterState.
 */

#include <vector>
#include <string.h>

#include <epicsThread.h>
#include <epicsMutex.h>
#include <epicsGuard.h>
#include <initHooks.h>
#include <epicsExport.h>
#include <errlog.h>

#include "luaEpics.h"


/*
 * =========================================================================
 * Global tracking of Lua states with pending sequencer programs
 * =========================================================================
 */

static std::vector<lua_State*> pending_states;
static epicsMutex pendingMutex;
static bool ioc_running = false;


/*
 * Background thread function: calls seq._scheduler() in the Lua state.
 * When the scheduler returns (all programs exited), closes the state
 * unless it was registered as a named state.
 */
static void seqThreadFunc(void* arg)
{
	lua_State* state = (lua_State*) arg;

	/* Retrieve the scheduler function from the registry */
	lua_getfield(state, LUA_REGISTRYINDEX, "_seq_scheduler");

	if (!lua_isfunction(state, -1))
	{
		errlogPrintf("seq: scheduler function not found in registry\n");
		lua_pop(state, 1);
		return;
	}

	int status = lua_pcall(state, 0, 0, 0);

	if (status)
	{
		errlogPrintf("seq: scheduler error: %s\n", lua_tostring(state, -1));
		lua_pop(state, 1);
	}

	/* Release the sequencer's reference to the Lua state.
	 * If no other references remain, the state is closed. */
	luaStateUnref(state);
}


/*
 * Spawn a background thread for a Lua state's sequencer programs.
 */
static void spawnSeqThread(lua_State* state)
{
	/* Get the program name for the thread name */
	const char* name = "luaSeq";

	lua_getfield(state, LUA_REGISTRYINDEX, "_seq_name");
	if (lua_isstring(state, -1))
	{
		name = lua_tostring(state, -1);
	}

	std::string threadname("seq:");
	threadname += name;

	lua_pop(state, 1);

	epicsThreadCreate(threadname.c_str(),
	                  epicsThreadPriorityLow,
	                  epicsThreadGetStackSize(epicsThreadStackMedium),
	                  seqThreadFunc, state);
}


/*
 * initHook callback: start all pending sequencer programs after iocInit.
 */
static void seqInitHook(initHookState state)
{
	if (state != initHookAfterIocRunning)    { return; }

	epicsGuard<epicsMutex> guard(pendingMutex);

	ioc_running = true;

	for (size_t i = 0; i < pending_states.size(); i++)
	{
		spawnSeqThread(pending_states[i]);
	}

	pending_states.clear();
}


/*
 * =========================================================================
 * Lua-callable functions
 * =========================================================================
 */

/*
 * seq._register(scheduler_func, program_name)
 *
 * Called by seq.register() in Lua. Stores the scheduler function
 * reference and registers this Lua state for post-iocInit thread
 * spawning. If iocInit has already happened, spawns the thread
 * immediately.
 */
static int l_seq_register(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TFUNCTION);
	const char* name = luaL_optstring(state, 2, "unnamed");

	/* Check if this state already has a scheduler registered */
	lua_getfield(state, LUA_REGISTRYINDEX, "_seq_registered");
	if (lua_toboolean(state, -1))
	{
		lua_pop(state, 1);

		/* Already registered -- just add the program to the pending
		 * list in Lua (handled by seq.lua before calling _register).
		 * No need to re-register the state or re-store the scheduler. */
		return 0;
	}
	lua_pop(state, 1);

	/* Store the scheduler function in the registry */
	lua_pushvalue(state, 1);
	lua_setfield(state, LUA_REGISTRYINDEX, "_seq_scheduler");

	/* Store the program name for the thread name */
	lua_pushstring(state, name);
	lua_setfield(state, LUA_REGISTRYINDEX, "_seq_name");

	/* Mark this state as registered */
	lua_pushboolean(state, 1);
	lua_setfield(state, LUA_REGISTRYINDEX, "_seq_registered");

	/* Hold a reference to keep the Lua state alive */
	luaStateRef(state);

	/* Either queue for post-iocInit start or start immediately */
	epicsGuard<epicsMutex> guard(pendingMutex);

	if (ioc_running)
	{
		spawnSeqThread(state);
	}
	else
	{
		pending_states.push_back(state);
	}

	return 0;
}


/*
 * seq._is_after_init()
 *
 * Returns true if iocInit has completed.
 */
static int l_seq_is_after_init(lua_State* state)
{
	lua_pushboolean(state, ioc_running);
	return 1;
}


/*
 * =========================================================================
 * Module registration
 * =========================================================================
 */

/*
 * luaopen_seq_support -- registers the C support functions into the
 * seq module table. Called from seq.lua via require("seq_support").
 *
 * This is NOT the main seq module -- seq.lua is. This provides
 * the C functions that seq.lua calls internally.
 */
int luaopen_seq_support(lua_State* L)
{
	static const luaL_Reg mylib[] = {
		{"_register",      l_seq_register},
		{"_is_after_init", l_seq_is_after_init},
		{NULL, NULL}
	};

	luaL_newlib(L, mylib);
	return 1;
}

static void libseqRegister(void)
{
	luaRegisterLibrary("seq_support", luaopen_seq_support);
	initHookRegister(seqInitHook);
}

extern "C"
{
	epicsExportRegistrar(libseqRegister);
}
