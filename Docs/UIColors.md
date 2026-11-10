# UI 共享阵营调色板

基础色值集中在 `/Game/UI/Foundation/Colors/DA_ArenaUIColorPalette`。修改颜色时同步更新 [UI 配色参考](../Skills/project-mayhem-umg/references/ui-color-palette.md) 和 [项目色卡](Art/FactionColorPalette.png)。

## 类型与接口

`FArenaFactionPalette` 保存 Primary、Highlight、Core、Secondary、Muted、Edge、Surface、Deep；`FArenaSharedPalette` 保存 Background、Panel、Text、TextMuted。字段均为线性 `FLinearColor`，蓝图只读，资产中可编辑。按色卡 sRGB 数值转换后录入，基础色 Alpha 固定为 1。

项目设置中的 Arena UI Colors 通过 `UArenaUIColorSettings` 配置唯一的调色板软引用。查询首次加载并持有资产，不依赖 World，支持设计器预览。缺失资产会输出错误并返回透明空值；配置和资产验证会报告失败，不生成备用色卡。

`UArenaUIColorLibrary` 的 C++ 静态函数和蓝图纯节点统一位于 `Arena|UI|Colors`：

- `GetTeamPalette(TeamId)`：完整阵营调色板。
- `GetTeamColor(TeamId, Role)`：指定 `EArenaFactionColorRole` 的颜色。
- `GetSharedPalette()`：完整共用调色板。
- `GetSharedColor(Role)`：指定 `EArenaSharedColorRole` 的颜色。

TeamId 1 为橙红，2 为青蓝。其他编号（包括 0、INDEX_NONE）使用中性回退：Primary / Highlight / Core 使用 Shared.Text；Secondary / Muted 使用 Shared.TextMuted；Edge / Surface 使用 Shared.Panel；Deep 使用 Shared.Background。

## 新 UI 接入

WBP 负责将查询结果应用到控件或独立的动态材质实例。自身 HUD 使用所属玩家阵营；名称、击杀信息、计分板行使用对应显示对象阵营；双方比分分别读取两个阵营。不要将本地玩家恒定映射为青蓝。

在创建、重新打开、显示对象变化和阵营变化时重新应用。复用 `ObserveTeam` 监听可变阵营，销毁时清理监听。不要在每帧查询，也不要把查询结果另存为可编辑默认色值。运行中保存调色板不会主动通知已经打开的 UI；重新创建界面或下次启动时读取新值。

材质基础 RGB 通过参数输入；透明度和发光强度独立维护。颜色动画不得用固定 RGB 覆盖调色板输入。避免父级 Tint 与子控件重复着色。若材质同时供世界表现使用，应分离 UI 路径再调整。

`Border` / `CommonBorder` 的材质位于背景画刷中，需要调用边框自身的 `GetDynamicMaterial`；不能只检查 `Image.Brush`。替换画刷或材质后重新应用调色板，避免丢弃已经设置颜色的动态实例。共用遮罩背景使用共用中性色，阵营面板的底色与边框分别使用 Surface、Edge。

富文本的内联 `color` 会覆盖控件默认文字色。需要分段着色时，在 WBP 中把查询结果通过 `ToHex (LinearColor)` 转成 sRGB RGBA 字符串，作为 `Format Text` 的参数；不得在文本模板中写死 HEX。正文和单位分别读取 Shared.Text、Shared.TextMuted。

击杀确认条目按事件快照设置显示阵营：击杀/友杀/自杀使用被显示的 VictimTeamId，被击杀提示使用 KillerTeamId。每次复用条目时先更新阵营，再应用材质颜色。准星和命中反馈使用所属玩家阵营，保留原有反馈类型与播放逻辑。

等待玩家提示由 `Phase_Warmup` 应用 `GE_PregameLobby`，通过 `GameplayCue.UserMessage.WaitingForPlayers` 找到 `GCNL_WaitingForPlayers`，再由 `GCNL_Widget_Base` 创建 `W_WaitingForPlayers_Message`。这类按标签加载的入口也需要纳入 UI 取色检查。提示复用所属玩家的队伍监听，在后续分配阵营或切队时刷新文字和颜色。

现有 `UArenaTeamSubsystem` 的颜色接口继续服务非 UI 表现，未与本调色板合并，不应再用于 UI 取色。

## 验证入口

`Arena.UI.Colors` 验证配置加载、20 个色值与规则表的一致性、线性转换、阵营映射、中性回退、反射查询与缺失配置处理；`Consumers` 检查计分板背景、富文本、等待提示入口、切队和准星材质。`Arena.UI.EliminationFeed.PersonalConfirmation` 同时检查真实列表条目的材质取色和复用。视觉、材质叠加、动画和双客户端阵营一致性还需要在实际界面验证。
