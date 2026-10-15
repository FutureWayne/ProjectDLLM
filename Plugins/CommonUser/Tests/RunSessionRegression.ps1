[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$ProjectPath = '',
    [string]$OutputDirectory = '',
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    $ProjectPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..\Arena.uproject'))
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path (Split-Path $ProjectPath) 'Saved\Automation\SessionRegression'
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
if (-not $SkipBuild) {
    & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') ArenaEditor Win64 Development "-Project=$ProjectPath" -WaitMutex -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) { throw "ArenaEditor build failed: $LASTEXITCODE" }
}

# NULL and IP networking apply only to this isolated fixture process. The test
# restores its transient driver/asset/world changes; no project INI is written.
$editorPath = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
& $editorPath $ProjectPath /Engine/Maps/Entry `
    '-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL' `
    '-ExecCmds=Automation RunTests CommonUser.Session+CommonUser.Integration.CleanupRecoveryHost+Online.Steam.Session.LateLobbyDataIsolation' `
    '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$OutputDirectory" `
    -unattended -NullRHI -nosplash -nosound -nop4 "-abslog=$(Join-Path $OutputDirectory 'Automation.log')"
if ($LASTEXITCODE -ne 0) { throw "Editor process failed: $LASTEXITCODE" }

# Unreal can exit zero even when an automation assertion fails. Check the report
# and test identities, rather than accepting the process exit code alone.
$report = Get-Content -LiteralPath (Join-Path $OutputDirectory 'index.json') -Raw | ConvertFrom-Json
$expected = @('CleanupRecoveryHost', 'CleanupTimeoutRecovery', 'CleanupWatchdog', 'RecoveryRegressions', 'RequestLifecycle', 'SearchReentry', 'LateLobbyDataIsolation')
foreach ($name in $expected) {
    $matches = @($report.tests | Where-Object { $_.testDisplayName -eq $name })
    if ($matches.Count -ne 1 -or $matches[0].state -ne 'Success') {
        throw "Automation test missing or failed: $name. See $OutputDirectory"
    }
}
if (@($report.tests | Where-Object { $_.state -ne 'Success' }).Count -gt 0) {
    throw "Additional automation failures found. See $OutputDirectory"
}
Write-Output "All $($expected.Count) session regressions passed. Report: $OutputDirectory"
