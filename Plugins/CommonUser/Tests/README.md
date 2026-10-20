# OSSv1 session regression suite

Compile `ArenaEditor`, then run the C++ tests in an isolated editor process (UE 5.8.3) from the repository root:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' ./Arena.uproject /Engine/Maps/Entry `
    '-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL' `
    '-ExecCmds=Automation RunTests CommonUser.Session+CommonUser.Integration.CleanupRecoveryHost+Online.Steam.Session.LateLobbyDataIsolation' `
    '-TestExit=Automation Test Queue Empty' -unattended -NullRHI -nosplash -nosound -nop4
```

Check the automation summary in the log for all seven tests; Unreal may exit zero after failed assertions. Asset configuration scripts and regression runners are local tools and are excluded from version control.

`CommonUser.Integration.CleanupRecoveryHost` uses the initialized production CommonSessionSubsystem and public HostSession API. It creates an isolated game world, injects an old backend stuck in Destroying, expires the real cleanup watchdog, verifies rejection while quarantined, lets the backend recover, and retries hosting. The retry creates a real OSSv1 NULL session, performs ServerTravel and map loading, starts an IP listen server, and completes through the production PostLoadMapWithWorld handler. It asserts no early success, one final success, released Busy, a live net driver, an active replacement session, and preservation of the replacement after old cleanup callbacks.

Only the old backend state and cleanup deadline are fault-injected; the successful creation and travel path is not manually completed. The fixture uses the supported server-host API without a viewport/local player. It checks recovery notification, not frontend widget navigation.

The command selects NULL using a command-line INI override for that process. The fixture temporarily uses IpNetDriver, duplicates an already-loaded Entry world, and restores its driver definitions, asset type, world context and GWorld. It never writes project configuration or changes production Steam/SteamSockets settings. Run it with the normal listen port 7777 available.

Existing state/lobby isolation regressions run in the same process after fixture teardown. This is deterministic OSSv1 integration coverage, not Steam SDK/SteamSockets interoperability or dual-account join coverage; those remain separate manual/integration checks.
