# 回合重开与玩家能力、网络映射生命周期

回合结束的 `Phase_PostGame` 调用 `UArenaSystemStatics::PlayNextGame`，使用同图无缝切换重开。

## 实际掉线日志

2026-10-26 的打包版客户端日志记录了以下过程（北京时间）：

- 23:16:35：第一次切换后，多项静态类/资产出现 `Attempt to reassign read-only guid`，其中 GUID 843 对应 `GA_Hero_Dash_C`。
- 23:22:18.028：下一次同图无缝切换完成。
- 23:22:18.187 起：多个先前使用过的类和资产映射变成 `NOT_IN_CACHE`，出现 `Unresolved Archetype GUID`。
- 23:22:18.368：GUID 843 已不在客户端缓存中，PlayerState 子对象类无法解析，触发 `ContentBlockHeaderNoSubObjectClass` 并关闭连接。
- 23:22:18.369：报告 `ConnectionLost`，随后退回启动地图。

这次断连的直接原因是客户端无法解析复制子对象的类，并非日志中的连接超时。断开后退回地图清理装备时才出现的 `ensure(ASC)` 在时间上晚于断连，不能据此认定为断连起点。压缩包中的骨骼映射和异步动作数量 ensure 属于另外的诊断项。

## 静态 NetGUID 修复

UE 5.8.3 的 `FNetGUIDCache::GetObjectFromNetGUID` 可以重新找到已加载的静态对象，但默认不清除旧缓存条目的 `ReadOnlyTimestamp`。`CleanReferences` 在 90 秒过期后仍会删除这些已重新使用的条目；服务器可能已确认过它们的导出，不会在每次引用时重发路径。

`Config/DefaultEngine.ini` 启用引擎现有的 `net.AllowClientRemapCacheObject=1`。客户端重新解析静态路径或重新使用已有对象时，可以恢复同一 NetGUID 并清除过期标记。该开关不让客户端成为 GUID 分配权威，不跳过网络校验，也不关闭超时或缓存清理。

## 玩家能力生命周期保护

另一个独立风险是：玩家的 PlayerState / PlayerController 会在迁移到过渡地图时临时注销、重新注册组件，而 `UAbilitySystemComponent::OnUnregister` 会调用 `DestroyActiveState`。提前销毁能力实例可能使在途客户端能力 RPC 引用失效子对象（[UE-193041](https://issues.unrealengine.com/issue/UE-193041)）。实际日志中的“客户端缺少静态类映射”与该问题不是同一个错误分支，不能仅靠这项保护认定已修复日志中的断连。

`UArenaAbilitySystemComponent` 只在已进入 BeginPlay、未在销毁的玩家对象进行无缝切换时，跳过本次 OnUnregister 调用内的 DestroyActiveState。普通注销、显式销毁、真正结束游戏及 GameState 等非玩家对象仍执行原来的清理。保留父类组件注销和注册流程。

## 回归验证

编译 ArenaEditor，在没有活动 PIE 会话的独立编辑器中运行 `Arena.Network.RoundRestart`：

```text
-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL
-ExecCmds="Automation RunTests Arena.Network.RoundRestart"
-TestExit="Automation Test Queue Empty"
```

- `PreservesConnections`：使用正式 PlayNextGame 入口，一台主机和两名远端玩家连续重开三次，并延迟主机加载。检查原始连接、监听驱动和能力对象保留，以及正常清理仍然有效。临时使用 IP 驱动并恢复设置。
- `ReloadedClassNetGUID`：使用实际日志中的 `GA_Hero_Dash_C` 和引擎真实客户端缓存，注入卸载/重新加载及超过 90 秒的只读条目边界。关闭开关的对照会在清理后丢失映射；项目配置开启时，路径解析和已有对象复用两条路径都保留同一 GUID，清理后仍可解析类。测试不等待实际 90 秒，不模拟缓存解析或清理的成功结果。

自动化验证本地网络和映射生命周期，不代替打包版 Steam 多账号实测。正式回归应至少覆盖两次重开、跨过缓存过期时间，并在结算时保留活动技能和网络延迟。原始日志和压缩包只保存在本地诊断目录，不加入仓库。
