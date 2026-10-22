# 可配置地面区域

燃烧弹允许空爆。`AArenaGrenadeBase` 只负责引信、碰撞策略、爆炸和通用效果生成，
不再判断燃烧弹类型、等待落地、回查轨迹或初始化燃烧逻辑。
正式燃烧弹先在弹体所在位置执行原有爆炸 Cue，并通过 `ExplosionGameplayEffect` 施加瞬时爆炸伤害，再从空爆点生成火场。
爆炸沿用 `GE_DamageGrenade` 与距离衰减曲线，`DetonationRadius=550cm`；直击沿用 `GE_DamageInstantKill`。
这两个字段属于手雷本身，配置或迁移地面火场时必须保留。
火场保持 X/Y 不变，加速垂直下落；逐段检测首次接触的表面，落地后才扩散并施加 GE。
下落时间不占用地面持续时间；下落中的火场不提前铺火或施加持续伤害，爆炸瞬间的范围伤害独立计算。
通用区域默认仍可直接投影到地面，燃烧弹通过 `FallToGround` 开启下落。
超出最大下落距离仍未接触有效表面时，区域自行销毁。
投掷能力会从引信扣除蓄力及投掷动画的时间；`SetGrenadeExplosionTimer` 收到剩余时间小于等于 0 时立即引爆，
不会清掉引信后留下无法爆炸的弹体。定义中的 `TimeBeforeExplosion <= 0` 仍表示发射时不自动启动引信。

## 配置入口

正式燃烧弹的 `DA_IncendiaryGrenadeDef → EffectActorsToSpawn`：

| 字段 | 用途 |
| --- | --- |
| `SpawnPolicy` | 爆炸生成、碰撞生成或关闭 |
| `EffectActorClass` | 地面区域使用 `ArenaGroundAreaEffect`，也可配置其他效果 Actor |
| `EffectDefinition` | 独立配置资产，燃烧弹使用 `DA_IncendiaryGroundArea` |
| `SpawnCount` | 一次生成多少片区域，0–64，默认 1 |
| `SpawnRadius` | 多片区域中心的水平散布半径，cm，第一片位于爆炸点下方 |

`DA_IncendiaryGroundArea` 位于 `/Game/Blueprints/Weapon/Data/GrenadeData/SecondaryGrenade`，
类型为 `ArenaGroundAreaDefinition`。每片区域的 `Settings`：

| 字段 | 用途 |
| --- | --- |
| `Radius` | 单片区域逻辑半径，cm，与数量和粒子缩放独立 |
| `Lifetime`、`DestroyDelay` | 生效时间、结束后的 Actor 清理延时 |
| `GameplayEffect`、`EffectLevel` | GE 和等级，支持 Instant、Duration、Infinite |
| `ApplicationInterval` | 区域检测和 Instant GE 施加间隔，默认 0.25 秒 |
| `SetByCallerMagnitudes` | 任意 Tag → 数值，例如伤害、减速倍率 |
| `EffectAssetTags` | 追加到 GE Spec 的标签 |
| `RemoveEffectOnExit` | 离开、区域到期或销毁时移除该区域施加的持续效果 |
| `AffectInstigator`、`AffectEnemies`、`AffectAllies` | 投掷者、敌方、友方的作用规则 |
| `CellEffect`、`FullAreaEffect`、`EffectScale` | 分格 Niagara、可选整片 Niagara、粒子缩放 |
| `MaxVisualCount` | 每片区域 Niagara 实例上限，不改变逻辑范围或 GE 强度 |
| `ClientCellMerge`、`VisualsPerFrame` | 表现合并密度、每帧创建预算 |
| `GroundTraceDepth` | 向下投影或下落的最大距离，默认 3000cm |
| `FallToGround`、`FallAcceleration`、`TerminalFallSpeed` | 开启垂直下落、下落加速度、最大下落速度 |
| `FallingEffect`、`FallingEffectScale` | 下落阶段的 Niagara 和缩放，落地后停止发射，已有粒子自然消散，同时开始贴地火焰 |
| `CellSize`、`CellsPerFrame` | 逻辑精度、每帧扩散检查预算 |
| `MaxRise`、`MaxDrop`、`StepHeight`、`EffectHeight` | 地形范围、台阶连通与有效高度 |
| `SurfaceMaxAngle`、`MinClearance` | 地面坡度上限、最低净空 |

半径支持 `CellSize` 到 `20 * CellSize`；越界配置在验证时提示，运行时钳制。
区域保持 Yaw，清除 Pitch/Roll/缩放，使用分帧 BFS 和表面连通射线铺设，保留墙面、台阶、悬崖和净空限制。

## 火焰和冰冻

正式火焰保留 `GE_IncendiaryDamage` 蓝图：Instant，
`ArenaHealthSet.Damage / Additive / SetByCaller.Incendiary.Damage`。
独立配置传入每轮 5 点伤害，半径 300cm，持续 10 秒，自身与敌方受到伤害，其他队友免伤。
正式燃烧弹的 `GroundTraceDepth=10000cm` 限制最大下落距离；加速度 1800cm/s²，最大速度 2400cm/s。
`NS_IncendiaryFallingFire` 使用独立的空中火滴表现：世界空间粒子在有纵向厚度的体积中随机生成，
每颗火滴独立运动、消散，形成有空隙和拖尾的燃烧碎片雨。落地后停止空中发射并开始连续铺火，
已有空中粒子脱离区域组件，继续运动至寿命结束，再自动清理，避免接触地面时突然消失。
空中特效的 `Fire_Licks` 使用圆柱半径 450、高度 800、非均匀缩放 `(1, 1, 1)`，
组件缩放 0.6；粒子大小 60–120、寿命 1.1–1.8 秒、发射密度 `User.FlamesEnableWisps=10`。
`AddVelocity001` 的 `UniformRangedVector` 随机输入控制各粒子的速度差异，范围为
`(-60, -60, -320)` 至 `(60, 60, -80)` cm/s。
`FallingFireTurbulence` 在速度求解前施加 Curl Noise，当前 `Noise Strength=90`、`Noise Frequency=12`，
使粒子横向飘动、改变运动方向；这些表现参数可直接在 Niagara 中维护。
空中渲染使用 `MI_IncendiaryFuelTrail`，以 `T_FireTiny_02` 的 8×8 火焰序列形成卷动的尾焰，
沿粒子速度方向对齐，向运动反方向延伸；各粒子的动画相位、长度和摆动存在差异，寿命末段逐渐消隐。
头部由两层错相的火焰序列叠加，头部长度根据粒子宽度和尾迹长度计算，避免长尾把火滴头部拉成细线。
`HeadWidth=1.8`、`HeadAspect=0.85` 控制头部宽度和纵横比例；尾部通过 `TailRootWidth=1.3`、
`TailTipWidth=0.16` 逐渐收窄，`TailBreakup=0.9` 控制末段的流动噪声侵蚀和破碎消散。
`FlameDetail=0.8` 控制流动的明暗细节，减少整块发亮；材质实例中的 `Brightness=1.4` 控制亮度。
`TrailSeconds=0.6` 控制随速度变化的延伸长度，
`MinTrailLength=150`、`MaxTrailLength=320` 将基础长度限制在 150–320cm；
`TailLengthRandomness=0.65` 再按各粒子的固定随机数，将长度乘以 0.35–1.65，形成明显的长短差异。
Initialize Particle 的 `Material Random` 使用 0–1 随机输入，控制各粒子的长度、卷动相位和序列帧错位。
`FlameFPS=24` 控制动画速度，`FlameDrift=0.14` 控制两层不同频率的弯曲摆动，尾端摆幅大于火滴头部。
透明度取自序列贴图的 Alpha 通道，并在粒子寿命后段渐隐。
序列帧由材质选择，渲染器的 `SubImageSize=(1, 1)` 且清空 `SubImageIndexBinding`，避免重复偏移 UV。
区域 Actor 继续按服务器轨迹垂直下落，火滴的散布不改变落地判定和伤害开始时间。
`ExplosionCueTag` 使用原有 `GameplayCue.Grenade.IncendiaryGrenade`，在空爆点执行 `NS_Grenade_Explosion`。
手雷发射时准备所配置的爆炸 Cue 类，爆炸前再次确认已加载，避免 Cue 异步加载时弹体已销毁而丢失爆炸表现。
粒子仍使用 `NS_IncendiaryGroundFire`，显示合并密度 3、缩放 0.6。

地面火焰使用独立 Niagara，以 `Fire_Licks` 在整块显示区域随机播撒小火舌。
实际分布为圆柱平面，半径输入为 `ShapeLocation.Cylinder Radius` 链接的 `Multiply_Float002.A`，
当前 135；球形半径不参与该分布。粒子大小在 45–90 之间随机变化，寿命为 0.6–1.2 秒，
`User.FlamesEnableWisps=26` 控制发射密度，再统一应用组件缩放 0.6。
相邻播撒范围互相覆盖，关闭每格固定的中心火焰和朝向一致的底层贴片，形成连续、自然变化的火面。
当前使用 `MI_IncendiaryFlameLicks`，位于 `/Game/Effects/Materials/Incendiary`；
原有立面、贴地材质保留供美术选择，原始爆炸特效与材质保持不变。
`M_IncendiaryOrganicFlame` 在整片火区上叠加不规则边界和稀疏外缘，材质中的
`GroundEdgeFeather` 控制边缘渐隐宽度，`GroundEdgeRoughness` 控制凹凸程度。
区域通过 Niagara 的可选 `User.GroundAreaMaterial` 材质绑定，传入整片区域的
`GroundAreaCenter`、`GroundAreaRadius` 并开启 `GroundAreaMaskEnabled`；各显示格共用同一坐标基准，
边界随落点和区域半径变化。未绑定该参数的其他区域特效继续使用原材质。
日常美术调整直接编辑这些 Niagara/材质资产，无需修改 C++ 或重复运行生成脚本。
自身伤害由 `Settings.AffectInstigator` 控制，与 `AffectAllies` 分开。

新增冰冻区：

1. 新建或复制 `ArenaGroundAreaDefinition`，把 `GameplayEffect` 换成冰冻/减速 GE，`CellEffect` 换成冰霜 Niagara。
2. 配置半径、持续时间和需要的 SetByCaller 参数；在手雷条目配置生成数量和散布半径。
3. 使用 Duration 或 Infinite GE，开启 `RemoveEffectOnExit`，离开区域即可恢复移速。
4. 在手雷效果条目引用该定义，无需修改 Grenade 基类或新增 C++ 区域子类。

Instant GE 按间隔重复施加。持续型 GE 在目标进入时施加，存续期间不会每轮叠加；
GE 自身先到期且目标仍在区域内时，下一轮可重新施加。持续型 GE 自带 Period 时，其周期由 GE 管理。
区域检测的离开响应延时最多为 `ApplicationInterval`；到期和销毁会立即清理所管理的持续 GE。
区域在生成时准备 GE Spec，保留 ASC Owner（玩家为 PlayerState）的伤害归属，并将区域设为 EffectCauser。
后续每轮由目标 ASC 应用该 Spec，目标属性仍在应用时捕获；投掷者 Pawn 或其 ASC 销毁后，区域仍可持续生效。
来源标签与快照属性以区域生成时为准，非快照属性遵循 GAS 的捕获规则。
关闭 `RemoveEffectOnExit` 后，持续效果按 GE 自身时长保留；Infinite 会一直保留。

多片区域是独立来源，同一目标每片区域每轮最多施加一次，不同区域之间的叠加遵循 GE 的 Stacking 配置。
需要按区域移除的 GE 应避免把不同区域的效果合并成封顶的共享 Stack，因为 GAS 无法再区分各区域的 Stack 所有权。

## 同步

客户端只接收下落起点、服务器开始时间、落地状态、配置及显示格，不生成逻辑网格、不施加 GE。
下落轨迹在两端按服务器时间计算，服务端负责首次表面接触并同步最终落地位置。
显示格使用有效代表点，数量受 `MaxVisualCount` 限制；Niagara 按每帧预算创建，监听服务器使用相同表现逻辑。
可选整片 Niagara 仅在平整且覆盖率足够时使用。

地面射线忽略 Pawn、手雷和 EffectActor；其他部署物可在 Actor 或组件添加 `GroundArea.TraceIgnore`，
旧的 `Incendiary.TraceIgnore` 标签仍兼容。

## 迁移与回归

旧的 `IncendiarySettings` 迁移字段已移除，运行配置统一使用独立区域定义。
正式资产与测试统一使用 `ArenaGroundAreaEffect`，燃烧等具体行为由独立区域定义配置，无需专用兼容子类。

资产配置以提交的 `.uasset` 为准，直接在编辑器中维护；资产配置脚本和回归运行脚本仅保留在本地。

正式 C++ 测试位于 `Source/ArenaEditor/Private/Tests`。编译 `ArenaEditor` 后，在 UE 自动化测试窗口运行
`Arena.Incendiary`，验证物理地形、空爆/无地面、多片区域、伤害与区域到期、减速 GE 生命周期与真实客户端复制。

- `Arena.Incendiary.Visual.GroundFireCapture`：使用 `-RenderOffscreen` 输出真实空爆、下落、落地残焰、地面持续火焰四阶段的火焰图，并验证残焰自然结束和清理。

- `Arena.Incendiary.Damage.ProductionExplosionDamage`：验证正式燃烧弹空爆时立即造成伤害，遵循墙体遮挡、伤害免疫和爆炸半径，随后落地火场仍每轮造成 5 点伤害。
