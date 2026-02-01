#!/bin/bash
# 后端诊断脚本：排查 curl 无响应问题
# 建议在项目根目录运行：bash scripts/diagnose.sh

cd "$(dirname "$0")/.." 2>/dev/null || true

echo "=== 1. 检查 8080 端口是否有进程监听 ==="
ss -tlnp | grep 8080 || echo "无进程监听 8080，后端可能未启动"
echo ""

echo "=== 2. 检查后端进程 ==="
ps aux | grep -E "device_server|requirement" | grep -v grep || echo "未找到后端进程"
echo ""

echo "=== 3. 测试连接（5 秒超时）==="
curl -v --connect-timeout 5 --max-time 10 http://127.0.0.1:8080/api/v1/health 2>&1 || true
echo ""

echo "=== 4. 检查 config.ini 存储模式 ==="
if [ -f config.ini ]; then
    grep -E "mode|storage" config.ini 2>/dev/null || echo "无法读取 config.ini"
else
    echo "config.ini 不存在，请从 config.ini.example 复制"
fi
echo ""

echo "=== 5. 检查 requirement_server.log 最后 20 行 ==="
if [ -f requirement_server.log ]; then
    tail -20 requirement_server.log
else
    echo "requirement_server.log 不存在"
fi
