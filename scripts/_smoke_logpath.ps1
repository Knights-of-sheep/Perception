# 临时冒烟脚本：验证启动时读取 QSettings 持久化日志路径（FR-016）
$key = 'HKCU:\Software\Perception\Perception'
New-Item -Path $key -Force | Out-Null
New-Item -Path "$key\log" -Force | Out-Null
Set-ItemProperty -Path "$key\log" -Name 'path' -Value 'E:\data\logsmoke\app.log' -Type String
New-Item -ItemType Directory -Force -Path 'E:\data\logsmoke' | Out-Null
Start-Process -FilePath 'E:\spec-work\Perception\bin\Release\perception.exe' -ArgumentList '--snapshot','E:\data\logsmoke_shot.png'
Start-Sleep 5
Write-Output '--- log file ---'
Get-Item 'E:\data\logsmoke\app.log' -ErrorAction SilentlyContinue | Select-Object FullName, Length, LastWriteTime
Get-Content 'E:\data\logsmoke\app.log' -Tail 3 -ErrorAction SilentlyContinue
Remove-Item -Path "$key\log" -Recurse -ErrorAction SilentlyContinue
