#pragma once

#include "Types.hpp"
#include "net/Replicator.hpp"

#include <string_view>

namespace sge {

enum class GeneralRealm {
    Server,
    Client
};

GeneralRealm CurrentRealm();

client_id_t CurrentClientID();

net::ReplicatorService &CurrentReplicatorService();

enum class Realm {
    Server,
    ServerReplicated,
    Client,
    Owner
};

constexpr Realm RealmOfString(std::string_view s) {
    if (s == "server") {
        return Realm::Server;
    } else if (s == "server_replicated") {
        return Realm::ServerReplicated;
    } else if (s == "client") {
        return Realm::Client;
    } else if (s == "owner") {
        return Realm::Owner;
    } else {
        // Fall back to server
        return Realm::Server;
    }
}

constexpr std::string_view StringOfRealm(Realm r) {
    using namespace std::string_view_literals;
    switch (r) {
    case Realm::Server:
        return "server"sv;
    case Realm::ServerReplicated:
        return "server_replicated"sv;
    case Realm::Client:
        return "client"sv;
    case Realm::Owner:
        return "owner"sv;
    }
}

} // namespace sge
