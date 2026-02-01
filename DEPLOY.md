# 需求收集网站 — 部署说明

## 快速部署流程（Ubuntu 服务器）

| 步骤 | 操作 |
|------|------|
| 1 | 数据库初始化（第 2 节） |
| 2 | 复制并修改 `config.ini`（第 3 节） |
| 3 | 编译并启动后端 `./device_server -c ../config.ini`（第 4 节） |
| 4 | 前端构建 `cd front-end && npm run build`（第 5 节） |
| 5 | 配置 Nginx 并重载（第 6 节，使用 `nginx.conf.example`） |
| 6 | 浏览器访问 `http://服务器IP/report` 验证 |

## 1. 环境要求

- **服务器**：2 核 2G 阿里云 ECS（Linux）
- **Node.js**：^20.19.0 或 >=22.12.0（前端构建需要，Vite 7 要求）
- **MySQL**：5.7+ 或 8.0+（可与后端同机或使用云数据库）
- **Nginx**：用于托管前端静态文件并反向代理 API

## 2. 数据库初始化

```bash
mysql -u root -p < sql/init.sql
```

或手动执行 `sql/init.sql` 中的 SQL 创建 `requirement_db` 数据库和 `requirements` 表。

## 3. 配置文件

复制 `config.ini.example` 为 `config.ini` 并修改：

```bash
cp config.ini.example config.ini
```

**2 核 2G 服务器建议配置**：

```ini
[server]
port = 8080
thread_pool_size = 0    ; 关闭线程池，节省内存

[mysql]
host = 127.0.0.1
port = 3306
user = root
password = your_password
database = requirement_db
pool_size_min = 2
pool_size_max = 5

[storage]
mode = mysql           ; 生产使用 MySQL
```

## 4. 后端编译与运行

```bash
mkdir build
cd build
cmake ..
make
./device_server
```

或指定配置文件：

```bash
./device_server -c /path/to/config.ini
```

## 5. 前端构建

```bash
cd front-end
npm install
npm run build
```
或许需要这个：chmod +x node_modules/.bin/*


构建产物位于 `front-end/dist/`。

**若 Node.js 版本低于 20.19**（如 18.x），会收到 `EBADENGINE` 警告。可选：
- **推荐**：升级 Node.js 至 20 LTS 或 22（见下方国内镜像安装）
- **临时**：将 `package.json` 中 `vite` 降级为 `^5.4.0`（支持 Node 18）

### 国内阿里云镜像安装 Node.js（nvm）

若 GitHub 访问较慢，可使用 Gitee + npmmirror 国内镜像：

```bash
# 1. 设置 Node.js 二进制下载镜像（npmmirror 阿里云系，nvm install 时使用）
export NVM_NODEJS_ORG_MIRROR=https://npmmirror.com/mirrors/node

# 2. 设置 nvm 从 Gitee 克隆（必须在安装脚本执行前设置）
export NVM_SOURCE=https://gitee.com/mirrors/nvm.git

# 3. 从 Gitee 下载 nvm 安装脚本并执行
curl -o- https://gitee.com/mirrors/nvm/raw/master/install.sh | bash

# 4. 加载 nvm（或重新登录终端后自动加载）
export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"

# 5. 安装并使用 Node 20（会从 npmmirror 下载，速度快）
nvm install 20
nvm use 20

# 6. 验证
node -v
```

**持久化镜像配置**：在 `~/.bashrc` 中添加：

```bash
export NVM_NODEJS_ORG_MIRROR=https://npmmirror.com/mirrors/node
```

**若 nvm 安装仍卡住**，可直接用 NodeSource 安装 Node 20：

```bash
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt-get install -y nodejs
node -v
```

## 6. Nginx 配置

项目根目录提供 `nginx.conf.example`，可按以下步骤使用：

```bash
# 1. 复制示例配置到 Nginx 目录
sudo cp nginx.conf.example /etc/nginx/sites-available/requirement-site

# 2. 编辑配置，将 root /path/to/project/front-end/dist 改为实际路径
#    例如：root /home/ubuntu/Requirements Gathering Website/front-end/dist;
sudo nano /etc/nginx/sites-available/requirement-site

# 3. 启用站点
sudo ln -sf /etc/nginx/sites-available/requirement-site /etc/nginx/sites-enabled/

# 4. 测试并重载 Nginx
sudo nginx -t && sudo systemctl reload nginx
```

**完整配置示例**（见 `nginx.conf.example`）：

```nginx
server {
    listen 80;
    server_name _;   # 匹配任意域名/IP

    root /path/to/project/front-end/dist;
    index index.html;
    try_files $uri $uri/ /index.html;

    location /api {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }
}
```

## 7. 使用 systemd 管理后端服务（可选）

创建 `/etc/systemd/system/requirement-server.service`：

```ini
[Unit]
Description=Requirement Collection Server
After=network.target mysql.service

[Service]
Type=simple
User=www-data
WorkingDirectory=/path/to/project
ExecStart=/path/to/project/build/device_server -c /path/to/project/config.ini
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable requirement-server
sudo systemctl start requirement-server
```

## 8. 常见问题：上报数据 504 Gateway Timeout

**现象**：前端页面可访问，但提交需求时报 `Request failed with status code 504`。

**可能原因与处理**：

1. **阿里云 SLB 超时**（若使用负载均衡）
   - 控制台 → 负载均衡 → 监听配置 → 编辑 → 将「后端超时时间」调大（如 60 秒）
   - SLB 默认约 15 秒，后端/数据库慢时会超时

2. **Nginx 代理超时**
   - 在 `location /api` 中增加 `proxy_connect_timeout`、`proxy_send_timeout`、`proxy_read_timeout`（见上文第 6 节）

3. **MySQL 连接慢或不可达**
   - 检查 `config.ini` 中 `[mysql]` 的 `host`、`port`、`password`、`database`
   - 若使用云数据库 RDS，确认 ECS 安全组放行访问 RDS 的 3306 端口
   - 查看后端日志：`tail -f requirement_server.log`，是否有 `mysql_real_connect failed` 等错误

4. **后端未启动或崩溃**
   - 先测健康检查：`curl http://127.0.0.1:8080/api/v1/health`（应快速返回 `{"code":0,"message":"ok"}`）
   - 再测上报接口：`curl -X POST http://127.0.0.1:8080/api/v1/requirement/report -H "Content-Type: application/json" -d '{"title":"test","content":"test"}'`
   - 若 health 有响应但 report 无响应（卡住），多半是 **MySQL 连接阻塞**，检查 config.ini 中 `[mysql]` 配置及 MySQL 服务状态

5. **curl 无响应（卡住或连接被拒）**
   - 先加超时测试：`curl -v --connect-timeout 5 --max-time 10 http://127.0.0.1:8080/api/v1/health`
   - **Connection refused**：8080 无进程监听 → 后端未启动或启动失败
     - 检查 `ss -tlnp | grep 8080` 是否有输出
     - 查看 `requirement_server.log`，若卡在 MySQL 初始化，将 config.ini 中 `mode = memory` 后重启
     - 确认已重新编译：`cd build && make`（修改源码后需重新 make）
   - **超时无响应**：有进程监听但不返回 → 可能是其他进程占用 8080，或后端异常
     - 运行 `scripts/diagnose.sh` 做完整诊断
   - 若 diagnose.sh 报 `$'\r': command not found` 或 `unexpected end of file`，执行 `sed -i 's/\r$//' scripts/diagnose.sh` 修复换行符
