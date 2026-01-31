# 设备数据监控前端应用

基于 React + TypeScript + Vite 构建的现代化前端应用，用于设备数据的上报和查询。

## 功能特性

- **设备数据上报**：支持向服务器上报设备数据，包括设备ID、时间戳和多个指标
- **设备数据查询**：支持按设备ID查询历史数据，支持分页和自动刷新
- **数据可视化**：使用 Recharts 展示设备数据的趋势图表
- **实时监控**：支持自动刷新功能，实时查看设备数据变化

## 技术栈

- **React 18** - UI 框架
- **TypeScript** - 类型安全
- **Vite** - 构建工具
- **React Router** - 路由管理
- **Axios** - HTTP 客户端
- **Recharts** - 数据可视化

## 安装依赖

```bash
npm install
```

## 开发

```bash
npm run dev
```

应用将在 `http://localhost:3000` 启动。

## 构建

```bash
npm run build
```

构建产物将输出到 `dist` 目录。

## 预览构建结果

```bash
npm run preview
```

## 项目结构

```
src/
├── api/              # API 服务层
│   └── deviceApi.ts  # 设备相关 API
├── components/       # React 组件
│   ├── DeviceReport.tsx    # 数据上报组件
│   ├── DeviceReport.css
│   ├── DeviceQuery.tsx     # 数据查询组件
│   ├── DeviceQuery.css
│   ├── Layout.tsx          # 布局组件
│   └── Layout.css
├── utils/            # 工具函数
│   └── format.ts     # 格式化工具
├── App.tsx           # 主应用组件
├── App.css           # 全局样式
└── main.tsx          # 应用入口
```

## API 配置

前端通过 Vite 的代理功能将 `/api` 请求转发到后端服务器（默认 `http://localhost:8080`）。

如需修改后端地址，请编辑 `vite.config.ts` 中的 `server.proxy` 配置。

## 使用说明

### 数据上报

1. 在"数据上报"页面输入设备ID
2. 添加指标数据（如心率、血氧等）
3. 点击"提交上报"按钮

### 数据查询

1. 在"数据查询"页面输入设备ID
2. 设置查询数据条数（1-1000）
3. 点击"查询"按钮
4. 可选择开启"自动刷新"功能，实时查看最新数据

## 注意事项

- 确保后端服务已启动并运行在 `http://localhost:8080`
- 时间戳格式为 Unix 秒级时间戳
- 指标值支持数字和字符串类型
