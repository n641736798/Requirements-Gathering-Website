# 提交并推送含 front-end 的版本到 GitHub
# 在 PowerShell 中运行: .\git-push-frontend.ps1
# 若遇权限问题，请在项目根目录打开独立终端执行。

$ErrorActionPreference = "Stop"
$repoRoot = $PSScriptRoot
Set-Location $repoRoot

# 移除可能残留的锁文件（若有其他 Git 在使用会删除失败，请先关闭 Cursor 源控制/其他 git）
$lock = Join-Path $repoRoot ".git\index.lock"
if (Test-Path $lock) {
  try { Remove-Item -Force $lock } catch { Write-Warning "无法删除 index.lock，请关闭 Cursor 源控制或其他 Git 后重试。" }
}

Write-Host ">>> git add ..." -ForegroundColor Cyan
git add .
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ">>> git status" -ForegroundColor Cyan
git status

Write-Host ">>> git commit -m '添加 front-end 前端'" -ForegroundColor Cyan
git commit -m "添加 front-end 前端"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ">>> git push origin master" -ForegroundColor Cyan
git push origin master
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`n完成: 已推送到 GitHub." -ForegroundColor Green
