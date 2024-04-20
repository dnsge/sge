#pragma once

#include <glm/glm.hpp>

#include "Types.hpp"
#include "game/Actor.hpp"
#include "physics/Raycast.hpp"
#include "scripting/EventSub.hpp"
#include "scripting/LuaInterface.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sge::client {

class ClientInterface : public scripting::LuaInterface {
public:
    ClientInterface() = default;
    ~ClientInterface() override = default;

    void debugLog(const std::string &message) override;
    void debugLogError(const std::string &message) override;

    void applicationQuit() override;
    void applicationSleep(int ms) override;
    unsigned int applicationGetFrame() override;
    void applicationOpenURL(std::string_view url) override;

    bool inputGetKey(std::string_view keycode) override;
    bool inputGetKeyDown(std::string_view keycode) override;
    bool inputGetKeyUp(std::string_view keycode) override;
    glm::vec2 inputGetMousePosition() override;
    glm::vec2 inputGetMousePositionScene() override;
    bool inputGetMouseButton(int button) override;
    bool inputGetMouseButtonDown(int button) override;
    bool inputGetMouseButtonUp(int button) override;
    float inputGetMouseScrollDelta() override;

    game::Actor* actorFind(std::string_view name) override;
    std::vector<game::Actor*> actorFindAll(std::string_view name) override;
    game::Actor* actorInstantiate(std::string_view templateName,
                                  std::optional<client_id_t> ownerClient) override;
    void actorDestroy(game::Actor* actor) override;

    void textDraw(std::string_view text, float x, float y, std::string_view fontName,
                  float fontSize, float r, float g, float b, float a) override;

    void audioPlay(int channel, std::string_view clipName, bool loop) override;
    void audioHalt(int channel) override;
    void audioSetVolume(int channel, float volume) override;

    void imageDrawUi(std::string_view imageName, float x, float y) override;
    void imageDrawUiEx(std::string_view imageName, float x, float y, float r, float g, float b,
                       float a, int sortOrder) override;
    void imageDraw(std::string_view imageName, float x, float y) override;
    void imageDrawEx(std::string_view imageName, float x, float y, float rotation, float scaleX,
                     float scaleY, float pivotX, float pivotY, float r, float g, float b, float a,
                     int sortOrder) override;
    void imageDrawPixel(float x, float y, float r, float g, float b, float a) override;

    void cameraSetPosition(float x, float y) override;
    float cameraGetPositionX() override;
    float cameraGetPositionY() override;
    void cameraSetZoom(float zoom) override;
    float cameraGetZoom() override;

    void sceneLoad(std::string_view name) override;
    std::string sceneGetCurrent() override;
    void sceneDontDestroy(game::Actor* actor) override;

    std::optional<physics::HitResult> physicsRaycast(const b2Vec2 &pos, const b2Vec2 &direction,
                                                     float distance) override;
    std::vector<physics::HitResult> physicsRaycastAll(const b2Vec2 &pos, const b2Vec2 &direction,
                                                      float distance) override;

    void eventPublish(std::string_view eventType, const luabridge::LuaRef &value) override;
    void eventPublishRemote(std::string_view eventType, const luabridge::LuaRef &value,
                            bool publishLocally) override;
    [[nodiscard]] scripting::subscription_handle eventSubscribe(
        std::string_view event, const luabridge::LuaRef &function) override;
    void eventUnsubscribe(scripting::subscription_handle handle) override;

    void multiplayerConnect(std::string_view host, std::string_view port) override;
    void multiplayerDisconnect() override;
    client_id_t multiplayerClientID() override;
    std::vector<client_id_t> multiplayerJoinedClients() override;

    void replicatorServiceReplicate(scripting::Component* component) override;
};

} // namespace sge::client
