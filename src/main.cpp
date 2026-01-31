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
#include "business/DeviceManager.hpp"
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
    
    // 解析命令行参数
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

    Logger::init("device_server.log");
    LOG_INFO("DeviceDataServer starting...");

    // 加载配置（支持从 build/ 目录运行时查找上级目录的 config.ini）
    Config& config = Config::getInstance();
    if (!config.loadFromFile(configFile)) {
        // 尝试常见相对路径（从 build/ 或 build/Debug/ 运行时的场景）
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
    config.loadFromEnv();  // 环境变量可覆盖配置文件

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 根据配置选择存储模式
    StorageMode storageMode = config.getStorageMode();
    std::unique_ptr<StoreInterface> store;
    DeviceManagerMode deviceMgrMode;

#ifdef ENABLE_MYSQL
    std::unique_ptr<MySQLStore> mysqlStore;
#endif

    switch (storageMode) {
        case StorageMode::MEMORY:
            LOG_INFO("Using MEMORY storage mode");
            store = std::make_unique<MemoryStore>();
            deviceMgrMode = DeviceManagerMode::MEMORY;
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
            
            mysqlStore = std::make_unique<MySQLStore>(config.getBatchSize(), config.getBatchIntervalMs());
            if (!mysqlStore->init(mysqlConfig, poolConfig)) {
                LOG_ERROR("Failed to initialize MySQL store, falling back to memory mode");
                store = std::make_unique<MemoryStore>();
                deviceMgrMode = DeviceManagerMode::MEMORY;
            } else {
                store.reset(mysqlStore.release());
                deviceMgrMode = DeviceManagerMode::MYSQL;
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
            
            mysqlStore = std::make_unique<MySQLStore>(config.getBatchSize(), config.getBatchIntervalMs());
            if (!mysqlStore->init(mysqlConfig, poolConfig)) {
                LOG_ERROR("Failed to initialize MySQL store, falling back to memory mode");
                store = std::make_unique<MemoryStore>();
                deviceMgrMode = DeviceManagerMode::MEMORY;
            } else {
                // 混合模式：暂时使用 MySQL 存储，可以扩展为同时写内存和 MySQL
                store.reset(mysqlStore.release());
                deviceMgrMode = DeviceManagerMode::HYBRID;
            }
            break;
        }
#else
        case StorageMode::MYSQL:
        case StorageMode::HYBRID:
            LOG_WARNING("MySQL support not compiled, falling back to memory mode");
            store = std::make_unique<MemoryStore>();
            deviceMgrMode = DeviceManagerMode::MEMORY;
            break;
#endif

        default:
            store = std::make_unique<MemoryStore>();
            deviceMgrMode = DeviceManagerMode::MEMORY;
            break;
    }

    // 初始化设备管理器
    DeviceManager deviceMgr(deviceMgrMode);
#ifdef ENABLE_MYSQL
    if (deviceMgrMode == DeviceManagerMode::MYSQL || deviceMgrMode == DeviceManagerMode::HYBRID) {
        deviceMgr.setMySQLStore(dynamic_cast<MySQLStore*>(store.get()));
    }
#endif

    ReportHandler handler(*store, deviceMgr);

    // 创建线程池
    int threadCount = config.getThreadPoolSize();
    if (threadCount <= 0) {
        unsigned int hwThreads = std::thread::hardware_concurrency();
        threadCount = (hwThreads > 0) ? static_cast<int>(hwThreads * 2) : 4;
    }
    
    ThreadPool threadPool;
    threadPool.start(static_cast<std::size_t>(threadCount));
    LOG_INFO("ThreadPool started with " + std::to_string(threadCount) + " threads");

    // 创建服务器
    TcpServer server;
    server.setThreadPool(&threadPool);
    server.setRequestHandler([&handler](const std::string& rawRequest, std::string& response) {
        HttpRequest req;
        if (!HttpParser::parseRequest(rawRequest, req)) {
            response = HttpParser::buildResponse(400, "{\"code\":400,\"message\":\"Invalid request\"}");
            return;
        }

        if (req.method == "POST" && req.path == "/api/v1/device/report") {
            // 处理上报请求
            JsonValue json = JsonParser::parse(req.body);
            ReportRequest reportReq;
            if (!ReportHandler::parseReportRequest(json, reportReq)) {
                response = HttpParser::buildResponse(400, "{\"code\":400,\"message\":\"Invalid request body\"}");
                return;
            }

            JsonValue result = handler.handleReport(reportReq);
            std::string jsonStr = JsonParser::stringify(result);
            response = HttpParser::buildResponse(200, jsonStr);

        } else if (req.method == "GET" && req.path == "/api/v1/device/query") {
            // 处理查询请求
            QueryRequest queryReq;
            if (!ReportHandler::parseQueryRequest(req.query, queryReq)) {
                response = HttpParser::buildResponse(400, "{\"code\":400,\"message\":\"Missing device_id\"}");
                return;
            }

            JsonValue result = handler.handleQuery(queryReq);
            std::string jsonStr = JsonParser::stringify(result);
            response = HttpParser::buildResponse(200, jsonStr);

        } else {
            response = HttpParser::buildResponse(404, "{\"code\":404,\"message\":\"Not found\"}");
        }
    });

    // 启动服务器
    int serverPort = config.getServerPort();
    if (!server.listen("0.0.0.0", serverPort)) {
        LOG_ERROR("Failed to start server on port " + std::to_string(serverPort));
        return 1;
    }
    LOG_INFO("Server listening on port " + std::to_string(serverPort));

    // 运行服务器（在单独线程中，以便主线程可以处理信号）
    std::thread serverThread([&server]() {
        server.run();
    });

    // 等待退出信号
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("Shutting down server...");
    server.stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    // 停止线程池
    threadPool.stop();
    LOG_INFO("ThreadPool stopped");

#ifdef ENABLE_MYSQL
    // 关闭 MySQL 存储
    if (storageMode == StorageMode::MYSQL || storageMode == StorageMode::HYBRID) {
        MySQLStore* mysqlStorePtr = dynamic_cast<MySQLStore*>(store.get());
        if (mysqlStorePtr) {
            mysqlStorePtr->shutdown();
            LOG_INFO("MySQL store shutdown");
        }
    }
#endif

    LOG_INFO("DeviceDataServer stopped.");
    return 0;
}
