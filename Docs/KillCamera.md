# 死亡后原位看向击杀者

服务端 `UArenaHealthComponent::HandleOutOfHealth` 在激活死亡能力之前解析原始击杀者 Pawn，调用受害者 `AArenaPlayerController::BeginKillCamera`。
控制器通过可靠的 owning-client RPC 启动本地 `AArenaPlayerCameraManager`。服务端的击杀者 ViewTarget 仅用于保持远距离网络相关性；本地画面单独计算受害者自己的相机。
不修改现有死亡能力、布娃娃、尸体消失或角色销毁时机，也不延迟复活。

开始时记录受害者正常相机的世界位置，并加上相机蓝图配置的 `KillCameraAnchorOffset` 作为固定观察点，不移动到击杀者身边，也不随尸体或死亡相机偏移改变位置。
每帧继续评估受害者原有死亡相机，保留原本的 FOV 变化及修饰器；最终画面只平滑转向击杀者实际显示网格的中心。
初始转向使用平滑曲线，后续只插值旋转。转向不写回 ControlRotation，避免原本位于受害者后方的相机被带着绕角色移动。
尸体按原逻辑隐藏、销毁后保持固定观察点与最后的 FOV，仍可转向原始击杀者；不延长尸体存活。
自杀、环境伤害或无有效击杀者时保留原有死亡视角。击杀者销毁后保持最后朝向，不自动追踪其复活后的新 Pawn。
本地玩家取得新的存活 Pawn 时退出，世界退出也清理。UI 专用镜头保留原有优先级。

`B_ArenaPlayerController` 使用 `/Game/Camera/B_ArenaPlayerCameraManager`。
相机蓝图负责击杀者显示网格的 CustomDepth 开关与 stencil，退出时恢复原值；持续更新发生在 Pawn Tick 之后，避免正常阵营描边覆盖。
`/Game/Camera/M_KillCameraOutline` 根据独立 stencil 250（青蓝）、251（橙红）、252（中性）画轮廓，不使用 SceneDepth 遮挡判定。
描边基础色来自 `Rule.md`，颜色与线宽保存在材质资源，C++ 只传递目标与启停状态。
正式控制器须保留上述相机蓝图配置，原生相机类本身不定义描边表现。

`Arena.Camera.KillCamera` 验证真实 GAS 致死伤害、只转向且保留原位与 FOV 变化、目标销毁、复活清理，以及真实 PIE 主机/远端客户端的 owner RPC、自杀和远距离同步。
无渲染测试只能验证状态与相机计算，穿墙描边和现有尸体消失效果须在带渲染的 PIE 中检查；本地 PIE 不代替 Steam 多账号验收。
