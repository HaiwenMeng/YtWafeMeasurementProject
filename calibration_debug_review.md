# 校准调试复盘

## 背景

本次问题发生在 `WafeMeasureTemplate` 接入旧版 `F:\new\wafer-measurement\WafeMeasurement` 的校准流程后. 目标是保持旧版 UI 和校准逻辑不变, 同时复用当前项目里的运动控制, 共聚焦控制, 算法和配置模块.

旧版校准的核心计算很简单:

```text
realTotal = average(filter(topDistance + bottomDistance + standardThickness), 30 samples) + compensation
```

所以如果总值异常, 优先要判断这三类输入:

- 标准片位置和运动轨迹是否一致.
- 共聚焦 top 和 bottom 的实时距离是否一致.
- 运行时 DLL 和采样方式是否一致.

## 最终结论

这次踩坑的主因是共聚焦运行时 DLL 版本和旧版不一致. 之前虽然有一次把工程 SDK 路径回退到 3.30, 但部署机器上的 `BIN\DLL_CCS.dll` 很可能没有同步替换. Windows 会优先加载 exe 同目录下的 DLL, 所以只改头文件和 lib 路径不够, 运行时仍可能跑 3.40 DLL.

第二个问题是校准采样直接使用 UI 缓存变量 `m_current_distance_top` 和 `m_current_distance_bottom`. 这些变量由共聚焦信号异步更新. 当采样线程, UI 线程和共聚焦线程之间出现队列延迟时, 校准带入值可能不是传感器此刻的最新采样值.

第三个问题是运动控制更严格. 旧版 30 次采样大约刚好覆盖 6mm 扫描段, 但新 `MotionCtrlDemo` 在扫描段末尾还会保持 `state=6`. 程序马上发下一段运动时, 会报 `axis X is not ready`.

## 现象复盘

### 1. 打开软件后运动未就绪

现象:

```text
轴系运动中, 或未处于准备就绪状态, 请确认!
moveMultiAxisLinear failed, axis X state is unknown.
```

判断:

这是新工程接入旧 UI 后, 初始化 timer 时序和旧版没有完全收敛导致. 运动状态没有进入 ready, UI 流程已经允许发运动.

处理:

按旧版收敛运动初始化 timer 时序, 同时兼容接口失败必须返回 false 和报错, 不做假成功.

结论:

这是初始化状态机问题, 不是校准值异常的根因.

### 2. 四个标准片校准值不一致

现象:

新程序校准结果中, 标准片 2 和 3 有时接近, 标准片 1 和 4 明显错误. 例如:

```text
standard1 total = 1713.245
standard2 total = 1938.931
standard3 total = 2009.954
standard4 total = 2594.346
```

旧版同一批标准片大约为:

```text
standard1 total = 1993.22
standard2 total = 1993.29
standard3 total = 1992.59
standard4 total = 1993.46
```

初步误判风险:

标准片 2 和 3 偶尔接近正确, 容易误以为公式部分没问题而只查位置. 但实际上 2 和 3 的期望 `top + bottom` 本来就在 1180 到 1265 这个区间, 新程序漂移后的平台值也在附近, 所以接近是偶然的.

结论:

2 和 3 接近不能证明采样正确. 标准片 1 和 4 更能暴露问题.

### 3. 运动位置肉眼看起来正确

现象:

校准时机台确实移动到 4 个标准片上方, 并沿 Y 轴移动一小段距离采样. 手动移动到标准片 4 起点, 中心点, 终点时, 旧版和新程序最初都能看到接近的实时共聚焦值.

判断:

运动位置大方向不是主因. 后续日志也证明 waitTarget 和 scanTarget 基本正确.

结论:

这排除了大部分起点计算, 中心点计算和扫描方向错误的可能.

### 4. 实时值和校准带入值不一致

关键现象:

某次 standard4 校准扫描中日志为:

```text
standard4 sample=1 top=598.570 bottom=726.320 raw=2886.290
standard4 sample=30 top=554.214 bottom=659.420 raw=2775.035
```

但校准报错后, UI 上共聚焦实时值显示大约:

```text
top = 306.403
bottom = 249.623
```

旧版软件在同一位置也能立即回到类似 252 + 157 左右.

判断:

如果机械位置和标准片没变, 校准采样中的 554 + 659 与 UI 实时值 306 + 249 不应差这么大. 这说明校准实际带入的值和此刻显示的共聚焦实时值不是同一个稳定来源.

结论:

问题重点转向共聚焦采集会话, SDK 版本, 以及异步缓存路径.

### 5. 直接 SDK 读取没有解决

过程:

曾经在校准采样中加入 direct SDK read 来比较 `m_current_distance_*` 和 `CCS_GetCurrentDistanceData`.

结果:

direct read 没有把问题收敛, 反而增加了共聚焦线程和校准线程同时访问 SDK 的风险.

结论:

校准流程不应该在 UI 线程缓存之外再随意打一个 SDK 读取旁路. 更稳的方案是共聚焦采集线程统一读 SDK, 然后提供一个线程安全的最新样本缓存.

### 6. SDK 回退曾经没有生效

现象:

之前试过把 SDK 切回 3.30, 但数值仍不对.

复判:

当时很可能只改了工程引用, 没有把部署机器或 `BIN` 下的 `DLL_CCS.dll` 替换成 3.30. 由于 exe 会优先加载同目录 DLL, 实际运行仍然可能是 3.40.

证据:

这次真正有效的状态是:

```text
WafeMeasureTemplate.pro -> BaseLib/SDK_V3_30_x64
colorFocusContrlDemo.pro -> BaseLib/SDK_V3_30_x64
BIN/DLL_CCS.dll -> 2025/6/12, length 79872
```

而 3.40 DLL 是:

```text
BaseLib/SDK_V3_40_x64/DLL_CCS.dll -> 2026/3/31, length 84992
```

结论:

编译 SDK 路径和运行时 DLL 必须同时一致. 只改一个没有意义.

### 7. 运动 `state=6` 报错

现象:

standard1 采样结束后日志显示:

```text
sample=30 axis=(277.824,279.658,78.462) state=(6,6,3)
result ... axis=(277.824,279.457,78.462) state=(6,6,3)
```

随后下一段运动报:

```text
moveMultiAxisLinear failed, axis X is not ready, state=6
```

判断:

6mm 扫描段以 2mm/s 运行, 理论约 3s. 30 次采样 * 100ms 也是约 3s. 旧版恰好能覆盖, 新运动封装多一点调度延迟后, 30 点结束时轴还没有停稳.

处理:

在每段校准扫描 30 点采样完成后增加:

```text
waitInPos(scanTarget)
```

并打印:

```text
[CAL-DIAG] standardX scanEnd axis=(...) state=(3,3,3)
```

结论:

这是运动衔接问题, 会导致弹框, 但不是厚度值错误的主因.

## 已完成修复

### 1. 切回旧版共聚焦 SDK 3.30

修改位置:

```text
WafeMeasureTemplate/WafeMeasureTemplate.pro
colorFocusContrlDemo/colorFocusContrlDemo.pro
```

关键点:

```text
SDK_DIR = BaseLib/SDK_V3_30_x64
DEFINES += CCS_SDK_V330
```

3.30 不支持的 3.40 API 不再假成功. Wrapper 会明确返回 false 并写日志, 例如:

```text
SetSinglePointSmoothingFactor is not supported by DLL_CCS SDK V3.30
SetDAParam is not supported by DLL_CCS SDK V3.30
```

### 2. 确认运行时 DLL 被替换

构建后复制:

```text
BaseLib/SDK_V3_30_x64/DLL_CCS.dll -> BIN/DLL_CCS.dll
```

验收值:

```text
BIN/DLL_CCS.dll length = 79872
BIN/DLL_CCS.dll LastWriteTime = 2025/6/12 18:49:08
```

### 3. 共聚焦采集线程增加最新样本缓存

修改位置:

```text
colorFocusContrlDemo/colorFocus_control.h
colorFocusContrlDemo/colorFocus_control.cpp
```

新增字段:

```text
m_hasLatestSample
m_latestDistance
m_latestIntensity
```

采集线程每次 `CCS_GetCurrentDistanceData` 成功后, 同步更新最新样本缓存, 再发 UI 信号.

### 4. 校准采样使用最新样本缓存

修改位置:

```text
WafeMeasureTemplate/mainwindow.cpp
```

新增:

```text
ReadLatestColorFocusDistances(...)
AbortCalibration(...)
```

校准 `CollectCalibrationTotal` 现在使用最新样本缓存计算:

```text
top + bottom + standardThickness
```

如果没有有效样本, 直接中止并提示:

```text
共聚焦校准采样失败, 请检查共聚焦采集状态!
```

不会写 0 结果或继续假成功.

### 5. 校准扫描段结束后等待轴 ready

修改位置:

```text
WafeMeasureTemplate/mainwindow.cpp
```

每个标准片扫描段采样完成后等待 scanTarget:

```text
waitInPos(QPointF(standard_x - dx, standard_y - dy))
```

日志:

```text
[CAL-DIAG] standardX scanEnd axis=(...) state=(3,3,3)
```

## 当前判断

本次校准异常不是以下问题导致:

- 不是标准片厚度公式错.
- 不是 4 个标准片运动目标点整体错.
- 不是肉眼可见的标准片放置错误.
- 不是厚度曲线点数或重力扫描逻辑直接导致.

主要问题是:

- 运行时 `DLL_CCS.dll` 没有真正按旧版落地.
- 校准采样依赖异步 UI 缓存, 容易和实时显示不同步.
- 新运动控制对 ready 状态更严格, 需要显式等待扫描段结束.

## 后续部署检查清单

每次部署前必须检查:

```powershell
Get-Item F:\YtWafeMeasurementProject\BIN\DLL_CCS.dll | Select-Object FullName,Length,LastWriteTime
```

期望:

```text
Length = 79872
LastWriteTime = 2025/6/12 18:49:08
```

不要只复制 exe. 必须同时确认:

- `WafeMeasureTemplate.exe`
- `DLL_CCS.dll`
- `AlgApi.dll`
- 其他运行依赖
- `config.xml`

每次改 `.pro` 后必须重新 qmake:

```powershell
qmake F:\YtWafeMeasurementProject\YtWafeMeasurementProject.pro
nmake /f Makefile sub-colorFocusContrlDemo sub-WafeMeasureTemplate
```

校准日志验收重点:

```text
[CAL-DIAG] standardX waitTarget=... source=latest
[CAL-DIAG] standardX sample=... top=... bottom=... raw=... filtered=...
[CAL-DIAG] standardX scanEnd ... state=(3,3,3)
```

如果 `source=latest` 不出现, 说明运行的不是最新 exe.

如果 `BIN\DLL_CCS.dll` 不是 3.30, 说明部署不完整.

如果 `scanEnd` 不是 `state=(3,3,3)`, 说明运动状态没有等稳.

## 经验教训

1. 对 DLL 项目, 不能只看编译路径. 必须核对运行目录的 DLL.
2. UI 显示值不能直接等价于校准计算值. 校准应使用统一采集层提供的最新样本.
3. 直接在多个线程同时读 SDK 很危险. 应统一由采集线程读 SDK, 其他逻辑读线程安全缓存.
4. 运动状态机差异会把旧版的隐式时间假设暴露出来. 旧版靠 30 * 100ms 刚好结束, 新版应显式等待 ready.
5. 日志必须区分问题类型. 运动状态, 共聚焦采样源, SDK/DLL 版本应分开记录.
6. 不能用标准片 2 和 3 偶然接近来证明逻辑正确. 标准片 1 和 4 更适合暴露 `top + bottom` 异常.

## 建议保留的诊断日志

建议暂时保留以下日志到整机流程稳定:

```text
[CAL-DIAG] begin mode=...
[CAL-DIAG] standardX waitTarget=... source=latest
[CAL-DIAG] standardX sum-check actualTopBottom=... expectedTopBottom=...
[CAL-DIAG] standardX sample=1/15/30 ...
[CAL-DIAG] standardX result ...
[CAL-DIAG] standardX scanEnd ...
```

等连续多次校准和产品测量都稳定后, 可以降低 sample 日志频率, 但建议保留 begin, result 和 scanEnd.
