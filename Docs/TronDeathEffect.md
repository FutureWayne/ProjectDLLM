# CyberNinja 的 Lyra 死亡视觉增强

正式 `GCNL_Death` 当前使用 `NS_DeathCubes_Digital`，从未修改的 Lyra `NS_DeathCubes`
复制，保留视觉增强，并增加轻微的表面向外释放速度。死亡时间线为原来的 2 秒，人物通过已有材质消散修复同步消失。

## 当前效果

保留原方块翻页图、旋转表现、原透明遮罩及原角色表面取色。方块材质增加更亮的霓虹边缘、
扫描闪烁和短促数据亮点，冲击图形的发光增强为原来的 2.4 倍。
`MI_SpriteCubes_Digital` 的 `Digital Visuals / DigitalBoost` 参数默认为 2.2，可调整视觉强度。

材质只改变 Emissive Color。原 Base Color、Normal、Opacity Mask、UV、深度及顶点位置
连接保留。人物的三个 CyberNinja 材质保留正常外观和已有 Lyra 消散连接。

## 原 Lyra 运动、扩散与留存

方块脱离身体时，原 `SetReferencePositionAndStick` 的 `normalContribution` 先从 6 调至
55 cm/s，当前进一步微调为 80 cm/s，使向外扩散更明显。高低质量的两个表面发射器、Spawn/Update 中的对应输入均同步设置。
这项输入在原有贴附和释放逻辑中生效，不新增持续向外的力。
方块的随机生命周期从 0.6–1.5 秒延长到 1.0–2.4 秒，落地后留存更久，
人物本身仍沿用原来的两秒消散时间线。

以下内容均从原版复制并保留：

- 发射器及其启用状态、发射数量和发射率；冲击圈及闪光粒子的生命周期。
- 骨骼采样、贴附、原始释放波传播与释放节奏。
- 角色运动的速度继承（velocityContribution=1.4）、原冲击受力、重力、拖拽、旋转和求解模块。
- 碰撞方式及其参数、质量档配置和原 Sprite Renderer。
- 方块的翻页图及原有尺寸曲线；不再使用体素人形保持阶段或自定义垂直坍落约束。

原 `NS_DeathCubes` 的文件 SHA-256 与最初备份一致。之前的 `NS_DeathVoxels` 和
`NS_DeathCubes_Tron` 保留作为历史资源，正式 Cue 已不使用它们。

## 资源

| 资源 | 用途 |
| --- | --- |
| `Content/Effects/Particles/Impacts/NS_DeathCubes_Digital.uasset` | 原 Lyra 模拟、材质增强与轻微向外释放速度 |
| `Content/Effects/Materials/Cubes/M_SpriteCubes_Digital.uasset` | 原方块父材质的发光增强 |
| `Content/Effects/Materials/Cubes/Instances/MI_SpriteCubes_Digital.uasset` | 保留原实例参数，使用新父材质 |
| `Content/Effects/Materials/BlastGraphics/M_ParticleBlast_Digital.uasset` | 原冲击图形的发光增强 |
| `Content/Effects/Materials/BlastGraphics/M_ParticleBlastDisc_Digital.uasset` | 原圆形冲击图形的发光增强 |
| `Content/GameplayCueNotifies/GCNL_Death.uasset` | 指向新系统，SpawningDuration=2 |

`M_Body`、`M_Head`、`M_Legs` 已通过精确的 Git 忽略例外纳入版本管理。
材质和特效生成辅助库由 Git 忽略，仅供本地制作与审计使用。

## 制作与审计

视觉制作脚本为 `Saved/TronLyraVisual/author_visual.py`。脚本复制原系统和原材质，
仅设置 Sprite Renderer 的材质引用，并在复制材质的发光输出后增加增强节点。
它不调用历史体素制作入口 `StyleDigitalDeath`。

`activate_visual.py` 保存正式 Cue 引用和两秒时长；写回前的 Cue 备份在同一目录。
`UArenaDeathVfxEditorLibrary::InspectDeathSystem` 等待正在进行的编译完成后再读取参数，
避免把异步编译的中间状态当作行为差异。

审计比较物理模块的脚本版本、输入、节点启用状态及连接，同时比较所有发射器状态、
原渲染器类型和 Rapid Iteration 参数。UE 保存时生成的对象序号、引脚 ID、继承快照
和编译缓存不作逐字节相等要求；连接按节点 GUID 与引脚名称解析。
纯视觉阶段的原版对照结果保存在 `Saved/TronLyraVisual/`。最新轻微扩散由
`Saved/TronLyraSpread/author_spread.py` 调用编辑器入口 `SetDeathReleaseSpread` 保存，
不调用历史体素制作入口。这一阶段与修改前的参数差异仅限法线释放速度；
原物理模块版本、连接及原 Lyra 源资产保持一致，见 `Saved/TronLyraSpread/author_report.json`
与 `parameter_diff.txt`。修改前资源备份也在该目录。

## 运行验证

2026-10-21，UE 5.8.3：

- `ArenaEditor Win64 Development` 编译通过。
- 原版对照在内存中执行原 `NS_DeathCubes`，不保存对照时的 Cue 改动。
- 正式红蓝验证直接从磁盘加载 `GCNL_Death`，确认使用 `NS_DeathCubes_Digital`，时长 2 秒。
- 使用实际 `CBP_BLUE`、`CBP_RED` 的 `NewMesh`，位置 (1820, -930, 275)，Yaw 73°，缩放 1.25。
  在隔离参考姿态场景中暂停角色移动组件，网格高度为 165，核对组件偏移。
- GPU SimCache 检查原版与增强版均有原始横向运动；轻微扩散版保留该运动并增加表面释放速度。
- 三个材质与 Niagara 时间线同步，最终模型独立截图无残留。

原版含随机采样，因此对照预览的单个粒子不要求逐个重合。验证覆盖实际组件层级、材质、
原模拟配置及两秒时间线，不覆盖完整联网战斗和队伍分配。

纯视觉阶段报告与预览在 `Saved/TronLyraVisual/`：

- `validation_summary.json`：最终采样数、横向运动和同步结果。
- `LyraVisualComparison.gif`：原 Lyra 与视觉增强版对照。
- `LyraDigitalTeams.gif`：正式红蓝角色的增强效果。

`Saved/TronVoxelDeath/`、`Saved/TronVoxelPlacement/` 中的预览属于已停用的体素坍落版本。

## 上一版轻微扩散验证（2026-10-21）

`ArenaEditor Win64 Development` 编译通过。最新红蓝验证均从磁盘加载正式 Cue 和
`NS_DeathCubes_Digital`，核对高低质量发射器的法线释放速度为 55 cm/s。
审计确认只有四项 Spawn/Update 的 `normalContribution` 输入变化；
物理模块版本、连接、其余参数和未修改的原版 Lyra 源资源保持一致。

实际 `CBP_BLUE`、`CBP_RED` 每组录制 42 帧，继续使用偏移位置、Yaw 73°、缩放 1.25
的测试场景。两组死亡时长均为 2 秒，材质与 Niagara 时间线差值为 0，
最终独立模型截图的可见像素为 0。画面对照可见方块释放后适度向外散开，
后续重力、阻力和碰撞继续使用原配置。原版随机采样会导致单粒子位置差异。
低质量发射器完成参数审计，画面验证使用当前默认质量档；未测试完整联网战斗。

最新结果位于 `Saved/TronLyraSpread/`：

- `validation_summary.json`：红蓝正式角色的帧数、运动、同步和残留检查。
- `SpreadComparison.gif`：左侧扩散前，右侧轻微扩散后。
- `SpreadTeams.gif`：红蓝角色的最新实际效果。
- `author_report.json`、`parameter_diff.txt`：资产及物理参数审计。
- `NS_DeathCubes_Digital_after_editor_close.uasset`：本次写入前的资源备份。

## 当前扩散与留存验证（2026-10-21）

当前制作脚本为 `Saved/TronLyraSpreadLong/author_spread.py`，先调用
`SetDeathReleaseSpread(System, 80)`，再调用 `SetDeathDebrisLifetime(System, 1.0, 2.4)`。
它们只处理高低质量的两个方块表面发射器；其他发射器、重力、阻力、原冲击力、
求解器和全部碰撞参数保持原值。编辑器目标 `ArenaEditor Win64 Development` 编译通过。

资产重载确认保存的新输入值生效。参数差异白名单仅包含两个表面发射器各自的
两项法线释放速度及 Lifetime Min/Max，共 8 项；原物理模块的版本及连接审计通过。
完整导出仍可能包含编译时清理的未连接节点和继承缓存差异，不据此声称文件逐字节相同。
原 Lyra 源资产哈希仍与初始备份一致。

实际 `CBP_BLUE`、`CBP_RED` 各录制 62 帧，采样持续到 6.7 秒。人物和 Niagara
死亡时间线差值为 0，人物消散时长仍为 2 秒，最终独立模型截图可见像素均为 0。
蓝红两组在约 3.4 秒时分别还有 19、25 个方块；约 4.2 秒时均已清理，5.5 秒复核为 0。
上一版在 3.4 秒时已没有方块。画面对照可见更明显的释放扩散和落地留存，后续继续
使用现有重力、阻力和碰撞。随机采样会使逐粒子的轨迹与数量略有差异。
低质量发射器完成参数审计，画面使用当前默认质量档；未测试完整联网战斗。

最新报告和预览位于 `Saved/TronLyraSpreadLong/`：

- `SpreadLongComparison.gif`：左侧上一版，右侧扩散和留存增强后，按相同时间取帧。
- `SpreadLongTeams.gif`：红蓝角色完整留存与清理预览。
- `validation_summary.json`：同步、粒子留存与最终清理结果。
- `author_report.json`、`parameter_diff.txt`：资产及物理输入审计。
- `NS_DeathCubes_Digital_after_editor_close.uasset`：本次保存前的最新资源备份。
