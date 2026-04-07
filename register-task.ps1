# register-task.ps1 — Register TaskSchedularServer Scheduled Task
# Run this script as Administrator once to set up auto-start on login.
# Usage: Right-click this file → "Run with PowerShell" (as Admin)

$taskName   = "TaskSchedularServer"
$scriptPath = "C:\Users\HP\Desktop\HostedProjects\MultiThreadingTaskSchedular\keep-alive.ps1"
$workDir    = "C:\Users\HP\Desktop\HostedProjects\MultiThreadingTaskSchedular"

# Remove old task if it exists
if (Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue) {
    Unregister-ScheduledTask -TaskName $taskName -Confirm:$false
    Write-Host "Removed old task: $taskName"
}

$action  = New-ScheduledTaskAction `
    -Execute "powershell.exe" `
    -Argument "-ExecutionPolicy Bypass -WindowStyle Hidden -File `"$scriptPath`"" `
    -WorkingDirectory $workDir

$trigger = New-ScheduledTaskTrigger -AtLogOn

$settings = New-ScheduledTaskSettingsSet `
    -ExecutionTimeLimit (New-TimeSpan -Hours 72) `
    -RestartCount 3 `
    -RestartInterval (New-TimeSpan -Minutes 1) `
    -StartWhenAvailable

Register-ScheduledTask `
    -TaskName    $taskName `
    -Action      $action `
    -Trigger     $trigger `
    -Settings    $settings `
    -Description "C++ Task Scheduler Backend - Port 8081" `
    -RunLevel    Highest `
    -Force

Write-Host "✅ Task '$taskName' registered successfully!" -ForegroundColor Green
Write-Host "   It will auto-start at next login." -ForegroundColor Cyan
Write-Host "   To start it now, run: Start-ScheduledTask -TaskName '$taskName'" -ForegroundColor Cyan
