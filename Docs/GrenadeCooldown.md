# 手雷冷却

手雷技能从当前装备物品的 `UInventoryFragment_GrenadeDef` 取得 `UArenaGrenadeDefinitionData`。`GrenadeAbilityCooldownTime` 是技能冷却秒数，`TimeBeforeExplosion` 是独立的引信时间。右键蓄雷技能在开始蓄雷时提交冷却。

`UArenaGameplayAbility_GrenadeFire::ApplyCooldown` 向技能配置的冷却 GE 写入 `Cooldown.Grenade` SetByCaller 数值，并动态授予定义中的 `GrenadeSlotCooldownTag`。实际使用的 `/Game/Blueprints/AbilitySystem/GameplayEffects/Cooldown/GE_Grenade_Cooldown` 必须设置为 Has Duration，Duration Magnitude 使用 SetByCaller，Data Tag 为 `Cooldown.Grenade`。

该 GE 的 Grant Tags to Target Actor 组件静态授予 `Cooldown.Grenade`，表达存在手雷冷却并满足 UE 默认技能资产校验。此通用标签不用于两个槽位之间的互锁；`CheckCooldown` 只检查当前手雷的 `GrenadeSlot.Slot1`、`GrenadeSlot.Slot2` 等槽位标签。共享 GE 不应静态授予某个具体槽位标签。授予组件需要在资产重新加载后仍能通过 `GetGrantedTags()` 查询，不能仅检查已弃用的旧标签字段。

缺少冷却 GE、手雷定义或槽位标签时，`ApplyCooldown` 记录配置错误并返回，避免空指针访问或施加没有槽位身份的冷却效果。不要关闭全局 `AbilitySystem.WarnCooldownEffectWithoutTags` 来掩盖配置问题。

`Arena.Grenade.Cooldown` 回归覆盖生产技能资产校验、SetByCaller 时长键、实际定义时长、槽位独立性、自然到期清理和缺少 GE 的处理。涉及技能或 GE 资产修改时，还应通过真实技能入口验证客户端冷却及 HUD。
