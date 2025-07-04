import Base: +,-,*,/
using LinearAlgebra

function +(v::NTuple{N, Float32}, w::NTuple{N, Float32})::NTuple{N, Float32} where {N}
    return ntuple(i -> v[i] + w[i], N)
end

function -(v::NTuple{N, Float32}, w::NTuple{N, Float32})::NTuple{N, Float32} where {N}
    return ntuple(i -> v[i] - w[i], N)
end

function *(s::Float32, v::NTuple{N, Float32})::NTuple{N, Float32} where {N}
    return ntuple(i -> s * v[i], N)
end

function *(v::NTuple{N, Float32}, s::Float32)::NTuple{N, Float32} where {N}
    return s * v
end

function /(v::NTuple{N, Float32}, s::Float32)::NTuple{N, Float32} where {N}
    return ntuple(i -> v[i] * 1f0/s, N)
end

function -(v::NTuple{N, Float32})::NTuple{N, Float32} where {N}
    return ntuple(i -> -v[i], N)
end

function norm(v::NTuple{N, Float32})::Float32 where {N}
    return sqrt(sum(vi^2 for vi in v))
end

function normalize(v::NTuple{N, Float32})::NTuple{N, Float32} where {N}
    return v/norm(v)
end

struct CSystemOpts
    emit_rate::Cfloat
    max_emit::Csize_t

    max_particles::Csize_t
    # 0 = nullopt
    reset_on_done::Csize_t

    origin::NTuple{3, Cfloat}
end

struct SystemOpts
    emit_rate::Float32
    max_emit::UInt

    max_particles::UInt
    reset_on_done::Union{UInt, Nothing}

    origin::NTuple{3, Float32}
end

SystemOpts(max_particles;
    emit_rate = typemax(UInt),
    max_emit = typemax(UInt),
    reset_on_done = nothing,
    origin = (0, 0, 0)
) = SystemOpts(Float32(emit_rate), UInt(max_emit), UInt(max_particles), isnothing(reset_on_done) ? nothing : UInt(reset_on_done), NTuple{3, Float32}(origin))

CSystemOpts(opts::SystemOpts) = CSystemOpts(opts.emit_rate, opts.max_emit, opts.max_particles, isnothing(opts.reset_on_done) ? UInt(0) : opts.reset_on_done, (opts.origin[1], opts.origin[2], opts.origin[3]))

struct CParticles
    pos::Ptr{NTuple{3, Cfloat}}
    velocity::Ptr{NTuple{3, Cfloat}}
    acceleration::Ptr{NTuple{3, Cfloat}}
    color::Ptr{NTuple{4, Cuchar}}
    alive::Ptr{Cuchar}
    lifetime::Ptr{Cfloat}
    size::Ptr{Cfloat}

    max_size::Csize_t
    alive_count::Csize_t
end

mutable struct Particles
    pos::Vector{NTuple{3, Float32}}
    velocity::Vector{NTuple{3, Float32}}
    acceleration::Vector{NTuple{3, Float32}}
    color::Vector{NTuple{4, UInt8}}
    alive::Vector{Bool}
    lifetime::Vector{Float32}
    size::Vector{Float32}
    alive_count::UInt
    max_size::UInt
    __ptr::Ptr{CParticles}
end

function kill!(p::Particles, ix::UInt)
    @ccall ffi__kill_particle(p.__ptr::Ptr{CParticles}, ix::Csize_t)::Cvoid
    p.alive_count -= 1
end

function wake!(p::Particles, ix::UInt)
    @ccall ffi__wake_particle(p.__ptr::Ptr{CParticles}, ix::Csize_t)::Cvoid
    p.alive_count += 1
end

function swap!(p::Particles, a::UInt, b::UInt)
    @ccall ffi__swap_particle(p.__ptr::Ptr{CParticles}, a::Csize_t, b::Csize_t)::Cvoid
end

function ffi_particles(c_particles_ptr::Ptr{CParticles})::Particles
    cpart = unsafe_load(c_particles_ptr)

    alive_count = UInt(cpart.alive_count)
    max_sz = UInt(cpart.max_size)

    # # reinterpret the pointers as arrays
    # pos_data = unsafe_wrap(Array, cpart.pos, (3 * max_sz,), own = false)
    # vel_data = unsafe_wrap(Array, cpart.velocity, (3 * max_sz,), own = false)
    # acc_data = unsafe_wrap(Array, cpart.acceleration, (3 * max_sz,), own = false)
    #
    # # reshape them into 3-element vectors without copying
    # pos_vecs = reinterpret(SVector{3, Float32}, pos_data)
    # vel_vecs = reinterpret(SVector{3, Float32}, vel_data)
    # acc_vecs = reinterpret(SVector{3, Float32}, acc_data)
    #
    # # For color: 4 * max_sz elements (RGBA)
    # color_data = unsafe_wrap(Array, cpart.color, (4 * max_sz,), own = false)
    # color_vecs = reinterpret(SVector{4, UInt8}, color_data)

    pos_vecs = unsafe_wrap(Array, cpart.pos, (max_sz,), own = false)
    vel_vecs = unsafe_wrap(Array, cpart.velocity, (max_sz,), own = false)
    acc_vecs = unsafe_wrap(Array, cpart.acceleration, (max_sz,), own = false)

    color_vecs = unsafe_wrap(Array, cpart.color, (max_sz,), own = false)

    alive_vec = unsafe_wrap(Array, cpart.alive, (max_sz,), own = false)

    lifetime_vec = unsafe_wrap(Array, cpart.lifetime, (max_sz,), own = false)
    size_vec = unsafe_wrap(Array, cpart.size, (max_sz,), own = false)

    return Particles(pos_vecs, vel_vecs, acc_vecs, color_vecs, alive_vec, lifetime_vec, size_vec, alive_count, max_sz, c_particles_ptr)
end

struct NewSystemThunk
    generator::Function
    updater::Function
end

global thunk_counter = UInt(0)
global thunks = Dict{UInt, NewSystemThunk}()
global system_id_to_thunk_id = Dict{UInt, UInt}()

function __generator(thunk_id::Ptr{Cvoid}, c_particles_ptr::Ptr{CParticles}, dt::Cfloat, start_ix::Csize_t, end_ix::Csize_t)::Cvoid
    thunk = thunks[UInt(thunk_id)]

    thunk.generator(ffi_particles(c_particles_ptr), dt, start_ix + 1, end_ix)
end

function __updater(thunk_id::Ptr{Cvoid}, c_particles_ptr::Ptr{CParticles}, dt::Cfloat)::Cvoid
    thunk = thunks[UInt(thunk_id)]

    thunk.updater(ffi_particles(c_particles_ptr), dt)
end

function new_system(generator::Function, updater::Function, opts::SystemOpts)::UInt
    thunk = NewSystemThunk(generator, updater)

    c_generator = @cfunction(__generator, Cvoid, (Ptr{Cvoid}, Ptr{CParticles}, Cfloat, Csize_t, Csize_t))
    c_updater = @cfunction(__updater, Cvoid, (Ptr{Cvoid}, Ptr{CParticles}, Cfloat))

    id = @ccall ffi__new_system(Ptr{Cvoid}(thunk_counter)::Ptr{Cvoid}, c_generator::Ptr{Cvoid}, c_updater::Ptr{Cvoid}, CSystemOpts(opts)::CSystemOpts)::Csize_t

    thunks[thunk_counter] = thunk
    system_id_to_thunk_id[id] = thunk_counter
    global thunk_counter += 1
    return id
end

function color_lerp(start_color::NTuple{4, UInt8}, end_color::NTuple{4, UInt8}, factor::Float32)
    factor = clamp(factor, 0, 1)

    return tuple((UInt8(round((1f0 - factor) * Float32(start_color[i]) + factor * Float32(end_color[i]))) for i in 1:4)...)
end

function float_random_dist(min::Float32, max::Float32)
    return () -> rand(Float32) * (max - min) + min
end

function OnCircleGen(center::NTuple{3, Float32}, min_rad::Float32, max_rad::Float32)::Function
    return (p::Particles, dt::Float32, start_ix::UInt, end_ix::UInt) -> begin
        angle_dist = float_random_dist(0f0, 2f0 * pi)
        radius_dist = float_random_dist(5f0, 10f0)

        @inbounds for i in start_ix:end_ix
            angle = angle_dist()
            radius = radius_dist()

            p.pos[i] = (center[1] + radius * cos(angle), center[2], center[3] + sin(angle))
        end
    end
end

function __main()
    on_circle = OnCircleGen((0f0, 0f0, 0f0), 5f0, 10f0)
    generator!(p::Particles, dt::Float32, start_ix::UInt, end_ix::UInt) = begin
        on_circle(p, dt, start_ix, end_ix)

        @inbounds for i in start_ix:end_ix
            p.velocity[i] = normalize((0f0, 0f0, 0f0) - p.pos[i])
        end

        dist = float_random_dist(20f0, 50f0)
        @inbounds for i in start_ix:end_ix
            p.velocity[i] *= dist()
        end

        magnitude = (50f0, 0f0, 50f0)
        @inbounds for i in start_ix:end_ix
            normalized = normalize(p.velocity[i])

            p.acceleration[i] = (normalized[1] * magnitude[1], normalized[2] * magnitude[2], normalized[3] * magnitude[3])
        end

        @inbounds for i in start_ix:end_ix
            p.color[i] = (255, 255, 255, 255)
        end

        dist = float_random_dist(0.1f0, 0.6f0)
        @inbounds for i in start_ix:end_ix
            p.lifetime[i] = dist()
        end

        @inbounds for i in start_ix:end_ix
            p.size[i] = 0.05f0 * norm(p.velocity[i])
        end
    end

    updater!(p::Particles, dt::Float32) = begin
        @inbounds for i in 1:p.alive_count
            p.velocity[i] += p.acceleration[i] * dt
            p.pos[i] += p.velocity[i] * dt
        end

        end_ix = p.alive_count
        i = UInt(1)

        @inbounds while i <= end_ix
            p.lifetime[i] -= dt

            if p.lifetime[i] <= 0.0f0
                kill!(p, i)

                end_ix = min(p.alive_count, p.max_size)
            else
                i += 1
            end
        end

        min_threshold = 0.0f0
        max_threshold = 120.0f0
        start_color = (UInt8(255), UInt8(0), UInt8(0), UInt8(255))
        end_color = (UInt8(255), UInt8(255), UInt8(255), UInt8(255))

        @inbounds for i in 1:p.alive_count
            speed = norm(p.velocity[i])
            factor = (speed - min_threshold)/(max_threshold - min_threshold)

            if factor < 0f0
                p.color[i] = color_lerp(color[i], start_color, abs(speed / min_threshold))
            else
                p.color[i] = color_lerp(start_color, end_color, factor)
            end
        end

        @inbounds for i = 1:p.alive_count
            p.size[i] = 0.05f0 * norm(p.velocity[i])
        end
    end

    println(new_system(generator!, updater!, SystemOpts(90000, origin=(0, 10, 0))))
end
