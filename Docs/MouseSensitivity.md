# 鼠标灵敏度

`Mouse & Keyboard` 中的 X/Y 灵敏度和轴反转由 `UArenaSettingsShared` 按本地玩家保存。
`AArenaPlayerController::Input_LookMouse` 在处理 `InputTag.LookMouse` 时读取当前设置，
分别缩放水平和垂直输入，再交给 Pawn 的控制器转向接口。默认倍率 1 保持原始手感。

设置修改和取消修改后的数值在下一次鼠标输入时生效，不需要重新进入对局；
设置页的应用按钮仍负责走已有共享设置存档流程。输入映射中的基础倍率继续有效，
不要在 Input Action 或 Mapping Context 中重复应用用户灵敏度。

此路径只处理鼠标视角，不改变手柄输入。`Targeting Sensitivity` 在 Secondary Grenade
成功激活且输入仍按住时额外乘到 X/Y 灵敏度上。默认右键输入，重绑后跟随
`InputTag.Grenade.Special`，不硬编码物理按键。倍率 1 保持普通灵敏度，0.5 为一半。

`UArenaAbilitySystemComponent::IsAbilityInputActive` 同时检查本地按住的输入句柄和
真实的技能激活状态。冷却或其他原因导致激活失败时不应用倍率；松开输入、技能结束、
取消或清理输入后自动恢复。控制器只读取以当前 Pawn 为 Avatar 的 ASC，避免换角色后
使用上一个角色的状态。无需新增复制字段或更改共享设置中的普通灵敏度值。

回归测试：`Arena.Input.MouseSensitivity.SettingsToLookInput`，覆盖设置页使用的反射 setter
到实际控制器转向输入、独立轴倍率、轴反转、恢复数值、存档序列化与输入屏蔽。
`Arena.Input.MouseSensitivity.SecondaryGrenadeTargeting` 覆盖真实 GAS 输入处理、激活、
释放、取消、激活阻止以及瞄准时设置倍率的即时更新。
