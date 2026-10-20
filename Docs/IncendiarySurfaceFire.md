# 燃烧弹地面区域

燃烧弹已改用通用地面区域实现，允许空爆。最新架构、配置字段、冰冻/减速接入和验证方式请见 [可配置地面区域](GroundAreaEffects.md)。

正式配置入口：

- `DA_IncendiaryGrenadeDef → EffectActorsToSpawn`：生成时机、区域数量、散布半径、区域配置引用。
- `DA_IncendiaryGroundArea → Settings`：区域大小、Niagara、GE、持续时间、作用对象。

旧的 `IncendiarySettings` 迁移字段已移除，配置统一使用独立区域定义；接触平面等待、窄台拒爆和轨迹回溯已移除。
