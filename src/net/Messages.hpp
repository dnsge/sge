#pragma once

#include "Types.hpp"
#include "net/Replicator.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <msgpack.hpp>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace sge::net {

enum MessageType : uint8_t {
    MessageTypeError = 0,
    MessageTypeHello = 1,
    MessageTypeWelcome = 2,
    MessageTypeLoadScene = 3,
    MessageTypeLoadSceneRequest = 4,
    MessageTypeTickReplication = 5,
    MessageTypeTickReplicationAck = 6,
    MessageTypeTickReplicationReject = 7,
    MessageTypeRoomState = 8,
    MessageTypeRemoteEvent = 9,
};

constexpr std::string_view StringOfMessageType(MessageType mty) {
    using namespace std::string_view_literals;
    switch (mty) {
    case MessageTypeError:
        return "MessageTypeError"sv;
    case MessageTypeHello:
        return "MessageTypeHello"sv;
    case MessageTypeWelcome:
        return "MessageTypeWelcome"sv;
    case MessageTypeLoadScene:
        return "MessageTypeLoadScene"sv;
    case MessageTypeLoadSceneRequest:
        return "MessageTypeLoadSceneRequest"sv;
    case MessageTypeTickReplication:
        return "MessageTypeTickReplication"sv;
    case MessageTypeTickReplicationAck:
        return "MessageTypeTickReplicationAck"sv;
    case MessageTypeTickReplicationReject:
        return "MessageTypeTickReplicationReject"sv;
    case MessageTypeRoomState:
        return "MessageTypeRoomState"sv;
    case MessageTypeRemoteEvent:
        return "MessageTypeRemoteEvent"sv;
    default:
        return "<invalid message type>"sv;
    }
}

/**
 * @brief A TypedMessage is a message that can be sent over the network according
 * to our protocol. It must have a Mty field with a MessageType value.
 */
template <typename T>
concept TypedMessage = requires {
    { T::Mty } -> std::convertible_to<const MessageType>;
};

/**
 * @brief Catch-all message indicating an error has occurred.
 */
struct MessageError {
    static constexpr MessageType Mty = MessageTypeError;
    std::string error;

    MSGPACK_DEFINE(error);
};

/**
 * @brief Sent by client after connecting to server. The server should register
 * the client and respond with MessageWelcome.
 */
struct MessageHello {
    static constexpr MessageType Mty = MessageTypeHello;
    MSGPACK_DEFINE();
};

/**
 * @brief Sent by server after client sends a MessageHello. Assigns the client
 * its client ID.
 */
struct MessageWelcome {
    static constexpr MessageType Mty = MessageTypeWelcome;
    client_id_t clientID;
    unsigned int serverTickRate;

    MSGPACK_DEFINE(clientID, serverTickRate);
};

/**
 * @brief Sent by server after client joins the game. Gives the client all
 * necessary information to reconstruct the game state.
 */
struct MessageLoadScene {
    static constexpr MessageType Mty = MessageTypeLoadScene;
    unsigned int generation;
    std::string sceneName;
    std::vector<RuntimeActor> runtimeActors;
    std::vector<ComponentReplication> sceneState;

    MSGPACK_DEFINE(generation, sceneName, runtimeActors, sceneState);
};

/**
 * @brief Sent by client when the client wants to load a new scene. The load won't
 * actually take place until the server sends a MessageLoadScene to the client.
 */
struct MessageLoadSceneRequest {
    static constexpr MessageType Mty = MessageTypeLoadSceneRequest;
    unsigned int generation;
    std::string sceneName;

    MSGPACK_DEFINE(generation, sceneName);
};

/**
 * @brief Sent between server/client when the state of the game changes and requires
 * replication over the network. Contains any newly instantiated actors at runtime
 * and any updated component states.
 */
struct MessageTickReplication {
    static constexpr MessageType Mty = MessageTypeTickReplication;
    unsigned int generation;
    std::vector<InstantiatedActor> instantiations;
    std::vector<ComponentReplication> replications;
    std::vector<actor_id_t> destructions;

    MSGPACK_DEFINE(generation, instantiations, replications, destructions);
};

/**
 * @brief Sent by server after a client instantiates a new actor at runtime. The ACK
 * message assigns the server-side actor id for each instantiated actor.
 * 
 */
struct MessageTickReplicationAck {
    static constexpr MessageType Mty = MessageTypeTickReplicationAck;
    std::vector<RemoteIDMapping> remoteIDMappings;

    MSGPACK_DEFINE(remoteIDMappings);
};

/**
 * @brief Sent by server when an actor instantiation is rejected due to 
 * mismatched generation numbers.
 */
struct MessageTickReplicationReject {
    static constexpr MessageType Mty = MessageTypeTickReplicationReject;
    unsigned int serverGeneration;
    std::vector<actor_id_t> rejectedInstantiations;

    MSGPACK_DEFINE(serverGeneration, rejectedInstantiations);
};

/**
 * @brief Sent by server when the room state (i.e. joined clients) changes.
 * Clients should compare the previous room state to the new room state to determine
 * who joined/left.
 */
struct MessageRoomState {
    static constexpr MessageType Mty = MessageTypeRoomState;
    std::vector<client_id_t> joinedClients;

    MSGPACK_DEFINE(joinedClients);
};

/**
 * @brief Sent by client or server when an Event.PublishRemote occurs.
 */
struct MessageRemoteEvents {
    static constexpr MessageType Mty = MessageTypeRemoteEvent;
    unsigned int generation;
    std::vector<EventPublish> publishes;

    MSGPACK_DEFINE(generation, publishes);
};

/**
 * @brief A message sent by a client to a server.
 */
using CMessage = std::variant<MessageError, MessageHello, MessageLoadSceneRequest,
                              MessageTickReplication, MessageRemoteEvents>;

/**
 * @brief A message sent by a server to a client.
 */
using SMessage = std::variant<MessageError, MessageWelcome, MessageLoadScene,
                              MessageTickReplication, MessageTickReplicationAck,
                              MessageTickReplicationReject, MessageRoomState, MessageRemoteEvents>;

/**
 * @brief Get the MessageType of a message.
 */
template <typename... MTs>
requires(TypedMessage<MTs> &&...) constexpr MessageType
    MessageTypeOfMessage(const std::variant<MTs...> &m) {
    return std::visit(
        [&](const TypedMessage auto &msg) -> MessageType {
            return std::remove_cvref<decltype(msg)>::type::Mty;
        },
        m);
}

/**
 * @brief Serialize a message into a compatible buffer with msgpack.
 * 
 * @param buffer Buffer to write to.
 * @param msg Message to serialize.
 */
template <typename Buf, TypedMessage Msg>
void SerializeMessage(Buf &buffer, const Msg &msg) {
    msgpack::pack(buffer, Msg::Mty);
    msgpack::pack(buffer, msg);
}

/**
 * @brief Serialize a Message variant into a compatible buffer with msgpack.
 * 
 * @param buffer Buffer to write to.
 * @param msg Message to serialize.
 */
template <typename Buf, typename... Ts>
requires(TypedMessage<Ts> &&...) void SerializeMessage(Buf &buffer,
                                                       const std::variant<Ts...> &msg) {
    std::visit(
        [&](const TypedMessage auto &m) {
            SerializeMessage(buffer, m);
        },
        msg);
}

// std::unique_ptr<Message> ParseMessage(std::span<const char> data);

namespace detail {

template <typename Message, TypedMessage... MTypes>
struct ParserExecutor;

// Magic template programming to implement a switch statement on ty
template <typename Message, TypedMessage Current, TypedMessage... Rest>
struct ParserExecutor<Message, Current, Rest...> {
    static std::unique_ptr<Message> parse(MessageType ty, msgpack::object_handle &objHandle) {
        // case Current::Mty
        if (ty == Current::Mty) {
            if constexpr (std::is_empty_v<Current>) {
                return std::make_unique<Message>(Current{});
            } else {
                auto msg = objHandle.get().as<Current>();
                return std::make_unique<Message>(std::move(msg));
            }
        }

        if constexpr (sizeof...(Rest) == 0) {
            // Invalid message type
            std::cerr << "parse message: invalid message type " << ty << std::endl;
            return nullptr;
        } else {
            // Try the next case
            return ParserExecutor<Message, Rest...>::parse(ty, objHandle);
        }
    }
};

template <typename T>
struct ParserVariantExecutor;

// Wrapper to extract the types inside the std::variant and pass them to ParserExecutor
template <TypedMessage... Ts>
struct ParserVariantExecutor<std::variant<Ts...>> {
    static std::unique_ptr<std::variant<Ts...>> parse(MessageType ty,
                                                      msgpack::object_handle &objHandle) {
        return ParserExecutor<std::variant<Ts...>, Ts...>::parse(ty, objHandle);
    }
};

} // namespace detail

/**
 * @brief Parse a Message from raw binary data.
 * 
 * @tparam MessageVariant Message std::variant type
 * @param data View into data to parse message from.
 * @return std::unique_ptr<MessageVariant> The parsed message, or nullptr if failed.
 */
template <class MessageVariant>
std::unique_ptr<MessageVariant> ParseMessage(std::span<const char> data) {
    try {
        std::size_t offset = 0;
        auto msgTypeHandle = msgpack::unpack(data.data(), data.size(), offset);
        auto msgBodyHandle = msgpack::unpack(data.data(), data.size(), offset);
        if (offset != data.size()) [[unlikely]] {
            std::cerr << "parse message: offset != data.size()" << std::endl;
            return nullptr;
        }

        const auto msgType = msgTypeHandle.get().as<MessageType>();

        return detail::ParserVariantExecutor<MessageVariant>::parse(msgType, msgBodyHandle);
    } catch (msgpack::unpack_error &e) {
        // Error occurred while unpacking message
        std::cerr << "parse message: unpack error: " << e.what() << std::endl;
        return nullptr;
    }
}

} // namespace sge::net

MSGPACK_ADD_ENUM(sge::net::MessageType);
