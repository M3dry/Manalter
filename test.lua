---@diagnostic disable: unused-local, unused-function

ffi.cdef([[
    typedef struct { float x, y, z; } Vector3;
    typedef struct { unsigned char r, g, b, a; } Color;

    Vector3* get_pos_ptr(void*);
    Vector3* get_velocity_ptr(void*);
    Vector3* get_acceleration_ptr(void*);
    Color* get_color_ptr(void*);
    bool* get_alive_ptr(void*);
    float* get_lifetime_ptr(void*);
    float* get_size_ptr(void*);

    uint64_t get_max_size(void*);
    uint64_t get_alive_count(void*);

    void kill_particle(void*, uint64_t);
    void wake_particle(void*, uint64_t);
    void swap_particle(void*, uint64_t, uint64_t);
]])
ffi.metatype("Vector3", {
	__index = {
		length = function(v)
			return math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
		end,
		normalize = function(v)
			return Vec3Scale(v, 1 / v:length())
		end,
		negate = function(v)
			return Vec3Scale(v, -1)
		end,
	},
})

local function _pos(p)
	return ffi.C.get_pos_ptr(ffi.cast("void*", p))
end

local function _velocity(p)
	return ffi.C.get_velocity_ptr(ffi.cast("void*", p))
end

local function _acceleration(p)
	return ffi.C.get_acceleration_ptr(ffi.cast("void*", p))
end

local function _color(p)
	return ffi.C.get_color_ptr(ffi.cast("void*", p))
end

local function _alive(p)
	return ffi.C.get_alive_ptr(ffi.cast("void*", p))
end

local function _lifetime(p)
	return ffi.C.get_lifetime_ptr(ffi.cast("void*", p))
end

local function _size(p)
	return ffi.C.get_size_ptr(ffi.cast("void*", p))
end

local function _max_size(p)
	return tonumber(ffi.C.get_max_size(ffi.cast("void*", p)))
end

local function _alive_count(p)
	return tonumber(ffi.C.get_alive_count(ffi.cast("void*", p)))
end

local function _kill(p, ix)
	ffi.C.kill_particle(ffi.cast("void*", p), ix)
end

local function _wake(p, ix)
	ffi.C.wake_particle(ffi.cast("void*", p), ix)
end

local function _swap(p, a, b)
	ffi.C.swap_particle(ffi.cast("void*", p), a, b)
end

local function get_particles(p)
	return {
		pos = _pos(p),
		velocity = _velocity(p),
		acceleration = _acceleration(p),
		color = _color(p),
		alive = _alive(p),
		lifetime = _lifetime(p),
		size = _size(p),
		max_size = _max_size(p),
		alive_count = function()
			return _alive_count(p)
		end,
		kill = function(ix)
			_kill(p, ix)
		end,
		wake = function(ix)
			_wake(p, ix)
		end,
		swap = function(a, b)
			_swap(p, a, b)
		end,
	}
end

function Vec3Add(v1, v2)
	return ffi.new("Vector3", v1.x + v2.x, v1.y + v2.y, v1.z + v2.z)
end

function Vec3Subtract(v1, v2)
	return ffi.new("Vector3", v1.x - v2.x, v1.y - v2.y, v1.z - v2.z)
end

function Vec3Scale(vec, scalar)
	return ffi.new("Vector3", vec.x * scalar, vec.y * scalar, vec.z * scalar)
end

function Vec3Dot(v1, v2)
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z
end

function FloatRandomDist(min, max)
	return function()
		return min + math.random() * (max - min)
	end
end

function Clamp(min, max, num)
	if num < min then
		return min
	elseif num > max then
		return max
	end

	return num
end

local function ColorLerp(start_color, end_color, factor)
	local color = ffi.new("Color", 0, 0, 0, 0)

	factor = Clamp(0, 1, factor)

	color.r = (1.0 - factor) * start_color.r + factor * end_color.r
	color.g = (1.0 - factor) * start_color.g + factor * end_color.g
	color.b = (1.0 - factor) * start_color.b + factor * end_color.b
	color.a = (1.0 - factor) * start_color.a + factor * end_color.a

	return color
end

local function OnCircleGen(center, min_rad, max_rad)
	return function(p, dt, start_ix, end_ix)
		local angle_dist = FloatRandomDist(0, 2 * math.pi)
		local radius_dist = FloatRandomDist(min_rad, max_rad)

		for i = start_ix, end_ix - 1, 1 do
			local angle = angle_dist()
			local radius = radius_dist()

			p.pos[i].x = center.x + radius * math.cos(angle)
			p.pos[i].y = center.y
			p.pos[i].z = center.z + radius * math.sin(angle)
		end
	end
end

local system = new_system(30000, Vector3.new(0, 0.7, 0))

local circle_gen = OnCircleGen(ffi.new("Vector3", 0, 0, 0), 5.0, 10.0)
add_generator(function(p, dt, start_ix, end_ix)
	p = get_particles(p)

	circle_gen(p, dt, start_ix, end_ix)

	for i = start_ix, end_ix - 1 do
		p.velocity[i] = (Vec3Subtract(ffi.new("Vector3"), p.pos[i])):normalize()
	end

	local dist = FloatRandomDist(20, 50)
	for i = start_ix, end_ix - 1 do
		p.velocity[i] = Vec3Scale(p.velocity[i], dist())
	end

	local magnitude = ffi.new("Vector3", 50, 0, 50)
	for i = start_ix, end_ix - 1 do
		local normalized = p.velocity[i]:normalize()

		p.acceleration[i].x = normalized.x * magnitude.x
		p.acceleration[i].y = normalized.y * magnitude.y
		p.acceleration[i].z = normalized.z * magnitude.z
	end

	for i = start_ix, end_ix - 1 do
		p.color[i] = ffi.new("Color", 255, 255, 255, 255)
	end

	local dist = FloatRandomDist(0.1, 0.6)
	for i = start_ix, end_ix - 1 do
		p.lifetime[i] = dist()
	end

	for i = start_ix, end_ix - 1 do
		p.size[i] = p.velocity[i]:length()
	end
end, system)

local worst = 0
local average = 0
local average_count = 0

add_updater(function(p, dt)
	local start = os.clock()

	p = get_particles(p)
    local pos = p.pos
    local velocity = p.velocity
    local acceleration = p.acceleration
    local color = p.color
    local alive = p.alive
    local lifetime = p.lifetime
    local size = p.size
    local max_size = p.max_size
    local alive_count = p.alive_count()

	for i = 0, alive_count - 1 do
		velocity[i] = Vec3Add(velocity[i], Vec3Scale(acceleration[i], dt))
		pos[i] = Vec3Add(pos[i], Vec3Scale(velocity[i], dt))
	end

	local end_ix = alive_count
	local i = 0

	while i < end_ix do
		lifetime[i] = lifetime[i] - dt

		if lifetime[i] <= 0.0 then
			p.kill(i)
            local tmp = p.alive_count()
			if tmp < max_size then
				end_ix = tmp
			else
				end_ix = max_size
			end
		end

		i = i + 1
	end
    alive_count = p.alive_count()

	local min_threshold = 0.0
	local max_threshold = 120
	local start_color = ffi.new("Color", 255, 0, 0, 255)
	local end_color = ffi.new("Color", 255, 255, 255, 255)

	for i = 0, alive_count - 1 do
		local speed = math.abs(velocity[i]:length())
		local factor = (speed - min_threshold) / (max_threshold - min_threshold)

		if factor < 0 then
			color[i] = ColorLerp(color[i], start_color, math.abs(speed / min_threshold))
		else
			color[i] = ColorLerp(start_color, end_color, factor)
		end
	end

	for i = 0, alive_count - 1 do
		size[i] = 0.05 * velocity[i]:length()
	end

	local finish = os.clock()

    worst = math.max(finish - start, worst)
    average_count = average_count + 1
    average = average + ((finish - start) - average)/average_count
end, system)

tracker(function()
    print("WORST: " .. worst)
    print("AVERAGE: " .. average)
end)
