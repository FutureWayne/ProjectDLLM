# 敌人伤害反馈

服务器在 `UArenaHealthSet` 确认敌人实际掉血后，通过攻击者控制器的 `ClientNotifyEnemyHit` 发送 `EArenaHitFeedbackType`。三种业务结果为：

- `Hit`：造成伤害但目标仍存活。
- `Kill`：目标由存活变为死亡，且不属于手雷直击秒杀。
- `InstantKill`：本次伤害的动态资产标签携带 `DamageType.Grenade.DirectHit` 并使目标死亡；该标签由手雷的实际直击伤害入口写入。

击杀类型只使用本次伤害携带的标记。目标自身的标签、捕获的目标标签和 GE 的静态资产标签均不作为直击依据，避免后续燃烧或范围伤害继承直击分类。地面燃烧的致死伤害应报告 `Kill`。

死亡后继续施加伤害不会再次发送击杀；免疫、零伤害和友方伤害不产生敌人反馈。通知只发送给攻击者所属客户端。

`AArenaHUD` 将结果转交给 `UArenaReticleWidget`；该原生类只分发 `OnHitFeedback`、`OnKillFeedback`、`OnInstantKillFeedback` 三个蓝图事件。视觉和播放逻辑保存在 `/Game/UI/W_Reticle` 及其引用的 `/Game/UI/Hud/HitFeedback` 材质资源中，不由 C++ 绘制。

控件蓝图每次接收有效事件时重置一次动画进度，结束后停止更新。正在播放的较高等级反馈不会被较低等级反馈覆盖；新的同级击杀可重新播放一次。准星不再监听通用击杀消息播放旧的 `Elimination` 动画；旧事件保留为空入口，旧图像保持折叠，动画也不再包含重新显示该图像的可见性轨道。击杀反馈统一由伤害结果事件触发，避免叠加旧反馈。

自动化测试 `Arena.UI.HitFeedback.DamageOutcomes` 经实际 GameplayEffect 伤害路径检查非致死、普通击杀、直击秒杀、死亡后重复伤害、免疫、友方过滤及目标标签隔离。`Arena.UI.HitFeedback.ProductionIncendiary` 使用正式燃烧区域配置验证持续伤害击杀分类，检查正式 WBP 选择普通击杀材质，并验证旧击杀入口不会启动动画或重新显示旧图像。
