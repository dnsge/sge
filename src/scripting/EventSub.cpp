#include "scripting/EventSub.hpp"

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sge::scripting {

subscription_handle EventSub::subscribe(std::string_view event, luabridge::LuaRef handler) {
    auto handle = this->nextSubscriptionHandle_++;
    this->pendingSubscribes_.push_back(EventSubscriptionRequest{
        .handle = handle,
        .event = std::string{event},
        .handler = {std::move(handler)},
    });
    return handle;
}

subscription_handle EventSub::subscribe(std::string_view event, CallableHandlerFunc handler) {
    auto handle = this->nextSubscriptionHandle_++;
    this->pendingSubscribes_.push_back(EventSubscriptionRequest{
        .handle = handle,
        .event = std::string{event},
        .handler = {std::move(handler)},
    });
    return handle;
}

void EventSub::unsubscribe(subscription_handle handle) {
    this->pendingUnsubscribes_.push_back(handle);
}

void EventSub::executePendingSubscriptions() {
    if (this->pendingSubscribes_.empty() && this->pendingUnsubscribes_.empty()) {
        return;
    }

    for (auto &req : this->pendingSubscribes_) {
        this->doSubscribe(std::move(req));
    }
    this->pendingSubscribes_.clear();
    for (auto handle : this->pendingUnsubscribes_) {
        this->doUnsubscribe(handle);
    }
    this->pendingUnsubscribes_.clear();
}

void EventSub::doSubscribe(EventSubscriptionRequest &&req) {
    assert(!this->eventHandlers_.contains(req.handle));

    // Add handler to mapping of handlers
    auto emplacement = this->eventHandlers_.emplace(req.handle,
                                                    MappedHandler{
                                                        .event = req.event,
                                                        .handler = std::move(req.handler),
                                                    });
    assert(emplacement.second);

    // Add handler to event list
    auto* handlerPtr = &emplacement.first->second;
    this->eventHandlersByEvent_[req.event].push_back(handlerPtr);
}

void EventSub::doUnsubscribe(subscription_handle handle) {
    auto handlerIt = this->eventHandlers_.find(handle);
    if (handlerIt == this->eventHandlers_.end()) {
        return;
    }

    // Extract the handle mapping
    auto node = this->eventHandlers_.extract(handlerIt);
    auto* handlerPtr = &node.mapped();

    auto eventIt = this->eventHandlersByEvent_.find(handlerPtr->event);
    if (eventIt == this->eventHandlersByEvent_.end()) {
        return;
    }

    // Erase the handler-to-event-name mapping
    auto &vec = eventIt->second;
    auto it = std::remove(vec.begin(), vec.end(), handlerPtr);
    vec.erase(it, vec.end());
}

} // namespace sge::scripting
