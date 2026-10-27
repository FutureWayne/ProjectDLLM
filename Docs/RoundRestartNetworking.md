# 回合重开与玩家能力生命周期

回合结束的 `Phase_PostGame` 调用 `UArenaSystemStatics::PlayNextGame`，使用同图无缝切换重开。玩家的 PlayerState / PlayerController 会在迁移到过渡地图时临时注销、重新注册组件。

UE 5.8.3 的 `UAbilitySystemComponent::OnUnregister` 会调用 `DestroyActiveState`。在上述临时注销中销毁能力实例，会让仍在途中的客户端能力 RPC 引用已不存在的子对象，存在单个玩家被断开的风险。引擎问题记录：[UE-193041](https://issues.unrealengine.com/issue/UE-193041)。

`UArenaAbilitySystemComponent` 仅在以下条件同时成立时，跳过本次 OnUnregister 内的 DestroyActiveState：

- 所有者是 PlayerState 或 PlayerController。
- 玩家和组件均已进入 BeginPlay，玩家未在销毁。
- 当前世界正在进行无缝切换。

保护只在 OnUnregister 的调用栈内有效。普通注销、显式 DestroyActiveState、组件或玩家真正结束游戏，以及 GameState 等非玩家对象仍执行原来的清理。保留父类的组件注销和重新注册流程；不修改引擎源码、Steam 会话或网络协议。

## 回归验证

编译 ArenaEditor，在没有活动 PIE 会话的独立编辑器中运行 `Arena.Network.RoundRestart.PreservesConnections`。命令行测试可选择进程级 NULL 在线后端：

```text
-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL
-ExecCmds="Automation RunTests Arena.Network.RoundRestart"
-TestExit="Automation Test Queue Empty"
```

测试通过生产 PlayNextGame 入口创建一台主机、两名远端玩家，连续重开三次，并延迟主机加载以覆盖客户端先完成加载的时序。检查原始连接和监听驱动保持有效、能力对象在迁移中保留，以及普通注销、显式销毁、切换中销毁玩家、非玩家对象清理仍然有效。测试临时使用 IP 驱动并在结束后恢复配置，不改项目 Steam 设置。

这组测试验证能力生命周期和真实本地网络切换，不代替打包版的 Steam 多账号验证。正式回归应在主机和至少两名客户端中多次重开，特别覆盖结算时仍有技能活动或网络延迟的场景；若再次断连，保留主机和掉线客户端的同次运行日志，检查子对象 RPC 错误与断连原因。
