--
-- seq: State machine sequencer library for EPICS IOCs
--
-- Provides a Lua-native alternative to the EPICS State Notation
-- Language (SNL) sequencer. Programs are defined as state machines
-- with transitions driven by PV values, timers, and event flags.
--
-- Programs run in background threads after iocInit, driven by
-- coroutines within a per-state scheduler.
--

local osi = require("osi")
local seq_support = require("seq_support")

local seq = {}

-- List of programs registered in this Lua state
local pending_programs = {}
local scheduler_registered = false


---------------------------------------------------------------------------
-- seq.when(condition) { action=fn, next="state" }
---------------------------------------------------------------------------

local when_mt = {}

when_mt.__call = function(self, tbl)
	if not tbl.next then
		error("seq.when: 'next' field is required")
	end
	self.action = tbl.action
	self.next   = tbl.next
	return self
end

function seq.when(condition)
	return setmetatable({ _type = "when", condition = condition }, when_mt)
end


---------------------------------------------------------------------------
-- seq.delay(seconds)
---------------------------------------------------------------------------

function seq.delay(seconds)
	return { _type = "delay", seconds = seconds }
end


---------------------------------------------------------------------------
-- seq.program(name [, opts])
---------------------------------------------------------------------------

function seq.program(name, opts)
	local prog = {
		name = name,
		states = {},
		_state_order = {},
		poll = (opts and opts.poll) or 0.1,
		_stop = false,
	}

	function prog:state(sname, def)
		local state = {
			entry = def.entry,
			exit  = def.exit,
			options = def.options or {},
			transitions = {},
		}

		-- Collect transitions from the array part
		for _, item in ipairs(def) do
			if type(item) == "table" and item._type == "when" then
				state.transitions[#state.transitions + 1] = item
			end
		end

		self.states[sname] = state
		self._state_order[#self._state_order + 1] = sname
		return self
	end

	function prog:stop()
		self._stop = true
	end

	return prog
end


---------------------------------------------------------------------------
-- Engine: runs a single program as a coroutine
---------------------------------------------------------------------------

local function engine(prog)
	local current = prog._state_order[1]
	local prev = nil
	local entry_time = osi.monotonic()

	while current ~= "exit" and not prog._stop do
		local state_def = prog.states[current]

		if not state_def then
			print("seq [" .. prog.name .. "]: unknown state '"
			      .. current .. "', stopping")
			break
		end

		local opts = state_def.options or {}
		local is_unique = (current ~= prev)

		-- Entry
		if state_def.entry then
			if is_unique or opts.always_enter then
				local ok, err = pcall(state_def.entry)
				if not ok then
					print("seq [" .. prog.name .. "] entry error in '"
					      .. current .. "': " .. tostring(err))
				end
			end
		end

		-- Mark that we've entered this state (so entry doesn't re-run
		-- on subsequent poll cycles within the same state)
		if is_unique then prev = current end

		-- Evaluate transitions in order
		local fired = false

		for _, trans in ipairs(state_def.transitions) do
			local result = false

			if trans.condition == nil then
				-- Unconditional
				result = true
			elseif type(trans.condition) == "function" then
				local ok, val = pcall(trans.condition)
				if ok then
					result = val
				else
					print("seq [" .. prog.name .. "] condition error in '"
					      .. current .. "': " .. tostring(val))
				end
			elseif type(trans.condition) == "table"
			       and trans.condition._type == "delay" then
				result = (osi.monotonic() - entry_time)
				         >= trans.condition.seconds
			end

			if result then
				-- Exit
				if state_def.exit then
					if is_unique or opts.always_exit then
						local ok, err = pcall(state_def.exit)
						if not ok then
							print("seq [" .. prog.name
							      .. "] exit error in '" .. current
							      .. "': " .. tostring(err))
						end
					end
				end

				-- Action
				if trans.action then
					local ok, err = pcall(trans.action)
					if not ok then
						print("seq [" .. prog.name
						      .. "] action error in '" .. current
						      .. "': " .. tostring(err))
					end
				end

				-- Transition
				prev = current
				current = trans.next

				-- Reset timer
				local is_self = (current == prev)
				if not is_self or (opts.always_reset ~= false) then
					entry_time = osi.monotonic()
				end

				fired = true
				break
			end
		end

		-- Yield to scheduler: true = transition fired, false = idle
		coroutine.yield(fired)
	end
end


---------------------------------------------------------------------------
-- Scheduler: runs all programs via coroutines in the background thread
---------------------------------------------------------------------------

function seq._scheduler()
	local coroutines = {}

	for _, prog in ipairs(pending_programs) do
		coroutines[#coroutines + 1] = {
			co = coroutine.create(function() engine(prog) end),
			prog = prog,
		}
	end

	-- Find minimum poll interval
	local min_poll = 0.1
	for _, prog in ipairs(pending_programs) do
		if prog.poll < min_poll then
			min_poll = prog.poll
		end
	end

	while #coroutines > 0 do
		local any_fired = false

		for i = #coroutines, 1, -1 do
			local entry = coroutines[i]
			local ok, fired = coroutine.resume(entry.co)

			if not ok then
				print("seq [" .. entry.prog.name
				      .. "] fatal error: " .. tostring(fired))
				table.remove(coroutines, i)
			elseif coroutine.status(entry.co) == "dead" then
				table.remove(coroutines, i)
			else
				if fired then
					any_fired = true
				end
			end
		end

		if #coroutines > 0 and not any_fired then
			osi.sleep(min_poll)
		end
	end
end


---------------------------------------------------------------------------
-- seq.register(prog)
---------------------------------------------------------------------------

function seq.register(prog)
	if not prog or not prog.states or not prog._state_order then
		error("seq.register: argument must be a seq.program")
	end

	if #prog._state_order == 0 then
		error("seq.register: program '" .. prog.name
		      .. "' has no states defined")
	end

	pending_programs[#pending_programs + 1] = prog

	-- Register the scheduler with the C support layer (once per state)
	if not scheduler_registered then
		seq_support._register(seq._scheduler, prog.name)
		scheduler_registered = true
	else
		-- If iocInit already happened and we're adding another program,
		-- re-register to trigger a new thread spawn
		if seq_support._is_after_init() then
			seq_support._register(seq._scheduler, prog.name)
		end
	end
end


---------------------------------------------------------------------------
-- Documentation
---------------------------------------------------------------------------

seq._doc = {
	["seq"]        = "State machine sequencer library",
	[".program"]   = "program(name [, {poll=seconds}]) -- create a program",
	[".when"]      = "when([condition]) {action=fn, next='state'} -- transition",
	[".delay"]     = "delay(seconds) -- delay condition for transitions",
	[".register"]  = "register(prog) -- register program for execution after iocInit",
}


return seq
