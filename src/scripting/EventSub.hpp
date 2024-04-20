#pragma once

#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "scripting/Invoke.hpp"
#include "scripting/LuaValue.hpp"
#include "util/HeterogeneousLookup.hpp"
#include "util/Visit.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace sge::scripting {

using subscription_handle = unsigned int;

using CallableHandlerFunc = std::function<void(const LuaValue &)>;

struct MappedHandler {
    std::string event;
    std::variant<luabridge::LuaRef, CallableHandlerFunc> handler;
};

namespace detail {

template <typename Param>
void InvokeEventHandlers(const std::vector<MappedHandler*> &handlers, Param &&invocationParam) {
    for (const auto &m : handlers) {
        ActorInvoke("<event invocation>", [&]() {
            std::visit(
                util::Visitor{
                    [&](const luabridge::LuaRef &ref) {
                        ref(std::forward<Param>(invocationParam));
                    },
                    [&](const CallableHandlerFunc &func) {
                        // Only invoke CallableHandlerFunc if valid invocation param
                        if constexpr (std::is_same_v<std::remove_reference_t<Param>, LuaValue>) {
                            func(std::forward<Param>(invocationParam));
                        }
                    }},
                m->handler);
        });
    }
}

} // namespace detail

class EventSub {
public:
    EventSub() = default;

    template <typename Param>
    void publish(std::string_view event, Param &&param) {
        auto eventIt = this->eventHandlersByEvent_.find(event);
        if (eventIt == this->eventHandlersByEvent_.end()) {
            return;
        }
        detail::InvokeEventHandlers(eventIt->second, std::forward<Param>(param));
    }

    subscription_handle subscribe(std::string_view event, luabridge::LuaRef handler);
    subscription_handle subscribe(std::string_view event, CallableHandlerFunc handler);
    void unsubscribe(subscription_handle handle);
    void executePendingSubscriptions();

private:
    struct EventSubscriptionRequest {
        subscription_handle handle;
        std::string event;
        std::variant<luabridge::LuaRef, CallableHandlerFunc> handler;
    };

    void doSubscribe(EventSubscriptionRequest &&req);
    void doUnsubscribe(subscription_handle handle);

    std::unordered_map<subscription_handle, MappedHandler> eventHandlers_;
    unordered_string_map<std::vector<MappedHandler*>> eventHandlersByEvent_;

    std::vector<EventSubscriptionRequest> pendingSubscribes_;
    std::vector<subscription_handle> pendingUnsubscribes_;

    subscription_handle nextSubscriptionHandle_{0};
};

} // namespace sge::scripting
