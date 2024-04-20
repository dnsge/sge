#include "scripting/Libs.hpp"

#include <box2d/box2d.h>
#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <LuaBridge/Optional.h>
#include <LuaBridge/Vector.h>

#include "Constants.hpp"
#include "Types.hpp"
#include "game/Actor.hpp"
#include "net/Replicator.hpp"
#include "physics/Collision.hpp"
#include "physics/Raycast.hpp"
#include "physics/Rigidbody.hpp"
#include "scripting/Component.hpp"
#include "scripting/EventSub.hpp"
#include "scripting/Scripting.hpp"
#include "scripting/Tracing.hpp"
#include "scripting/components/CppComponent.hpp"
#include "scripting/components/InterpTransform.hpp"
#include "scripting/components/Transform.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sge::scripting {

namespace libs {

std::unique_ptr<LuaInterface> Interface = nullptr;

void DebugLog(const std::string &message) {
    TRACE_EVENT("Debug.Log");
    Interface->debugLog(message);
}

void DebugLogError(const std::string &message) {
    TRACE_EVENT("Debug.LogError");
    Interface->debugLogError(message);
}

void ApplicationQuit() {
    Interface->applicationQuit();
}

void ApplicationSleep(int ms) {
    TRACE_EVENT("Application.Sleep");
    Interface->applicationSleep(ms);
}

int ApplicationGetFrame() {
    TRACE_EVENT("Application.GetFrame");
    return Interface->applicationGetFrame();
}

void ApplicationOpenURL(std::string_view url) {
    TRACE_EVENT("Application.OpenURL");
    Interface->applicationOpenURL(url);
}

bool InputGetKey(std::string_view keycode) {
    TRACE_EVENT("Input.GetKey");
    return Interface->inputGetKey(keycode);
}

bool InputGetKeyDown(std::string_view keycode) {
    TRACE_EVENT("Input.GetKeyDown");
    return Interface->inputGetKeyDown(keycode);
}

bool InputGetKeyUp(std::string_view keycode) {
    TRACE_EVENT("Input.GetKeyUp");
    return Interface->inputGetKeyUp(keycode);
}

glm::vec2 InputGetMousePosition() {
    TRACE_EVENT("Input.GetMousePosition");
    return Interface->inputGetMousePosition();
}

glm::vec2 InputGetMousePositionScene() {
    TRACE_EVENT("Input.GetMousePositionScene");
    return Interface->inputGetMousePositionScene();
}

bool InputGetMouseButton(int button) {
    TRACE_EVENT("Input.GetMouseButton");
    return Interface->inputGetMouseButton(button);
}

bool InputGetMouseButtonDown(int button) {
    TRACE_EVENT("Input.GetMouseButtonDown");
    return Interface->inputGetMouseButtonDown(button);
}

bool InputGetMouseButtonUp(int button) {
    TRACE_EVENT("Input.GetMouseButtonUp");
    return Interface->inputGetMouseButtonUp(button);
}

float InputGetMouseScrollDelta() {
    TRACE_EVENT("Input.GetMouseScrollDelta");
    return Interface->inputGetMouseScrollDelta();
}

game::Actor* ActorFind(std::string_view name) {
    TRACE_EVENT("Actor.Find");
    return Interface->actorFind(name);
}

std::vector<game::Actor*> ActorFindAll(std::string_view name) {
    TRACE_EVENT("Actor.FindAll");
    return Interface->actorFindAll(name);
}

game::Actor* ActorInstantiate(std::string_view templateName) {
    TRACE_EVENT("Actor.Instantiate");
    return Interface->actorInstantiate(templateName, std::nullopt);
}

game::Actor* ActorInstantiateOwned(std::string_view templateName, client_id_t ownerClient) {
    TRACE_EVENT("Actor.InstantiateOwned");
    return Interface->actorInstantiate(templateName, ownerClient);
}

void ActorDestroy(game::Actor* actor) {
    TRACE_EVENT("Actor.Destroy");
    Interface->actorDestroy(actor);
}

void TextDraw(std::string_view text, float x, float y, std::string_view fontName, float fontSize,
              float r, float g, float b, float a) {
    TRACE_EVENT("Text.Draw");
    Interface->textDraw(text, x, y, fontName, fontSize, r, g, b, a);
}

void AudioPlay(int channel, std::string_view clipName, bool loop) {
    TRACE_EVENT("Audio.Play");
    Interface->audioPlay(channel, clipName, loop);
}

void AudioHalt(int channel) {
    TRACE_EVENT("Audio.Halt");
    Interface->audioHalt(channel);
}

void AudioSetVolume(int channel, float volume) {
    TRACE_EVENT("Audio.SetVolume");
    Interface->audioSetVolume(channel, volume);
}

void ImageDrawUI(std::string_view imageName, float x, float y) {
    TRACE_EVENT("Image.DrawUI");
    Interface->imageDrawUi(imageName, x, y);
}

void ImageDrawUIEx(std::string_view imageName, float x, float y, float r, float g, float b, float a,
                   int sortOrder) {
    TRACE_EVENT("Image.DrawUIEx");
    Interface->imageDrawUiEx(imageName, x, y, r, g, b, a, sortOrder);
}

void ImageDraw(std::string_view imageName, float x, float y) {
    TRACE_EVENT("Image.Draw");
    Interface->imageDraw(imageName, x, y);
}

void ImageDrawEx(std::string_view imageName, float x, float y, float rotation, float scaleX,
                 float scaleY, float pivotX, float pivotY, float r, float g, float b, float a,
                 int sortOrder) {
    TRACE_EVENT("Image.DrawEx");
    Interface->imageDrawEx(
        imageName, x, y, rotation, scaleX, scaleY, pivotX, pivotY, r, g, b, a, sortOrder);
}

void ImageDrawPixel(float x, float y, float r, float g, float b, float a) {
    TRACE_EVENT("Image.DrawPixel");
    Interface->imageDrawPixel(x, y, r, g, b, a);
}

void CameraSetPosition(float x, float y) {
    TRACE_EVENT("Camera.SetPosition");
    Interface->cameraSetPosition(x, y);
}

float CameraGetPositionX() {
    TRACE_EVENT("Camera.GetPositionX");
    return Interface->cameraGetPositionX();
}

float CameraGetPositionY() {
    TRACE_EVENT("Camera.GetPositionY");
    return Interface->cameraGetPositionY();
}

void CameraSetZoom(float zoom) {
    TRACE_EVENT("Camera.SetZoom");
    Interface->cameraSetZoom(zoom);
}

float CameraGetZoom() {
    TRACE_EVENT("Camera.GetZoom");
    return Interface->cameraGetZoom();
}

void SceneLoad(std::string_view name) {
    TRACE_EVENT("Scene.Load");
    Interface->sceneLoad(name);
}

std::string SceneGetCurrent() {
    TRACE_EVENT("Scene.GetCurrent");
    return Interface->sceneGetCurrent();
}

void SceneDontDestroy(game::Actor* actor) {
    TRACE_EVENT("Scene.DontDestroy");
    Interface->sceneDontDestroy(actor);
}

auto PhysicsRaycast(const b2Vec2 &pos, const b2Vec2 &direction, float distance) {
    TRACE_EVENT("Physics.Raycast");
    return Interface->physicsRaycast(pos, direction, distance);
}

auto PhysicsRaycastAll(const b2Vec2 &pos, const b2Vec2 &direction, float distance) {
    TRACE_EVENT("Physics.RaycastAll");
    return Interface->physicsRaycastAll(pos, direction, distance);
}

void EventPublish(const std::string &eventType, const luabridge::LuaRef &eventObject) {
    TRACE_EVENT("Event.Publish");
    Interface->eventPublish(eventType, eventObject);
}

void EventPublishRemote(const std::string &eventType, const luabridge::LuaRef &eventObject,
                        bool publishLocally) {
    TRACE_EVENT("Event.PublishRemote");
    Interface->eventPublishRemote(eventType, eventObject, publishLocally);
}

subscription_handle EventSubscribe(const std::string &event, const luabridge::LuaRef &function) {
    TRACE_EVENT("Event.Subscribe");
    return Interface->eventSubscribe(event, function);
}

void EventUnsubscribe(subscription_handle handle) {
    TRACE_EVENT("Event.Subscribe");
    Interface->eventUnsubscribe(handle);
}

void MultiplayerConnect(std::string_view host, std::string_view port) {
    TRACE_EVENT("Multiplayer.Connect");
    Interface->multiplayerConnect(host, port);
}

void MultiplayerDisconnect() {
    TRACE_EVENT("Multiplayer.Disconnect");
    Interface->multiplayerDisconnect();
}

client_id_t MultiplayerClientID() {
    TRACE_EVENT("Multiplayer.ClientID");
    return Interface->multiplayerClientID();
}

std::vector<client_id_t> MultiplayerJoinedClients() {
    TRACE_EVENT("Multiplayer.JoinedClients");
    return Interface->multiplayerJoinedClients();
}

subscription_handle MultiplayerOnClientJoin(const luabridge::LuaRef &function) {
    TRACE_EVENT("MultiplayerOnClientJoin");
    return Interface->eventSubscribe(events::MultiplayerOnClientJoin, function);
}

subscription_handle MultiplayerOnClientLeave(const luabridge::LuaRef &function) {
    TRACE_EVENT("MultiplayerOnClientLeave");
    return Interface->eventSubscribe(events::MultiplayerOnClientLeave, function);
}

void ReplicatorServiceReplicate(Component* component) {
    TRACE_EVENT("ReplicatorService.Replicate");
    Interface->replicatorServiceReplicate(component);
}

} // namespace libs

void InitializeScriptingLibs() {
    auto* state = GetGlobalState();
    // clang-format off
    luabridge::getGlobalNamespace(state)
        .beginNamespace("Debug")
            // No clue why compiler can't deduce template parameters
            .addFunction<void, const std::string&>("Log", &libs::DebugLog)
            .addFunction<void, const std::string&>("LogError", &libs::DebugLogError)
        .endNamespace()
        .beginNamespace("Application")
            .addFunction("Quit", &libs::ApplicationQuit)
            .addFunction("Sleep", &libs::ApplicationSleep)
            .addFunction("GetFrame", &libs::ApplicationGetFrame)
            .addFunction("OpenURL", &libs::ApplicationOpenURL)
        .endNamespace()
        .beginNamespace("Input")
            .addFunction("GetKey", &libs::InputGetKey)
            .addFunction("GetKeyDown", &libs::InputGetKeyDown)
            .addFunction("GetKeyUp", &libs::InputGetKeyUp)
            .addFunction("GetMousePosition", &libs::InputGetMousePosition)
            .addFunction("GetMousePositionScene", &libs::InputGetMousePositionScene)
            .addFunction("GetMouseButton", &libs::InputGetMouseButton)
            .addFunction("GetMouseButtonDown", &libs::InputGetMouseButtonDown)
            .addFunction("GetMouseButtonUp", &libs::InputGetMouseButtonUp)
            .addFunction("GetMouseScrollDelta", &libs::InputGetMouseScrollDelta)
        .endNamespace()
        .beginNamespace("Actor")
            .addFunction("Find", &libs::ActorFind)
            .addFunction("FindAll", &libs::ActorFindAll)
            .addFunction("Instantiate", &libs::ActorInstantiate)
            .addFunction("InstantiateOwned", &libs::ActorInstantiateOwned)
            .addFunction("Destroy", &libs::ActorDestroy)
        .endNamespace()
        .beginNamespace("Text")
            .addFunction("Draw", &libs::TextDraw)
        .endNamespace()
        .beginNamespace("Audio")
            .addFunction("Play", &libs::AudioPlay)
            .addFunction("Halt", &libs::AudioHalt)
            .addFunction("SetVolume", &libs::AudioSetVolume)
        .endNamespace()
        .beginNamespace("Image")
            .addFunction("DrawUI", &libs::ImageDrawUI)
            .addFunction("DrawUIEx", &libs::ImageDrawUIEx)
            .addFunction("Draw", &libs::ImageDraw)
            .addFunction("DrawEx", &libs::ImageDrawEx)
            .addFunction("DrawPixel", &libs::ImageDrawPixel)
        .endNamespace()
        .beginNamespace("Camera")
            .addFunction("SetPosition", &libs::CameraSetPosition)
            .addFunction("GetPositionX", &libs::CameraGetPositionX)
            .addFunction("GetPositionY", &libs::CameraGetPositionY)
            .addFunction("SetZoom", &libs::CameraSetZoom)
            .addFunction("GetZoom", &libs::CameraGetZoom)
        .endNamespace()
        .beginNamespace("Scene")
            .addFunction("Load", &libs::SceneLoad)
            .addFunction("GetCurrent", &libs::SceneGetCurrent)
            .addFunction("DontDestroy", &libs::SceneDontDestroy)
        .endNamespace()
        .beginNamespace("Physics")
            .addFunction("Raycast", &libs::PhysicsRaycast)
            .addFunction("RaycastAll", &libs::PhysicsRaycastAll)
        .endNamespace()
        .beginNamespace("Event")
            .addFunction("Publish", &libs::EventPublish)
            .addFunction("PublishRemote", &libs::EventPublishRemote)
            .addFunction("Subscribe", &libs::EventSubscribe)
            .addFunction("Unsubscribe", &libs::EventUnsubscribe)
        .endNamespace()
        .beginNamespace("Multiplayer")
            .addFunction("Connect", &libs::MultiplayerConnect)
            .addFunction("Disconnect", &libs::MultiplayerDisconnect)
            .addFunction("ClientID", &libs::MultiplayerClientID)
            .addFunction("JoinedClients", &libs::MultiplayerJoinedClients)
            .addFunction("OnClientJoin", &libs::MultiplayerOnClientJoin)
            .addFunction("OnClientLeave", &libs::MultiplayerOnClientLeave)
        .endNamespace()
        .beginNamespace("ReplicatorService")
            .addFunction("Replicate", &libs::ReplicatorServiceReplicate)
        .endNamespace();
    // clang-format on
}

void InitializeScriptingClasses() {
    auto* state = GetGlobalState();
    // clang-format off
    luabridge::getGlobalNamespace(state)
        .beginClass<OpaqueComponentPointer>("OpaqueComponentPointer")
        .endClass()
        .beginClass<game::Actor>("game::Actor")
            .addFunction("GetName", &game::Actor::getName)
            .addFunction("GetID", &game::Actor::getID)
            .addFunction("GetComponentByKey", &game::Actor::getComponentByKey)
            .addFunction("GetComponent", &game::Actor::getComponent)
            .addFunction("GetComponents", &game::Actor::getComponents)
            .addFunction("AddComponent", &game::Actor::addComponent)
            .addFunction("RemoveComponent", &game::Actor::removeComponent)
            .addFunction("GetOwner", &game::Actor::getOwnerClient)
        .endClass()
        .beginClass<glm::vec2>("vec2")
            .addProperty("x", &glm::vec2::x)
            .addProperty("y", &glm::vec2::y)
        .endClass()
        .beginClass<b2Vec2>("Vector2")
            .addConstructor<void(*) (float, float)>()
            .addProperty("x", &b2Vec2::x)
            .addProperty("y", &b2Vec2::y)
            .addFunction("Normalize", &b2Vec2::Normalize)
            .addFunction("Length", &b2Vec2::Length)
            .addFunction("__add", &b2Vec2::Add)
            .addFunction("__sub", &b2Vec2::Sub)
            .addFunction("__mul", &b2Vec2::Mul)
            .addStaticFunction("Distance", &b2Distance)
            .addStaticFunction("Dot", static_cast<float (*)(const b2Vec2 &, const b2Vec2 &)>(&b2Dot))
        .endClass()
        .beginClass<Component>("Component")
            .addProperty("key", &Component::key, false)
            .addProperty("type", &Component::type, false)
            .addProperty("actor", &Component::actor, false)
        .endClass()
        .beginClass<CppComponent>("CppComponent")
            .addProperty("enabled", &CppComponent::enabled)
        .endClass()
        .deriveClass<Transform, CppComponent>("Transform")
            .addProperty(OpaqueComponentPointerKey, &Transform::__opaquePointer)
            .addProperty("x", &Transform::x)
            .addProperty("y", &Transform::y)
            .addProperty("rotation", &Transform::rotation)
        .endClass()
        .deriveClass<InterpTransform, CppComponent>("InterpTransform")
            .addProperty(OpaqueComponentPointerKey, &InterpTransform::__opaquePointer)
            .addProperty("x", &InterpTransform::x)
            .addProperty("y", &InterpTransform::y)
            .addProperty("rotation", &InterpTransform::rotation)
        .endClass()
        .deriveClass<physics::Rigidbody, CppComponent>("Rigidbody")
            .addProperty(OpaqueComponentPointerKey, &physics::Rigidbody::__opaquePointer)
            .addProperty("x", &physics::Rigidbody::x)
            .addProperty("y", &physics::Rigidbody::y)
            .addProperty("rotation", &physics::Rigidbody::rotation)
            .addProperty("gravity_scale", &physics::Rigidbody::gravity_scale)
            .addFunction("GetPosition", &physics::Rigidbody::GetPosition)
            .addFunction("GetRotation", &physics::Rigidbody::GetRotation)
            .addFunction("GetVelocity", &physics::Rigidbody::GetVelocity)
            .addFunction("GetAngularVelocity", &physics::Rigidbody::GetAngularVelocity)
            .addFunction("GetGravityScale", &physics::Rigidbody::GetGravityScale)
            .addFunction("GetUpDirection", &physics::Rigidbody::GetUpDirection)
            .addFunction("GetRightDirection", &physics::Rigidbody::GetRightDirection)
            .addFunction("AddForce", &physics::Rigidbody::AddForce)
            .addFunction("SetVelocity", &physics::Rigidbody::SetVelocity)
            .addFunction("SetPosition", &physics::Rigidbody::SetPosition)
            .addFunction("SetRotation", &physics::Rigidbody::SetRotation)
            .addFunction("SetAngularVelocity", &physics::Rigidbody::SetAngularVelocity)
            .addFunction("SetGravityScale", &physics::Rigidbody::SetGravityScale)
            .addFunction("SetUpDirection", &physics::Rigidbody::SetUpDirection)
            .addFunction("SetRightDirection", &physics::Rigidbody::SetRightDirection)
        .endClass()
        .beginClass<physics::Collision>("Collision")
            .addProperty("other", &physics::Collision::other, false)
            .addProperty("point", &physics::Collision::point, false)
            .addProperty("relative_velocity", &physics::Collision::relative_velocity, false)
            .addProperty("normal", &physics::Collision::normal, false)
        .endClass()
        .beginClass<physics::HitResult>("HitResult")
            .addProperty("actor", &physics::HitResult::actor, false)
            .addProperty("point", &physics::HitResult::point, false)
            .addProperty("normal", &physics::HitResult::normal, false)
            .addProperty("is_trigger", &physics::HitResult::is_trigger, false)
        .endClass()
        .beginClass<net::ReplicatePush>("ReplicatePush")
            .addFunction("WriteInt", &net::ReplicatePush::writeInt)
            .addFunction("WriteNumber", &net::ReplicatePush::writeNumber)
            .addFunction("WriteBool", &net::ReplicatePush::writeBool)
            .addFunction("WriteString", &net::ReplicatePush::writeString)
            .addFunction("BeginArray", &net::ReplicatePush::beginArray)
            .addFunction("BeginMap", &net::ReplicatePush::beginMap)
        .endClass()
        .beginClass<net::ReplicatePull>("ReplicatePull")
            .addFunction("ReadInt", &net::ReplicatePull::readInt)
            .addFunction("ReadNumber", &net::ReplicatePull::readNumber)
            .addFunction("ReadBool", &net::ReplicatePull::readBool)
            .addFunction("ReadString", &net::ReplicatePull::readString)
            .addFunction("ReadArray", &net::ReplicatePull::readArray)
            .addFunction("ReadMap", &net::ReplicatePull::readMap)
            .addFunction("DoInterp", &net::ReplicatePull::doInterp)
        .endClass();
    // clang-format on
}

void InitializeInterface(std::unique_ptr<LuaInterface> interface) {
    libs::Interface = std::move(interface);
}

} // namespace sge::scripting
