# 击杀信息流

服务端到客户端的完整分发链路，以及与中间连杀播报的关系，见 `Docs/EliminationNotifications.md`。

`B_EliminationFeedRelay` 继承 `UArenaEliminationFeedRelay`，继续通过原有组件配置加载。
服务端监听 `Arena.Elimination.Feed`，调用 `AArenaGameState` 的可靠 multicast。
GameState 只在客户端重新发布消息，主机使用原始本地消息，避免重复显示。
重复启动监听会先注销旧监听，组件结束时注销监听。

`UElimStreakProcessor` 继续处理 `Arena.Elimination.Message`，维护勋章使用的权威连杀历史。
完成计数和被击杀者的死亡重置后，发布同类型的 `Arena.Elimination.Feed` 消息，
将当次类型对应的勋章计数写入 `Magnitude`：

| 类型 | 计数 | 上限（界面） |
| --- | --- | --- |
| 敌人击杀 | 当前敌人连杀，攻击者死亡后清零 | 6（显示 6+） |
| 队友击杀 | 本局累计友杀，与敌人连杀互不影响 | 30（显示 30+） |
| 自杀 | 本局累计自杀 | 10（显示 10+） |

缺少攻击者（环境死亡）时写入 0。上限沿用各勋章历史的封顶语义。
处理器只在服务端监听，原始消息仍供伤害、计分等既有系统使用。

`W_EliminationFeed` 继承 `UArenaEliminationFeedWidget`，监听上述已补充计数的消息。
客户端因此不再依赖旧蓝图转发器的空 Remote 分支，也不再使用蓝图结构
`EliminationFeedMessage` 与对象 `ElimFeedEntry` 作为运行时数据。旧资产保留以兼容历史引用。

C++ 将攻击者和被击杀者解析为 PlayerState，复制当时的姓名和队伍 ID 到
`UArenaEliminationFeedItem`，同时保存上述计数 `EliminationStreak`（字段名保持不变以兼容蓝图）。队伍直接从 PlayerState 查询，不依赖死亡后可能已失效的 Pawn。
缺少有效被击杀者时忽略消息；缺少攻击者时使用可本地化的 Environment 文案。
自杀身份或 SelfDestruct 标签优先，其次识别 FromAlly、FromEnemy 标签；未知上下文沿用自毁图标。
姓名和队伍的历史快照不会因改名、重生或离线而改变。

手雷击杀归因：

- `AArenaGrenadeBase` 造成伤害时把投掷手雷的 `UArenaGrenadeDefinitionData` 写入 EffectContext 的 SourceObject。
  子弹药（`SecondaryGrenadesToSpawn`）和手雷生成的效果 Actor（含 `AArenaGroundAreaEffect`）
  继承最初投掷的定义，因此集束、燃烧区域等击杀记在玩家选择的手雷上，而不是内部子弹药定义。
- `UArenaHealthComponent` 构建 `Arena.Elimination.Message` 时解析该来源：SourceObject 为手雷定义则直接使用，
  为 `AArenaEffectActor` 则读取其 `GetSourceGrenadeDefinition()`，结果写入 `FArenaVerbMessage::ContextObject`。
  连杀处理器原样复制该字段，经 GameState 可靠 multicast 到达客户端；定义是资产，可按路径在客户端解析，
  不依赖已销毁的手雷 Actor。
- `UArenaEliminationFeedWidget` 只接受 `UArenaGrenadeDefinitionData` 类型的 `ContextObject`，写入
  `UArenaEliminationFeedItem::GrenadeDefinition`（BlueprintReadOnly）；非手雷伤害为空。
  `PresentElimination` 签名不变，条目 WBP 可通过列表项对象读取该字段决定图标或名称。
- 未覆盖：效果 Actor 被销毁后才结算的持续伤害（SourceObject 为弱引用）以及蓝图重写的
  `SpawnSecondaryGrenade` / `SpawnEffectActor` 若未调用 C++ 传递，会退回为空或子弹药自身定义。

信息流按接收顺序追加，默认保留最多 8 条，每条存活 6 个游戏秒。
`MaxEntries` 和 `EntryLifetime` 可在 WBP 类默认值中配置；修改存活时间时需同步调整
条目 WBP 的 `IntroWithTimedOutro` 动画。运行时将条数限制为至少 1，非法存活时间回退为 6 秒。
由信息流根据条目各自的绝对到期时间调度一个计时器，不由可复用的行控件删除列表数据。
移除最早条目不会提前删除后来条目；控件销毁或重建会清空列表、计时器和旧监听。
专用服务器不注册 UI 监听。

`W_EliminationFeedEntryWidget` 继承 `UArenaEliminationFeedEntry`。
原生 `IUserObjectListEntry` 回调接收条目数据，WBP 实现以下纯表现事件：

- `PresentElimination`：接收姓名、队伍 ID、死亡分类和权威计数，选择表现并播放动画。
  行布局为 `| [计数图标 ×N |] 攻击者 [类型图标] 被击杀者`，整行按内容宽度右对齐。
  姓名不设像素宽度上限，行宽随名字自适应；原生层在调用 `PresentElimination` 前按
  `MaxNameLength`（默认 12 个字符，可在条目 WBP 类默认值中修改）裁剪，超出部分以“…”结尾，
  按 Unicode 码点计数，不拆分代理对。条目快照仍保存完整姓名。
  `WBP_CharacterOverlay` 中信息流宽 800 px，可容纳上限内最宽字形（双方 12 个 W/M 加 6+ 计数，约 755 px）；
  调大 `MaxNameLength` 时需同步加宽该容器。左侧竖条按分类着色，姓名按队伍着色；
  自杀行只显示被淘汰者姓名、自杀图标和“自毁”，环境死亡同样适用。
  计数区（勋章图标、×N、分隔线）在计数至少为 2 时显示，否则整体折叠；
  图标随计数档位切换对应的勋章贴图，客户端不单独累计计数。
  计数图标、×N 与自杀图标使用触发者的队伍色（自杀的触发者即被淘汰者），
  通过 `M_UI_EliminationFeed_IconTint` 的动态材质参数 `Icon` / `Tint` 设置。
  测试依赖的控件名 `AttackerName`、`WeaponIconSwitcher`、`EnemyKill`、`StreakBadge`、`StreakText` 保持不变。
- `ResetEliminationPresentation`：复用或释放行控件时停止旧动画。
- 信息流的 `OnFeedEntriesChanged`：WBP 根据是否有条目切换容器可见性。

布局、颜色、材质、图标资源、动画和插值全部保留在 WBP 中。
伤害和击杀判定继续使用既有 GAS 与 HealthComponent 路径。

自动化测试：

- `Arena.UI.EliminationFeed.LifecycleAndDamage`：真实 GAS 非致死／致死／死后伤害，
  正式 WBP 加载、分类、姓名与队伍快照、条数上限、无可见行时到期、监听与计时器清理、非法配置，
  以及共享连杀历史、6+ 封顶、友杀与自杀各自计数、环境死亡无计数与死亡重置，
  姓名字符上限裁剪（中日韩字符、代理对、尾随空格、非法上限），
  经真实 GAS 死亡路径的手雷归因（SourceObject 为手雷定义或手雷生成的效果 Actor），以及非手雷上下文对象被忽略。
- `Arena.UI.EliminationFeed.Network.ReliableDelivery`：真实 PIE 主机和两个远程客户端，
  加载正式连杀处理器、转发器与 WBP，经 GameState RPC 检查各端每条只接收一次、
  连杀数与徽记一致、正式手雷定义 `DA_NormalGrenadeDef` 到达各端条目、列表行文字和图标、各端到期。
  使用隔离编辑器与临时测试地图，测试后恢复网络驱动和 GameInstance 配置。

网络测试使用回环 IP 驱动，不覆盖 Steam、多账号、多进程或丢包环境。
无渲染测试中的文字和图标绑定断言不能代替视觉效果验收。
