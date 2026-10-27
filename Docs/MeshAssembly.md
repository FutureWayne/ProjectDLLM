# Mesh assembly lifecycle and asset contract

`UArenaMeshAssemblyComponent` starts the configured Niagara system for supported
visible skeletal meshes. Character initialization calls `AutoPlayAssembly`; the
component retains its next-tick fallback for other owners. Dedicated servers,
hidden owners and dead or dying owners do not start an assembly.

Niagara owns the assembly clock through `User.AssemblyDuration` and completes its
emitters on that clock. The component does not calculate animation progress or
override artistic colors. It restores the original materials after all spawned
systems report `OnSystemFinished`. Death, hiding and `EndPlay` interrupt the
effect and release its components. Cleanup preserves materials replaced by
another system while assembly was active.

`ScanMaterials` maps source materials to compatible scan variants. Each temporary
material receives `AssemblyStartTime` from world game time and `AssemblyDuration`
from the Niagara asset. Its material graph computes progress using the game-time
Time expression, so pausing stops both the material and Niagara clocks.

The component updates mesh bounds through `AssemblyMinZ` and `AssemblyHeight` for
materials, and `User.AssemblyMinZ`, `User.AssemblyHeight`, `User.AssemblyRadius` and
`User.AssemblyCenter` for Niagara. These are geometry inputs for moving meshes.
`User.BodyColor` carries the semantic team color when a team is available; the
asset supplies its own default otherwise. Print colors remain in the Niagara and
scan-material assets. The retired component properties `Duration` and
`PrintingColor` are no longer runtime configuration points.
