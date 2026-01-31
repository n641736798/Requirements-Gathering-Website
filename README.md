# 高并发设备数据接入与查询服务

基于 Linux C++ 的高并发设备数据上报与查询服务，支持 HTTP 接口，使用 epoll 实现高并发网络处理。

本项目包含后端服务（C++）和前端应用（React + TypeScript），前后端代码完全分离。

## 功能特性

- **设备数据上报**：接收设备通过 HTTP POST 上报的实时数据
- **设备数据查询**：支持按设备 ID 查询最近 N 条数据
- **高并发支持**：基于 epoll 的 Reactor 模式，支持万级并发连接
- **内存存储**：使用读写锁保护的内存数据结构，读写分离设计
- **自动设备注册**：首次上报时自动注册设备

## 技术栈

- C++17
- Linux epoll（IO 多路复用）
- pthread / std::thread（多线程）
- 自实现简易 HTTP 解析器
- 自实现简易 JSON 解析器
- CMake 构建系统

## 编译

```bash
mkdir build
cd build
cmake ..
make
```

编译后的可执行文件位于 `build/device_server`。

## 运行

```bash
./device_server
```

服务器默认监听 `0.0.0.0:8080`。

## API 接口

### 1. 设备数据上报

**接口**：`POST /api/v1/device/report`

**请求体**（JSON）：
```json
{
  "device_id": "ECG_10086",
  "timestamp": 1700000000,
  "metrics": {
    "heart_rate": 78,
    "spo2": 98
  }
}
```

**响应**（成功）：
```json
{
  "code": 0,
  "message": "ok"
}
```

### 2. 设备数据查询

**接口**：`GET /api/v1/device/query?device_id=ECG_10086&limit=100`

**参数**：
- `device_id`（必填）：设备 ID
- `limit`（选填）：返回数据条数上限，默认 100，最大 1000

**响应**：
```json
{
  "device_id": "ECG_10086",
  "data": [
    { "timestamp": 1700000000, "heart_rate": 78, "spo2": 98 },
    { "timestamp": 1700000010, "heart_rate": 80, "spo2": 97 }
  ]
}
```

## 性能测试

### 使用 curl 测试

**上报数据**：
```bash
curl -X POST http://localhost:8080/api/v1/device/report \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "ECG_10086",
    "timestamp": 1700000000,
    "metrics": {
      "heart_rate": 78,
      "spo2": 98
    }
  }'
```

**查询数据**：
```bash
curl "http://localhost:8080/api/v1/device/query?device_id=ECG_10086&limit=10"
```

### 使用 Apache Bench (ab) 进行压测

```bash
# 安装 ab（如果未安装）
# Ubuntu/Debian: sudo apt-get install apache2-utils
# CentOS/RHEL: sudo yum install httpd-tools

# 准备测试文件
echo '{
  "device_id": "TEST_001",
  "timestamp": 1700000000,
  "metrics": {
    "heart_rate": 78,
    "spo2": 98
  }
}' > test_data.json

# 压测上报接口（1000 请求，10 并发）
ab -n 1000 -c 10 -p test_data.json -T application/json \
   http://localhost:8080/api/v1/device/report

# 压测查询接口（1000 请求，10 并发）
ab -n 1000 -c 10 http://localhost:8080/api/v1/device/query?device_id=TEST_001&limit=10
```

### 使用 wrk 进行压测（推荐）

```bash
# 安装 wrk
# Ubuntu/Debian: sudo apt-get install wrk
# 或从源码编译: https://github.com/wg/wrk

# 压测上报接口
wrk -t4 -c100 -d30s -s report.lua http://localhost:8080/api/v1/device/report

# 创建 report.lua 脚本：
# wrk.method = "POST"
# wrk.body = '{"device_id":"TEST_001","timestamp":1700000000,"metrics":{"heart_rate":78,"spo2":98}}'
# wrk.headers["Content-Type"] = "application/json"

# 压测查询接口
wrk -t4 -c100 -d30s http://localhost:8080/api/v1/device/query?device_id=TEST_001&limit=10
```

## 项目结构

```
.
├── CMakeLists.txt          # 后端 CMake 构建脚本
├── README.md               # 项目说明文档（本文件）
├── 需求文档.md             # 后端需求规格说明书
├── src/                    # 后端源代码（C++）
│   ├── main.cpp           # 程序入口
│   ├── net/               # 网络模块
│   │   ├── TcpServer.cpp  # TCP 服务器（epoll）
│   │   ├── Connection.cpp # 连接管理
│   │   └── HttpParser.cpp # HTTP 解析器
│   ├── business/          # 业务逻辑模块
│   │   ├── ReportHandler.cpp  # 上报/查询处理
│   │   └── DeviceManager.cpp  # 设备管理
│   ├── storage/           # 存储模块
│   │   └── MemoryStore.cpp    # 内存存储
│   ├── thread/            # 线程模块
│   │   ├── ThreadPool.cpp     # 线程池
│   │   └── BlockingQueue.hpp  # 阻塞队列
│   └── utils/             # 工具模块
│       ├── Logger.cpp         # 日志
│       └── JsonParser.cpp     # JSON 解析
├── front-end/             # 前端应用（React + TypeScript）
│   ├── package.json       # 前端依赖配置
│   ├── vite.config.ts     # Vite 构建配置
│   ├── tsconfig.json      # TypeScript 配置
│   ├── README.md          # 前端项目说明文档
│   ├── src/               # 前端源代码
│   │   ├── api/           # API 服务层
│   │   ├── components/   # React 组件
│   │   ├── utils/         # 工具函数
│   │   ├── App.tsx        # 主应用组件
│   │   └── main.tsx       # 应用入口
│   └── public/            # 静态资源
└── build/                 # 后端编译输出目录
```

## 架构设计

- **网络层**：基于 epoll 的 Reactor 模式，边缘触发（ET）模式
- **业务层**：处理 HTTP 请求，解析 JSON，调用存储层
- **存储层**：使用 `std::unordered_map` + `std::vector` 存储数据，读写锁保护
- **线程模型**：单 Reactor 多线程模式（可扩展为多 Reactor）

## 日志

日志文件：`device_server.log`

## 快速开始

### 后端服务

1. **编译后端**：
```bash
mkdir build
cd build
cmake ..
make
```

2. **运行后端**：
```bash
./device_server
```

后端服务默认监听 `0.0.0.0:8080`。

### 前端应用

1. **进入前端目录**：
```bash
cd front-end
```

2. **安装依赖**：
```bash
npm install
```

3. **启动开发服务器**：
```bash
npm run dev
```

前端应用将在 `http://localhost:3000` 启动，并通过代理访问后端 API。

详细的前端使用说明请参考 [front-end/README.md](front-end/README.md)。

## 注意事项

1. 本项目为学习/训练项目，生产环境使用需要进一步完善：
   - 错误处理与重试机制
   - 持久化存储（MySQL/SQLite/Redis）
   - 配置管理
   - 监控与指标收集
   - 更完善的 JSON 解析（支持嵌套对象、数组等）

2. 当前实现为内存存储版本，服务重启后数据会丢失。

3. 建议在 Linux 环境下编译和运行后端（Windows 下需要 WSL 或虚拟机）。

4. 前端应用可以在任何支持 Node.js 的环境下运行。

## 许可证

本项目仅供学习使用。
