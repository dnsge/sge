#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/thread.hpp>

#include "client/Client.hpp"
#include "resources/Configs.hpp"
#include "resources/Resources.hpp"

#include <utility>

int main(int /*argc*/, char** /*argv*/) {
    sge::resources::EnsureResourcesDirectoryExists();
    auto clientConfig = sge::resources::LoadClientConfig();
    auto gameConfig = sge::resources::LoadGameConfig();

    boost::asio::io_context io;
    boost::asio::executor_work_guard<decltype(io.get_executor())> work{io.get_executor()};

    // Initialize client
    sge::client::InitClient(std::move(clientConfig), std::move(gameConfig), io);

    // Spawn a thread for ASIO execution.
    boost::thread ioThread([&] {
        io.run();
    });

    // Run the Game loop. This needs to be called from the main thread
    // for reasons.
    sge::client::CurrentClient().run();

    // Clean up client.
    sge::client::DeinitClient();

    // Stop io execution and wait for return.
    io.stop();
    ioThread.join();
    return 0;
}
