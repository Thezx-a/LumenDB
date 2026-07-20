#include <iostream>
#include <cstdlib>
#include <memory>
#include <string>
#include <csignal>
#include "core/db_impl.h"
#include "network/server.h"
#include "minikv/options.h"
#include "utils/log.h"

static volatile bool g_running = true;

void signalHandler(int) { g_running = false; }

int main(int argc, char* argv[]) {
    int port = 8888;
    std::string host = "0.0.0.0";
    std::string dbPath = "./minikv_data";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (arg == "--host" && i + 1 < argc) host = argv[++i];
        else if (arg == "--db" && i + 1 < argc) dbPath = argv[++i];
    }

    ::signal(SIGINT, signalHandler);
    ::signal(SIGTERM, signalHandler);

::minikv::Options opts;
    opts.db_path = dbPath;
    opts.wal_sync = false;

    std::unique_ptr<::minikv::DB> db;
    auto status = ::minikv::core::DBImpl::open(opts, &db);
    if (!status.ok()) {
        std::cerr << "Failed to open DB: " << status.message() << std::endl;
        return 1;
    }

    LOG_INFO("MiniKV starting on " + host + ":" + std::to_string(port));

    ::minikv::network::Server server(host, port, db.get());
    server.run();

LOG_INFO("MiniKV shutting down");
    return 0;
}
