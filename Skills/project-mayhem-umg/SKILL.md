---
name: project-mayhem-umg
description: 在 Project Mayhem / Arena 中按 MVC 职责划分制作和审查 Unreal UMG 界面，复用现有 Widget Blueprint 与共享 UI 调色板。设计、新增、修改或审查 UMG、Widget Blueprint（WBP）、HUD、菜单、CommonUI 界面、UI 材质，或接入其业务数据、交互、状态、配色及动画时使用。仅涉及玩法逻辑、世界特效或非 UI 材质的任务无需调用。
---

# Project Mayhem UMG

## 开始前

1. 从实际入口找到最接近的既有界面，确认 WBP、父类、创建或激活方式、数据来源、输入路由和刷新时机。检查 HUD/UI Layer、CommonUI 或 GameplayCue 等实际路径。优先复用已接入的控件、样式、材质与数据接口，沿用同目录资产命名。
2. 根据任务选择下表的实现，结合对应维护文档确认要复用的部分。涉及 C++ 新增、修改或审查时，先完整阅读 [编码规范](../../Docs/CodingStandards.md)。
3. 涉及配色、材质或状态表现时，先读 [UI 配色参考](references/ui-color-palette.md)，按语义选择颜色。API、蓝图节点与现有接入路径见 [UI 共享阵营调色板](../../Docs/UIColors.md)。

| 任务 | 参考现有实现 |
| --- | --- |
| HUD 数据与动画分离 | [血条](../../Docs/HealthBar.md) 的 `/Game/UI/Hud/HealthBar/W_Healthbar`。原生父类订阅 GAS 生命值并发出初始快照和变化事件，WBP 负责数字插值、材质和伤害/治疗动画。 |
| 分数、倒计时与状态事件 | [顶部比分栏](../../Docs/TopBar.md) 的 `/Game/UI/Hud/TopBar/WBP_ArenaTopBar`。参考目标比例和时钟状态事件，以及 WBP 里的进度插值和动画。 |
| 列表项复用与监听清理 | [计分板](../../Docs/ScoreboardListenerLifecycle.md) 的 `/Game/UI/Scoreboard/PlayerRow/W_SB_PlayerStat`。参考重复赋值、重建和世界切换时的监听生命周期。 |

## MVC 职责划分

- **Model** 持有权威业务状态和规则。优先使用已有的 GAS、PlayerState、GameState、组件或 Subsystem。UI 可以保留只读快照或展示所需的派生值，业务操作通过已有 API 发出。
- **Controller 职责** 负责订阅、解绑、输入转发及业务状态转换。简单界面可由现有原生 Widget 父类或已有控制器承担这些职责。只有多个 View 共享绑定逻辑或交互流程需要独立管理时，再提取 Controller。
- **View** 由 WBP 及其 UI 材质、样式资产实现。WBP 消费初始快照与变化事件，将输入转交已有业务入口，并定义布局、字体、颜色用途、可见性、材质参数、视觉插值、动画关键帧与播放、取消和重置时机。
- **C++ 边界** 传递数值、状态和事件，优先复用只读属性、委托或 Blueprint 事件。现有 `BindWidget` 文本字段可继续承载业务数据。禁止在 C++ 写控件布局、样式、Tint、透明度、材质表现参数、视觉插值、`PlayAnimation` 或动画时序。以「受伤」「得分」「进入倒计时警告」等业务语义描述变化，由 WBP 决定如何呈现。
- **生命周期** 绑定后先读取当前状态，区分初始快照与实际变化，避免打开界面就误播变化动画。更换数据源时先解绑旧源，重开或复用时刷新，并在对应的关闭、停用、销毁或世界清理时释放监听。优先使用已有事件和委托驱动更新。

## 使用共享调色板

- 基础色统一来自 `/Game/UI/Foundation/Colors/DA_ArenaUIColorPalette`，由项目设置 `Arena UI Colors` 指定。通过 `UArenaUIColorLibrary`（蓝图分类 `Arena|UI|Colors`）按角色查询 `GetTeamColor(TeamId, Role)` 或 `GetSharedColor(Role)`，需要完整色组时使用 `GetTeamPalette` 或 `GetSharedPalette`。禁止用旧 `UArenaTeamSubsystem` 色值接口为 UI 取色，也禁止在 WBP、UI 材质、动画、富文本或 C++ 中写死基础色或维护副本。
- 根据实际展示对象选择 TeamId。自身 HUD 取所属玩家，名称标记、击杀信息和列表行取显示对象，双方比分各取对应阵营。TeamId 1 固定橙红，2 固定青蓝。未分配或不支持的队伍使用库的中性回退。
- 在创建、重开、对象变化或阵营变化时应用颜色，复用 `ObserveTeam` 并清理监听，避免逐帧查询。由 WBP 将颜色写入控件与各自的动态材质实例，替换画刷后重新应用。检查父子 Tint 是否重复乘色。材质和动画独立控制 Alpha、发光与时序，基础 RGB 来自查询结果。
- 修改基础色或新增语义色时，同步维护调色板资产、[配色参考](references/ui-color-palette.md)及项目色卡。资产缺失时修复设置或资产引用。

## 验证

- 通过 UE 编辑器或受支持的资产 API 修改 `.uasset`，检查蓝图编译、引用和加载。由生产入口打开界面，检查实际存在的默认、悬停、按下、选中、禁用与过渡状态。
- 检查实际画面中的可读性、材质叠加、动画打断与重置。涉及阵营时，检查未分队、初次分队、切队、界面重开及双方客户端的颜色。涉及订阅时，检查关闭、重建或列表复用是否产生重复回调。
- 按改动运行相关已有回归，例如 `Arena.UI.Colors`、`Arena.UI.HealthBar.Lifecycle`、`Arena.UI.TopBar.Snapshot`。交付时记录实际执行的检查与未覆盖范围。
