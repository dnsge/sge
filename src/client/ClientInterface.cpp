#include "client/ClientInterface.hpp"

#include "Client.hpp"
#include "Common.hpp"
#include "Constants.hpp"
#include "Types.hpp"
#include "client/AudioPlayer.hpp"
#include "game/Actor.hpp"
#include "game/Input.hpp"
#include "gea/AudioHelper.h"
#include "gea/Helper.h"
#include "physics/Raycast.hpp"
#include "render/RenderQueue.hpp"
#include "render/Text.hpp"
#include "resources/Resources.hpp"
#include "scripting/Component.hpp"
#include "scripting/EventSub.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32)
#    define START_APPLICATION_STR "start"
#elif defined(__APPLE__)
#    define START_APPLICATION_STR "open"
#else
#    define START_APPLICATION_STR "xdg-open"
#endif

namespace sge::client {

void ClientInterface::debugLog(const std::string &message) {
    std::cout << message << std::endl;
}

void ClientInterface::debugLogError(const std::string &message) {
    std::cerr << message << std::endl;
}

void ClientInterface::applicationQuit() {
    std::exit(0);
}

void ClientInterface::applicationSleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

unsigned int ClientInterface::applicationGetFrame() {
    return Helper::GetFrameNumber();
}

void ClientInterface::applicationOpenURL(std::string_view url) {
    std::stringstream ss;
    ss << START_APPLICATION_STR << " " << url;
    std::system(ss.str().c_str());
}

bool ClientInterface::inputGetKey(std::string_view keycode) {
    return game::Input::getKey(keycode);
}

bool ClientInterface::inputGetKeyDown(std::string_view keycode) {
    return game::Input::getKeyDown(keycode);
}

bool ClientInterface::inputGetKeyUp(std::string_view keycode) {
    return game::Input::getKeyUp(keycode);
}

glm::vec2 ClientInterface::inputGetMousePosition() {
    return glm::vec2{game::Input::mousePosition()};
}

glm::vec2 ClientInterface::inputGetMousePositionScene() {
    auto screenCoords = this->inputGetMousePosition();
    auto screenSize = CurrentClient().config().rendering_config.size();
    return glm::vec2{
        (screenCoords.x - (screenSize.x / 2)) / PixelsPerMeter,
        (screenCoords.y - (screenSize.y / 2)) / PixelsPerMeter,
    };
}

bool ClientInterface::inputGetMouseButton(int button) {
    return game::Input::getMouseButton(button);
}

bool ClientInterface::inputGetMouseButtonDown(int button) {
    return game::Input::getMouseButtonDown(button);
}

bool ClientInterface::inputGetMouseButtonUp(int button) {
    return game::Input::getMouseButtonUp(button);
}

float ClientInterface::inputGetMouseScrollDelta() {
    return game::Input::mouseScroll();
}

game::Actor* ClientInterface::actorFind(std::string_view name) {
    return CurrentScene().findActor(name);
}

std::vector<game::Actor*> ClientInterface::actorFindAll(std::string_view name) {
    return CurrentScene().findAllActors(name);
}

game::Actor* ClientInterface::actorInstantiate(std::string_view templateName,
                                               std::optional<client_id_t> ownerClient) {
    auto owner = ownerClient.value_or(CurrentClient().clientID());
    auto* a = CurrentScene().instantiateRuntimeActor(templateName, owner);
    CurrentClient().replicatorService().instantiate(a);
    return a;
}

void ClientInterface::actorDestroy(game::Actor* actor) {
    actor->destroy();
}

void ClientInterface::textDraw(std::string_view text, float x, float y, std::string_view fontName,
                               float fontSize, float r, float g, float b, float a) {
    auto color = SDL_Color{static_cast<uint8_t>(r),
                           static_cast<uint8_t>(g),
                           static_cast<uint8_t>(b),
                           static_cast<uint8_t>(a)};
    CurrentGame().renderQueue().enqueueText(render::DrawTextArgs{
        render::Text{text, std::string{fontName}, static_cast<int>(fontSize), color},
        SDL_Point{static_cast<int>(x), static_cast<int>(y)},
    });
}

void ClientInterface::audioPlay(int channel, std::string_view clipName, bool loop) {
    PlayAudio(channel, clipName, loop);
}

void ClientInterface::audioHalt(int channel) {
    StopAudio(channel);
}

void ClientInterface::audioSetVolume(int channel, float volume) {
    AudioHelper::Mix_Volume498(channel, static_cast<int>(volume));
}

void ClientInterface::imageDrawUi(std::string_view imageName, float x, float y) {
    CurrentGame().renderQueue().enqueueUi(render::DrawUiArgs{
        resources::GetImage(std::string{imageName}
        ),
        SDL_Point{static_cast<int>(x), static_cast<int>(y)},
    });
}

void ClientInterface::imageDrawUiEx(std::string_view imageName, float x, float y, float r, float g,
                                    float b, float a, int sortOrder) {
    auto color = SDL_Color{static_cast<uint8_t>(r),
                           static_cast<uint8_t>(g),
                           static_cast<uint8_t>(b),
                           static_cast<uint8_t>(a)};
    CurrentGame().renderQueue().enqueueUiEx(
        render::DrawUiExArgs{
            resources::GetImage(std::string{imageName}
            ),
            SDL_Point{static_cast<int>(x), static_cast<int>(y)},
            color,
    },
        sortOrder);
}

void ClientInterface::imageDraw(std::string_view imageName, float x, float y) {
    CurrentGame().renderQueue().enqueueImage(render::DrawImageArgs{
        resources::GetImage(std::string{imageName}),
        x,
        y,
    });
}

void ClientInterface::imageDrawEx(std::string_view imageName, float x, float y, float rotation,
                                  float scaleX, float scaleY, float pivotX, float pivotY, float r,
                                  float g, float b, float a, int sortOrder) {
    auto color = SDL_Color{static_cast<uint8_t>(r),
                           static_cast<uint8_t>(g),
                           static_cast<uint8_t>(b),
                           static_cast<uint8_t>(a)};
    CurrentGame().renderQueue().enqueueImageEx(
        render::DrawImageExArgs{
            resources::GetImage(std::string{imageName}),
            x,
            y,
            static_cast<int>(rotation),
            scaleX,
            scaleY,
            pivotX,
            pivotY,
            color,
        },
        sortOrder);
}

void ClientInterface::imageDrawPixel(float x, float y, float r, float g, float b, float a) {
    CurrentGame().renderQueue().enqueuePixel(render::DrawPixelArgs{
        SDL_Point{static_cast<int>(x), static_cast<int>(y)},
        SDL_Color{static_cast<uint8_t>(r),
                  static_cast<uint8_t>(g),
                  static_cast<uint8_t>(b),
                  static_cast<uint8_t>(a)},
    });
}

void ClientInterface::cameraSetPosition(float x, float y) {
    CurrentGame().setCameraPos(glm::vec2{x, y});
}

float ClientInterface::cameraGetPositionX() {
    return CurrentGame().cameraPos().x;
}

float ClientInterface::cameraGetPositionY() {
    return CurrentGame().cameraPos().y;
}

void ClientInterface::cameraSetZoom(float zoom) {
    CurrentGame().setZoom(zoom);
}

float ClientInterface::cameraGetZoom() {
    return CurrentGame().zoom();
}

void ClientInterface::sceneLoad(std::string_view name) {
    CurrentClient().setNextScene(name);
}

std::string ClientInterface::sceneGetCurrent() {
    return CurrentScene().name();
}

void ClientInterface::sceneDontDestroy(game::Actor* actor) {
    actor->persistent = true;
}

std::optional<physics::HitResult> ClientInterface::physicsRaycast(const b2Vec2 &pos,
                                                                  const b2Vec2 &direction,
                                                                  float distance) {
    return CurrentGame().physicsWorld().raycast(pos, direction, distance);
}

std::vector<physics::HitResult> ClientInterface::physicsRaycastAll(const b2Vec2 &pos,
                                                                   const b2Vec2 &direction,
                                                                   float distance) {
    return CurrentGame().physicsWorld().raycastAll(pos, direction, distance);
}

void ClientInterface::eventPublish(std::string_view eventType, const luabridge::LuaRef &value) {
    CurrentGame().eventSub().publish(eventType, value);
}

void ClientInterface::eventPublishRemote(std::string_view eventType, const luabridge::LuaRef &value,
                                         bool publishLocally) {
    auto replicableValue = value.cast<scripting::LuaValue>();
    CurrentClient().replicatorService().eventPublish(eventType, std::move(replicableValue));
    if (publishLocally) {
        CurrentGame().eventSub().publish(eventType, value);
    }
}

scripting::subscription_handle ClientInterface::eventSubscribe(std::string_view event,
                                                               const luabridge::LuaRef &function) {
    return CurrentGame().eventSub().subscribe(event, function);
}

void ClientInterface::eventUnsubscribe(scripting::subscription_handle handle) {
    CurrentGame().eventSub().unsubscribe(handle);
}

void ClientInterface::multiplayerConnect(std::string_view host, std::string_view port) {
    CurrentClient().connect(host, port);
}

void ClientInterface::multiplayerDisconnect() {
    CurrentClient().disconnect();
}

client_id_t ClientInterface::multiplayerClientID() {
    return CurrentClient().clientID();
}

std::vector<client_id_t> ClientInterface::multiplayerJoinedClients() {
    return CurrentClient().joinedClients();
}

void ClientInterface::replicatorServiceReplicate(scripting::Component* component) {
    CurrentClient().replicatorService().replicate(component);
}

} // namespace sge::client
