using StaticArrays

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

    origin::SVector{3, Float32}
end

CSystemOpts(opts::SystemOpts) = CSystemOpts(opts.emit_rate, opts.max_emit, opts.max_particles, isnothing(opts.reset_on_done) ? UInt(0) : opts.reset_on_done, (opts.origin[1], opts.origin[2], opts.origin[3]))

struct CParticles
    pos::Ptr{Cfloat}
    velocity::Ptr{Cfloat}
    acceleration::Ptr{Cfloat}
    color::Ptr{Cuchar}
    alive::Ptr{Cuchar}
    lifetime::Ptr{Cfloat}
    size::Ptr{Cfloat}

    max_size::Csize_t
    alive_count::Csize_t
end

struct Particles
    pos::AbstractVector{SVector{3, Float32}}
    velocity::AbstractVector{SVector{3, Float32}}
    acceleration::AbstractVector{SVector{3, Float32}}
    color::AbstractVector{SVector{4, UInt8}}
    alive::AbstractVector{UInt8}
    lifetime::AbstractVector{Float32}
    size::AbstractVector{Float32}

    max_size::UInt
    alive_count::UInt

    __ptr::Ptr{CParticles}
end

function kill_particle(p::Particles, ix::UInt)
    @ccall ffi__kill_particle(p.__ptr::Ptr{CParticles}, ix::Csize_t)::Cvoid
end

function wake_particle(p::Particles, ix::UInt)
    @ccall ffi__wake_particle(p.__ptr::Ptr{CParticles}, ix::Csize_t)::Cvoid
end

function swap_particle(p::Particles, a::UInt, b::UInt)
    @ccall ffi__swap_particle(p.__ptr::Ptr{CParticles}, a::Csize_t, b::Csize_t)::Cvoid
end

function particles(c_particles_ptr::Ptr{CParticles})
    cpart = unsafe_load(c_particles_ptr)

    max_sz = UInt(cpart.max_size)
    alive_ct = UInt(cpart.alive_count)

    # reinterpret the pointers as arrays
    pos_data = unsafe_wrap(Array, cpart.pos, (3 * max_sz,), own = false)
    vel_data = unsafe_wrap(Array, cpart.velocity, (3 * max_sz,), own = false)
    acc_data = unsafe_wrap(Array, cpart.acceleration, (3 * max_sz,), own = false)

    # reshape them into 3-element vectors without copying
    pos_vecs = reinterpret(SVector{3, Float32}, pos_data)
    vel_vecs = reinterpret(SVector{3, Float32}, vel_data)
    acc_vecs = reinterpret(SVector{3, Float32}, acc_data)

    # For color: 4 * max_sz elements (RGBA)
    color_data = unsafe_wrap(Array, cpart.color, (4 * max_sz,), own = false)
    color_vecs = reinterpret(SVector{4, UInt8}, color_data)

    alive_vec = unsafe_wrap(Array, cpart.alive, (max_sz,), own = false)

    lifetime_vec = unsafe_wrap(Array, cpart.lifetime, (max_sz,), own = false)
    size_vec = unsafe_wrap(Array, cpart.size, (max_sz,), own = false)

    return Particles(pos_vecs, vel_vecs, acc_vecs, color_vecs, alive_vec, lifetime_vec, size_vec, max_sz, alive_ct, c_particles_ptr)
end

struct NewSystemThunk
    generator::Function
    updater::Function
end

global thunks = Dict{UInt, NewSystemThunk}()

function __generator(_thunk::Ptr{Cvoid}, c_particles_ptr::Ptr{CParticles}, dt::Cfloat, start_ix::Csize_t, end_ix::Csize_t)::Cvoid
    thunk = unsafe_pointer_to_objref(_thunk)::NewSystemThunk

    thunk.generator(particles(c_particles_ptr), dt, start_ix, end_ix)
end

function __updater(_thunk::Ptr{Cvoid}, c_particles_ptr::Ptr{CParticles}, dt::Cfloat)::Cvoid
    thunk = unsafe_pointer_to_objref(_thunk)::NewSystemThunk

    thunk.updater(particles(c_particles_ptr), dt)
end

function new_system(generator::Function, updater::Function, opts::SystemOpts)::UInt
    thunk = NewSystemThunk(generator, updater)

    c_generator = @cfunction(__generator, Cvoid, (Ptr{Cvoid}, Ptr{CParticles}, Cfloat, Csize_t, Csize_t))
    c_updater = @cfunction(__updater, Cvoid, (Ptr{Cvoid}, Ptr{CParticles}))

    id = @ccall ffi__new_system(thunk::Any, c_generator::Ptr{Cvoid}, c_updater::Ptr{Cvoid}, CSystemOpts(opts)::CSystemOpts)::Csize_t

    thunks[id] = thunk
    return id
end

function __main()
    generator(p::Particles, dt::Float32, start_ix::UInt, end_ix::UInt) = begin
        @show typeof(p.pos)
        print("START IX: ")
        print(start_ix)
        print("; END IX: ")
        println(end_ix)
    end
    updater(p::Particles, dt::Float32) = begin

    end

    println(new_system(generator, updater, SystemOpts(5000, 10000, 30000, nothing, (0f0, 0f0, 0f0))))
end
