# Health bar logic and presentation

`/Game/UI/Hud/HealthBar/W_Healthbar` derives from `UArenaHealthBarWidget`.

The C++ widget observes its owning controller's possessed pawn and subscribes to
`UArenaHealthComponent::OnHealthChanged` and `OnMaxHealthChanged`. It removes
subscriptions when the pawn changes or the widget is destructed, and refreshes
from the current pawn whenever the widget is constructed again.

C++ computes health percentages from the current maximum health, clamps progress
to [0, 1], and classifies healing and zero health. A missing pawn/component
produces a zero snapshot. Initialization and maximum-health notifications refresh
the snapshot without playing a damage/healing transition.

Blueprint implements three presentation events:

- `PrepareHealthbarVisuals`: create dynamic materials.
- `InitializeHealthbarVisuals(Health, NormalizedHealth)`: cancel previous
  animations and callbacks, set material progress and numeric health, and play
  the spawn presentation.
- `PresentHealthbarChange(OldHealth, NewHealth, OldNormalizedHealth,
  NewNormalizedHealth, bHealing, bOutOfHealth)`: apply material parameters,
  interpolate the number and play the selected animation. Lethal damage plays
  elimination after the damage animation finishes. New snapshots and health
  transitions clear the previous damage-finished callback.

Keep GAS queries, health-source subscriptions and health calculations in C++.
Keep widget layout, materials, numeric interpolation and animation timing in the
Blueprint. Health values remain authoritative in the existing GAS health set.

## Presentation (Topbar style)

The layout follows the HealthBar_TopbarStyle design: cross icon and
current health on the left (three-digit reserved width, no maximum), a chamfered
bracket, a slanted track with a bottom rail and 25/50/75% ticks, and three tail
blocks. There is no backplate behind the bar. `HealthBarCanvas` places two
full-size images that share `M_UI_TronHealthBar` (design box 528x66, origin
offset 24/18), selected by the `Layer` scalar of each instance:

| Widget | Instance | Layer content |
| --- | --- | --- |
| `Border` | `MI_UI_TronHealthBar_Frame` | Track, bracket, rail, ticks, tail blocks, cross icon |
| `BarFill` | `MI_UI_TronHealthBar_Fill` | Gradient fill, damage trail, healing hatch, head shards, low-health glow |

`HealthNumber` and `HealAmountText` use the Topbar numeric face
(`Orbitron-Regular_Font`). Fill material inputs: `Health_Current`,
`Health_Updated`, `DamageOrHealing`, `Animate_HealthNewToUpdated` (snapshot
reveal / healing catch-up), `Animate_UpdatedFade` (damage trail retraction),
`ShardScatter` and `LowHealthGlow`. Palette inputs are `UIColor_Own_Primary`,
`Secondary`, `Highlight`, `Core`, `Deep` and `UIColor_Shared_Text`, written by
`ApplySharedPalette` into each instance.

Animations: `OnSpawned` (0.3 s left-to-right reveal), `OnDamaged` (head snaps,
trail holds 0.15 s and retracts over 0.35 s), `OnHealed` (hatch preview, fill
catches up, `+N` above the target), `OnEliminated` (shards scatter, frame fades
to 0.4, number switches to Shared.TextMuted) and the looping `OnLowHealth`
(fill and number breathe 1.0 to 0.55 over 0.8 s). `UpdateLowHealthState` starts
the loop when the normalized health is above zero and at or below the
Blueprint default `LowHealthThreshold` (0.25); no warning color is introduced.

The WBP reads the shared UI palette through `UArenaUIColorLibrary` and owns a
cancelable `ObserveTeam` subscription to its owning controller. Late team
assignment, team changes and widget reconstruction refresh the palette inputs
on its own dynamic material instances; destruction cancels the subscription.
The palette source and integration contract are documented in `UIColors.md`.

`Arena.UI.HealthBar.Lifecycle` exercises the native widget through possession and
real instant GameplayEffects, including a maximum health of 200, damage, healing,
lethal damage, respawn, unpossess and widget reconstruction.
