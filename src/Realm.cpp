#include "Realm.hpp"

#include "Types.hpp"
#include "net/Replicator.hpp"

#ifdef SGE_SERVER
#    include "server/Server.hpp"
#else
#    include "client/Client.hpp"
#endif

namespace sge {

GeneralRealm CurrentRealm() {
#ifdef SGE_SERVER
    return GeneralRealm::Server;
#else
    return GeneralRealm::Client;
#endif
}

client_id_t CurrentClientID() {
#ifdef SGE_SERVER
    return 0;
#else
    return client::CurrentClient().clientID();
#endif
}

net::ReplicatorService &CurrentReplicatorService() {
#ifdef SGE_SERVER
    return server::CurrentServer().replicatorService();
#else
    return client::CurrentClient().replicatorService();
#endif
}

} // namespace sge
