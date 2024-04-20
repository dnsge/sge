#pragma once

#include <box2d/box2d.h>

#include "game/Scene.hpp"
#include "physics/World.hpp"
#include "render/RenderQueue.hpp"
#include "resources/Configs.hpp"
#include "scripting/EventSub.hpp"

#include <chrono>
#include <memory>
#include <string>

namespace sge::game {

constexpr auto SixtyFPSFrameDuration = std::chrono::microseconds{16667};

class Game : public b2ContactListener {
public:
    Game(resources::GameConfig gameConfig);
    ~Game();

    void update();
    void render();
    void destroy();

    Scene &currentScene();
    const Scene &currentScene() const;
    physics::World &physicsWorld();
    render::RenderQueue &renderQueue();
    scripting::EventSub &eventSub();

    std::chrono::microseconds tickDuration() const;
    void setTickDuration(std::chrono::microseconds tickDuration);

    float zoom() const;
    void setZoom(float zoom);

    const glm::vec2 &cameraPos() const;
    void setCameraPos(glm::vec2 pos);

    /**
     * @brief Immediately loads a scene.
     * 
     * @param name Name of scene to load.
     */
    void loadScene(const std::string &name);

private:
    void BeginContact(b2Contact* contact) override;
    void EndContact(b2Contact* contact) override;

    void updateOnStart();
    void updateOnUpdate(float dt);
    void updateOnLateUpdate(float dt);

    resources::GameConfig gameConfig_;

    // Important: physicsWorld_ must be before scene_ to ensure resources are
    // cleaned up in the correct order.
    std::unique_ptr<Scene> scene_{nullptr};
    std::unique_ptr<physics::World> physicsWorld_{nullptr};

    render::RenderQueue renderQueue_{};
    scripting::EventSub eventSub_{};

    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrame_{};
    bool lastFrameValid_{false};
    std::chrono::microseconds tickDuration_{SixtyFPSFrameDuration};

    glm::vec2 cameraPos_{0.0F, 0.0F};
    float zoom_{1.0F};
};

} // namespace sge::game
