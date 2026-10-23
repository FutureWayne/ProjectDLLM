# TDM TopBar

`WBP_CharacterOverlay.NativeTopBar` uses `UArenaTopBarWidget`.
The native widget reads the current GameState's existing replicated
`B_ArenaDeathMatchScoring` component at 20 Hz. The Blueprint schema adapter is
restricted to `TargetScore` and `CountdownTime`,
and the widget reads `Score.Elimination` through `UArenaTeamSubsystem` for blue (team 2)
and red (team 1). `AttackerScore`/`DefenderScore` are Blueprint caches that can
remain zero after a real elimination; they must not drive the native HUD.
Match rules remain authoritative in the existing scoring component. No reference-image values are used as defaults.
Missing state and an unconfigured target show a syncing placeholder.

The presentation uses the project's Orbitron font, cyan cut contours and light
beams matching the HealthBar, with amber on the opposing side. Both team fills
start at their outside edge. Score changes trigger a numeric bounce and an
expanding rail pulse; progress interpolates and carries a moving light scan.
The clock pulses amber below 30 seconds and accelerates below 10 seconds.
Zero time stops pulsing. Initial snapshots do not animate historical kills.

The retired TeamWidget is collapsed and its tick disabled to preserve existing
Blueprint references. The separate legacy countdown is collapsed.
`Scripts/topbar_install.py` installs the native widget at top center (1040x73,
y=18); its editor helper disables unnecessary designer variables before compile.

Validation: `Arena.UI.TopBar.Snapshot` exercises the actual TDM Blueprint fields,
a target of 75, score changes, progress interpolation/clamping, negative time,
missing source and teardown. With rendering enabled it exports normal and
warning previews to Saved/topbar-normal.png and Saved/topbar-warning.png.
A two-player PIE lethal GameplayEffect validation confirmed a real red-team
elimination increments team tags from 0 to 1 on server and client while both
Blueprint score caches remain zero. The native HUD displays 1/10 with 10% fill.
Evidence: Saved/TopbarKillVerification/kill-state.json and kill00000.png.

