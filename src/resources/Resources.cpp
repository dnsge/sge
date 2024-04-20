#include "resources/Resources.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <glm/glm.hpp>
#include <rapidjson/document.h>

#include "Realm.hpp"
#include "Renderer.hpp"
#include "gea/AudioHelper.h"
#include "resources/Deserialize.hpp"
#include "util/HeterogeneousLookup.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace sge::resources {

namespace {

ComponentDefinition deserializeComponentDefinition(const rapidjson::Value::ConstObject &obj) {
    ComponentDefinition def;

    def.type = GetKeyOrZero<std::string>(obj, "type");

    auto realmStr = GetKeySafe<std::string>(obj, "realm");
    if (realmStr.has_value()) {
        def.realm = RealmOfString(*realmStr);
    } else {
        // Fall back to current realm
        switch (CurrentRealm()) {
        case GeneralRealm::Server:
            def.realm = Realm::Server;
            break;
        case GeneralRealm::Client:
            def.realm = Realm::Client;
            break;
        }
    }

    for (const auto &member : obj) {
        auto name = std::string{member.name.GetString()};
        if (name == "type") {
            continue;
        }

        auto val = ParseComponentValueType(member.value);
        if (!val) {
#ifdef DEBUG
            std::cerr << "Warning: failed to parse component value for key " << name << std::endl;
#endif
            continue;
        }
        def.values.emplace_back(std::move(name), std::move(*val));
    }

    return def;
}

} // namespace

void EnsureResourcesDirectoryExists() {
    if (!std::filesystem::exists(ResourcesDirectoryPath)) {
        std::cout << "error: " << ResourcesDirectoryPath.string() << " missing";
        std::exit(0);
    }
}

void DeserializeActor(const rapidjson::Value::ConstObject &doc, ActorDescription &actor) {
    actor.template_name = GetKeySafe<std::string>(doc, "template");
    actor.name = GetKeySafe<std::string>(doc, "name");

    auto components = GetObjectSafe(doc, "components");
    if (components.has_value()) {
        for (const auto &member : *components) {
            actor.components.insert({
                member.name.GetString(),
                deserializeComponentDefinition(member.value.GetObject()),
            });
        }
    }
}

Image::Image(SDL_Texture* texture)
    : texture_(texture)
    , width_(0)
    , height_(0) {
    auto res = SDL_QueryTexture(this->texture_, nullptr, nullptr, &this->width_, &this->height_);
    if (res != 0) {
#ifdef DEBUG
        std::cerr << "SDL QueryTexture failed: " << SDL_GetError() << std::endl;
#endif
    }
}

int Image::width() const {
    return this->width_;
}

int Image::height() const {
    return this->height_;
}

glm::ivec2 Image::size() const {
    return glm::ivec2{this->width_, this->height_};
}

SDL_Rect Image::rect() const {
    return SDL_Rect{0, 0, this->width_, this->height_};
}

unordered_string_map<ActorTemplateDescription> LoadedActorTemplates;
unordered_string_map<SceneDescription> LoadedScenes;
unordered_string_map<Mix_Chunk*> LoadedAudio;
std::map<std::pair<std::string, int>, TTF_Font*, std::less<>> LoadedFonts;
unordered_string_map<Image> LoadedImages;

ActorTemplateDescription LoadActorTemplate(std::string_view name) {
    auto nameWithExtension = std::string(name) + ".template";
    auto scenePath = ActorTemplatesDirectoryPath / nameWithExtension;
    if (!std::filesystem::exists(scenePath)) {
        std::cout << "error: template " << name << " is missing";
        std::exit(0);
    }

    ActorTemplateDescription actor;
    rapidjson::Document doc;
    ReadJsonFile(scenePath, doc);

    actor.name = GetKeyOrZero<std::string>(doc, "name");
    auto components = GetObjectSafe(doc, "components");
    if (components.has_value()) {
        for (const auto &member : *components) {
            actor.components.insert({
                member.name.GetString(),
                deserializeComponentDefinition(member.value.GetObject()),
            });
        }
    }

    return actor;
}

const ActorTemplateDescription &GetActorTemplateDescription(std::string_view name) {
    auto it = LoadedActorTemplates.find(name);
    if (it != LoadedActorTemplates.end()) {
        // Template has already been loaded
        return it->second;
    }

    // Load requested template and insert into cache
    auto scene = LoadActorTemplate(name);
    auto inserted = LoadedActorTemplates.insert({std::string{name}, std::move(scene)});

    // Return reference to inserted node
    return inserted.first->second;
}

SceneDescription LoadSceneDescription(std::string_view name) {
    auto nameWithExtension = std::string(name) + ".scene";
    auto scenePath = ScenesDirectoryPath / nameWithExtension;
    if (!std::filesystem::exists(scenePath)) {
        std::cout << "error: scene " << name << " is missing";
        std::exit(0);
    }

    SceneDescription scene;
    scene.name = name;

    rapidjson::Document doc;
    ReadJsonFile(scenePath, doc);

    auto actorsArray = GetArraySafe(doc, "actors");
    if (actorsArray.has_value()) {
        scene.actors.resize(actorsArray->Size());
        for (size_t i = 0; i < actorsArray->Size(); ++i) {
            DeserializeActor((*actorsArray)[i].GetObject(), scene.actors[i]);
        }
    }

    return scene;
}

const SceneDescription &GetSceneDescription(std::string_view name) {
    auto it = LoadedScenes.find(name);
    if (it != LoadedScenes.end()) {
        // Scene has already been loaded
        return it->second;
    }

    // Load requested scene and insert into cache
    auto scene = LoadSceneDescription(name);
    auto inserted = LoadedScenes.insert({std::string{name}, std::move(scene)});

    // Return reference to inserted node
    return inserted.first->second;
}

Mix_Chunk* MaybeLoadAudio(std::string_view name) {
    auto wavName = std::string(name) + ".wav";
    auto wavPath = AudioDirectoryPath / wavName;
    if (std::filesystem::exists(wavPath)) {
        auto* audio = AudioHelper::Mix_LoadWAV498(wavPath.string().c_str());
        if (audio == nullptr) {
#ifdef DEBUG
            std::cerr << "SDL failed to load .wav: " << Mix_GetError() << std::endl;
#endif
        }
        return audio;
    }

    auto oggName = std::string(name) + ".ogg";
    auto oggPath = AudioDirectoryPath / oggName;
    if (std::filesystem::exists(oggPath)) {
        auto* audio = AudioHelper::Mix_LoadWAV498(oggPath.string().c_str());
        if (audio == nullptr) {
#ifdef DEBUG
            std::cerr << "SDL failed to load .ogg: " << Mix_GetError() << std::endl;
#endif
        }
        return audio;
    }

    // Neither a .wav nor a .ogg exist
    return nullptr;
}

Mix_Chunk* GetAudio(std::string_view name) {
    auto it = LoadedAudio.find(name);
    if (it != LoadedAudio.end()) {
        // Audio has already been loaded
        return it->second;
    }

    auto* audio = MaybeLoadAudio(name);
    if (audio == nullptr) {
        std::cout << "error: failed to play audio clip " << name;
        std::exit(0);
    }

    LoadedAudio.insert({std::string{name}, audio});

    return audio;
}

TTF_Font* LoadFont(std::string_view name, int size) {
    auto nameWithExtension = std::string(name) + ".ttf";
    auto fontPath = FontsDirectoryPath / nameWithExtension;
    if (!std::filesystem::exists(fontPath)) {
        std::cout << "error: font " << name << " missing";
        std::exit(0);
    }

    auto* font = TTF_OpenFont(fontPath.string().c_str(), size);
    assert(font != nullptr);

    return font;
}

TTF_Font* GetFont(std::string_view fontName, int fontSize) {
    auto key = std::pair<std::string, int>{fontName, fontSize};
    auto it = LoadedFonts.find(key);
    if (it != LoadedFonts.end()) {
        // Font has already been loaded
        return it->second;
    }

    // Load requested font and insert into cache
    auto* font = LoadFont(fontName, fontSize);
    LoadedFonts.insert({key, font});

    return font;
}

SDL_Texture* LoadImageTexture(std::string_view name) {
    auto nameWithExtension = std::string(name) + ".png";
    auto imagePath = ImagesDirectoryPath / nameWithExtension;
    if (!std::filesystem::exists(imagePath)) {
        std::cout << "error: missing image " << name;
        std::exit(0);
    }

    auto* texture = IMG_LoadTexture(Renderer::globalRenderer(), imagePath.string().c_str());
    assert(texture != nullptr);

    return texture;
}

const Image &GetImage(std::string_view name) {
    auto it = LoadedImages.find(name);
    if (it != LoadedImages.end()) {
        // Image has already been loaded
        return it->second;
    }

    // Load requested image and insert into cache
    auto* texture = LoadImageTexture(name);
    auto img = Image{texture};
    auto inserted = LoadedImages.insert({std::string{name}, img});

    return inserted.first->second;
}

} // namespace sge::resources
