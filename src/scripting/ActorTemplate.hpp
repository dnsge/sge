#pragma once

#include "resources/Resources.hpp"
#include "scripting/Component.hpp"

#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace sge::scripting {

class ActorTemplate {
public:
    ActorTemplate(const resources::ActorTemplateDescription &description);

    ActorTemplate(const ActorTemplate &) = delete;
    ActorTemplate &operator=(const ActorTemplate &) = delete;

    ActorTemplate(ActorTemplate &&) = default;
    ActorTemplate &operator=(ActorTemplate &&) = default;

    const std::string &name() const;
    const std::map<std::string, std::unique_ptr<Component>> &components() const;

private:
    std::string name_;
    std::map<std::string, std::unique_ptr<Component>> components_;
};

const ActorTemplate &GetActorTemplateInstance(std::string_view name);

} // namespace sge::scripting
