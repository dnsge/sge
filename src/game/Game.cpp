#include "game/Game.hpp"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include "game/Scene.hpp"
#include "physics/Collision.hpp"
#include "physics/World.hpp"
#include "render/RenderQueue.hpp"
#include "resources/Configs.hpp"
#include "resources/Resources.hpp"
#include "scripting/EventSub.hpp"
#include "scripting/Tracing.hpp"

#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <utility>

namespace sge::game {

Game::Game(resources::GameConfig gameConfig)
    : gameConfig_(std::move(gameConfig))
    , physicsWorld_(std::make_unique<physics::World>(this)) {}

Game::~Game() {
    this->destroy();
}

void Game::update() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds dtUs{};
    if (this->lastFrameValid_) [[likely]] {
        dtUs = std::chrono::duration_cast<std::chrono::microseconds>(now - this->lastFrame_);
    } else {
        dtUs = SixtyFPSFrameDuration;
        this->lastFrameValid_ = true;
    }
    this->lastFrame_ = now;

    float dtS = dtUs.count() / 1000000.0F;

    if (this->scene_ == nullptr) {
        // No scene set, nothing to update
        return;
    }

    // Run lifecycle functions
    this->updateOnStart();
    this->updateOnUpdate(dtS);
    this->updateOnLateUpdate(dtS);

    // Process Event.Subscribe/Event.Unsubscribe calls made during the frame
    this->eventSub_.executePendingSubscriptions();

    // Insert any actors that were created during the frame
    this->scene_->insertInstantiatedActors();
    // Remove any actors that were destroyed during the frame
    this->scene_->removeDestroyedActors();

    // Step physics world
    this->physicsWorld_->step();
}

void Game::destroy() {
    if (this->physicsWorld_ != nullptr) {
        this->physicsWorld_->disableCollisionReporting();
    }
    // Destroy scene actors
    this->scene_->clear();
    // Destroy box2d world
    this->physicsWorld_.reset();
}

void Game::updateOnStart() {
    assert(this->scene_ != nullptr);
    // Execute all pending OnStart calls
    for (auto &actor : this->scene_->actors()) {
        if (!actor->alive && !actor->destroyed) {
            actor->alive = true;
        }
        TRACE_BEGIN("OnStart", actor->getName());
        actor->onStart();
        TRACE_END;
    }
}

void Game::updateOnUpdate(float dt) {
    assert(this->scene_ != nullptr);
    // Execute all OnUpdate calls
    for (auto &actor : this->scene_->actors()) {
        TRACE_BEGIN("OnUpdate", actor->getName());
        actor->onUpdate(dt);
        TRACE_END;
    }
}

void Game::updateOnLateUpdate(float dt) {
    assert(this->scene_ != nullptr);
    // Execute all OnLateUpdate calls
    for (auto &actor : this->scene_->actors()) {
        TRACE_BEGIN("OnLateUpdate", actor->getName());
        actor->onLateUpdate(dt);
        TRACE_END;
    }
}

void Game::render() {
    this->renderQueue_.render(this->cameraPos_, this->zoom_);
}

void Game::BeginContact(b2Contact* contact) {
    const auto &[collisionA, collisionB, kind] = physics::CollisionFromContactEnter(contact);
    switch (kind) {
    case physics::CollisionKind::Collider:
        TRACE_BEGIN("OnCollisionEnter", collisionA.me->name);
        collisionA.me->onCollisionEnter(collisionA.collision);
        TRACE_END;
        TRACE_BEGIN("OnCollisionEnter", collisionB.me->name);
        collisionB.me->onCollisionEnter(collisionB.collision);
        TRACE_END;
        break;
    case physics::CollisionKind::Trigger:
        TRACE_BEGIN("OnTriggerEnter", collisionA.me->name);
        collisionA.me->onTriggerEnter(collisionA.collision);
        TRACE_END;
        TRACE_BEGIN("OnTriggerEnter", collisionB.me->name);
        collisionB.me->onTriggerEnter(collisionB.collision);
        TRACE_END;
        break;
    }
}

void Game::EndContact(b2Contact* contact) {
    const auto &[collisionA, collisionB, kind] = physics::CollisionFromContactExit(contact);
    switch (kind) {
    case physics::CollisionKind::Collider:
        TRACE_BEGIN("OnCollisionExit", collisionA.me->name);
        collisionA.me->onCollisionExit(collisionA.collision);
        TRACE_END;
        TRACE_BEGIN("OnCollisionExit", collisionB.me->name);
        collisionB.me->onCollisionExit(collisionB.collision);
        TRACE_END;
        break;
    case physics::CollisionKind::Trigger:
        TRACE_BEGIN("OnTriggerExit", collisionA.me->name);
        collisionA.me->onTriggerExit(collisionA.collision);
        TRACE_END;
        TRACE_BEGIN("OnTriggerExit", collisionB.me->name);
        collisionB.me->onTriggerExit(collisionB.collision);
        TRACE_END;
        break;
    }
}

void Game::loadScene(const std::string &name) {
    this->physicsWorld_->disableCollisionReporting();

    // Load scene from resources
    const auto &source = sge::resources::GetSceneDescription(name);
    if (this->scene_ != nullptr) {
        // Inherit any persistent actors
        this->scene_ = std::make_unique<Scene>(source, *this->scene_);
    } else {
        // Fresh scene (game just started)
        this->scene_ = std::make_unique<Scene>(source);
    }

    this->physicsWorld_->enableCollisionReporting();
}

Scene &Game::currentScene() {
    assert(this->scene_ != nullptr);
    return *this->scene_;
}

const Scene &Game::currentScene() const {
    assert(this->scene_ != nullptr);
    return *this->scene_;
}

physics::World &Game::physicsWorld() {
    assert(this->physicsWorld_ != nullptr);
    return *this->physicsWorld_;
}

render::RenderQueue &Game::renderQueue() {
    return this->renderQueue_;
}

scripting::EventSub &Game::eventSub() {
    return this->eventSub_;
}

std::chrono::microseconds Game::tickDuration() const {
    return this->tickDuration_;
}

void Game::setTickDuration(std::chrono::microseconds tickDuration) {
    this->tickDuration_ = tickDuration;
}

float Game::zoom() const {
    return this->zoom_;
}

void Game::setZoom(float zoom) {
    this->zoom_ = zoom;
}

const glm::vec2 &Game::cameraPos() const {
    return this->cameraPos_;
}

void Game::setCameraPos(glm::vec2 pos) {
    this->cameraPos_ = pos;
}

} // namespace sge::game
