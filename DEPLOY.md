# 需求收集网站 — 部署说明

## 1. 环境要求

- **服务器**：2 核 2G 阿里云 ECS（Linux）
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

构建产物位于 `front-end/dist/`。

## 6. Nginx 配置

```nginx
server {
    listen 80;
    server_name your-domain.com;

    root /path/to/front-end/dist;
    index index.html;
    try_files $uri $uri/ /index.html;

    location /api {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
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
