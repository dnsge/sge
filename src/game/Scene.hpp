#pragma once

#include <glm/glm.hpp>

#include "Types.hpp"
#include "game/Actor.hpp"
#include "resources/Resources.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sge::game {

class Scene {
public:
    Scene(const resources::SceneDescription &source);
    Scene(const resources::SceneDescription &source, Scene &oldScene);

    void clear();

    std::vector<std::unique_ptr<Actor>> &actors();
    const std::vector<std::unique_ptr<Actor>> &actors() const;
    const std::string &name() const;

    Actor* instantiateActor(bool runtime, const sge::resources::ActorDescription &source,
                            std::optional<client_id_t> ownerClient);
    Actor* instantiateRuntimeActor(std::string_view templateName,
                                   std::optional<client_id_t> ownerClient);
    void removeDestroyedActors();
    void insertInstantiatedActors();

    Actor* findActor(std::string_view name);
    std::vector<Actor*> findAllActors(std::string_view name);

    Actor* findActorByID(actor_id_t id);
    Actor* findActorByRemoteID(actor_id_t remoteID);
    void registerActorRemoteID(Actor* actor, actor_id_t remoteID);

private:
    std::string name_;
    std::vector<std::unique_ptr<Actor>> actors_;
    std::vector<std::unique_ptr<Actor>> pendingInstantiatedActors_;
    std::unordered_map<actor_id_t, Actor*> actorIDMap_;
    std::unordered_map<actor_id_t, Actor*> remoteActorIDMap_;

    actor_id_t nextActorId_{0};
};

} // namespace sge::game
