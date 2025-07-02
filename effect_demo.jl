using StaticArrays
using BenchmarkTools
using LinearAlgebra

mutable struct Particles
    pos::Vector{SVector{3, Float32}}
    velocity::Vector{SVector{3, Float32}}
    acceleration::Vector{SVector{3, Float32}}
    color::Vector{SVector{4, UInt8}}
    alive::Vector{Bool}
    lifetime::Vector{Float32}
    size::Vector{Float32}
    alive_count::Int
    max_size::Int
end

function swap!(p::Particles, a::Int, b::Int)
    @inbounds begin
        p.pos[a], p.pos[b] = p.pos[b], p.pos[a]
        p.velocity[a], p.velocity[b] = p.velocity[b], p.velocity[a]
        p.acceleration[a], p.acceleration[b] = p.acceleration[b], p.acceleration[a]
        p.color[a], p.color[b] = p.color[b], p.color[a]
        p.alive[a], p.alive[b] = p.alive[b], p.alive[a]
        p.lifetime[a], p.lifetime[b] = p.lifetime[b], p.lifetime[a]
        p.size[a], p.size[b] = p.size[b], p.size[a]
    end
end

function kill!(p::Particles, ix::Int)
    if p.alive_count <= 0
        return
    end

    @inbounds begin
        p.alive[ix] = false
        p.alive_count -= 1
        swap!(p, ix, p.alive_count + 1)  # julia uses 1 based indexing
    end
end

function wake!(p::Particles, ix::Int)
    if p.alive_count >= length(p.pos)
        return
    end

    @inbounds begin
        p.alive[ix] = true
        p.alive_count += 1
        swap!(p, ix, p.alive_count)
    end
end

function color_lerp(start_color::SVector{4, UInt8}, end_color::SVector{4, UInt8}, factor::Float32)
    factor = clamp(factor, 0, 1)

    return @SVector [UInt8(round((1f0 - factor) * Float32(start_color[i]) + factor * Float32(end_color[i]))) for i in 1:4]
end

function update_particles!(p::Particles, dt::Float32)
    @inbounds for i in 1:p.alive_count
        p.velocity[i] += + p.acceleration[i] * dt
        p.pos[i] += + p.velocity[i] * dt
    end

    end_ix = p.alive_count
    i = 1

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
    start_color = SVector{4, UInt8}(255, 0, 0, 255)
    end_color = SVector{4, UInt8}(255, 255, 255, 255)

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

function create_particles(n::Int)::Particles
    pos = [SVector{3, Float32}(0, 0, 0) for _ in 1:n]
    velocity = [SVector{3, Float32}(0, 0, 0) for _ in 1:n]
    acceleration = [SVector{3, Float32}(0, 0, 0) for _ in 1:n]
    color = [SVector{4, UInt8}(255, 255, 255, 255) for _ in 1:n]
    alive = [true for _ in 1:n]
    lifetime = [1.0f0 for _ in 1:n]
    size = [1.0f0 for _ in 1:n]

    return Particles(pos, velocity, acceleration, color, alive, lifetime, size, n, n)
end

particles = create_particles(5000000)
@btime update_particles!($particles, 0.016f0)
