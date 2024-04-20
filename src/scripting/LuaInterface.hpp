
#pragma once

#include <glm/glm.hpp>

#include "Types.hpp"
#include "game/Actor.hpp"
#include "physics/Raycast.hpp"
#include "scripting/EventSub.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sge::scripting {

class Component;

class LuaInterface {
public:
    LuaInterface() = default;
    virtual ~LuaInterface() = default;

    virtual void debugLog(const std::string &message) = 0;
    virtual void debugLogError(const std::string &message) = 0;

    virtual void applicationQuit() = 0;
    virtual void applicationSleep(int ms) = 0;
    virtual unsigned int applicationGetFrame() = 0;
    virtual void applicationOpenURL(std::string_view url) = 0;

    virtual bool inputGetKey(std::string_view keycode) = 0;
    virtual bool inputGetKeyDown(std::string_view keycode) = 0;
    virtual bool inputGetKeyUp(std::string_view keycode) = 0;
    virtual glm::vec2 inputGetMousePosition() = 0;
    virtual glm::vec2 inputGetMousePositionScene() = 0;
    virtual bool inputGetMouseButton(int button) = 0;
    virtual bool inputGetMouseButtonDown(int button) = 0;
    virtual bool inputGetMouseButtonUp(int button) = 0;
    virtual float inputGetMouseScrollDelta() = 0;

    virtual game::Actor* actorFind(std::string_view name) = 0;
    virtual std::vector<game::Actor*> actorFindAll(std::string_view name) = 0;
    virtual game::Actor* actorInstantiate(std::string_view templateName,
                                          std::optional<client_id_t> ownerClient) = 0;
    virtual void actorDestroy(game::Actor* actor) = 0;

    virtual void textDraw(std::string_view text, float x, float y, std::string_view fontName,
                          float fontSize, float r, float g, float b, float a) = 0;

    virtual void audioPlay(int channel, std::string_view clipName, bool loop) = 0;
    virtual void audioHalt(int channel) = 0;
    virtual void audioSetVolume(int channel, float volume) = 0;

    virtual void imageDrawUi(std::string_view imageName, float x, float y) = 0;
    virtual void imageDrawUiEx(std::string_view imageName, float x, float y, float r, float g,
                               float b, float a, int sortOrder) = 0;
    virtual void imageDraw(std::string_view imageName, float x, float y) = 0;
    virtual void imageDrawEx(std::string_view imageName, float x, float y, float rotation,
                             float scaleX, float scaleY, float pivotX, float pivotY, float r,
                             float g, float b, float a, int sortOrder) = 0;
    virtual void imageDrawPixel(float x, float y, float r, float g, float b, float a) = 0;

    virtual void cameraSetPosition(float x, float y) = 0;
    virtual float cameraGetPositionX() = 0;
    virtual float cameraGetPositionY() = 0;
    virtual void cameraSetZoom(float zoom) = 0;
    virtual float cameraGetZoom() = 0;

    virtual void sceneLoad(std::string_view name) = 0;
    virtual std::string sceneGetCurrent() = 0;
    virtual void sceneDontDestroy(game::Actor* actor) = 0;

    virtual std::optional<physics::HitResult> physicsRaycast(const b2Vec2 &pos,
                                                             const b2Vec2 &direction,
                                                             float distance) = 0;
    virtual std::vector<physics::HitResult> physicsRaycastAll(const b2Vec2 &pos,
                                                              const b2Vec2 &direction,
                                                              float distance) = 0;

    virtual void eventPublish(std::string_view eventType, const luabridge::LuaRef &value) = 0;
    virtual void eventPublishRemote(std::string_view eventType, const luabridge::LuaRef &value,
                                    bool publishLocally) = 0;
    [[nodiscard]] virtual subscription_handle eventSubscribe(std::string_view event,
                                                             const luabridge::LuaRef &function) = 0;
    virtual void eventUnsubscribe(subscription_handle handle) = 0;

    virtual void multiplayerConnect(std::string_view host, std::string_view port) = 0;
    virtual void multiplayerDisconnect() = 0;
    virtual client_id_t multiplayerClientID() = 0;
    virtual std::vector<client_id_t> multiplayerJoinedClients() = 0;

    virtual void replicatorServiceReplicate(Component* component) = 0;
};

} // namespace sge::scripting
