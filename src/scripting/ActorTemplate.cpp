#include "scripting/ActorTemplate.hpp"

#include "resources/Resources.hpp"
#include "scripting/Component.hpp"
#include "util/HeterogeneousLookup.hpp"

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace sge::scripting {

ActorTemplate::ActorTemplate(const resources::ActorTemplateDescription &description)
    : name_(description.name) {
    for (const auto &componentDescription : description.components) {
        auto component = InstantiateComponent(componentDescription.second.type,
                                              componentDescription.second.realm);
        component->setValues(componentDescription.second.values);
        this->components_.insert({componentDescription.first, std::move(component)});
    }
}

const std::string &ActorTemplate::name() const {
    return this->name_;
}

const std::map<std::string, std::unique_ptr<Component>> &ActorTemplate::components() const {
    return this->components_;
}

unordered_string_map<ActorTemplate> LoadedActorTemplateInstances;

const ActorTemplate &GetActorTemplateInstance(std::string_view name) {
    auto it = LoadedActorTemplateInstances.find(name);
    if (it != LoadedActorTemplateInstances.end()) {
        return it->second;
    }

    auto actorTemplate = ActorTemplate{resources::GetActorTemplateDescription(name)};
    auto inserted =
        LoadedActorTemplateInstances.insert({std::string{name}, std::move(actorTemplate)});
    return inserted.first->second;
}

} // namespace sge::scripting
