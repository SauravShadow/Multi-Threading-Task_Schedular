# keep-alive.ps1 - Task Scheduler C++ Backend Keep-Alive
# Mirrors the same pattern as FlaskCloudflareServer and PortfolioServer tasks
# Runs at system startup, restarts the server if it crashes

$exePath  = "C:\Users\HP\Desktop\HostedProjects\MultiThreadingTaskSchedular\build\Release\task_schedular.exe"
$workDir  = "C:\Users\HP\Desktop\HostedProjects\MultiThreadingTaskSchedular"
$logFile  = "C:\Users\HP\Desktop\HostedProjects\MultiThreadingTaskSchedular\server.log"

# Log helper
function Write-Log($msg) {
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    "$timestamp  $msg" | Tee-Object -FilePath $logFile -Append | Out-Null
}

Write-Log "=== TaskSchedular Keep-Alive Started ==="

while ($true) {
    Write-Log "Starting task_schedular.exe ..."

    $proc = Start-Process `
        -FilePath       $exePath `
        -WorkingDirectory $workDir `
        -PassThru `
        -NoNewWindow

    Write-Log "Server PID: $($proc.Id)  - listening on http://localhost:8081"

    # Block until the process exits
    $proc.WaitForExit()

    Write-Log "Server exited with code $($proc.ExitCode). Restarting in 5 seconds ..."
    Start-Sleep -Seconds 5
}
