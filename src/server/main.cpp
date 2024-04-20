#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/thread.hpp>

#include "resources/Configs.hpp"
#include "resources/Resources.hpp"
#include "server/Server.hpp"

#include <cstdlib>
#include <iostream>
#include <utility>

int main(int /*argc*/, char** /*argv*/) {
    sge::resources::EnsureResourcesDirectoryExists();
    auto serverConfig = sge::resources::LoadServerConfig();
    auto gameConfig = sge::resources::LoadGameConfig();

    if (serverConfig.io_workers == 0) {
        std::cerr << "io_workers must be greater than zero" << std::endl;
        std::exit(1);
    }

    std::cout << "starting server on 0.0.0.0:" << serverConfig.port << std::endl;

    boost::asio::io_context io;
    boost::asio::executor_work_guard<decltype(io.get_executor())> work{io.get_executor()};

    // Spawn a thread for ASIO execution.
    boost::thread_group group;
    for (unsigned int i = 0; i < serverConfig.io_workers; ++i) {
        group.create_thread([&] {
            io.run();
        });
    }

    // Initialize server
    sge::server::InitServer(std::move(serverConfig), std::move(gameConfig), io);

    // Run the Game loop.
    sge::server::CurrentServer().run();

    // Clean up server.
    sge::server::DeinitServer();

    // Stop io execution and wait for return.
    io.stop();
    group.join_all();
    return 0;
}
