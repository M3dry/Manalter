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

function *(v::NTuple{N, Float32}, s::Float32)::NTuple{N, Float32} where {N} return s * v
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

function dot(v::NTuple{N, Float32}, w::NTuple{N, Float32})::Float32 where {N}
    return sum(map(*, v, w))
end

if !isdefined(Main, :SystemOpts)
    struct SystemOpts
        emit_rate::Float32
        max_emit::UInt

        max_particles::UInt
        # 0 = nullopt
        reset_on_done::UInt

        origin::NTuple{3, Float32}
    end
end

SystemOpts(max_particles;
    emit_rate = typemax(UInt),
    max_emit = typemax(UInt),
    reset_on_done = nothing,
    origin = (0, 0, 0)
) = SystemOpts(Float32(emit_rate), UInt(max_emit), UInt(max_particles), isnothing(reset_on_done) ? UInt(0) : UInt(reset_on_done), NTuple{3, Float32}(origin))

if !isdefined(Main, :CParticles)
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
    __ptr::Ptr{Nothing}
end

function kill!(p::Particles, ix::UInt)
    p.alive_count = _kill!(p.__ptr, ix)
end

function wake!(p::Particles, ix::UInt)
    p.alive_count = _wake!(p.__ptr, ix)
end

function swap!(p::Particles, a::UInt, b::UInt)
    _swap!(p.__ptr, a, b)
end

function load_particles(c_particles_ptr::Ptr{CParticles})::Particles
    cpart = unsafe_load(c_particles_ptr)

    alive_count = UInt(cpart.alive_count)
    max_sz = UInt(cpart.max_size)

    pos_vecs = unsafe_wrap(Array, cpart.pos, (max_sz,), own = false)
    vel_vecs = unsafe_wrap(Array, cpart.velocity, (max_sz,), own = false)
    acc_vecs = unsafe_wrap(Array, cpart.acceleration, (max_sz,), own = false)

    color_vecs = unsafe_wrap(Array, cpart.color, (max_sz,), own = false)

    alive_vec = unsafe_wrap(Array, cpart.alive, (max_sz,), own = false)

    lifetime_vec = unsafe_wrap(Array, cpart.lifetime, (max_sz,), own = false)
    size_vec = unsafe_wrap(Array, cpart.size, (max_sz,), own = false)

    return Particles(pos_vecs, vel_vecs, acc_vecs, color_vecs, alive_vec, lifetime_vec, size_vec, alive_count, max_sz, c_particles_ptr)
end

function color_lerp(start_color::NTuple{4, UInt8}, end_color::NTuple{4, UInt8}, factor::Float32)
    factor = clamp(factor, 0, 1)

    return tuple((UInt8(round((1f0 - factor) * Float32(start_color[i]) + factor * Float32(end_color[i]))) for i in 1:4)...)
end

function float_random_dist(min::Float32, max::Float32)
    return () -> rand(Float32) * (max - min) + min
end

function OnCircleGen(center::NTuple{3, Float32}, min_rad::Float32, max_rad::Float32)::Function
    angle_dist = float_random_dist(0f0, 2f0 * pi)
    radius_dist = float_random_dist(5f0, 10f0)
    return (p::Particles, dt::Float32, start_ix::UInt, end_ix::UInt) -> begin

        @inbounds for i in start_ix:end_ix
            angle = angle_dist()
            radius = radius_dist()

            p.pos[i] = (center[1] + radius * cos(angle), center[2], center[3] + radius * sin(angle))
        end
    end
end

const circle_radius = 10f0

on_circle = OnCircleGen((0f0, 15f0, 0f0), 0.001f0, 1f0)
velocity_dist = float_random_dist(5f0, 100f0)
generator!(c_ptr::Ptr{CParticles}, dt::Float32, start_ix::UInt, end_ix::UInt) = begin
    p = load_particles(c_ptr)

    on_circle(p, dt, start_ix, end_ix)

    @inbounds for i in start_ix:end_ix
        p.velocity[i] = velocity_dist() * normalize((0f0, 0f0, 0f0) - p.pos[i])
    end

    @inbounds for i in start_ix:end_ix
        p.color[i] = (rand(UInt8), rand(UInt8), rand(UInt8), UInt8(255))
    end

    @inbounds for i in start_ix:end_ix
        p.size[i] = clamp(100f0/norm(p.velocity[i]), 2f0, 5f0)
    end
end

updater!(c_ptr::Ptr{CParticles}, dt::Float32) = begin
    p = load_particles(c_ptr);

    @inbounds for i in 1:p.alive_count
        to_center = p.pos[i] - (0f0, 15f0, 0f0)

        dist = norm(to_center)
        if dist > circle_radius
            normal = normalize(to_center)
            p.velocity[i] = p.velocity[i] - 2f0*dot(p.velocity[i], normal)*normal;

            p.pos[i] = (0f0, 15f0, 0f0) + normal * (circle_radius - 0.0001f0)
        end
    end

    @inbounds for i in 1:p.alive_count
        p.pos[i] += p.velocity[i] * dt
    end

    @inbounds for i in 1:p.alive_count
        p.size[i] = clamp(100f0/norm(p.velocity[i]), 2f0, 5f0)
    end
end

try
    println(new_system(generator!, updater!, SystemOpts(1000, origin=(0, 5, 0))))
catch e
    println(e)
end
