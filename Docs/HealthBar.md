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

`Arena.UI.HealthBar.Lifecycle` exercises the native widget through possession and
real instant GameplayEffects, including a maximum health of 200, damage, healing,
lethal damage, respawn, unpossess and widget reconstruction.
