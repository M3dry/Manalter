---@diagnostic disable: unused-local, unused-function

local function vec3add(v1, v2)
	return Vector3.new(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z)
end

local function vec3subtract(v1, v2)
	return Vector3.new(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z)
end

local function vec3scale(vec, scalar)
	return Vector3.new(vec.x * scalar, vec.y * scalar, vec.z * scalar)
end

local function vec3dot(v1, v2)
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z
end

local function vec3length(vec)
	return math.sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z)
end

local function vec3normalize(vec)
	return vec3scale(vec, 1 / vec3length(vec))
end

local function float_random_dist(min, max)
	return function()
		return min + math.random() * (max - min)
	end
end

local function on_circle_gen(center, min_rad, max_rad)
	return function(particles, dt, start_ix, end_ix)
		local angle_dist = float_random_dist(0, 2 * math.pi)
		local radius_dist = float_random_dist(min_rad, max_rad)

		for i = start_ix, end_ix - 1, 1 do
			local angle = angle_dist()
			local radius = radius_dist()
			particles.pos[i].x = center.x + radius * math.cos(angle)
			particles.pos[i].y = center.y
			particles.pos[i].z = center.z + radius * math.sin(angle)
		end
	end
end

local system = new_system(3000)

add_generator(on_circle_gen(Vector3.new(0, 0, 0), 5.0, 10.0), system)
add_generator(function(p, _, start_ix, end_ix)
	for i = start_ix, end_ix - 1 do
		p.velocity[i] = vec3normalize(vec3subtract(Vector3.new(0, 0, 0), p.pos[i]))
	end
end, system)
add_generator(function(p, dt, start_ix, end_ix)
    local dist = float_random_dist(20, 50)

    for i = start_ix, end_ix - 1 do
        p.velocity[i] = vec3scale(p.velocity[i], dist())
    end
end, system)
