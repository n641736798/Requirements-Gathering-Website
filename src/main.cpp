#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>

#include "utils/Logger.hpp"
#include "utils/Config.hpp"
#include "net/TcpServer.hpp"
#include "net/HttpParser.hpp"
#include "business/ReportHandler.hpp"
#include "storage/MemoryStore.hpp"
#include "storage/StoreInterface.hpp"
#include "utils/JsonParser.hpp"
#include "thread/ThreadPool.hpp"

#ifdef ENABLE_MYSQL
#include "storage/MySQLStore.hpp"
#include "storage/ConnectionPool.hpp"
#endif

static std::atomic<bool> g_running{true};

void signalHandler(int sig) {
    (void)sig;
    g_running = false;
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -c, --config <file>  Configuration file path (default: config.ini)\n"
              << "  -h, --help           Show this help message\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string configFile = "config.ini";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                configFile = argv[++i];
            } else {
                std::cerr << "Error: Missing config file path" << std::endl;
                return 1;
            }
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
    }

    Logger::init("requirement_server.log");
    LOG_INFO("RequirementServer starting...");

    Config& config = Config::getInstance();
    if (!config.loadFromFile(configFile)) {
        const char* fallbackPaths[] = {"../config.ini", "../../config.ini"};
        bool loaded = false;
        for (const char* path : fallbackPaths) {
            if (config.loadFromFile(path)) {
                loaded = true;
                break;
            }
        }
        if (!loaded) {
            LOG_WARNING("Failed to load config file: " + configFile + ", using defaults");
        }
    }
    config.loadFromEnv();

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    StorageMode storageMode = config.getStorageMode();
    std::unique_ptr<StoreInterface> store;

#ifdef ENABLE_MYSQL
    std::unique_ptr<MySQLStore> mysqlStore;
#endif

    switch (storageMode) {
        case StorageMode::MEMORY:
            LOG_INFO("Using MEMORY storage mode");
            store = std::make_unique<MemoryStore>();
            break;

#ifdef ENABLE_MYSQL
        case StorageMode::MYSQL: {
            LOG_INFO("Using MYSQL storage mode");
            MySQLConfig mysqlConfig;
            mysqlConfig.host = config.getMySQLHost();
            mysqlConfig.port = config.getMySQLPort();
            mysqlConfig.user = config.getMySQLUser();
            mysqlConfig.password = config.getMySQLPassword();
            mysqlConfig.database = config.getMySQLDatabase();
            mysqlConfig.connectTimeout = config.getConnectTimeout();

            PoolConfig poolConfig;
            poolConfig.minSize = config.getPoolMinSize();
            poolConfig.maxSize = config.getPoolMaxSize();

            mysqlStore = std::make_unique<MySQLStore>();
            if (!mysqlStore->init(mysqlConfig, poolConfig)) {
                LOG_ERROR("Failed to initialize MySQL store, falling back to memory mode");
                store = std::make_unique<MemoryStore>();
            } else {
                store.reset(mysqlStore.release());
            }
            break;
        }

        case StorageMode::HYBRID: {
            LOG_INFO("Using HYBRID storage mode");
            MySQLConfig mysqlConfig;
            mysqlConfig.host = config.getMySQLHost();
            mysqlConfig.port = config.getMySQLPort();
            mysqlConfig.user = config.getMySQLUser();
            mysqlConfig.password = config.getMySQLPassword();
            mysqlConfig.database = config.getMySQLDatabase();
            mysqlConfig.connectTimeout = config.getConnectTimeout();

            PoolConfig poolConfig;
            poolConfig.minSize = config.getPoolMinSize();
            poolConfig.maxSize = config.getPoolMaxSize();

            mysqlStore = std::make_unique<MySQLStore>();
            if (!mysqlStore->init(mysqlConfig, poolConfig)) {
                LOG_ERROR("Failed to initialize MySQL store, falling back to memory mode");
                store = std::make_unique<MemoryStore>();
            } else {
                store.reset(mysqlStore.release());
            }
            break;
        }
#else
        case StorageMode::MYSQL:
        case StorageMode::HYBRID:
            LOG_WARNING("MySQL support not compiled, falling back to memory mode");
            store = std::make_unique<MemoryStore>();
            break;
#endif

        default:
            store = std::make_unique<MemoryStore>();
            break;
    }

    ReportHandler handler(*store);

    // 线程池：thread_pool_size=0 时禁用（适用于 2 核 2G 小服务器）
    int threadCount = config.getThreadPoolSize();
    ThreadPool* threadPoolPtr = nullptr;
    std::unique_ptr<ThreadPool> threadPool;

    if (threadCount > 0) {
        threadPool = std::make_unique<ThreadPool>();
        threadPool->start(static_cast<std::size_t>(threadCount));
        threadPoolPtr = threadPool.get();
        LOG_INFO("ThreadPool started with " + std::to_string(threadCount) + " threads");
    } else {
        LOG_INFO("ThreadPool disabled (thread_pool_size=0)");
    }

    TcpServer server;
    server.setThreadPool(threadPoolPtr);
    server.setRequestHandler([&handler](const std::string& rawRequest, std::string& response) {
        HttpRequest req;
        if (!HttpParser::parseRequest(rawRequest, req)) {
            response = HttpParser::buildResponse(400, "{\"code\":400,\"message\":\"Invalid request\"}");
            return;
        }

        if (req.method == "POST" && req.path == "/api/v1/requirement/report") {
            JsonValue json = JsonParser::parse(req.body);
            RequirementReportRequest reportReq;
            if (!ReportHandler::parseRequirementReportRequest(json, reportReq)) {
                response = HttpParser::buildResponse(400, "{\"code\":400,\"message\":\"Invalid request body\"}");
                return;
            }
            JsonValue result = handler.handleRequirementReport(reportReq);
            std::string jsonStr = JsonParser::stringify(result);
            response = HttpParser::buildResponse(200, jsonStr);

        } else if (req.method == "GET" && req.path == "/api/v1/requirement/query") {
            RequirementQueryRequest queryReq;
            ReportHandler::parseRequirementQueryRequest(req.query, queryReq);
            JsonValue result = handler.handleRequirementQuery(queryReq);
            std::string jsonStr = JsonParser::stringify(result);
            response = HttpParser::buildResponse(200, jsonStr);

        } else {
            response = HttpParser::buildResponse(404, "{\"code\":404,\"message\":\"Not found\"}");
        }
    });

    int serverPort = config.getServerPort();
    if (!server.listen("0.0.0.0", serverPort)) {
        LOG_ERROR("Failed to start server on port " + std::to_string(serverPort));
        return 1;
    }
    LOG_INFO("Server listening on port " + std::to_string(serverPort));

    std::thread serverThread([&server]() {
        server.run();
    });

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("Shutting down server...");
    server.stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }

    if (threadPool) {
        threadPool->stop();
        LOG_INFO("ThreadPool stopped");
    }

#ifdef ENABLE_MYSQL
    MySQLStore* mysqlStorePtr = dynamic_cast<MySQLStore*>(store.get());
    if (mysqlStorePtr) {
        mysqlStorePtr->shutdown();
        LOG_INFO("MySQL store shutdown");
    }
#endif

    LOG_INFO("RequirementServer stopped.");
    return 0;
}
