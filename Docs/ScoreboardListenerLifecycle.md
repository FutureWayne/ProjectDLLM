# 排行榜监听生命周期

日志中的 `GlobalAsyncActionCount <= MaxActionCount` 超限调用栈来自 `W_SB_PlayerList` 的定时排序：`SortPlayerList -> W_SB_PlayerRow_TDM.SetPlayerState -> W_BasePlayerRow.SetPlayerState -> W_SB_PlayerStat.SetPlayerState -> ListenForGameplayMessages`。每次刷新都会创建新的异步监听并注册到 GameInstance，原节点没有主动取消入口。控件失效后的被动清理还依赖下一次消息广播，不能保证及时释放。

## 修复

`W_SB_PlayerStat` 继承 `UArenaScoreboardStatWidget`。原有玩家赋值入口只调用 `EnsureScoreListener`，继续刷新已有的分数文本；分数消息通过 `OnScoreChanged` 调用原蓝图 `UpdateState`。外观、统计标签和排序保持现有资产配置。

- 每个已显示的统计格最多持有一个监听，重复赋值复用该对象。
- 显示前只记录监听需求，不创建 GameInstance 引用。
- NativeDestruct 主动解绑委托并取消监听；池化控件再次构造时恢复一个监听。
- 注册源世界清理委托：即使控件仍被其他对象保留，无缝切换清理旧世界时也取消旧监听。
- BeginDestroy 提供最终清理；正在销毁的世界不会重新注册。

`UArenaScoreboardEditorLibrary` 提供受支持的图节点迁移，替换原异步节点并接回既有刷新逻辑。资产修改只涉及统计格蓝图，不修改消息插件或其他 HUD。

## 回归

独立编辑器运行 `Arena.UI.Scoreboard.TwelveRoundSoak`，可以与 `Arena.Network.RoundRestart` 一起执行：

```text
-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL
-ExecCmds="Automation RunTests Arena.UI.Scoreboard.TwelveRoundSoak+Arena.Network.RoundRestart"
-TestExit="Automation Test Queue Empty"
```

回归使用生产 PlayNextGame 入口创建主机和两名远端客户端，连续无缝重开 12 次。每名客户端在初始地图及每次重开后加载实际 `W_SB_PlayerRow_TDM`，执行 500 次真实 SetPlayerState，再关闭、重新构造并释放。总计 13,000 次玩家行赋值；检查监听保持激活且复用同一对象、关闭后解除 GameInstance 注册和委托、控件可回收且异步监听数量恢复基线。另在每次切换时保留实际统计格 UObject 和 Slate，验证自动取消来自真实世界清理，而非测试主动销毁控件。

2026-10-27 验证结果：ArenaEditor、Arena Win64 Development 编译通过；12 轮排行榜回归及两个原网络回归均成功。每次控件清理后的监听对象数均为 `0 -> 0`；两名远端连接跨 12 轮保持有效。

测试夹具使用真实 Arena 游戏模式的无 Pawn 子类，避免无关默认 Pawn 在同图切换中产生复制噪声；测试玩家通过服务端队伍接口和真实复制获得有效队伍。旧 NetGUID 缓存测试改用客户端角色缓存子类，避免未初始化模拟连接在后续 GC 时崩溃。测试结束恢复临时驱动、GameInstance 和控制台设置。

这是 NULL/IP、本地单进程 PIE、NullRHI 的加速生命周期回归，不包含 Steam 多账号、实际渲染帧率或长时间打包版内存性能基准。重新打包后的多账号体验仍需复测；日志里的高频 Foley 配置警告不属于本次修复范围。