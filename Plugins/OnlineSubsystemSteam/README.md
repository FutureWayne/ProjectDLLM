# Project Steam OSSv1 override

Upstream: Epic UE 5.8.3 OnlineSubsystemSteam (5.8.3-release). Source and descriptor copied without binaries or intermediates. The project plugin overrides the engine plugin of the same name.

Local changes isolate lobby search data per query, filter updates against the expected lobby IDs, and retire cancelled tasks without clearing a replacement query. See SteamLobbySearchState.h and CommonUser session regression tests. Keep these changes when updating the engine plugin.

Imported source whitespace is normalized; copyright headers and upstream behavior are preserved except for the documented lobby search fix.

Installed upstream build: UE 5.8.3, changelist 58210709.

Review [LOCAL_CHANGES.patch](LOCAL_CHANGES.patch) for the focused changes against whitespace-normalized upstream files. New files are SteamLobbySearchState.h and SteamLobbySearchStateTests.cpp.

Each task owns a thread-safe expected/received lobby set. Lobby update events accept only IDs expected by the active query; duplicates do not advance completion. Cancelled tasks retire without clearing another task's state. Steam supplies no query ID with a LobbyDataUpdate: a late update for an ID also expected by the retry may refresh that lobby, but unrelated lobby IDs cannot enter the retry.

Validation: ArenaEditor Win64 Development built with UE 5.8.3; CommonUser.Session (5 tests) and Online.Steam.Session.LateLobbyDataIsolation passed. Tests exercise query collection and session recovery without a live Steam connection; dual-account integration remains outstanding.

CommonUser cleanup has an independent 30-second watchdog. Expiry detaches its End/Destroy delegates, invalidates the cleanup generation, completes pending UI work, and requests frontend recovery once. The old OSS session interface remains quarantined and is polled to finish End/Destroy work. Its named session must actually disappear (a NoSession enum alone is insufficient) before multiplayer requests are accepted again. While quarantined, requests fail explicitly instead of leaving the UI Busy. A permanently stalled Steam backend may require restarting the game; removing the local session prematurely would allow old SDK tasks to affect a new GameSession.

CommonUser.Session.CleanupWatchdog uses an OSSv1 NULL-backend fixture for deterministic missing-callback, late-callback, generation, quarantine, and independent-deadline checks. It does not change the project's Steam configuration.

Full recovery-to-host integration is covered by `CommonUser.Integration.CleanupRecoveryHost`: it invokes public HostSession after watchdog expiry and actual backend retirement, then verifies real OSSv1 creation, map travel, a live IP listen server, exactly one success callback, and survival of late old cleanup callbacks. See [the isolated seven-test runner](../CommonUser/Tests/README.md). It exercises creation rather than a two-client join and does not replace Steam/SteamSockets integration testing. The real game-world fixture also exposed two missing parent lifecycle calls in ArenaAudioMixEffectsSubsystem; PostInitialize and OnWorldBeginPlay now call their base implementations.
