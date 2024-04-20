#pragma once

#include <msgpack/adaptor/cpp17/variant.hpp>

#include "Types.hpp"
#include "net/Packing.hpp" // IWYU pragma: keep
#include "scripting/LuaValue.hpp"

#include <cstddef>
#include <memory>
#include <msgpack.hpp>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sge {

namespace scripting {
class Component;
} // namespace scripting

namespace game {
class Game;
class Actor;
} // namespace game

namespace net {

struct ComponentReplication {
    actor_id_t actorID;
    std::string componentKey;
    std::vector<char> packed;

    ComponentReplication() = default;
    ComponentReplication(actor_id_t actorID, std::string componentKey, std::vector<char> &&packed);

    MSGPACK_DEFINE(actorID, componentKey, packed);
};

struct RuntimeActor {
    std::string actorTemplate;
    actor_id_t id;
    std::optional<client_id_t> owner;

    RuntimeActor() = default;
    RuntimeActor(std::string actorTemplate, actor_id_t id, std::optional<client_id_t> owner);

    MSGPACK_DEFINE(actorTemplate, id, owner);
};

struct InstantiatedActorComponentState {
    std::string componentKey;
    std::vector<char> packed;

    InstantiatedActorComponentState() = default;
    InstantiatedActorComponentState(std::string componentKey, std::vector<char> &&packed);

    MSGPACK_DEFINE(componentKey, packed);
};

struct InstantiatedActor {
    std::string actorTemplate;
    actor_id_t id;
    std::optional<client_id_t> owner;
    std::vector<InstantiatedActorComponentState> componentState;

    InstantiatedActor() = default;
    InstantiatedActor(std::string actorTemplate, actor_id_t id, std::optional<client_id_t> owner,
                      std::vector<InstantiatedActorComponentState> &&componentState);

    MSGPACK_DEFINE(actorTemplate, id, owner, componentState);
};

struct RemoteIDMapping {
    client_id_t clientID;
    client_id_t serverID;

    RemoteIDMapping() = default;
    RemoteIDMapping(client_id_t clientID, client_id_t serverID);

    MSGPACK_DEFINE(clientID, serverID);
};

struct EventPublish {
    std::string event;
    scripting::LuaValue value;

    EventPublish() = default;
    EventPublish(std::string event, scripting::LuaValue value);
    EventPublish(std::string_view event, scripting::LuaValue value);

    MSGPACK_DEFINE(event, value);
};

class ReplicatePush {
public:
    ReplicatePush();

    void writeInt(int i);
    void writeNumber(float n);
    void writeBool(bool b);
    void writeString(std::string_view s);

    void beginArray(int size);
    void beginMap(int size);

    std::span<const char> data() const;
    void clear();

private:
    struct Internal {
        msgpack::sbuffer buf;
        msgpack::packer<msgpack::sbuffer> packer;

        Internal();
    };

    std::shared_ptr<Internal> wrapper_;
};

class ReplicatePull {
public:
    ReplicatePull(std::span<const char> data, bool doInterp);

    int readInt();
    float readNumber();
    bool readBool();
    std::string readString();

    int readArray();
    int readMap();

    bool doInterp() const;

private:
    std::span<const char> data_;
    bool doInterp_;
    std::size_t offset_{0};
};

class ReplicatorService {
public:
    ReplicatorService() = default;

    void instantiate(game::Actor* actor);
    void replicate(scripting::Component* component);
    void destroy(game::Actor* actor);
    void eventPublish(std::string_view event, scripting::LuaValue value);
    std::vector<ComponentReplication> replicateGame(game::Game &game);

    bool hasPendingReplications() const;
    std::vector<InstantiatedActor> serializeInstantiations();
    std::vector<ComponentReplication> serializeComponents();
    std::vector<actor_id_t> serializeDestructions();

    bool hasPendingEventPublishes() const;
    std::vector<EventPublish> serializeEventPublishes();

    void erasePendingReplications(game::Actor* actor);
    void clear();

    static void dispatchReplication(
        game::Actor* actor, const std::vector<InstantiatedActorComponentState> &componentState);
    static void dispatchReplication(game::Game &game, const ComponentReplication &replication,
                                    bool doInterp);
    static std::vector<RuntimeActor> replicateRuntimeActors(const game::Game &game);

private:
    std::vector<game::Actor*> toInstantiate_;
    std::set<scripting::Component*> toReplicate_;
    std::vector<actor_id_t> toDestroy_;
    std::vector<EventPublish> toPublish_;

    ReplicatePush pusher_;
};
} // namespace net

} // namespace sge
