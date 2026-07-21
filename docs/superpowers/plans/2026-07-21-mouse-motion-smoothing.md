# Mouse Motion Smoothing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 消除 ESP32 体感鼠标运动链路中的阻塞、BLE 突发报告和整数截断，使指针移动更连续并烧录到 COM7 实机验证。

**Architecture:** 新增一个不依赖 Arduino/BLE 的纯 C++ `MouseMotionFilter`，负责灵敏度、亚像素残差和 HID 范围限制。MPU 层只在取得新数据时触发一次转换，BLE 层每个样本最多发送一个非零报告；逐帧串口日志默认关闭。

**Tech Stack:** ESP32-WROOM-32E、Arduino framework、PlatformIO Core 6.1.19、MPU6050 DMP、ESP32 BLE Combo、Unity embedded tests、CH340 COM7。

## Global Constraints

- 保留现有 ESP32 BLE Combo、BLE 名称、配对方式、硬件引脚、按键、RGB、电磁后坐和依赖版本。
- 不迁移 NimBLE，不修改 BLE 连接间隔。
- 删除运动路径中的活动 `delay(9)`、`delay(3)` 和四次重复 `Mouse.move()`。
- 初始灵敏度固定为 `4.0f / 250.0f`，保持接近旧版总移动速度。
- 每个新 MPU 样本最多发送一次非零 HID 报告，X/Y 限制为 `-127..127`。
- 使用现有 `C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe`；不得下载或安装额外开发环境。
- 板载测试和正式固件均使用 CH340 `COM7`；测试固件完成后必须重新烧录正式固件。

---

### Task 1: 用测试驱动实现亚像素位移转换器

**Files:**
- Create: `firmware/test/test_mouse_motion/test_main.cpp`
- Create: `firmware/include/MouseMotionFilter.h`

**Interfaces:**
- Consumes: `int16_t gy`, `int16_t gz` 和构造时传入的 `float sensitivity`。
- Produces: `MouseDelta MouseMotionFilter::update(int16_t gy, int16_t gz)` 与 `void MouseMotionFilter::reset()`。

- [ ] **Step 1: 创建失败的板载单元测试**

创建 `firmware/test/test_mouse_motion/test_main.cpp`：

```cpp
#include <Arduino.h>
#include <unity.h>
#include "MouseMotionFilter.h"

void setUp() {}
void tearDown() {}

void test_subpixel_motion_accumulates()
{
    MouseMotionFilter filter(0.01f);
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
    TEST_ASSERT_EQUAL_INT8(-1, filter.update(0, 25).x);
}

void test_negative_input_accumulates_in_opposite_direction()
{
    MouseMotionFilter filter(0.01f);
    filter.update(0, -25);
    filter.update(0, -25);
    filter.update(0, -25);
    TEST_ASSERT_EQUAL_INT8(1, filter.update(0, -25).x);
}

void test_axes_keep_current_final_hid_direction()
{
    MouseMotionFilter filter(4.0f / 250.0f);
    const MouseDelta delta = filter.update(250, 250);
    TEST_ASSERT_EQUAL_INT8(-4, delta.x);
    TEST_ASSERT_EQUAL_INT8(4, delta.y);
}

void test_output_is_clamped_to_hid_range()
{
    MouseMotionFilter filter(4.0f / 250.0f);
    const MouseDelta delta = filter.update(32767, 32767);
    TEST_ASSERT_EQUAL_INT8(-127, delta.x);
    TEST_ASSERT_EQUAL_INT8(127, delta.y);
}

void test_reset_discards_fractional_residual()
{
    MouseMotionFilter filter(0.01f);
    filter.update(0, 25);
    filter.update(0, 25);
    filter.reset();
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
}

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_subpixel_motion_accumulates);
    RUN_TEST(test_negative_input_accumulates_in_opposite_direction);
    RUN_TEST(test_axes_keep_current_final_hid_direction);
    RUN_TEST(test_output_is_clamped_to_hid_range);
    RUN_TEST(test_reset_discards_fractional_residual);
    UNITY_END();
}

void loop() {}
```

- [ ] **Step 2: 运行测试并验证因缺少实现而失败**

```powershell
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' test -d firmware -e esp32dev --upload-port COM7 --test-port COM7 -f test_mouse_motion
```

Expected: FAIL，编译器报告 `MouseMotionFilter.h: No such file or directory`；测试固件不会上传。

- [ ] **Step 3: 创建最小转换器实现**

创建 `firmware/include/MouseMotionFilter.h`：

```cpp
#ifndef CAINIAO_MOUSE_MOTION_FILTER_H
#define CAINIAO_MOUSE_MOTION_FILTER_H

#include <stdint.h>

struct MouseDelta
{
    int8_t x;
    int8_t y;
};

class MouseMotionFilter
{
public:
    explicit MouseMotionFilter(float sensitivity)
        : sensitivity_(sensitivity), residualX_(0.0f), residualY_(0.0f)
    {
    }

    MouseDelta update(int16_t gy, int16_t gz)
    {
        residualX_ += -static_cast<float>(gz) * sensitivity_;
        residualY_ += static_cast<float>(gy) * sensitivity_;

        const int32_t wholeX = static_cast<int32_t>(residualX_);
        const int32_t wholeY = static_cast<int32_t>(residualY_);
        residualX_ -= static_cast<float>(wholeX);
        residualY_ -= static_cast<float>(wholeY);

        return MouseDelta{clampHid(wholeX), clampHid(wholeY)};
    }

    void reset()
    {
        residualX_ = 0.0f;
        residualY_ = 0.0f;
    }

private:
    static int8_t clampHid(int32_t value)
    {
        if (value > 127)
            return 127;
        if (value < -127)
            return -127;
        return static_cast<int8_t>(value);
    }

    float sensitivity_;
    float residualX_;
    float residualY_;
};

#endif
```

- [ ] **Step 4: 重新运行板载测试并确认全部通过**

```powershell
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' test -d firmware -e esp32dev --upload-port COM7 --test-port COM7 -f test_mouse_motion
```

Expected: 5 个测试通过，0 个失败。

- [ ] **Step 5: 提交转换器和测试**

```powershell
git add firmware/include/MouseMotionFilter.h firmware/test/test_mouse_motion/test_main.cpp
git commit -m "test: add mouse motion quantization coverage"
```

---

### Task 2: 将实时运动链路接入转换器

**Files:**
- Modify: `firmware/src/main.cpp`

**Interfaces:**
- Consumes: Task 1 的 `MouseMotionFilter` 和 `MouseDelta`。
- Produces: 新数据驱动、单报告、无阻塞的 BLE 鼠标运动路径。

- [ ] **Step 1: 运行旧链路静态检查并验证问题存在**

```powershell
$violations = rg -n 'delay\(9\)|delay\(3\)|_i < 4|"  mouseX:" \+ String' firmware/src/main.cpp
if (-not $violations) { throw 'expected legacy motion violations were not found' }
$violations
```

Expected: 找到 `delay(9)`、`delay(3)`、四次发送循环和动态 `String` 日志。

- [ ] **Step 2: 引入转换器并定义单一灵敏度**

在 include 区增加：

```cpp
#include "MouseMotionFilter.h"
```

把原来的 `int16_t mouseX = 0, mouseY = 0;` 替换为：

```cpp
constexpr float MOUSE_SENSITIVITY = 4.0f / 250.0f;
MouseMotionFilter mouseMotion(MOUSE_SENSITIVITY);
int8_t mouseX = 0, mouseY = 0;

#define MOUSE_DEBUG 0
```

- [ ] **Step 3: 将 MPU 读取改为新数据返回值且删除阻塞**

用下面完整实现替换 `mpu_DMP()`：

```cpp
bool mpu_DMP()
{
    if (!dmpReady)
        return false;

    if (!mpu.dmpGetCurrentFIFOPacket(fifoBuffer))
        return false;

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    return true;
}
```

- [ ] **Step 4: 将鼠标发送改为单个非零 HID 报告**

用下面完整实现替换 `mouse_Move()`：

```cpp
void mouse_Move(int8_t x, int8_t y)
{
    if (x != 0 || y != 0)
        Mouse.move(x, y);
}
```

- [ ] **Step 5: 将调试输出改为默认关闭的低频固定格式**

用下面完整实现替换 `_print()`：

```cpp
void _print()
{
#if MOUSE_DEBUG
    static uint32_t lastPrintMs = 0;
    const uint32_t now = millis();
    if (now - lastPrintMs < 500)
        return;

    lastPrintMs = now;
    Serial.printf("gy:%d gz:%d mouseX:%d mouseY:%d\n", gy, gz, mouseX, mouseY);
#endif
}
```

- [ ] **Step 6: 用新数据驱动的主循环替换旧 loop**

用下面完整实现替换整个 `loop()`；删除其中仅作为注释保留的示例代码，不改其他功能函数：

```cpp
void loop()
{
    if (!KeyBoard.isConnected())
    {
        mouseMotion.reset();
        mouseX = 0;
        mouseY = 0;
        return;
    }

    if (!mpu_DMP())
        return;

    const MouseDelta delta = mouseMotion.update(gy, gz);
    mouseX = delta.x;
    mouseY = delta.y;
    mouse_Move(mouseX, mouseY);
    _print();
}
```

- [ ] **Step 7: 验证旧运动链路模式已消失**

```powershell
$violations = rg -n 'delay\(9\)|delay\(3\)|_i < 4|"  mouseX:" \+ String' firmware/src/main.cpp
if ($violations) { throw "legacy motion violations remain: $violations" }
```

Expected: PASS，无输出。

- [ ] **Step 8: 编译正式固件并重新运行转换器测试**

```powershell
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' run -d firmware
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' test -d firmware -e esp32dev --upload-port COM7 --test-port COM7 -f test_mouse_motion
```

Expected: 正式固件 `SUCCESS`；5 个测试通过、0 个失败。

- [ ] **Step 9: 提交实时链路优化**

```powershell
git add firmware/src/main.cpp
git commit -m "fix: smooth BLE mouse motion reports"
```

---

### Task 3: 烧录正式固件并完成验收

**Files:**
- Verify: `firmware/.pio/build/esp32dev/firmware.bin`
- Verify: `firmware/src/main.cpp`
- Verify: Git working tree

**Interfaces:**
- Consumes: Task 2 通过编译与测试的正式固件。
- Produces: COM7 上运行的优化固件与可复现验证记录。

- [ ] **Step 1: 再次编译最终提交状态**

```powershell
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' run -d firmware
```

Expected: `SUCCESS`，RAM 和 Flash 均未超出容量。

- [ ] **Step 2: 通过 CH340 COM7 烧录正式固件**

```powershell
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' run -d firmware -t upload --upload-port COM7
```

Expected: 检测到 ESP32、写入并校验完成，最终显示 `SUCCESS`。

- [ ] **Step 3: 执行最终静态与仓库验收**

```powershell
$required = @(
  'firmware/include/MouseMotionFilter.h',
  'firmware/test/test_mouse_motion/test_main.cpp',
  'firmware/src/main.cpp',
  'firmware/.pio/build/esp32dev/firmware.bin'
)
$missing = $required | Where-Object { -not (Test-Path $_) }
if ($missing) { throw "missing: $($missing -join ', ')" }
$violations = rg -n 'delay\(9\)|delay\(3\)|_i < 4|"  mouseX:" \+ String' firmware/src/main.cpp
if ($violations) { throw "legacy motion violations remain: $violations" }
git diff --check HEAD
$status = git status --short
if ($status) { throw "working tree is not clean: $status" }
```

Expected: PASS，无输出。

- [ ] **Step 4: 报告实机调节入口**

向用户报告测试数量、编译内存占用、COM7 烧录结果和当前灵敏度 `4.0f / 250.0f`。用户实际体验后，如只需快慢调整，仅修改 `MOUSE_SENSITIVITY`，不再引入重复 BLE 报告或阻塞延时。
