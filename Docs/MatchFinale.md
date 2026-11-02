# 比赛结算生命周期

`Phase_PostGame` 继承 `UArenaGamePhaseAbility_MatchFinale`。蓝图负责阶段顺序、等待与时间膨胀；原生基类发布冻结比分和阶段 GameplayCue，`AArenaGameState` 将 Cue 转为本地阶段，`UArenaMatchFinaleWidget` 消费阶段和结算数据。布局、动画和视觉参数仍由 WBP 维护。

## 中途加入的计时

阶段 Cue 的 `RawMagnitude` 是阶段的真实秒数。已在结算中的客户端从 Cue 到达时使用本地真实时间计时，不随 GameState 平滑服务器时钟的后续修正而重置。

首次直接进入 `FinalScore`、`PlayerStats` 或 `NextMatch` 的客户端，需要用服务器阶段开始时间扣除已经过去的秒数。`FArenaMatchFinaleData.TimingStage` 标识时间戳所属阶段，防止 Cue 与数据分开到达时使用上一阶段的时间戳。GameState 必须收到首个服务器时间样本及匹配阶段的数据后才执行一次校准；Cue、数据和时间 RepNotify 的到达顺序不构成前置条件。下一阶段或结束结算会撤销未完成的校准。

## 结束和取消

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
- `ConsoleCommand`：通过注册的控制台命令验证胜负参数、正式结算入口、无评分组件及非法参数处理、重复输入与取消。
- `LateJoin`：阶段开始两秒后，通过编辑器正式 Late Join 入口建立新客户端连接，检查初次复制后的倒计时及后续时间样本。
- `InitialSync`：故障注入覆盖 Cue、时间戳及首个时钟样本的不同到达顺序，拒绝其他阶段的时间戳，并检查重复样本和取消后的迟到样本。
- `Network`：阶段网络同步和慢动作交接；通过主机的 `Arena.MatchFinale` 命令调用正式评分流程，确认客户端不能自行触发，创建真实结算界面后取消阶段，检查主机与远端的界面、HUD、覆盖层、Cue 和原始 ESC 绑定均已清理。
- `Snapshot`：WBP 数据绑定、个人统计、阶段显示，以及控件被保留时的结束清理和迟到确认回调。

本地 NULL/IP、单进程 PIE 和 NullRHI 回归不能替代打包版 Steam 多账号及视觉验证。
