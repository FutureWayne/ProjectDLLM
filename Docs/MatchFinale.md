# 比赛结算生命周期

`Phase_PostGame` 继承 `UArenaGamePhaseAbility_MatchFinale`。蓝图负责阶段顺序、等待与时间膨胀；原生基类发布冻结比分和阶段 GameplayCue，`AArenaGameState` 将 Cue 转为本地阶段，`UArenaMatchFinaleWidget` 消费阶段和结算数据。布局、动画和视觉参数仍由 WBP 维护。WBP 通过共享调色板查询本地玩家及比分双方的颜色，未分配阵营时使用统一的共用中性色回退；阵营观察与材质参数更新由 WBP 管理，接口约定见 `UIColors.md`。

## 阶段展示时间

`Phase_PostGame` 的 `FinaleTiming` 配置为：结果阶段 4.5 秒、最终比分 5.5 秒、个人数据 25 秒、下一局倒计时 8 秒。比赛结束立即进入慢动作，前 1 个真实秒内不显示结算 UI；1 秒后开始 WBP 入场，结果标题在阶段开始 2 秒后揭示，由 WBP 的 `ResultRevealDelaySeconds` 配置。新增的 1 秒停留计入结果阶段，保留原有入场、标题展示和后续阶段的时长。个人数据阶段允许玩家阅读完整统计，下一局倒计时仍提供明确的离场准备时间。客户端中途加入时从服务器阶段时间戳扣除已流逝时间，不会重新获得完整时长。

## 结算画面流程

结算 WBP 的 `Stage_Result` 时间轴以 1 秒不可见停留开头，`FinaleRoot` 在这段时间保持透明，所有原入场轨道顺延 1 秒。之后先用正六边形蜂巢遮罩向中心收拢，随后展示折叠导轨、双层箭头扫过、竖向速度线和结果标题。结果标题的揭示由独立的 `ResultReveal` 动画延迟启动；胜利、失败或平局文案使用本地玩家的结算数据与阵营状态确定。揭示及后续阶段均显式恢复根控件不透明度，确保中途接管后续阶段时不会继承隐藏状态。

进入 `FinalScore` 后标题保持在中心，蓝红双方的比分块从画面两侧进入，并显示最终比分提示。之后依次展示个人统计和下一局倒计时。阶段顺序与时长由 `Phase_PostGame` 驱动；蜂巢、导轨、箭头、文字、比分位置及过渡都保存在 `W_MatchDecided_Message` 与其 UI 材质中，C++ 只提供阶段、比分、结果和动画时钟数据。

## 界面时钟与阶段接管

`UArenaMatchFinaleWidget::GetStageElapsedSeconds` 只提供 GameState 当前阶段已流逝的真实秒数。WBP 负责把阶段时间映射到自己的动画轨道；C++ 不控制布局、动画位置或播放参数。

`OnFinaleStageChanged` 的 `bSnap` 表示控件接管已经开始的阶段。正常 GameplayCue 流程也可能先开始阶段、后构建控件，因此不能把它解释为无条件跳到动画最后一帧。阶段动画从已流逝时间继续播放，超出动画范围时定位到结束状态。结果揭示动画使用阶段时间减去同一份 `ResultRevealDelaySeconds`；已经进入后续阶段时不会重播揭示。阶段切换会先停止全部旧轨道，再启动新阶段轨道；揭示开始时停止 `Result` 入场轨道，避免多个动画同时写入同一控件。

WBP 在普通界面 Tick 中校准活动阶段动画的播放位置，避免 Slate 帧间隔被限制或渲染卡顿后落后于真实阶段时钟。结果揭示轨道按 `阶段已流逝时间 - ResultRevealDelaySeconds` 对齐；其他阶段轨道按阶段已流逝时间对齐。只有播放位置偏差超过 0.02 秒时才定位并刷新动画，确保校准后的控件姿态在当前帧更新，同时避免每帧强制刷新。

## 中途加入的计时

阶段 Cue 的 `RawMagnitude` 是阶段的真实秒数。已在结算中的客户端从 Cue 到达时使用本地真实时间计时，不随 GameState 平滑服务器时钟的后续修正而重置。

首次直接进入 `FinalScore`、`PlayerStats` 或 `NextMatch` 的客户端，需要用服务器阶段开始时间扣除已经过去的秒数。`FArenaMatchFinaleData.TimingStage` 标识时间戳所属阶段，防止 Cue 与数据分开到达时使用上一阶段的时间戳。GameState 必须收到首个服务器时间样本及匹配阶段的数据后才执行一次校准；Cue、数据和时间 RepNotify 的到达顺序不构成前置条件。下一阶段或结束结算会撤销未完成的校准。

## 结束和取消

`FinalScore` 展示结束、进入 `PlayerStats` 后，`AArenaGameState::IsMatchFinaleGameplayInputBlocked` 在本机屏蔽移动、视角和游戏技能输入，并持续到 `NextMatch` 结束。输入限制直接读取阶段状态，不依赖结算控件是否已经构建；取消结算后自动恢复。ASC 丢弃已缓存的游戏按下/持续输入，并为此前按住的输入发送释放事件，避免遮挡后继续开火或恢复时重放。`InputTag.ShowScoreboard` 保留原来的按下/松开链路。

整个活动结算阶段均允许 ESC 打开退出确认，包括比分板已打开时。结算控件监听 CommonUI 活动输入模式变化，使用与当前 `Game` 或 `Menu` 模式匹配的单个持久 ESC 绑定；活动模式为 `All` 时使用 `Menu` 绑定。CommonUI 绑定自身的 `All` 不代表匹配所有活动模式，且同一控件不能重复注册同一动作。确认框打开时移除结算绑定，将 ESC 交给对话框返回操作；取消确认后恢复绑定，结算结束或控件销毁时一并注销绑定和模式监听。

结算控件通过 `bShowHUD = false` 隐藏原生 HUD，并单独折叠 `UI.Layer.Game`。结算数据或阶段仍有效时，`UArenaUIManagerSubsystem` 必须保留根布局，供 `UI.Layer.GameMenu` 中的比分板和 `UI.Layer.Modal` 中的退出确认继续显示；不能仅根据 `bShowHUD` 折叠整个根布局。结算结束后恢复通常的 HUD 可见性同步规则。

结束阶段技能时清空结算数据，并移除评分组件在启动阶段前添加的 `GameplayCue.UserMessage.MatchDecided`。该 Cue 不属于技能自动清理的阶段 Cue 列表，需要单独移除。

控件收到 `None` 阶段立即从视口移除、解绑 GameState 和 ESC、恢复 HUD 及原先隐藏的控件。清理与 `NativeDestruct` 共用幂等路径；即使 Slate 或其他对象继续保留控件，后续 Tick 和延迟确认回调也不得重新隐藏 HUD 或注册输入。

## 手动触发结算

开发构建和编辑器 PIE 中，在当前 TDM 对局的主机或服务端控制台输入：

```text
Arena.MatchFinale
Arena.MatchFinale 0
Arena.MatchFinale 1
Arena.MatchFinale 2
```

无参数默认蓝队获胜；`0` 为平局，`1` 为红队获胜，`2` 为蓝队获胜。命令调用现有评分组件的 `HandleVictory(WiningTeam)`，使用当前真实比分和个人统计，并按正式阶段等待、倒计时和切换下一局。命令不创建临时评分组件；在非对局世界、客户端、没有 TDM 评分组件、参数无效或结算已经开始时只输出提示。Shipping 不注册该命令。

## 回归入口

独立编辑器、无活动 PIE 时运行 `Arena.GameFlow.MatchFinale` 和 `Arena.UI.MatchFinale`。

- `Stages`：阶段 Cue、冻结分数、真实时间计时、重复及过期事件、技能结束与中途阶段定位。
- `Input`：通过正式玩家控制器和 ASC 输入入口，检查比分后屏蔽移动、视角、技能，释放已按住的输入，保留比分板按下/松开，并在取消后恢复且不重放被屏蔽的按键。
- `ConsoleCommand`：通过注册的控制台命令验证胜负参数、正式结算入口、无评分组件及非法参数处理、重复输入与取消。
- `LateJoin`：阶段开始两秒后，通过编辑器正式 Late Join 入口建立新客户端连接，检查初次复制后的倒计时及后续时间样本。
- `InitialSync`：故障注入覆盖 Cue、时间戳及首个时钟样本的不同到达顺序，拒绝其他阶段的时间戳，并检查重复样本和取消后的迟到样本。
- `Network`：阶段网络同步和慢动作交接；通过主机的 `Arena.MatchFinale` 命令调用正式评分流程，确认客户端不能自行触发，创建真实结算界面后取消阶段，检查主机与远端的界面、HUD、覆盖层、Cue 和原始 ESC 绑定均已清理。
- `Snapshot`：WBP 数据绑定、个人统计、阶段显示，以及控件被保留时的结束清理和迟到确认回调。
- `StageEntry`：通过正式阶段 Cue 后构建结算 WBP，检查入场恢复位置、慢动作与渲染间断后的时间校准、揭示中途接管，以及比分阶段不会被旧揭示动画覆盖。
- `Layers`：使用具体的 CommonGame 测试实例承载正式 UI 策略、根布局、比分板和退出确认，显式构建 Slate，并跨真实帧等待 CommonUI 注册输入。在四个结算阶段逐一检查隐藏 HUD 仍保留菜单/模态层、比分板打开时 ESC 可显示确认框、点击正式取消按钮后可再次打开确认框，最后检查取消结算后恢复普通 HUD 显隐规则。

本地 NULL/IP、单进程 PIE 和 NullRHI 回归不能替代打包版 Steam 多账号及视觉验证。
