# Project Steam OSSv1 override

Upstream: Epic UE 5.8.3 OnlineSubsystemSteam (5.8.3-release). Source and descriptor copied without binaries or intermediates. The project plugin overrides the engine plugin of the same name.

Local changes isolate lobby search data per query, filter updates against the expected lobby IDs, and retire cancelled tasks without clearing a replacement query. See SteamLobbySearchState.h and CommonUser session regression tests. Keep these changes when updating the engine plugin.

Imported source whitespace is normalized; copyright headers and upstream behavior are preserved except for the documented lobby search fix.

Installed upstream build: UE 5.8.3, changelist 58210709.

Review [LOCAL_CHANGES.patch](LOCAL_CHANGES.patch) for the focused changes against whitespace-normalized upstream files. New files are SteamLobbySearchState.h and SteamLobbySearchStateTests.cpp.

Each task owns a thread-safe expected/received lobby set. Lobby update events accept only IDs expected by the active query; duplicates do not advance completion. Cancelled tasks retire without clearing another task's state. Steam supplies no query ID with a LobbyDataUpdate: a late update for an ID also expected by the retry may refresh that lobby, but unrelated lobby IDs cannot enter the retry.

Validation: ArenaEditor Win64 Development built with UE 5.8.3; CommonUser.Session (4 tests) and Online.Steam.Session.LateLobbyDataIsolation passed. Tests exercise query collection and session recovery without a live Steam connection; dual-account integration remains outstanding.
