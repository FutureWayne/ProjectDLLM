# Project Mayhem / Arena 编码规范

## 1. 适用范围与依据

本文是根据本仓库现有代码归纳的约定，供后续新增代码、局部修改和代码审查使用。游戏工程为根目录的 `Arena.uproject`，主要业务模块为 `Source/Arena`。项目级协作规则（资源、版本管理、文档与交付）见根目录 `AGENTS.md`。

归纳时优先参考已跟踪的非 `DEPRECATED_` 业务代码；对于本地正在修改的手雷与效果 Actor，另外对照了 Git `HEAD` 中的原始实现。近期新增的区域效果与编辑器测试代码用于识别差异，不作为原有风格的依据。

- 新增业务代码遵守本文；修改旧文件时，兼顾本文与文件所在模块的既有组织方式。
- 项目明确约定：自有 `Source` 模块的头文件统一放在 `Public`，实现文件放在 `Private`；内部辅助类型、编辑器工具和测试夹具同样遵循这一目录约定。
- 本文区分已观察到的习惯与新增代码的统一约定。历史代码并非完全一致，不能把个别旧写法或缺陷当作推荐模式。
- `Plugins` 中的 Epic / Lyra 等上游代码，优先保持对应插件的风格、版权和可对比性。
- 本次检查未发现仓库内的 `.editorconfig`、`.clang-format` 或 `.clang-tidy`。本文不假定存在自动格式化或静态检查配置，也不规定未经项目确认的固定行宽、函数长度或文件行数上限。

## 2. 目录与模块边界

| 内容 | 放置约定 |
| --- | --- |
| 游戏业务头文件（含内部辅助声明） | `Source/Arena/Public/<业务目录>/<类型名>.h` |
| 游戏业务实现 | `Source/Arena/Private/<业务目录>/<类型名>.cpp` |
| 编辑器工具头文件 | `Source/ArenaEditor/Public/<业务目录>/<类型名>.h` |
| 编辑器工具实现 | `Source/ArenaEditor/Private/<业务目录>/<类型名>.cpp` |
| 游戏自动化测试辅助头文件 | `Source/ArenaEditor/Public/Tests/<类型名>.h` |
| 游戏自动化测试实现 | `Source/ArenaEditor/Private/Tests/<测试名>.cpp` |
| 插件代码与插件测试 | 保持插件现有模块、目录和测试布局 |
| 项目配置 | `Config/Default*.ini`、模块的 `.Build.cs`、目标的 `.Target.cs` |
| 设计与维护说明 | `Docs`；根目录保留项目级说明 |

`Public` 与 `Private` 按相同的业务子目录组织，例如 `Actor`、`Weapon`、`AbilitySystem`、`Inventory`、`Equipment`、`Character`、`Teams`、`UI`、`Input`、`Settings`。头文件放在所属模块的 `Public`，对应 `.cpp` 放在 `Private` 的相同相对目录。优先扩展现有职责目录，避免建立含义重复的新目录。

自有模块不得在 `Private` 下新增或保留 `.h` 文件。仅供内部使用的辅助声明也放在 `Public`；只在单个 `.cpp` 内使用且无需独立头文件的实现细节，可直接定义在该 `.cpp` 中。头文件的目录位置遵循项目组织规范，跨模块接口及导出宏按实际调用需求确定。

一个业务类型通常使用同名的 `.h` / `.cpp` 文件，去掉 UE 类型前缀作为文件名，例如 `UArenaEquipmentInstance` 对应 `ArenaEquipmentInstance.h/.cpp`。只含声明的数据类型可以只有头文件；现有模块入口及 `ArenaTypes` 等历史布局不因此迁移。

编辑器依赖放在编辑器模块或现有条件编译边界内；`Arena` 运行时模块不因编辑器工具或测试而无条件依赖 `UnrealEd`。模块依赖按实际跨模块接口与构建需求配置 `PublicDependencyModuleNames` / `PrivateDependencyModuleNames`，不为迁移头文件或绕过 include 问题随意扩大依赖。

## 3. 命名

| 对象 | 新代码约定 | 现有示例 |
| --- | --- | --- |
| Actor 类 | `A` + `Arena` + 业务名 | `AArenaEffectActor` |
| UObject、组件、子系统类 | `U` + `Arena` + 业务名 | `UArenaHealthComponent`、`UArenaTeamSubsystem` |
| 结构体、原生委托 | `F` 前缀，业务类型通常含 `Arena` | `FArenaEquipmentActorToSpawn` |
| 枚举 | `E` 前缀，通常使用 `enum class ... : uint8` | `EArenaDeathState` |
| UE 接口 | 沿用 `U...` / `I...` 配对命名 | `IArenaTeamAgentInterface` |
| 函数 | PascalCase，名称表达动作或查询意图 | `ApplyEffectToTarget`、`GetHealth` |
| 成员、参数、局部变量 | PascalCase | `GrenadeDefinitionData`、`SpawnTransform` |
| 布尔量 | `b` + PascalCase | `bShouldBounce`、`bAllowDamageToSelf` |
| 输入、输出参数 | 有歧义时使用 `In`、`Out` 前缀 | `InASC`、`OutLifetimeProps` |

- 不引入 `m_`、`s_` 或业务变量的 `snake_case` 命名体系。不把普通成员统一改成全大写常量风格。
- 查询优先使用 `Get`、`Find`、`Is`、`Has`、`Can` 等已有命名习惯；回调沿用 `On...` / `Handle...`，复制回调使用 `OnRep_...`。
- 保持已有家族名，例如 `UInventoryFragment_...`、`UArenaGameplayAbility_...`、`K2_...`、`TAG_...`，不机械套用统一重命名。
- `ArenaGameplayTags` 等已有命名空间继续使用。业务辅助代码需要命名空间时，名称表达所属系统；不包裹整套反射类型来建立新的命名层次。
- 避免 `d`、`dz`、`v2` 等非局部数学语境中的缩写；名称应能说明单位、用途或角色。

## 4. 缩进、换行与留白

现有 `Arena` 业务代码绝大多数使用 Tab 缩进、Allman 花括号和 CRLF 换行。新代码统一遵循以下写法：

1. 每层缩进使用一个 Tab；必要的连续行对齐可以使用空格，不把整段缩进改为空格。
2. 类、结构体、枚举、函数和控制流的花括号独占一行。
3. `if`、`else`、`for`、`while` 等控制体使用花括号，即使只有 `return`、`continue` 或一次调用。
4. 一行只写一个声明或操作，不压缩多个字段、判断和副作用。
5. `UPROPERTY`、`UFUNCTION` 与其声明分行。每组反射宏及声明后通常留一个空行；相关的普通状态字段可以连续排列。
6. 函数定义之间、逻辑步骤之间保留空行。版权、`#pragma once`、include 区、类型声明之间保留分隔。
7. `if (`、`for (` 等关键字后有空格，调用写成 `Function(...)`；逗号后有空格，二元运算符两侧有空格。
8. 新写的反射参数统一采用 `Category = "..."`、`meta = (...)` 风格。旧文件中 `Category=...`、`Meta` 等差异不触发全文件重排。
9. 长签名或调用按参数边界换行，连续行清晰缩进；不为了固定行宽切碎易读的表达式。
10. 简单、无分支的 getter 可以保持单行内联；含判断、循环或多步处理的函数展开书写。
11. 新文件使用 UTF-8、CRLF，文件末尾保留换行，不引入尾随空白。已有文件保持原编码与换行方式，避免产生整文件差异。

推荐的控制流：

```cpp
if (!HasAuthority())
{
	return;
}

if (const UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>())
{
	SetGenericTeamId(IntegerToGenericTeamId(TeamSubsystem->FindTeamFromObject(GetInstigator())));
}
```

不要写成 `if (!HasAuthority()) return;`，也不要写成 `UFUNCTION() void OnRep_State();` 或 `UPROPERTY() int32 Index = 0;`。

## 5. 文件头与 include

- 新的项目自有 C++ / 构建文件使用 `// Copyright Ludens Studio. All Rights Reserved.`。上游文件保留原版权；少量旧文件中的 UE 模板版权占位语不作为新增文件模板。
- 头文件使用 `#pragma once`，不再额外添加一套 include guard。
- 反射头文件的 `<文件名>.generated.h` 必须是最后一个 include。
- `.cpp` 通常先 include 自身头文件，再空一行列出其他依赖。没有对应头文件的测试、模块入口等按所在模块现有写法组织。
- 项目头文件按所属模块的 `Public` 根目录使用相对 include 路径，例如 `"Actor/ArenaGroundAreaTrace.h"`、`"Tests/ArenaIncendiaryTestPawn.h"`；不写绝对路径，也不包含 `Source/<模块>/Public` 或 `Private` 目录段。
- 基类、值类型等需要完整定义时直接 include；只有指针或引用且允许不完整类型时使用前向声明。不要只依赖传递 include 或 Unity Build 恰好提供的声明。
- 现有代码同时存在包含与不包含 `CoreMinimal.h` 的头文件，不要求所有头文件机械添加它；确保直接依赖完整即可。
- 部分 `.cpp` 使用 `#include UE_INLINE_GENERATED_CPP_BY_NAME(...)`。保留所在文件和模块的既有方式，不将其当作所有新增文件的必填模板。
- include 的二级分组在旧文件间并不完全一致；保持清晰分组，不因局部修改全量排序。

## 6. 类结构与 UE 反射声明

- `GENERATED_BODY()` 放在反射类型体的开头，单独占一行。
- 面向模块外使用的业务类按需带 `ARENA_API`；跨模块调用的结构体和函数同样检查导出需求，不给所有内部类型机械加导出宏。
- 一般先提供构造和公共 API，再组织继承接口、受保护扩展点与私有实现。已有类会按职责重复使用 `public` / `protected` / `private` 分组，修改时保持其局部结构。
- 接口覆盖较多时，可沿用 `//~ Begin ... Interface` / `//~ End ... Interface` 标记，分组内格式一致即可。
- 覆盖函数写 `override`；沿用业务代码常见的 `virtual ... override`。父类有生命周期工作时，显式调用 `Super`，调用次序由初始化、清理依赖决定。
- 普通运行时状态默认保留在 `private` / `protected`；数据资产的配置字段可以像原有 Definition 类型一样公开，不强制为每个字段增加 getter/setter。
- `EditDefaultsOnly`、`EditAnywhere`、`VisibleAnywhere` 按设计用途选择；蓝图只需要读取时使用 `BlueprintReadOnly`。不要为了方便调试将所有内部状态暴露为可编辑、可写字段。
- Category 延续所属系统已有层级，例如 `Grenade|Projectile`、`Grenade|Behavior`、`Arena|Health`，不建立同义的重复分类。
- 蓝图扩展沿用已有 `BlueprintNativeEvent` / `BlueprintImplementableEvent`，C++ 实现使用对应 `_Implementation`；调用事件入口，保留蓝图覆盖能力。

以下为新数据资产头文件的格式示例，文件名为 `ArenaExampleDefinition.h`：

```cpp
// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "ArenaExampleDefinition.generated.h"

/** Shared configuration for an example effect. Distances are in cm. */
UCLASS(BlueprintType)
class ARENA_API UArenaExampleDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Example|Behavior", meta = (ClampMin = "0.0"))
	float Radius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Example|Behavior", meta = (ClampMin = "0.0"))
	float Lifetime = 10.0f;
};
```

## 7. 类型、const 与对象引用

- 优先使用 UE 类型与容器：`int32`、`uint8`、`FVector`、`FName`、`FString`、`FText`、`TArray`、`TMap`、`TSet` 等。
- 普通对象参数与返回值可以使用 `UObject*` / `AActor*` 等裸指针；新增的、需要由 UObject 持有并参与 GC 的强引用成员优先使用 `UPROPERTY()` + `TObjectPtr<T>`。容器内对象引用也检查 GC 可达性。
- 不拥有生命周期的缓存或观察关系使用 `TWeakObjectPtr<T>`；需要延迟加载的资产使用 `TSoftObjectPtr<T>` / `TSoftClassPtr<T>`。不要仅为了统一类型，把所有现有硬引用改成软引用。
- 用 `TSubclassOf<T>` 表达可配置的类引用。`TSharedPtr` 等用于 Slate 或普通 C++ 对象，不能代替 UObject 的 GC 所有权机制。
- UObject 创建与销毁沿用 `CreateDefaultSubobject`、`NewObject`、`SpawnActor`、`Destroy` 等机制，不直接 `new` / `delete` UObject。
- 大型只读值通常通过 `const T&` 传递；只读成员函数加 `const`，不修改的局部变量可声明为 `const`。
- 在业务代码中优先写出有意义的具体类型。只有迭代器、模板返回值等类型显然或冗长时使用 `auto`，不把所有 `Cast` 结果都改成 `auto*`。
- UObject 类型检查与转换使用 `Cast<T>`、`IsA`；只有已保证类型的路径才使用 `CastChecked<T>`。
- 新状态字段显式初始化；类内默认值和构造函数初始化均是已有方式。复杂构造链正确传递 `FObjectInitializer`。
- 指针空值使用 `nullptr`。浮点量带适合类型的后缀；旧代码有 `1.f` 和 `1.0f` 两种形式，同一文件保持一致。新写小数保留前导零，例如 `0.25f`。
- 使用 `FVector::ZeroVector`、`FTransform::Identity`、`INDEX_NONE` 等已有语义常量，避免散落含义不明的数字。

## 8. 数据驱动、蓝图与业务职责

现有设计通过 Definition、Fragment、Instance、Component、Ability、Subsystem 分担职责。新功能应先找到对应扩展点，再决定是否增加类型。

- C++ 提供可复用的行为和生命周期；DataAsset、蓝图默认值及 GameplayEffect 资产承载具体内容与调参。
- 手雷和部署配置沿用 `UArenaGrenadeDefinitionData`、`UArenaDeploymentDefinitionData`；物品通过 `UArenaInventoryItemDefinition` 及 `UInventoryFragment_...` 组合能力，装备定义与运行时实例分开。
- 不为了某一种效果在通用系统中增加一串具体资产名、具体手雷名或效果类型判断；也不为一个局部功能引入与现有结构重复的框架。
- 通用 Actor 处理世界中的行为；技能处理激活、消耗、冷却与技能事件；UI / WidgetController 负责显示与绑定，避免直接改写伤害结算。
- 伤害、治疗和属性变化沿用现有 GAS 路径：ASC、GameplayEffect、EffectContext、Spec、Handle。应用与移除逻辑成对维护，不绕过系统另写一套生命值字段。
- 队伍关系、友伤规则优先复用 `UArenaTeamSubsystem` / `IArenaTeamAgentInterface`；输入沿用 `ArenaInputConfig` 与 GameplayTag；跨系统通知优先复用 GameplayMessageSubsystem 和现有消息结构。
- 可配置资产需要约束时，沿用 `IsDataValid(FDataValidationContext&)`、父类结果合并及 `Context.AddError`；声明、实现和相关 include 置于对应 `WITH_EDITOR` 边界。
- 编辑器的 `ClampMin` 等元数据用于编辑约束；运行时入口按数据来源补充必要校验，不能假定元数据覆盖所有赋值方式。
- 不通过批量改名、删除字段来统一现有蓝图 API。涉及反射属性、类名、资产路径的变更时，同时考虑已有资产引用及迁移方式。

## 9. 网络与生命周期

这些规则延续现有手雷、物品、装备、生命组件与队伍系统的实现方式，同时约束新增代码的资源管理。

- 服务端负责伤害、生成权威 Actor、物品与队伍等权威状态修改，在入口明确检查 `HasAuthority()` 或沿用已有权威路径。
- 客户端表现使用既有 GameplayCue、复制状态、`OnRep_...` 或已有 RPC 机制。RPC 的可靠性和广播范围按行为需要选择，不机械复制 `Reliable` 到所有高频更新。
- 复制成员声明与 `GetLifetimeReplicatedProps` 中的 `DOREPLIFETIME` / 条件复制注册保持一致；覆盖时保留 `Super::GetLifetimeReplicatedProps`。
- Push Model 属性按原系统的 dirty 标记方式更新；`BlueprintAuthorityOnly` 不代替 C++ 路径的权限判断。
- 设置 Actor 的 Owner、Instigator、队伍或定义时注意 `BeginPlay` 时序。若生成后的初始化来不及满足 `BeginPlay` 前置条件，沿用 `SpawnActorDeferred` / `FinishSpawning` 模式。
- 不需要逐帧逻辑的 Actor / Component 关闭 Tick。确需 Tick、周期计时器或分帧预算时，明确启停条件，避免把所有工作无条件放进 Tick。
- 定时器、委托、消息监听、已授予能力和持续 GE Handle 应有对应的清理路径，放在合适的 `EndPlay`、`OnUnregister`、`Deinitialize` 或取消流程中。
- 可能跨帧销毁的对象在使用前检查有效性，弱引用通过 `.Get()` / `IsValid()` 访问。世界销毁时不能假定所有子系统仍存在。
- 新增纯表现逻辑注意 Dedicated Server 执行边界；新增复制数据只承载客户端所需状态，不复制无用途的内部缓存。

## 10. 错误处理、日志、注释与文本

- 可预期的缺失输入、无目标或无可选资产，使用清晰分支返回合理结果；优先提前返回，避免无必要的深层嵌套。
- `check` / `checkf` 用于必须成立的编程不变量；`ensure` / `ensureMsgf` 用于记录异常并允许处理继续。不能将必须执行的操作放进断言表达式。
- 业务日志优先使用 `ArenaLogChannels.h` 的 `LogArena`、`LogArenaAbilitySystem`、`LogArenaTeams`、`LogArenaInventory`。旧代码存在 `LogTemp`，新增正式业务日志不默认沿用它。
- 日志说明失败动作和相关对象，使用 `GetNameSafe` / `GetPathNameSafe` 等辅助方法处理可能为空的对象；不要在 Tick 或循环中刷屏。
- 源码标识符、日志和注释主要使用英文，延续现有代码习惯；项目说明与维护文档可以使用中文。
- 注释解释职责、单位、网络侧、前置条件、时序和设计原因，不逐行复述代码；不要新增空的 `/** */`、模板占位注释、AI 对话或生成过程说明。
- 公共类型、非直观配置项和扩展点可以使用 `/** ... */`；短实现说明使用 `//`。不新增大段注释掉的旧实现或临时调试代码。
- UI 可见文本使用 `FText` 与已有 `LOCTEXT` / `NSLOCTEXT` 模式；使用 `LOCTEXT_NAMESPACE` 时在文件末尾取消定义。对象名、资产键等标识使用 `FName` / `FString`，不要把所有字符串都当作用户文案。

## 11. 插件与构建配置

- `Plugins/CommonUser`、`CommonGame`、`GameSettings` 等保留各自的上游结构与风格，局部补丁围绕问题本身，避免混入全文件格式化。
- 项目内 `OnlineSubsystemSteam` 与 `SteamSockets` 是现有引擎插件覆盖方案。修改或升级前阅读各自 README，保留所记录的本地修改与依赖关系，不用项目规范重排导入的整套源码。
- `.Build.cs` / `.Target.cs` 使用 UBT 的现有 C# 写法、Tab 缩进和分行花括号；依赖列表按现有分组一项一行。不要因编码规范调整引擎关联、目标构建版本或插件启用列表。
- 不在源码、共享配置或脚本中写死某台机器的工程、引擎绝对路径；现有本机路径不作为新增代码模板。

## 12. 代码验证

- 改动 C++ 行为或反射声明时，编译实际受影响目标；编辑器模块通过后，还要按改动判断是否需要验证 Game 目标，避免编辑器依赖泄漏。
- 新增非平凡逻辑或修复回归时，使用现有 UE Automation 测试验证可观察行为，测试名保持系统层级，如 `Arena.<系统>.<行为>`、`CommonUser.Session...`；已有测试名保持稳定。
- 测试按模块使用 `WITH_DEV_AUTOMATION_TESTS` 等编译边界；测试夹具、故障注入和测试依赖不无条件进入 Shipping 运行路径。
- 测试创建的 World、全局状态、委托、计时器和配置覆盖应恢复或释放；集成测试覆盖真实生产入口，不能手动制造成功回调来替代目标行为。
- 修改复制、网络会话、蓝图接口或资产配置时，补充相关客户端 / 服务端、PIE 或资产加载验证。无渲染测试不能证明视觉效果，多账号联机也不能由单进程状态测试替代。
- 自动化结果检查测试摘要和断言，不能只看进程退出码。现有 CommonUser 回归命令和验证边界见 `Plugins/CommonUser/Tests/README.md`。

## 13. 新增文件最先检查的项目

近期新增区域效果代码中存在的紧凑写法，不延续为项目规范。审查时优先检查：

- 头文件是否全部位于所属自有模块的 `Public`，实现是否位于 `Private`，相同类型是否保持对应子目录。
- 版权与 `#pragma once`、include、类型之间是否有空行。
- 是否将 `UPROPERTY` / `UFUNCTION` 与声明挤在同一行。
- 配置字段之间是否缺少分隔，是否把枚举、辅助结构体和复杂函数压成一行。
- 是否出现 `if (...) return;`、`if (...) Call();` 或无花括号的 `else` / 循环。
- 是否只为减少行数而滥用 `auto`、缩写或密集表达式。
- 是否将配置、通用行为、具体效果和测试夹具混在一个公共接口中。
- 是否因整理格式混入行为变化、蓝图字段重命名或资产迁移。

上述检查用于后续新增和整理工作；本文本身不代表现有新增文件已完成整改。

## 14. 代表性参考文件

以下路径均相对于仓库根目录。对仍有本地修改的手雷和效果文件，原有约定以 Git `HEAD` 版本为对照。

| 主题 | 参考文件 |
| --- | --- |
| Definition 配置、属性分类、数据校验 | `Source/Arena/Public/Weapon/ArenaGrenadeDefinitionData.h`、`Source/Arena/Private/Weapon/ArenaGrenadeDefinitionData.cpp` |
| 部署配置与编辑器条件编译 | `Source/Arena/Public/Weapon/ArenaDeploymentDefinitionData.h`、`Source/Arena/Private/Weapon/ArenaDeploymentDefinitionData.cpp` |
| 物品定义与 Fragment 组合 | `Source/Arena/Public/Inventory/ArenaInventoryItemDefinition.h`、`Source/Arena/Public/Inventory/InventoryFragment_GrenadeDef.h` |
| 装备定义、运行实例、蓝图事件与复制 | `Source/Arena/Public/Equipment/ArenaEquipmentDefinition.h`、`Source/Arena/Public/Equipment/ArenaEquipmentInstance.h`、`Source/Arena/Private/Equipment/ArenaEquipmentInstance.cpp` |
| 组件、委托、GAS、复制状态 | `Source/Arena/Public/Character/ArenaHealthComponent.h`、`Source/Arena/Private/Character/ArenaHealthComponent.cpp` |
| 技能家族、数据访问、冷却与消息 | `Source/Arena/Private/AbilitySystem/Ability/ArenaGameplayAbility_FromGrenade.cpp`、`Source/Arena/Private/AbilitySystem/Ability/ArenaGameplayAbility_GrenadeFire.cpp` |
| 生成 Actor、权威入口与系统辅助函数 | `Source/Arena/Private/System/ArenaSystemStatics.cpp` |
| 辅助工具的头文件与实现目录 | `Source/Arena/Public/Actor/ArenaGroundAreaTrace.h`、`Source/Arena/Private/Actor/ArenaGroundAreaTrace.cpp` |
| 队伍查询、子系统与接口 | `Source/Arena/Public/Teams/ArenaTeamSubsystem.h`、`Source/Arena/Public/Teams/ArenaTeamAgentInterface.h` |
| UI 参数、对象引用与只读访问 | `Source/Arena/Public/UI/WidgetController/ArenaWidgetController.h` |
| 输入数据资产与 Tag 查询 | `Source/Arena/Public/Input/ArenaInputConfig.h`、`Source/Arena/Private/Input/ArenaInputConfig.cpp` |
| 原生 Tag 与日志分类 | `Source/Arena/Public/ArenaGameplayTags.h`、`Source/Arena/Private/ArenaGameplayTags.cpp`、`Source/Arena/Public/ArenaLogChannels.h` |
| 监听生命周期 | `Source/Arena/Private/Messages/GameplayMessageProcessor.cpp` |
| 设置文本本地化 | `Source/Arena/Private/Settings/ArenaGameSettingRegistry_Gameplay.cpp` |
| 构建与目标配置 | `Source/Arena/Arena.Build.cs`、`Source/Arena.Target.cs`、`Source/ArenaEditor.Target.cs` |
| 插件补丁与回归验证边界 | `Plugins/OnlineSubsystemSteam/README.md`、`Plugins/SteamSockets/README.md`、`Plugins/CommonUser/Tests/README.md` |
