#include "server/ServerInterface.hpp"

#include "Common.hpp"
#include "Types.hpp"
#include "game/Actor.hpp"
#include "physics/Raycast.hpp"
#include "scripting/Component.hpp"
#include "scripting/EventSub.hpp"
#include "server/Server.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sge::server {

void ServerInterface::debugLog(const std::string &message) {
    std::cout << message << std::endl;
}

void ServerInterface::debugLogError(const std::string &message) {
    std::cerr << message << std::endl;
}

void ServerInterface::applicationQuit() {}

void ServerInterface::applicationSleep(int ms) {}

unsigned int ServerInterface::applicationGetFrame() {
    return CurrentServer().tickNum();
}

void ServerInterface::applicationOpenURL(std::string_view url) {}

bool ServerInterface::inputGetKey(std::string_view keycode) {
    return false;
}

bool ServerInterface::inputGetKeyDown(std::string_view keycode) {
    return false;
}

bool ServerInterface::inputGetKeyUp(std::string_view keycode) {
    return false;
}

glm::vec2 ServerInterface::inputGetMousePosition() {
    return glm::vec2{};
}

glm::vec2 ServerInterface::inputGetMousePositionScene() {
    return glm::vec2{};
}

bool ServerInterface::inputGetMouseButton(int button) {
    return false;
}

bool ServerInterface::inputGetMouseButtonDown(int button) {
    return false;
}

bool ServerInterface::inputGetMouseButtonUp(int button) {
    return false;
}

float ServerInterface::inputGetMouseScrollDelta() {
    return 0.0F;
}

game::Actor* ServerInterface::actorFind(std::string_view name) {
    return CurrentScene().findActor(name);
}

std::vector<game::Actor*> ServerInterface::actorFindAll(std::string_view name) {
    return CurrentScene().findAllActors(name);
}

game::Actor* ServerInterface::actorInstantiate(std::string_view templateName,
                                               std::optional<client_id_t> ownerClient) {
    auto owner = ownerClient.value_or(0);
    auto* a = CurrentScene().instantiateRuntimeActor(templateName, owner);
    CurrentServer().replicatorService().instantiate(a);
    return a;
}

void ServerInterface::actorDestroy(game::Actor* actor) {
    actor->destroy();
}

void ServerInterface::textDraw(std::string_view text, float x, float y, std::string_view fontName,
                               float fontSize, float r, float g, float b, float a) {}

void ServerInterface::audioPlay(int channel, std::string_view clipName, bool loop) {}

void ServerInterface::audioHalt(int channel) {}

void ServerInterface::audioSetVolume(int channel, float volume) {}

void ServerInterface::imageDrawUi(std::string_view imageName, float x, float y) {}

void ServerInterface::imageDrawUiEx(std::string_view imageName, float x, float y, float r, float g,
                                    float b, float a, int sortOrder) {}

void ServerInterface::imageDraw(std::string_view imageName, float x, float y) {}

void ServerInterface::imageDrawEx(std::string_view imageName, float x, float y, float rotation,
                                  float scaleX, float scaleY, float pivotX, float pivotY, float r,
                                  float g, float b, float a, int sortOrder) {}

void ServerInterface::imageDrawPixel(float x, float y, float r, float g, float b, float a) {}

void ServerInterface::cameraSetPosition(float x, float y) {}

float ServerInterface::cameraGetPositionX() {
    return 0.0F;
}

float ServerInterface::cameraGetPositionY() {
    return 0.0F;
}

void ServerInterface::cameraSetZoom(float zoom) {}

float ServerInterface::cameraGetZoom() {
    return 1.0F;
}

void ServerInterface::sceneLoad(std::string_view name) {
    CurrentServer().setNextScene(name);
}

std::string ServerInterface::sceneGetCurrent() {
    return CurrentScene().name();
}

void ServerInterface::sceneDontDestroy(game::Actor* actor) {
    actor->persistent = true;
}

std::optional<physics::HitResult> ServerInterface::physicsRaycast(const b2Vec2 &pos,
                                                                  const b2Vec2 &direction,
                                                                  float distance) {
    return CurrentGame().physicsWorld().raycast(pos, direction, distance);
}

std::vector<physics::HitResult> ServerInterface::physicsRaycastAll(const b2Vec2 &pos,
                                                                   const b2Vec2 &direction,
                                                                   float distance) {
    return CurrentGame().physicsWorld().raycastAll(pos, direction, distance);
}

void ServerInterface::eventPublish(std::string_view eventType, const luabridge::LuaRef &value) {
    CurrentGame().eventSub().publish(eventType, value);
}

void ServerInterface::eventPublishRemote(std::string_view eventType, const luabridge::LuaRef &value,
                                         bool publishLocally) {
    auto replicableValue = value.cast<scripting::LuaValue>();
    CurrentServer().replicatorService().eventPublish(eventType, std::move(replicableValue));
    if (publishLocally) {
        CurrentGame().eventSub().publish(eventType, value);
    }
}

scripting::subscription_handle ServerInterface::eventSubscribe(std::string_view event,
                                                               const luabridge::LuaRef &function) {
    return CurrentGame().eventSub().subscribe(event, function);
}

void ServerInterface::eventUnsubscribe(scripting::subscription_handle handle) {
    CurrentGame().eventSub().unsubscribe(handle);
}

void ServerInterface::multiplayerConnect(std::string_view host, std::string_view port) {}

void ServerInterface::multiplayerDisconnect() {}

client_id_t ServerInterface::multiplayerClientID() {
    return 0;
}

std::vector<client_id_t> ServerInterface::multiplayerJoinedClients() {
    return CurrentServer().joinedClients();
}

void ServerInterface::replicatorServiceReplicate(scripting::Component* component) {
    CurrentServer().replicatorService().replicate(component);
}

} // namespace sge::server
