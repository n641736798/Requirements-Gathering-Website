# 需求收集网站

基于 C++ 后端 + React 前端的需求收集网站，服务「有需求的人」和「愿意解决需求的人」两个群体。支持需求上报与查询，适配 2 核 2G 阿里云服务器。

## 功能特性

- **需求上报**：有需求的人填写题目、内容、愿意付费、联系方式、备注并提交
- **需求查询**：愿意解决需求的人浏览最近 100 条需求，支持关键词搜索、愿意付费筛选、分页
- **内存/MySQL 存储**：支持内存模式（开发）和 MySQL 模式（生产）
- **线程池可配置**：`thread_pool_size=0` 可关闭线程池，适用于小服务器

## 技术栈

- **后端**：C++17、epoll、自实现 HTTP/JSON 解析
- **前端**：React 18、TypeScript、Vite
- **数据库**：MySQL（可选）

## 快速开始

### 1. 后端

```bash
mkdir build
cd build
cmake ..
make
./device_server
```

默认监听 `0.0.0.0:8080`。首次运行建议复制 `config.ini.example` 为 `config.ini` 并修改配置。

### 2. 前端开发

```bash
cd front-end
npm install
npm run dev
```

访问 `http://localhost:3000`，前端通过 Vite 代理访问后端 API。

### 3. 生产部署

详见 [DEPLOY.md](DEPLOY.md)。

## API 接口

### 需求上报

`POST /api/v1/requirement/report`

```json
{
  "title": "需要开发一个xxx功能",
  "content": "详细描述...",
  "willing_to_pay": 1,
  "contact": "email@example.com",
  "notes": "可选备注"
}
```

- `title`、`content`：必填
- `willing_to_pay`：0=不愿意，1=愿意，null=不填
- `contact`、`notes`：选填

### 需求查询

`GET /api/v1/requirement/query?page=1&limit=100&keyword=xxx&willing_to_pay=1`

- `page`：页码，默认 1
- `limit`：每页条数，默认 100
- `keyword`：关键词模糊搜索（题目/内容）
- `willing_to_pay`：筛选 0/1/2（2=未填）

## 项目结构

```
├── src/                 # C++ 后端
│   ├── main.cpp
│   ├── net/             # 网络模块
│   ├── business/        # 业务逻辑
│   ├── storage/         # 存储（MemoryStore/MySQLStore）
│   ├── thread/          # 线程池
│   └── utils/
├── front-end/           # React 前端
├── sql/init.sql         # 数据库初始化
├── config.ini.example   # 配置示例
└── DEPLOY.md            # 部署说明
```

## 配置说明

| 配置项 | 说明 | 2核2G 建议 |
|--------|------|------------|
| `thread_pool_size` | 0=禁用线程池 | 0 |
| `pool_size_min/max` | MySQL 连接池 | 2/5 |
| `storage.mode` | memory/mysql/hybrid | mysql |

## 许可证

本项目仅供学习使用。
