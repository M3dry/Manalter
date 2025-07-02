#include "particle/particle.hpp"
#include <cstddef>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <julia.h>
#include <print>
#include <raylib.h>
#include <sstream>
#include <string>
#include <vector>

JULIA_DEFINE_FAST_TLS

extern "C" struct CSystemOpts {
    float emit_rate;
    size_t max_emit;

    size_t max_particles;
    // 0 - nullopt
    size_t reset_on_done;

    float origin[3];
};

struct SystemOpts {
    float emit_rate;
    std::size_t max_emit;

    std::size_t max_particles;
    std::optional<std::size_t> reset_on_done;

    Vector3 origin;

    SystemOpts(CSystemOpts opts)
        : emit_rate(opts.emit_rate), max_emit(opts.max_emit), max_particles(opts.max_particles),
          reset_on_done(opts.reset_on_done == 0 ? std::nullopt : std::make_optional(opts.reset_on_done)),
          origin(opts.origin[0], opts.origin[1], opts.origin[2]) {
    };

    void print() const {
        std::println("emit rate: {}", emit_rate);
        std::println("max emit: {}", max_emit);
        std::println("max particles: {}", max_particles);
        std::println("reset on done: {}", reset_on_done ? std::to_string(*reset_on_done) : "nullopt");
        std::println("origin: [{}; {}; {}]", origin.x, origin.y, origin.z);
    }
};

struct System {
    using Generator = void(void*, particle_system::Particles*, float, size_t, size_t);
    using Updater = void(void*, particle_system::Particles*, float);

    SystemOpts opts;
    Generator* generator;
    Updater* updater;

    void* thunk;
};

std::vector<System> systems;

extern "C" {
size_t ffi__new_system(void* thunk, System::Generator* generator, System::Updater* updater, CSystemOpts opts) {
    systems.emplace_back(opts, generator, updater, thunk);

    return systems.size() - 1;
}

void ffi__kill_particle(particle_system::Particles* p, size_t ix) {
    p->kill(ix);
}

void ffi__wake_particle(particle_system::Particles* p, size_t ix) {
    p->wake(ix);
}

void ffi__swap_particles(particle_system::Particles* p, size_t a, size_t b) {
    p->swap(a, b);
}
}

particle_system::Particles particles(3000);

extern "C" void on_particles(void (*f)(particle_system::Particles*)) {
    f(&particles);
}

int main(int argc, char* argv[]) {
    // magic to make julia -> FFI work
    void* handle = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "dlopen(NULL) failed: %s\n", dlerror());
        exit(1);
    }

    const char* script_path = argv[1];
    std::ifstream ifs(script_path);
    std::ostringstream ss;
    ss << ifs.rdbuf();
    std::string script = ss.str();

    jl_init();
    jl_eval_string(script.c_str());

    jl_function_t* f = jl_get_function(jl_main_module, "__main");

    jl_call0(f);

    // if (jl_exception_occurred()) {
    //     jl_printf(jl_stderr_stream(), "Exception: %s\n", jl_typeof_str(jl_exception_occurred()));
    // }

    if (systems.size() > 0) {
        systems[0].generator(systems[0].thunk, &particles, 0.0f, 69, 420);
    }

    jl_atexit_hook(0);
    return 0;
}
