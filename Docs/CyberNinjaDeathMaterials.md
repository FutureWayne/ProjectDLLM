# CyberNinja 的 Lyra 死亡消散

`CBP_RED` 和 `CBP_BLUE` 的 `NewMesh` 覆盖使用 `SK_CyberNinja`，实际材质为
`Content/Characters/CyberNinja/Materials/` 下的 `M_Legs`、`M_Body`、`M_Head`。
检查继承组件时必须读取子蓝图的覆盖对象，不能只读取父蓝图的 Manny 模板。

## 材质连接

三个材质保留原有贴图、节点、Base Color、Metallic、Specular、Roughness、Normal、AO
和自发光计算，新增同样的连接：

- `MF_MannequinEdge.MaskedOpacity` → Opacity Mask，Blend Mode 为 Masked。
- `MF_MannequinEdge.customUVData` → Customized UV1，Num Customized UVs 为 2。
- `KillEdgeColor` 向量参数 → 函数的同名输入。
- 原自发光结果与 `MF_MannequinEdge.KillEdge` 相加 → Emissive Color。

使用函数自带的 `KillAge = 0`、`SpawnAge = 1` 默认值。`GCNL_Death` 在两秒内通过
现有 `KillTimeline` 曲线同时更新模型的 `KillAge` 与 Niagara 的 `KillTimeline`，
材质不另建计时器。

从 Opaque 迁移时还必须清除 `bCanMaskedBeAssumedOpaque`。只修改 `BlendMode`
和图连接，旧资源仍可能因该缓存而以 Opaque 渲染，出现发光但不消失的情况。
`UArenaMaterialEditorLibrary::ConnectOpacityMask` 完成遮罩连接并清除该缓存；
`ConnectCustomizedUV` 用于接入 Python 枚举未暴露的 UV 属性。调用后重新编译并保存材质。
函数输出索引应按输出名查询。

视觉增强版本在上述连接上增加队伍配色转换与死亡轮廓充能，死亡 Cue 当前使用
`NS_DeathCubes_Digital`，保留原 Lyra 受力、碰撞与两秒时间线，增强粒子材质发光，并给方块增加适度向外释放速度及更长留存（1.0–2.4 秒）。当前资源与验证说明见 [TronDeathEffect.md](TronDeathEffect.md)。

## 验证记录

2026-10-21 在 UE 5.8.3 中完成：

- `ArenaEditor Win64 Development` 编译通过。
- 三个材质原有节点与表面输出逐项一致，保留原始自发光作为新增 Add 节点的输入。
- 相同镜头和灯光下，正常状态与修改前的 RGB 图像逐像素一致。
- 离屏 PIE 中执行生产 `GCNL_Death`，403 次采样的三个材质与 Niagara 曲线误差为 0。
- 效果的两秒时间线结束后，2.20 秒及 4.00 秒截图均无可见模型残留。

本地备份、导出图结构、执行日志和截图位于 `Saved/DeathAudit/`。
验证使用隔离 PIE 中的 CyberNinja 显示网格与生产 Cue，不覆盖联网战斗及死亡技能调度。

上述三个生产材质已从 `Content/Characters/` 的忽略规则中单独排除并纳入版本管理。
材质和特效生成辅助库仅保留在本地，由 Git 忽略。
