# RTS 单位显示与编队信息设计方案

本文只回答两个问题：

1. 选择面板怎么显示单个单位、中量单位、大量单位。
2. “编队信息”应该怎么设计，后续如何接 UMG 前端和 C++/Mass 后端。

这不是现有代码说明，而是建议的目标方案。

## 设计目标

选择面板不要只是“当前选中了什么”的列表，而应该是玩家的作战控制台。

核心原则：

- 单个单位显示细节。
- 中量单位显示可直接操作的个体列表。
- 大量单位显示按单位类型聚合的“单位 x 数量”摘要。
- 命令卡永远跟“当前激活对象”或“当前激活分组/编队”走。
- UI 不直接理解 Mass Entity、Actor、蓝图类名，只消费后端整理好的 ViewModel。

建议把选择面板分成三个稳定区域：

```text
┌──────────────────────────────────────────────┐
│ A. 当前焦点 / 头像 / 编队标题                  │
├──────────────────────────────────────────────┤
│ B. 单位显示区：单体详情 / 小队列表 / 大军摘要   │
├──────────────────────────────────────────────┤
│ C. 编队信息条：阵型、指令、状态、队形健康度     │
└──────────────────────────────────────────────┘
```

命令卡可以放在选择面板旁边，但数据上不要和选择面板互相猜。选择面板只输出 `ActiveSelectionId`、`ActiveGroupId`、`ActiveFormationId`，命令卡根据这些 ID 解析按钮。

## 三种单位显示模式

### 1. 单个单位

触发条件：

- 当前选择数量为 1。

显示目标：

- 玩家要看清楚“这个单位是谁、状态怎样、属于哪个编队、正在干什么”。

布局建议：

```text
┌──────────────┬──────────────────────────────┐
│ 大头像        │ 单位名称 / 类型 / 等级         │
│              │ HP / Shield / Energy          │
│              │ 当前命令 / 状态标签             │
│              │ 所属编队 / 所在阵型位置         │
└──────────────┴──────────────────────────────┘
```

必显信息：

- 头像。
- 显示名。
- 单位类型。
- 血量条。
- 当前命令：移动、攻击、停止、建造、撤退、集结等。
- 编队归属：例如 `第1步兵连 / A排 / 火力组`。

可选信息：

- 护盾、能量、弹药、士气、补给。
- 等级、经验、击杀数。
- Buff/Debuff。
- 当前目标。
- 阵型槽位：前排、后排、左翼、右翼、预备队。

交互：

- 点击头像：聚焦镜头到单位。
- 双击头像：镜头跟随。
- 右键头像或更多按钮：弹出单位操作菜单。
- Hover 状态图标：显示 tooltip。

UMG 建议：

- `WBP_RTSSelectionPanel`
- `WBP_RTSUnitDetail`
- `WBP_RTSStatusBar`
- `WBP_RTSFormationBadge`
- `WBP_RTSOrderBadge`

后端 ViewModel：

```cpp
USTRUCT(BlueprintType)
struct FRTSUnitDetailView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName UnitId;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) FText UnitTypeName;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Portrait;

    UPROPERTY(BlueprintReadOnly) float HealthPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly) float ShieldPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly) float EnergyPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly) FText CurrentOrderText;
    UPROPERTY(BlueprintReadOnly) FText FormationText;
    UPROPERTY(BlueprintReadOnly) TArray<FGameplayTag> StatusTags;
};
```

### 2. 中量单位

触发条件：

- 当前选择数量大于 1，并且不超过 `SelectionSummaryThreshold`。默认阈值是 16。

这里没有 `2` 和 `7` 的逻辑区别。  
中量单位全部属于同一种显示模式：单位列表。区别只应该是视觉自适应，例如控件空间足够时图标稍大，空间紧张时图标稍紧凑，但后端数据、交互、命令逻辑都完全一样。

为什么默认上限用 16：

- 用户当前设计指定 `TotalCount > 16` 进入大量单位摘要。
- 16 是配置项 `SelectionSummaryThreshold`，不是写死在选择逻辑里。
- 超过该阈值以后，不再逐个显示单位，而是显示“单位类型 + 数量”。

显示目标：

- 玩家要能快速看每个单位的血量和类型，并能点掉、切焦点、按类型筛选。

布局建议：

```text
┌────┬────┬────┬────┬────┬────┐
│ U1 │ U2 │ U3 │ U4 │ U5 │ U6 │
├────┼────┼────┼────┼────┼────┤
│ U7 │ U8 │ U9 │ U10│ U11│ U12│
└────┴────┴────┴────┴────┴────┘
```

每个单位格显示：

- 小头像。
- 血量细条。
- 关键状态角标：受伤、正在攻击、移动中、无法行动、缺补给。
- Active 高亮。

视觉规则：

- 所有中量单位都使用同一个 `WBP_RTSUnitRoster`。
- 单位格尺寸由 UMG 布局自适应，不由后端切模式。
- 是否显示短名称、命令小图标，可以由控件根据可用宽度决定。
- Hover 永远显示完整 tooltip。
- 点击热区必须固定，不随文字显示与否变化。

交互：

- 左键单位格：只选这个单位。
- Shift + 左键：从选择中移除。
- Ctrl + 左键：只保留同类型单位。
- 双击：选择屏幕内同类型单位。
- Tab：切换 active group。
- Hover：显示完整信息。

UMG 建议：

- `WBP_RTSUnitRoster`
- `WBP_RTSUnitTile`
- `WBP_RTSCompactUnitIcon`

后端 ViewModel：

```cpp
USTRUCT(BlueprintType)
struct FRTSUnitRosterItemView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName UnitId;
    UPROPERTY(BlueprintReadOnly) FName GroupId;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
    UPROPERTY(BlueprintReadOnly) float HealthPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool bIsActive = false;
    UPROPERTY(BlueprintReadOnly) bool bIsWounded = false;
    UPROPERTY(BlueprintReadOnly) FGameplayTag CurrentOrderTag;
};
```

### 3. 大量单位

触发条件：

- 当前选择数量大于 `SelectionSummaryThreshold`。默认即 `TotalCount > 16`。

显示目标：

- 玩家不需要看每一个个体，而是要知道“我选中的部队由哪些单位类型构成、每类有多少”。

布局建议：

```text
┌──────┬──────┐ ┌──────┬──────┐ ┌──────┬──────┐
│ Rifle│  42  │ │ MG   │   8  │ │ Tank │  12  │
└──────┴──────┘ └──────┴──────┘ └──────┴──────┘
```

每个摘要项是两个格子：

- 左格：单位类型，显示图标或兵种剪影。
- 右格：数量，显示该类型的 `Count`。

大规模选择不建议显示分页个体列表作为主模式。分页可以有，但应该是二级操作：

- 点击某类型摘要：只保留该类型。
- Alt/详情按钮：展开该类型的个体列表。
- Shift + 点击摘要：从选择中移除这一类型。

大量单位还应该显示“编队摘要”，而不是只显示单位类型：

```text
第1攻击群 | 86单位 | 楔形阵 | 队形完整 72% | 12掉队 | 当前命令：攻击移动
```

UMG 建议：

- `WBP_RTSArmySummary`
- `WBP_RTSUnitGroupRow`
- `WBP_RTSArmyHealthStrip`
- `WBP_RTSFormationSummaryBar`

后端 ViewModel：

```cpp
USTRUCT(BlueprintType)
struct FRTSUnitGroupSummaryView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName GroupId;
    UPROPERTY(BlueprintReadOnly) FText GroupName;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly) int32 Count = 0;
    UPROPERTY(BlueprintReadOnly) int32 WoundedCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 IdleCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 OutOfFormationCount = 0;

    UPROPERTY(BlueprintReadOnly) float AverageHealthPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool bIsActive = false;
    UPROPERTY(BlueprintReadOnly) FGameplayTag DominantOrderTag;
};
```

## 总选择面板 ViewModel

建议不要让 UMG 自己从 `SelectedActors`、`SelectedEntities` 里判断怎么显示。后端直接提供一个稳定 ViewModel。

```cpp
UENUM(BlueprintType)
enum class ERTSSelectionDisplayMode : uint8
{
    Empty,
    Single,
    SmallRoster,
    ArmySummary
};

USTRUCT(BlueprintType)
struct FRTSSelectionPanelView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) ERTSSelectionDisplayMode DisplayMode;
    UPROPERTY(BlueprintReadOnly) int32 TotalUnitCount = 0;

    UPROPERTY(BlueprintReadOnly) FName ActiveUnitId;
    UPROPERTY(BlueprintReadOnly) FName ActiveGroupId;
    UPROPERTY(BlueprintReadOnly) FName ActiveFormationId;

    UPROPERTY(BlueprintReadOnly) FRTSUnitDetailView SingleUnit;
    UPROPERTY(BlueprintReadOnly) TArray<FRTSUnitRosterItemView> RosterItems;
    UPROPERTY(BlueprintReadOnly) TArray<FRTSUnitGroupSummaryView> GroupSummaries;

    UPROPERTY(BlueprintReadOnly) FRTSFormationInfoView FormationInfo;
};
```

切换规则：

```cpp
if (TotalUnitCount == 0) Empty
else if (TotalUnitCount == 1) Single
else if (TotalUnitCount <= SelectionSummaryThreshold) SmallRoster
else ArmySummary
```

阈值只从配置项来，不要在 UI 或后端其它地方再写一份 `12/16/24`。

## 编队信息设计

“编队信息”不是单位类型分组。它应该表示一批单位在战术上被组织成什么队形、由谁带队、当前执行什么命令、队形是否完整。

### 编队信息应该回答的问题

玩家看到编队信息时，应立刻知道：

- 这支部队叫什么。
- 有多少单位。
- 主体构成是什么。
- 当前阵型是什么。
- 当前命令是什么。
- 是否有人掉队。
- 是否有人受损严重。
- 是否还能维持队形。

### 编队 UI

建议放在选择面板底部或顶部，作为固定信息条。

```text
┌──────────────────────────────────────────────┐
│ 第1攻击群  86/92  楔形阵  完整度 72%  攻击移动 │
│ 前锋正常  左翼掉队4  右翼交战  后排迫击炮冷却中 │
└──────────────────────────────────────────────┘
```

单选时：

```text
第1攻击群 / A排 / 火力组    槽位：左翼    距队形点：18m
```

中量单位时：

```text
混合选择：3个编队    Active: 第1攻击群    5单位不在编队
```

大量单位时：

```text
第1攻击群    86单位    楔形阵    完整度72%    12掉队    攻击移动
```

### 编队 ViewModel

```cpp
UENUM(BlueprintType)
enum class ERTSFormationShape : uint8
{
    None,
    Line,
    Column,
    Wedge,
    Box,
    Skirmish,
    Custom
};

UENUM(BlueprintType)
enum class ERTSFormationHealthState : uint8
{
    Stable,
    Stretched,
    Broken,
    Reforming
};

USTRUCT(BlueprintType)
struct FRTSFormationGroupCountView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName GroupId;
    UPROPERTY(BlueprintReadOnly) FText GroupName;
    UPROPERTY(BlueprintReadOnly) int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FRTSFormationInfoView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName FormationId;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;

    UPROPERTY(BlueprintReadOnly) ERTSFormationShape Shape = ERTSFormationShape::None;
    UPROPERTY(BlueprintReadOnly) ERTSFormationHealthState HealthState = ERTSFormationHealthState::Stable;

    UPROPERTY(BlueprintReadOnly) int32 UnitCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 ExpectedUnitCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 OutOfFormationCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 WoundedCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 IdleCount = 0;

    UPROPERTY(BlueprintReadOnly) float CohesionPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly) float AverageHealthPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly) FGameplayTag CurrentOrderTag;
    UPROPERTY(BlueprintReadOnly) FText CurrentOrderText;

    UPROPERTY(BlueprintReadOnly) TArray<FRTSFormationGroupCountView> Composition;
};
```

### 编队信息来源

短期方案：

- 先不要求 Mass 真的有 Formation fragment。
- 后端从当前选择推导一个临时编队视图：
  - 如果所有单位有同一个 `FormationId`，显示这个编队。
  - 如果没有 FormationId，但属于同一个控制组，显示控制组。
  - 如果都没有，显示“临时选择”。

中期方案：

- 给 Mass Entity 增加或读取编队相关数据：
  - `FormationId`
  - `FormationSlotIndex`
  - `FormationShape`
  - `FormationAnchor`
  - `FormationLeader`
  - `DesiredLocation`
  - `bOutOfFormation`

长期方案：

- 编队成为独立逻辑对象，不挂在某一个单位上。
- 单位只引用 `FormationId`。
- 编队系统负责队形点、掉队判定、重整、队形切换。
- 选择面板只显示 `FRTSFormationInfoView`。

## 前后端分工

### 后端负责

后端只负责生成 ViewModel，不关心控件怎么摆。

建议新增或改造：

- `URTSSelectionPresentationSubsystem`
- 或在 `URTSSelectionSubsystem` 内新增 `BuildSelectionPanelView()`

职责：

- 把 Actor 和 Mass 统一成 `UnitId`。
- 决定 `DisplayMode`。
- 生成单体详情、小队列表、大军摘要。
- 生成编队信息。
- 维护 active unit/group/formation。
- 广播 `OnSelectionPanelViewChanged`。

后端不要做：

- 不要直接找 `RTSAvatar` 这种 UMG 名字。
- 不要判断具体 widget class。
- 不要在 UI 层分页时重新查 Mass。

### UMG 前端负责

前端只负责把 ViewModel 画出来。

建议控件树：

```text
WBP_RTSSelectionPanel
├─ WBP_RTSFocusHeader
├─ WidgetSwitcher DisplayModeSwitcher
│  ├─ EmptyView
│  ├─ WBP_RTSUnitDetail
│  ├─ WBP_RTSUnitRoster
│  └─ WBP_RTSArmySummary
└─ UnitFormationList
```

每个控件只暴露一个刷新函数：

```cpp
UFUNCTION(BlueprintImplementableEvent)
void SetView(const FRTSSelectionPanelView& View);
```

或者拆细：

```cpp
void SetUnitDetail(const FRTSUnitDetailView& View);
void SetRoster(const TArray<FRTSUnitRosterItemView>& Items);
void SetArmySummary(const TArray<FRTSUnitGroupSummaryView>& Groups);
void SetFormationInfo(const FRTSFormationInfoView& Formation);
```

UMG MCP 后续可以按这个控件树生成前端；C++ 后端只保证字段稳定。

## Active 概念重新定义

现有 `ActiveGroupKey` 容易混。建议拆成三个 ID：

- `ActiveUnitId`：单个单位焦点。
- `ActiveGroupId`：单位类型或战术分组焦点。
- `ActiveFormationId`：编队焦点。

不同显示模式下的 active 规则：

| 模式 | ActiveUnitId | ActiveGroupId | ActiveFormationId |
| --- | --- | --- | --- |
| Single | 当前单位 | 当前单位类型 | 所属编队 |
| SmallRoster | 最近点击单位 | 最近点击单位类型 | 主要编队 |
| ArmySummary | 空或代表单位 | 当前摘要类型 | 主要编队 |

Tab 建议只切 `ActiveGroupId`。  
Ctrl+Tab 或专门按钮再切 `ActiveFormationId`。  
点击单个单位格才切 `ActiveUnitId`。

## 阈值建议

默认阈值：

- `0`：空选择。
- `1`：单体详情。
- `2..SelectionSummaryThreshold`：中量单位列表，同一套 roster UI。
- `> SelectionSummaryThreshold`：大量单位摘要。

可以配成：

```cpp
UPROPERTY(Config, EditAnywhere)
int32 SelectionSummaryThreshold = 16;
```

但建议不要把 `SelectionSummaryThreshold` 改得过大。超过阈值后应该切“单位 x 数量”摘要，不应该继续铺个体格子。

## 推荐实施顺序

### 第一步：先做 ViewModel，不碰 UMG

新增 `FRTSSelectionPanelView` 和相关子结构。  
先用日志或调试函数验证：

- 单选时生成 `Single`。
- `2..SelectionSummaryThreshold` 生成 `SmallRoster`。
- `> SelectionSummaryThreshold` 生成 `ArmySummary`。
- 编队信息先生成“临时选择”。

### 第二步：替换现有选择面板刷新入口

让 `URTSUnitPanelWidget` 不再直接吃 `FRTSSelectionView`，而是吃 `FRTSSelectionPanelView`。

旧 `FRTSSelectionView` 可以暂时保留给命令卡，但选择面板不要继续依赖它。

### 第三步：用 UMG MCP 做控件树

按前面的控件树创建：

- 单体详情页。
- 中量单位 roster。
- 大军 summary。
- 编队信息条。

### 第四步：接真实编队数据

先接最小字段：

- FormationId。
- FormationName。
- Shape。
- CohesionPercent。
- OutOfFormationCount。

其余状态后续补。

## 验收标准

单个单位：

- 能看到头像、名字、血量、当前命令、所属编队。
- 命令卡能跟这个单位或它的类型走。

中量单位：

- 中量单位都使用同一个单位列表模式。
- 每个单位能点选、Shift 移除、Ctrl 选同类。

大量单位：

- 不显示一堆小格子。
- 按类型聚合。
- 能看到每类数量、平均健康、异常数量。
- 点击类型摘要能切 active group 或只保留该类型。

编队信息：

- 单选能知道该单位属于哪个编队和哪个槽位。
- 多选能知道主要编队、阵型、完整度、掉队数量。
- 混合选择时能显示“多个编队”而不是乱选一个。

架构：

- UMG 只吃 ViewModel。
- C++/Mass 只生成 ViewModel。
- 不再让 UI 通过控件名硬找外部头像。
- 不再用一个 `ActiveGroupKey` 同时表达单位类型、当前头像、命令卡上下文、编队焦点。
