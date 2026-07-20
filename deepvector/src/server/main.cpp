#include "dv/server/server.h"
#include "dv/collection.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

using namespace dv;
using namespace dv::server;

static std::atomic<bool> running{true};

void signalHandler(int) {
    running = false;
}

int main(int argc, char* argv[]) {
    ::signal(SIGINT, signalHandler);
    ::signal(SIGTERM, signalHandler);

    ServerConfig config;
    int dim = 384;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) config.port = std::atoi(argv[++i]);
        else if (arg == "--host" && i + 1 < argc) config.host = argv[++i];
        else if (arg == "--data-dir" && i + 1 < argc) config.data_dir = argv[++i];
        else if (arg == "--api-key" && i + 1 < argc) config.api_key = argv[++i];
        else if (arg == "--dim" && i + 1 < argc) dim = std::atoi(argv[++i]);
        else if (arg == "--collection" && i + 1 < argc) config.default_collection = argv[++i];
    }

    CollectionConfig cc;
    cc.dim = static_cast<Dimension>(dim);
    cc.metric = DistanceMetric::Cosine;

    std::cout << "DeepVector starting, dim=" << dim
              << " default_collection=" << config.default_collection << std::endl;

    DeepVectorServer server(config, cc);
    server.start();

    std::cout << "Press Ctrl+C to stop" << std::endl;
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Shutting down..." << std::endl;
    server.stop();
    return 0;
}
