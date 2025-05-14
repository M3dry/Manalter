#include "effects.hpp"
#include "particle_system.hpp"
#include <ranges>

namespace effect {
    particle_system::System Plosion::operator()(Vector2 _origin) {
        using namespace particle_system;

        auto origin = Vector3{
            .x = _origin.x,
            .y = center_y,
            .z = _origin.y,
        };

        System system(particle_count, renderers::Point(particle_count));

        emitters::CustomEmitter emitter(emit_rate, max_emit);
        emitter.add_generator(generators::pos::OnSphere(origin, {radius * 0.7f, radius + 1.0f}));
        emitter.add_generator(
            [origin = origin, type = type](Particles& particles, float _, std::size_t start_ix, std::size_t end_ix) {
                for (std::size_t i = start_ix; i < end_ix; i++) {
                    switch (type) {
                        case Ex:
                            particles.velocity[i] = Vector3Normalize(Vector3Subtract(particles.pos[i], origin));
                            break;
                        case Im:
                            particles.velocity[i] = Vector3Normalize(Vector3Subtract(origin, particles.pos[i]));
                            break;
                    }
                }
            });
        emitter.add_generator(generators::velocity::ScaleRange(velocity_scale.first, velocity_scale.second));
        emitter.add_generator(generators::acceleration::Uniform(acceleration));
        emitter.add_generator(generators::color::Fixed(WHITE));
        emitter.add_generator(generators::lifetime::Range(lifetime.first, lifetime.second));
        emitter.add_generator([](Particles& particles, float _, std::size_t start_ix, std::size_t end_ix) {
            for (std::size_t i = start_ix; i < end_ix; i++) {
                particles.size[i] = Vector3Length(particles.velocity[i]);
            }
        });
        system.add_emitter(std::move(emitter));

        if (floor_y || type == Im) {
            system.add_updater([y = floor_y, type = type, origin = origin](Particles& particles, float _) {
                auto end_ix = particles.alive_count;
                for (std::size_t i = 0; i < end_ix; i++) {
                    if ((y && particles.pos[i].y < *y) ||
                        (type == Im && Vector3Distance(origin, particles.pos[i]) <= 0.1f)) {
                        particles.kill(i);
                        end_ix =
                            particles.alive_count < particles.max_size ? particles.alive_count : particles.max_size;
                    }
                }
            });
        }
        system.add_updater(updaters::Position());
        system.add_updater(updaters::Lifetime());
        system.add_updater(
            updaters::ColorByVelocity(color.first.first, color.second.first, color.first.second, color.second.second));
        system.add_updater([scale = particle_size_scale](Particles& particles, float _) {
            for (std::size_t i = 0; i < particles.alive_count; i++) {
                particles.size[i] = scale * Vector3Length(particles.velocity[i]);
            }
        });

        return system;
    }

    particle_system::System ItemDrop::operator()(Vector2 _origin, const Color& rarity_color) {
        using namespace particle_system;

        auto origin = Vector3{
            .x = _origin.x,
            .y = y,
            .z = _origin.y,
        };

        System system(particle_count, renderers::Point(particle_count));

        emitters::CustomEmitter emitter(emit_rate, std::numeric_limits<std::size_t>::max());
        emitter.add_generator(generators::pos::OnCircle(origin, {2.0f * 0.7f, 3.0f}));
        emitter.add_generator(
            [origin = origin](Particles& particles, float _, std::size_t start_ix, std::size_t end_ix) {
                for (std::size_t i = start_ix; i < end_ix; i++) {
                    particles.velocity[i] = Vector3Normalize(Vector3Subtract(particles.pos[i], origin));
                }
            });
        emitter.add_generator(generators::velocity::ScaleRange(10.0f, 30.0f));
        emitter.add_generator(generators::acceleration::Uniform({50.0f, 0.0f, 50.0f}));
        emitter.add_generator(generators::color::Fixed(WHITE));
        emitter.add_generator(generators::lifetime::Range(0.1f, 0.6f));
        emitter.add_generator([](Particles& particles, float _, std::size_t start_ix, std::size_t end_ix) {
            for (std::size_t i = start_ix; i < end_ix; i++) {
                particles.size[i] = Vector3Length(particles.velocity[i]);
            }
        });
        system.add_emitter(std::move(emitter));

        system.add_updater(updaters::Position());
        system.add_updater(updaters::Lifetime());
        system.add_updater(updaters::ColorByVelocity(rarity_color, WHITE, 0.0f, 120.0f));
        system.add_updater([](Particles& particles, float _) {
            for (std::size_t i = 0; i < particles.alive_count; i++) {
                particles.size[i] = 0.05f * Vector3Length(particles.velocity[i]);
            }
        });

        return system;
    }
}

namespace effects {
    std::size_t next_id = 0;
    std::vector<std::pair<std::size_t, particle_system::System>> _effects;

    Id push_effect(particle_system::System&& eff) {
        _effects.emplace_back(next_id, std::forward<particle_system::System>(eff));

        Id id = {next_id++, _effects.size() - 1};
        return id;
    }

    void pop_effect(Id _id) {
        auto [id, ix] = _id;

        if (ix == std::size_t(-1) || id == std::size_t(-1)) {
            return;
        }

        if (_effects.size() > ix && _effects[ix].first == id) {
            _effects[ix] = std::move(_effects.back());
            _effects.pop_back();
            return;
        }

        for (std::size_t i = 0; i < _effects.size(); i++) {
            if (_effects[i].first != id) continue;

            _effects[i] = std::move(_effects.back());
            _effects.pop_back();
            return;
        }
    }

    void update(float dt) {
        std::size_t i = 0;
        while (i < _effects.size()) {
            if (_effects[i].second.update(dt)) {
                i++;
                continue;
            }

            std::swap(_effects[i], _effects.back());
            _effects.pop_back();
        }
    }

    void draw(Vector3 offset) {
        for (std::size_t i = 0; i < _effects.size(); i++) {
            std::get<1>(_effects[i]).draw(offset);
        }
    }

    void clean() {
        _effects.clear();
        _effects.shrink_to_fit();
    }
}
