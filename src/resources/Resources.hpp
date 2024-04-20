#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <glm/glm.hpp>
#include <rapidjson/document.h>

#include "Realm.hpp"
#include "resources/Deserialize.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sge {

class Renderer;

namespace resources {

const auto ResourcesDirectoryPath = std::filesystem::path{"resources/"};

void EnsureResourcesDirectoryExists();

const auto GameConfigPath = ResourcesDirectoryPath / "game.config";
const auto ServerConfigPath = ResourcesDirectoryPath / "server.config";
const auto ClientConfigPath = ResourcesDirectoryPath / "client.config";
const auto ScenesDirectoryPath = ResourcesDirectoryPath / "scenes";
const auto ActorTemplatesDirectoryPath = ResourcesDirectoryPath / "actor_templates";
const auto ImagesDirectoryPath = ResourcesDirectoryPath / "images";
const auto FontsDirectoryPath = ResourcesDirectoryPath / "fonts";
const auto AudioDirectoryPath = ResourcesDirectoryPath / "audio";
const auto ComponentTypesPath = ResourcesDirectoryPath / "component_types";

struct ComponentDefinition {
    std::string type{};
    Realm realm{Realm::Server};
    std::vector<std::pair<std::string, ComponentValueType>> values{};
};

struct ActorDescription {
    std::optional<std::string> template_name{};
    std::optional<std::string> name{};
    std::map<std::string, ComponentDefinition> components{};
};

void DeserializeActor(const rapidjson::Value::ConstObject &doc, ActorDescription &actor);

struct ActorTemplateDescription {
    std::string name;
    std::map<std::string, ComponentDefinition> components{};
};

class Image {
public:
    Image(SDL_Texture* texture);

    int width() const;
    int height() const;
    glm::ivec2 size() const;

    SDL_Rect rect() const;

private:
    friend class sge::Renderer;

    SDL_Texture* texture_;
    int width_;
    int height_;
};

struct SceneDescription {
    std::string name;
    std::vector<ActorDescription> actors;
};

const ActorTemplateDescription &GetActorTemplateDescription(std::string_view name);

const SceneDescription &GetSceneDescription(std::string_view name);

Mix_Chunk* GetAudio(std::string_view audioName);

TTF_Font* GetFont(std::string_view fontName, int fontSize);

const Image &GetImage(std::string_view name);

} // namespace resources

} // namespace sge
