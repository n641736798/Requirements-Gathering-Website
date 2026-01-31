# 项目结构说明

本文档说明项目的目录结构和前后端分离的组织方式。

## 目录结构

```
kqx/
├── .gitignore              # Git 忽略文件（包含前后端忽略规则）
├── CMakeLists.txt         # 后端 C++ 项目构建配置
├── README.md              # 项目主说明文档
├── 需求文档.md            # 后端需求规格说明书
│
├── src/                   # 后端源代码（C++）
│   ├── main.cpp          # 后端服务入口
│   ├── net/              # 网络通信模块
│   ├── business/         # 业务逻辑模块
│   ├── storage/           # 数据存储模块
│   ├── thread/            # 多线程模块
│   └── utils/             # 工具模块
│
└── front-end/            # 前端应用（React + TypeScript）
    ├── package.json      # 前端依赖配置
    ├── vite.config.ts    # Vite 构建工具配置
    ├── tsconfig.json     # TypeScript 配置
    ├── .eslintrc.cjs     # ESLint 代码检查配置
    ├── .gitignore        # 前端 Git 忽略文件
    ├── index.html        # HTML 入口文件
    ├── README.md         # 前端项目说明文档
    ├── public/           # 静态资源目录
    │   └── vite.svg
    └── src/              # 前端源代码
        ├── api/          # API 服务层
        ├── components/   # React 组件
        ├── utils/         # 工具函数
        ├── App.tsx       # 主应用组件
        ├── App.css       # 全局样式
        └── main.tsx      # 应用入口
```

## 前后端分离说明

### 后端（C++）

- **位置**：`src/` 目录
- **技术栈**：C++17, CMake, epoll, pthread
- **构建方式**：使用 CMake 编译
- **运行方式**：编译后运行可执行文件
- **端口**：默认监听 8080 端口
- **API 接口**：提供 RESTful API（`/api/v1/device/report` 和 `/api/v1/device/query`）

### 前端（React + TypeScript）

- **位置**：`front-end/` 目录
- **技术栈**：React 18, TypeScript, Vite, React Router, Axios, Recharts
- **构建方式**：使用 npm/yarn/pnpm 安装依赖，Vite 构建
- **运行方式**：开发模式使用 `npm run dev`，生产模式使用 `npm run build`
- **端口**：开发服务器默认 3000 端口
- **代理配置**：通过 Vite 代理将 `/api` 请求转发到后端（`http://localhost:8080`）

## 开发工作流

### 后端开发

1. 修改 `src/` 目录下的 C++ 源代码
2. 在 `build/` 目录下使用 CMake 编译
3. 运行编译后的可执行文件进行测试

### 前端开发

1. 进入 `front-end/` 目录
2. 安装依赖：`npm install`
3. 启动开发服务器：`npm run dev`
4. 修改 `front-end/src/` 目录下的源代码
5. 浏览器自动热更新

### 前后端联调

1. 确保后端服务已启动（监听 8080 端口）
2. 启动前端开发服务器（3000 端口）
3. 前端通过 Vite 代理访问后端 API
4. 修改 `front-end/vite.config.ts` 中的 `server.proxy` 配置可调整后端地址

## 构建和部署

### 后端构建

```bash
mkdir build
cd build
cmake ..
make
```

### 前端构建

```bash
cd front-end
npm install
npm run build
```

构建产物位于 `front-end/dist/` 目录，可以部署到任何静态文件服务器。

## Git 忽略规则

- **后端忽略**：`build/`, `*.o`, `*.a`, `*.so`, `*.exe`, CMake 相关文件等
- **前端忽略**：`front-end/node_modules/`, `front-end/dist/`, `front-end/.vite/` 等
- **通用忽略**：IDE 配置、日志文件、系统文件等

## 注意事项

1. 前后端代码完全独立，可以分别开发和部署
2. 前端通过 HTTP API 与后端通信，不直接访问后端代码
3. 开发时前端使用 Vite 代理，生产环境需要配置反向代理（如 Nginx）
4. 确保后端服务先启动，前端才能正常工作
