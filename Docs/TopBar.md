# TDM TopBar

`WBP_CharacterOverlay.TopBar` is an instance of `/Game/UI/Hud/TopBar/WBP_ArenaTopBar`,
whose parent class is `UArenaTopBarWidget`.

## Responsibilities

`UArenaTopBarWidget` (C++) is business logic only. It reads the current GameState's
existing replicated `B_ArenaDeathMatchScoring` component at 20 Hz. The Blueprint schema
adapter is restricted to `TargetScore` and `CountdownTime`, and the widget reads
`Score.Elimination` through `UArenaTeamSubsystem` for blue (team 2) and red (team 1).
`AttackerScore`/`DefenderScore` are Blueprint caches that can remain zero after a real
elimination; they must not drive the HUD. Match rules remain authoritative in the
existing scoring component. Missing state and an unconfigured target show `--` / `--:--`.

C++ pushes values into `BindWidget` members and raises presentation events:

| Member / event | Purpose |
| --- | --- |
| `BlueKillsText`, `RedKillsText`, `BlueGoalText`, `RedGoalText`, `ClockText` | Text values |
| `OnScoreRatiosChanged(BlueRatio, RedRatio, bInitialize)` | Current clamped kill ratios; initialization, missing data and score resets snap to the supplied values |
| `OnTeamScored(TeamIndex, NewKills)` | Kill count increased (never for the initial snapshot) |
| `OnClockStateChanged(State)` | `Syncing / Normal / Warning / Critical / Expired`, thresholds `WarningSeconds` / `CriticalSeconds` |

`WBP_ArenaTopBar` owns all presentation: Orbitron fonts, sizes and outlines, team
colors, glass track, glow rails, ticks, brackets, mirrored score frames, progress bar
styles and fill directions (both teams fill from their outside edge).

The WBP stores the target ratios and interpolates `BlueScoreBar` / `RedScoreBar` in
its Tick graph using its own `ProgressInterpSpeed`. C++ exposes `GetTeamScoreRatio`
and sends ratio changes, without maintaining animated progress or setting bar fills.

The WBP also owns these animations:

- `BlueScorePulse` / `RedScorePulse`: number flash + bounce and an expanding rail pulse.
- `ClockWarningBeat`: looping red clock with pulsing chevrons; the event graph plays it
  at 1x for `Warning`, 2x for `Critical`, and stops (restoring state) otherwise.

The retired TeamWidget is collapsed and its tick disabled to preserve existing
Blueprint references. The separate legacy countdown is collapsed.
`Scripts/topbar_install.py` installs the WBP at top center (1040x73, y=18).

Validation: `Arena.UI.TopBar.Snapshot` creates `WBP_ArenaTopBar` and exercises the
actual TDM Blueprint fields, a target of 75, bound text values, score changes, immediate
business ratios, WBP progress interpolation/clamping, all clock states, negative time,
missing source and teardown.
With rendering enabled it exports previews to Saved/topbar-normal.png and
Saved/topbar-warning.png.
