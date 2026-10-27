# 敌人伤害反馈

服务器在 `UArenaHealthSet` 确认敌人实际掉血后，通过攻击者控制器的 `ClientNotifyEnemyHit` 发送 `EArenaHitFeedbackType`。三种业务结果为：

- `Hit`：造成伤害但目标仍存活。
- `Kill`：目标由存活变为死亡，且不属于手雷直击秒杀。
- `InstantKill`：携带 `DamageType.Grenade.DirectHit` 的伤害使目标死亡，沿用项目既有的手雷直击判定。

死亡后继续施加伤害不会再次发送击杀；免疫、零伤害和友方伤害不产生敌人反馈。通知只发送给攻击者所属客户端。

`AArenaHUD` 将结果转交给 `UArenaReticleWidget`；该原生类只分发 `OnHitFeedback`、`OnKillFeedback`、`OnInstantKillFeedback` 三个蓝图事件。视觉和播放逻辑保存在 `/Game/UI/W_Reticle` 及其引用的 `/Game/UI/Hud/HitFeedback` 材质资源中，不由 C++ 绘制。

控件蓝图每次接收有效事件时重置一次动画进度，结束后停止更新。正在播放的较高等级反馈不会被较低等级反馈覆盖；新的同级击杀可重新播放一次。旧的击杀图像已隐藏，避免叠加第四种反馈。

自动化测试 `Arena.UI.HitFeedback.DamageOutcomes` 经实际 GameplayEffect 伤害路径检查非致死、普通击杀、直击秒杀、死亡后重复伤害、免疫和友方过滤。
