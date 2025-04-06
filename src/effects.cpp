#include "effects.hpp"
#include "particle_system.hpp"

namespace effect {
    particle_system::System Explosion::operator()(Vector2 origin) {
    }

    particle_system::System Implosion::operator()(Vector2 _origin) {
        using namespace particle_system;

        auto origin = Vector3{
            .x = _origin.x,
            .y = origin_y,
            .z = _origin.y,
        };

        System system(particle_count, renderers::Point(particle_count));

        emitters::CustomEmitter emitter;
        emitter.add_generator(generators::pos::OnSphere(origin, { radius*0.7f, radius + 1.0f }));
        emitter.add_generator(
            [origin = origin](Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                for (std::size_t i = start_ix; i < end_ix; i++) {
                    particles.velocity[i] = Vector3Normalize(Vector3Subtract(origin, particles.pos[i]));
                }
            });
        emitter.add_generator(generators::velocity::ScaleRange(velocity_scale.first, velocity_scale.second));
        emitter.add_generator(generators::acceleration::Uniform(acceleration));
        emitter.add_generator(generators::color::Fixed(WHITE));
        emitter.add_generator(generators::lifetime::Range(lifetime.first, lifetime.second));
        emitter.add_generator([](Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
            for (std::size_t i = start_ix; i < end_ix; i++) {
                particles.size[i] = Vector3Length(particles.velocity[i]);
            }
        });
        system.add_emitter(std::move(emitter));

        system.add_updater([y = floor_y, origin = origin](Particles& particles, float dt) {
            auto end_ix = particles.alive_count;
            for (std::size_t i = 0; i < end_ix; i++) {
                if ((y && particles.pos[i].y < *y) || Vector3Distance(origin, particles.pos[i]) <= 0.1f) {
                    particles.kill(i);
                    end_ix = particles.alive_count < particles.max_size ? particles.alive_count : particles.max_size;
                }
            }
        });
        system.add_updater(updaters::Position());
        system.add_updater(updaters::Lifetime());
        system.add_updater(
            updaters::ColorByVelocity(color.first.first, color.second.first, color.first.second, color.second.second));
        system.add_updater([origin](Particles& particles, float dt) {
            for (std::size_t i = 0; i < particles.alive_count; i++) {
                particles.size[i] = Vector3Length(particles.velocity[i]);
            }
        });

        return system;
    }
}

namespace effects {
    Id next_id = 0;
    std::vector<std::tuple<Id, particle_system::System, bool>> effects;

    Id push_effect(particle_system::System&& eff, bool reset_after_end) {
        effects.emplace_back(next_id, std::forward<particle_system::System>(eff), reset_after_end);

        return next_id++;
    }

    void pop_effect(Id id) {
        if (effects.size() < id && std::get<0>(effects[id]) == id) {
            std::swap(effects[id], effects[effects.size() - 1]);
            effects.pop_back();
            return;
        }

        for (std::size_t i = 0; i < effects.size(); i++) {
            if (std::get<0>(effects[i]) != id) continue;

            std::swap(effects[id], effects[effects.size() - 1]);
            effects.pop_back();
            return;
        }
    }

    void update(float dt) {
        std::size_t i = 0;
        while (i < effects.size()) {
            auto& [id, eff, reset] = effects[i];
            eff.update(dt);

            /*if (eff.particles.alive_count != 0) {*/
            /*    i++;*/
            /*    continue;*/
            /*}*/
            /**/
            /*if (reset) {*/
            /*    eff.reset();*/
            /*    i++;*/
            /*    continue;*/
            /*}*/
            /**/
            /*std::swap(effects[i], effects[effects.size() - 1]);*/
            /*effects.pop_back();*/
        }
    }

    void draw() {
        for (auto& [id, eff, reset] : effects) {
            eff.draw();
        }
    }
}
