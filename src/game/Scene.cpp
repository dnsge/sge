#include "game/Scene.hpp"

#include <glm/glm.hpp>

#include "Realm.hpp"
#include "Types.hpp"
#include "game/Actor.hpp"
#include "resources/Resources.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sge::game {

Scene::Scene(const resources::SceneDescription &source)
    : name_(source.name) {
    for (const auto &description : source.actors) {
        this->instantiateActor(false, description, std::nullopt);
    }
    this->insertInstantiatedActors();
}

Scene::Scene(const resources::SceneDescription &source, Scene &oldScene)
    : name_(source.name)
    , nextActorId_(oldScene.nextActorId_) {
    // Copy any persistent existing actors
    for (auto &oldActor : oldScene.actors_) {
        if (oldActor->persistent) {
            this->actorIDMap_.emplace(oldActor->id, oldActor.get());
            this->actors_.emplace_back(std::move(oldActor));
        } else {
            oldActor->onDestroy();
        }
    }
    oldScene.actors_.clear();
    oldScene.actorIDMap_.clear();

    // Copy any pending persistent actors
    for (auto &oldActor : oldScene.pendingInstantiatedActors_) {
        if (oldActor->persistent) {
            this->actorIDMap_.emplace(oldActor->id, oldActor.get());
            this->actors_.emplace_back(std::move(oldActor));
        }
    }
    oldScene.pendingInstantiatedActors_.clear();

    // Proceed with standard instantiation of new actors for scene
    for (const auto &description : source.actors) {
        this->instantiateActor(false, description, std::nullopt);
    }
    this->insertInstantiatedActors();
}

void Scene::clear() {
    this->actorIDMap_.clear();
    this->remoteActorIDMap_.clear();
    this->actors_.clear();
    this->pendingInstantiatedActors_.clear();
}

std::vector<std::unique_ptr<Actor>> &Scene::actors() {
    return this->actors_;
}

const std::vector<std::unique_ptr<Actor>> &Scene::actors() const {
    return this->actors_;
}

const std::string &Scene::name() const {
    return this->name_;
}

Actor* Scene::instantiateActor(bool runtime, const sge::resources::ActorDescription &source,
                               std::optional<client_id_t> ownerClient) {
    // 1. Create new actor instance
    this->pendingInstantiatedActors_.emplace_back(
        std::make_unique<Actor>(this->nextActorId_++, runtime, source));
    auto* newActor = this->pendingInstantiatedActors_.back().get();

    // Set owner client
    newActor->ownerClient = ownerClient;

    // Update local actor id mapping
    this->actorIDMap_.emplace(newActor->id, newActor);

    // If this is a non-runtime actor (i.e. defined in a scene file), we know
    // the local actor id and remote actor id are the same. Register the id.
    //
    // HOWEVER, if we are on the server, remote id == local id. So register
    // the mapping anyway.
    if (!runtime || CurrentRealm() == GeneralRealm::Server) {
        this->registerActorRemoteID(newActor, newActor->id);
    }

    // Return pointer to created actor
    return newActor;
}

Actor* Scene::instantiateRuntimeActor(std::string_view templateName,
                                      std::optional<client_id_t> ownerClient) {
    auto source = resources::ActorDescription{
        std::string{templateName}, // template name
        std::nullopt,              // actor name
        {}                         // components
    };
    return this->instantiateActor(true, source, ownerClient);
}

void Scene::removeDestroyedActors() {
    auto it = std::remove_if(
        this->actors_.begin(), this->actors_.end(), [&](const std::unique_ptr<Actor> &a) {
            assert(a != nullptr);
            if (!a->destroyed) {
                return false;
            }

            // Run OnDestroy lifecycle
            a->onDestroy();

            // First, update any state we need to maintain to no longer contain
            // the actor we are removing.
            this->actorIDMap_.erase(a->id);
            if (a->remoteID.has_value()) {
                this->remoteActorIDMap_.erase(*a->remoteID);
            }

            return true;
        });
    this->actors_.erase(it, this->actors_.end());
}

void Scene::insertInstantiatedActors() {
    for (auto &ptr : this->pendingInstantiatedActors_) {
        this->actors_.emplace_back(std::move(ptr));
    }
    this->pendingInstantiatedActors_.clear();
}

Actor* Scene::findActor(std::string_view name) {
    auto nameCompare = [&](const auto &a) -> bool {
        return a->name == name && !a->destroyed;
    };

    // Check existing actors
    auto it = std::find_if(this->actors_.begin(), this->actors_.end(), nameCompare);
    if (it != this->actors_.end()) {
        return it->get();
    }

    // Check pending instantiation actors
    it = std::find_if(this->pendingInstantiatedActors_.begin(),
                      this->pendingInstantiatedActors_.end(),
                      nameCompare);
    if (it != this->pendingInstantiatedActors_.end()) {
        return it->get();
    }

    return nullptr;
}

std::vector<Actor*> Scene::findAllActors(std::string_view name) {
    std::vector<Actor*> results;
    for (auto &actor : this->actors_) {
        if (actor->name == name && !actor->destroyed) {
            results.push_back(actor.get());
        }
    }
    for (auto &actor : this->pendingInstantiatedActors_) {
        if (actor->name == name && !actor->destroyed) {
            results.push_back(actor.get());
        }
    }
    return results;
}

Actor* Scene::findActorByID(actor_id_t id) {
    auto it = this->actorIDMap_.find(id);
    if (it == this->actorIDMap_.end()) {
        return nullptr;
    }
    return it->second;
}

Actor* Scene::findActorByRemoteID(actor_id_t remoteID) {
    auto it = this->remoteActorIDMap_.find(remoteID);
    if (it == this->remoteActorIDMap_.end()) {
        return nullptr;
    }
    return it->second;
}

void Scene::registerActorRemoteID(Actor* actor, actor_id_t remoteID) {
    if (actor->remoteID.has_value()) {
        // Remove old mapping of remote id
        this->remoteActorIDMap_.erase(*actor->remoteID);
    }

    // Update actor remoteID field
    actor->remoteID.emplace(remoteID);
    // Store mapping for later lookup
    this->remoteActorIDMap_.emplace(remoteID, actor);
}

} // namespace sge::game
