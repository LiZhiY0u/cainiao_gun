# Firmware Directory Consolidation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 `new_pro_delay10ms/` 收敛为唯一正式固件目录 `firmware/`，删除旧 `software/`，并提供准确的构建说明。

**Architecture:** 保持正式固件源码和依赖配置不变，只改变仓库级目录边界。`firmware/` 是唯一发布入口，`examples/` 继续承载实验工程，README 负责清楚区分二者。

**Tech Stack:** ESP32-WROOM-32E、Arduino framework、PlatformIO、MPU6050 I2Cdevlib、Adafruit NeoPixel、内置 ESP32 BLE Combo 源码。

## Global Constraints

- 旧 `software/` 直接删除，不建立归档副本。
- `new_pro_delay10ms/` 重命名为 `firmware/`。
- 不修改鼠标控制行为、硬件引脚、依赖版本或构建目标。
- 保留 `examples/`、`images/` 与完整的 `firmware/src/combo/`。
- 删除正式固件中的 `.vscode/`、`include/README`、`lib/README`、`test/README` 及由此产生的空目录。
- 正式固件必须能通过 `pio run -d firmware` 构建，或明确记录由本机缺少 PlatformIO/网络导致的环境阻塞。

---

### Task 1: 收敛正式固件目录

**Files:**
- Rename: `new_pro_delay10ms/` → `firmware/`
- Delete: `software/`
- Delete: `firmware/.vscode/extensions.json`
- Delete: `firmware/include/README`
- Delete: `firmware/lib/README`
- Delete: `firmware/test/README`
- Preserve: `firmware/platformio.ini`
- Preserve: `firmware/src/main.cpp`
- Preserve: `firmware/src/combo/**`

**Interfaces:**
- Consumes: 当前 `new_pro_delay10ms/` PlatformIO 工程。
- Produces: 唯一正式构建入口 `firmware/platformio.ini`。

- [ ] **Step 1: 运行整理前结构断言，确认它按预期失败**

```powershell
$ok = (Test-Path firmware/platformio.ini) -and
      -not (Test-Path new_pro_delay10ms) -and
      -not (Test-Path software)
if (-not $ok) { throw 'formal firmware structure is not consolidated' }
```

Expected: FAIL，错误为 `formal firmware structure is not consolidated`。

- [ ] **Step 2: 使用 Git 感知的移动和删除完成目录收敛**

```powershell
git mv new_pro_delay10ms firmware
git rm -r software
git rm firmware/.vscode/extensions.json firmware/include/README firmware/lib/README firmware/test/README
```

- [ ] **Step 3: 运行结构断言并确认正式源码完整**

```powershell
$required = @(
  'firmware/platformio.ini',
  'firmware/src/main.cpp',
  'firmware/src/combo/BleCombo.cpp',
  'firmware/src/combo/BleComboMouse.cpp'
)
$missing = $required | Where-Object { -not (Test-Path $_) }
if ($missing) { throw "missing required firmware files: $($missing -join ', ')" }
if (Test-Path new_pro_delay10ms) { throw 'new_pro_delay10ms still exists' }
if (Test-Path software) { throw 'software still exists' }
if (Test-Path firmware/.vscode) { throw 'firmware/.vscode still exists' }
if (Test-Path firmware/include) { throw 'firmware/include still exists' }
if (Test-Path firmware/lib) { throw 'firmware/lib still exists' }
if (Test-Path firmware/test) { throw 'firmware/test still exists' }
```

Expected: PASS，无输出。

- [ ] **Step 4: 确认正式固件文件仅发生路径变化或明确删除**

```powershell
git diff --stat --cached
git diff --summary --cached
```

Expected: 显示 `new_pro_delay10ms` 到 `firmware` 的重命名、`software` 删除和四个模板文件删除，不出现 `main.cpp` 或 `src/combo` 内容修改。

- [ ] **Step 5: 提交目录收敛**

```powershell
git commit -m "refactor: consolidate production firmware"
```

Expected: 提交成功，工作区不再包含已暂存的目录变更。

---

### Task 2: 更新仓库入口文档

**Files:**
- Modify: `README.md`

**Interfaces:**
- Consumes: Task 1 产生的 `firmware/` 目录。
- Produces: 面向开发者的正式入口、目录说明和构建命令。

- [ ] **Step 1: 运行 README 断言，确认旧文档按预期失败**

```powershell
$readme = Get-Content -Raw -Encoding UTF8 README.md
if ($readme -notmatch 'pio run -d firmware' -or $readme -match 'software：源码') {
  throw 'README does not describe the consolidated firmware entry'
}
```

Expected: FAIL，错误为 `README does not describe the consolidated firmware entry`。

- [ ] **Step 2: 将 README 改为下面的完整内容**

```markdown
# cainiao_gun

基于 ESP32-WROOM-32E 和 MPU6050 的 BLE 体感游戏控制器，可模拟键盘和鼠标输入。

## 工程结构

- `firmware/`：唯一正式固件，使用 VS Code + PlatformIO 开发和构建。
- `examples/`：开发过程中的功能实验和硬件验证工程，不作为正式发布固件。
- `images/`：项目图片。

## 构建正式固件

安装 PlatformIO 后，在仓库根目录运行：

```powershell
pio run -d firmware
```

需要烧录时运行：

```powershell
pio run -d firmware -t upload
```

正式固件目标板为 `esp32dev`，使用 Arduino framework。依赖由 `firmware/platformio.ini` 管理。

## 已实现功能

- 摇杆和体感控制模式
- BLE 键盘与鼠标组合设备
- MPU6050 运动数据读取
- 基本按键映射
- RGB 灯效
- 电磁后坐控制

## BLE Combo

正式固件将 BLE 键盘和鼠标组合实现内置在 `firmware/src/combo/`，构建时不需要单独复制该库。

参考项目：

- [ESP32-BLE-Combo](https://github.com/ServAlex/ESP32-BLE-Combo)
- [ESP32-BLE-Mouse](https://github.com/T-vK/ESP32-BLE-Mouse)

## License

本项目使用仓库中 [LICENSE](LICENSE) 所述许可证。
```

- [ ] **Step 3: 验证 README 与实际结构一致**

```powershell
$readme = Get-Content -Raw -Encoding UTF8 README.md
$requiredText = @('`firmware/`', '`examples/`', '`images/`', 'pio run -d firmware')
$missing = $requiredText | Where-Object { -not $readme.Contains($_) }
if ($missing) { throw "README missing: $($missing -join ', ')" }
if ($readme -match 'new_pro_delay10ms|software：源码') { throw 'README contains obsolete firmware paths' }
```

Expected: PASS，无输出。

- [ ] **Step 4: 检查 Markdown 和空白错误**

```powershell
git diff --check
git diff -- README.md
```

Expected: `git diff --check` 无输出；README diff 只包含上述文档更新。

- [ ] **Step 5: 提交 README 更新**

```powershell
git add README.md
git commit -m "docs: document production firmware entry"
```

Expected: 提交成功。

---

### Task 3: 构建并完成仓库验收

**Files:**
- Verify: `firmware/platformio.ini`
- Verify: `firmware/src/**`
- Verify: repository working tree

**Interfaces:**
- Consumes: 整理后的正式固件和 README。
- Produces: 可复现的结构与构建验证结果。

- [ ] **Step 1: 验证 PlatformIO 是否可用**

```powershell
pio --version
```

Expected: 输出 PlatformIO Core 版本。若命令不存在，记录环境阻塞并继续 Step 3 的静态验收。

- [ ] **Step 2: 构建唯一正式固件**

```powershell
pio run -d firmware
```

Expected: `SUCCESS`，环境 `esp32dev` 构建完成。若依赖下载因网络策略失败，记录具体错误，不修改依赖版本规避。

- [ ] **Step 3: 执行最终静态验收**

```powershell
$required = @('firmware/platformio.ini', 'firmware/src/main.cpp', 'README.md', 'LICENSE')
$missing = $required | Where-Object { -not (Test-Path $_) }
if ($missing) { throw "missing: $($missing -join ', ')" }
if (Test-Path software) { throw 'software still exists' }
if (Test-Path new_pro_delay10ms) { throw 'new_pro_delay10ms still exists' }
$rootFirmwareProjects = Get-ChildItem -Directory | Where-Object {
  Test-Path (Join-Path $_.FullName 'platformio.ini')
}
if ($rootFirmwareProjects.Name -ne 'firmware') {
  throw "unexpected root PlatformIO projects: $($rootFirmwareProjects.Name -join ', ')"
}
```

Expected: PASS，无输出。

- [ ] **Step 4: 验证提交和工作区状态**

```powershell
git log -3 --oneline
git status --short
git diff --check HEAD
```

Expected: 最近提交包含设计、目录收敛和 README 更新；`git status --short` 与 `git diff --check HEAD` 均无输出。PlatformIO 生成的 `firmware/.pio/` 应被 `.gitignore` 忽略。
