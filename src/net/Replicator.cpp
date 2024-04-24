#include "net/Replicator.hpp"

#include "Realm.hpp"
#include "Types.hpp"
#include "game/Actor.hpp"
#include "game/Game.hpp"
#include "scripting/Component.hpp"
#include "scripting/Invoke.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <msgpack.hpp>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sge::net {

namespace {

void replicateComponent(ReplicatePush &pusher, scripting::Component* component,
                        std::vector<ComponentReplication> &out) {
    auto id =
        component->actor->remoteID.has_value() ? *component->actor->remoteID : component->actor->id;
    // 1. Pack component representation into pusher
    pusher.clear();
    bool ok = scripting::ActorInvoke(component->actor->name, [&] {
        component->replicatePush(pusher);
    });
    if (!ok) {
        return;
    }

    // 2. View into data of pusher
    auto packedData = pusher.data();

    // 3. Create replication request, copying data of pusher
    out.emplace_back(id, component->key, std::vector<char>{packedData.begin(), packedData.end()});
}

void replicateComponent(ReplicatePush &pusher, scripting::Component* component,
                        std::vector<InstantiatedActorComponentState> &out) {
    // 1. Pack component representation into pusher
    pusher.clear();
    bool ok = scripting::ActorInvoke(component->actor->name, [&] {
        component->replicatePush(pusher);
    });
    if (!ok) {
        return;
    }

    // 2. View into data of pusher
    auto packedData = pusher.data();

    // 3. Create replication request, copying data of pusher
    out.emplace_back(component->key, std::vector<char>{packedData.begin(), packedData.end()});
}

void dispatchComponentReplication(game::Actor* actor, const std::string &componentKey,
                                  const std::vector<char> &packed, bool doInterp) {
    auto* component = actor->getComponentByKey(componentKey);
    if (component == nullptr) {
        return;
    }

    // 3. Call ReplicatePull
    auto data = std::span<const char>{packed.data(), packed.size()};
    auto puller = ReplicatePull{data, doInterp};
    component->replicatePull(puller);
}

} // namespace

ComponentReplication::ComponentReplication(actor_id_t actorID, std::string componentKey,
                                           std::vector<char> &&packed)
    : actorID(actorID)
    , componentKey(std::move(componentKey))
    , packed(std::move(packed)) {}

RuntimeActor::RuntimeActor(std::string actorTemplate, actor_id_t id,
                           std::optional<client_id_t> owner)
    : actorTemplate(std::move(actorTemplate))
    , id(id)
    , owner(owner) {}

InstantiatedActorComponentState::InstantiatedActorComponentState(std::string componentKey,
                                                                 std::vector<char> &&packed)
    : componentKey(std::move(componentKey))
    , packed(std::move(packed)) {}

InstantiatedActor::InstantiatedActor(std::string actorTemplate, actor_id_t id,
                                     std::optional<client_id_t> owner,
                                     std::vector<InstantiatedActorComponentState> &&componentState)

    : actorTemplate(std::move(actorTemplate))
    , id(id)
    , owner(owner)
    , componentState(std::move(componentState)) {}

RemoteIDMapping::RemoteIDMapping(client_id_t clientID, client_id_t serverID)
    : clientID(clientID)
    , serverID(serverID) {}

EventPublish::EventPublish(std::string event, scripting::LuaValue value)
    : event(std::move(event))
    , value(std::move(value)) {}

EventPublish::EventPublish(std::string_view event, scripting::LuaValue value)
    : event(event)
    , value(std::move(value)) {}

//-----------------------------------------------------------------------------
// ReplicatePush

ReplicatePush::Internal::Internal()
    : packer(this->buf) {}

ReplicatePush::ReplicatePush()
    : wrapper_(std::make_shared<ReplicatePush::Internal>()) {}

void ReplicatePush::writeInt(int i) {
    this->wrapper_->packer.pack_int(i);
}

void ReplicatePush::writeNumber(float n) {
    this->wrapper_->packer.pack_float(n);
}

void ReplicatePush::writeBool(bool b) {
    if (b) {
        this->wrapper_->packer.pack_true();
    } else {
        this->wrapper_->packer.pack_false();
    }
}

void ReplicatePush::writeString(std::string_view s) {
    this->wrapper_->packer.pack_str(s.length());
    this->wrapper_->packer.pack_str_body(s.data(), s.length());
}

void ReplicatePush::beginArray(int size) {
    if (size < 0) {
        throw std::runtime_error("invalid size < 0");
    }
    this->wrapper_->packer.pack_array(static_cast<uint32_t>(size));
}

void ReplicatePush::beginMap(int size) {
    if (size < 0) {
        throw std::runtime_error("invalid size < 0");
    }
    this->wrapper_->packer.pack_map(static_cast<uint32_t>(size));
}

std::span<const char> ReplicatePush::data() const {
    return std::span<const char>{this->wrapper_->buf.data(), this->wrapper_->buf.size()};
}

void ReplicatePush::clear() {
    this->wrapper_->buf.clear();
}

//-----------------------------------------------------------------------------
// ReplicatePull

ReplicatePull::ReplicatePull(std::span<const char> data, bool doInterp)
    : data_(data)
    , doInterp_(doInterp) {}

int ReplicatePull::readInt() {
    auto handle = msgpack::unpack(this->data_.data(), this->data_.size(), this->offset_);
    return handle.get().as<int>();
}

float ReplicatePull::readNumber() {
    auto handle = msgpack::unpack(this->data_.data(), this->data_.size(), this->offset_);
    return handle.get().as<float>();
}

bool ReplicatePull::readBool() {
    auto handle = msgpack::unpack(this->data_.data(), this->data_.size(), this->offset_);
    return handle.get().as<bool>();
}

std::string ReplicatePull::readString() {
    std::string s;
    auto handle = msgpack::unpack(this->data_.data(), this->data_.size(), this->offset_);
    handle.get().convert(s);
    return s;
}

int ReplicatePull::readArray() {
    auto handle = msgpack::unpack(this->data_.data(), this->data_.size(), this->offset_);
    return static_cast<int>(handle.get().via.array.size);
}

int ReplicatePull::readMap() {
    auto handle = msgpack::unpack(this->data_.data(), this->data_.size(), this->offset_);
    return static_cast<int>(handle.get().via.map.size);
}

bool ReplicatePull::doInterp() const {
    return this->doInterp_;
}

//-----------------------------------------------------------------------------
// ReplicatorService

void ReplicatorService::instantiate(game::Actor* actor) {
    assert(actor->runtime());
    this->toInstantiate_.push_back(actor);
}

void ReplicatorService::replicate(scripting::Component* component) {
    if (component->realm != Realm::ServerReplicated) {
        throw std::runtime_error("tried to replicate non-server_replicated component");
    } else if (component->actor->pendingServerDestroy()) {
        // Ignore replications if the actor has already been destroyed on the server
        return;
    }
    this->toReplicate_.insert(component);
}

void ReplicatorService::destroy(game::Actor* actor) {
    auto idToDestroy = actor->remoteID.value_or(actor->id);
    this->toDestroy_.emplace_back(idToDestroy);
    // Cancel any pending replications of components that are getting destroyed
    this->erasePendingReplications(actor);
}

void ReplicatorService::eventPublish(std::string_view event, scripting::LuaValue value) {
    this->toPublish_.emplace_back(event, std::move(value));
}

std::vector<ComponentReplication> ReplicatorService::replicateGame(game::Game &game) {
    std::vector<ComponentReplication> res;
    for (const auto &actor : game.currentScene().actors()) {
        for (const auto &componentEntry : actor->components) {
            if (componentEntry.second->realm != Realm::ServerReplicated) {
                continue;
            }
            replicateComponent(this->pusher_, componentEntry.second.get(), res);
        }
    }
    return res;
}

bool ReplicatorService::hasPendingReplications() const {
    return !this->toInstantiate_.empty() || !this->toReplicate_.empty() ||
           !this->toDestroy_.empty();
}

std::vector<InstantiatedActor> ReplicatorService::serializeInstantiations() {
    if (this->toInstantiate_.empty()) {
        return {};
    }

    std::vector<InstantiatedActor> res;
    res.reserve(this->toInstantiate_.size());

    for (auto* a : this->toInstantiate_) {
        std::vector<InstantiatedActorComponentState> cs;
        for (const auto &componentEntry : a->components) {
            if (componentEntry.second->realm != Realm::ServerReplicated) {
                continue;
            }
            replicateComponent(this->pusher_, componentEntry.second.get(), cs);
        }
        res.emplace_back(a->runtimeTemplate(), a->id, a->ownerClient, std::move(cs));
    }

    // Instantiations have been consumed
    this->toInstantiate_.clear();
    return res;
}

std::vector<ComponentReplication> ReplicatorService::serializeComponents() {
    if (this->toReplicate_.empty()) {
        return {};
    }

    std::vector<ComponentReplication> replications;
    replications.reserve(this->toReplicate_.size());

    for (auto it = this->toReplicate_.cbegin(); it != this->toReplicate_.cend();) {
        auto* component = *it;
        // Only replicate components if the actor has a remote id (i.e. the
        // instantiation has been acknowledged).
        if (component->actor->remoteID.has_value() || CurrentRealm() == GeneralRealm::Server)
            [[likely]] {
            replicateComponent(this->pusher_, component, replications);
            it = this->toReplicate_.erase(it);
        } else {
            ++it;
        }
    }

    return replications;
}

std::vector<actor_id_t> ReplicatorService::serializeDestructions() {
    std::vector<actor_id_t> res;
    std::swap(res, this->toDestroy_);
    return res;
}

bool ReplicatorService::hasPendingEventPublishes() const {
    return !this->toPublish_.empty();
}

std::vector<EventPublish> ReplicatorService::serializeEventPublishes() {
    std::vector<EventPublish> res;
    std::swap(res, this->toPublish_);
    return res;
}

void ReplicatorService::erasePendingReplications(game::Actor* actor) {
    // Un-replicate any components on the actor
    for (const auto &entry : actor->components) {
        if (entry.second->realm == Realm::ServerReplicated) {
            this->toReplicate_.erase(entry.second.get());
        }
    }
    // Un-instantiate the actor
    std::erase(this->toInstantiate_, actor);
}

void ReplicatorService::clear() {
    this->toInstantiate_.clear();
    this->toReplicate_.clear();
    this->toDestroy_.clear();
    this->toPublish_.clear();
}

void ReplicatorService::dispatchReplication(
    game::Actor* actor, const std::vector<InstantiatedActorComponentState> &componentState) {
    for (const auto &state : componentState) {
        // Execute the replication without interp (instantiation)
        dispatchComponentReplication(actor, state.componentKey, state.packed, false);
    }
}

void ReplicatorService::dispatchReplication(game::Game &game,
                                            const ComponentReplication &replication,
                                            bool doInterp) {
    // 1. Locate target actor containing component
    auto* actor = game.currentScene().findActorByRemoteID(replication.actorID);
    if (actor == nullptr) {
        // On the server, remote id = local id so remote id is not registered.
        if (CurrentRealm() == GeneralRealm::Server) {
            actor = game.currentScene().findActorByID(replication.actorID);
            if (actor == nullptr) {
                return;
            }
        } else {
            return;
        }
    }

    // 2. Execute the replication on the component within the found actor
    dispatchComponentReplication(actor, replication.componentKey, replication.packed, doInterp);
}

std::vector<RuntimeActor> ReplicatorService::replicateRuntimeActors(const game::Game &game) {
    std::vector<RuntimeActor> res;
    for (const auto &actor : game.currentScene().actors()) {
        if (!actor->runtime()) {
            continue;
        }
        res.emplace_back(actor->runtimeTemplate(), actor->id, actor->ownerClient);
    }
    return res;
}

} // namespace sge::net
