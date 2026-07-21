# 鼠标轴向校准实施计划

> **供智能代理执行：** 必须使用 `superpowers:executing-plans`，逐项执行本计划。每一步使用复选框跟踪。

**目标：** 修正垂直鼠标方向，并在保持水平手感不变的前提下把垂直灵敏度提高到两倍。

**架构：** `MouseMotionFilter` 负责把 MPU 原始轴数据统一转换成 HID 位移，因此在该类中引入独立的 X/Y 灵敏度。主程序只传入校准常量并发送计算结果，不在 BLE 调用处增加额外方向或倍率逻辑。

**技术栈：** ESP32、Arduino、PlatformIO、Unity、MPU6050、BLE HID。

## 全局约束

- 水平输出固定为 `-gz * (4.0f / 250.0f)`。
- 垂直输出固定为 `-gy * (8.0f / 250.0f)`。
- 不修改 BLE 上报节奏、MPU/DMP 配置、按键行为和水平灵敏度。
- 使用现有全局 PlatformIO：`C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe`。
- 不下载或安装额外环境；如果现有依赖缺失，先征得用户同意。

---

### 任务 1：用失败测试定义独立轴校准行为

**文件：**
- 修改：`firmware/test/test_mouse_motion/test_main.cpp`

**接口：**
- 使用：`MouseMotionFilter(float sensitivityX, float sensitivityY)`
- 验证：`MouseDelta update(int16_t gy, int16_t gz)` 返回独立缩放且 Y 反向的 HID 位移。

- [ ] **步骤 1：把现有构造调用改成双参数，并定义方向与倍率测试**

```cpp
void test_axes_use_independent_gain_and_device_direction()
{
    MouseMotionFilter filter(4.0f / 250.0f, 8.0f / 250.0f);
    const MouseDelta delta = filter.update(250, 250);
    TEST_ASSERT_EQUAL_INT8(-4, delta.x);
    TEST_ASSERT_EQUAL_INT8(-8, delta.y);
}
```

其他测试使用相同的 X/Y 参数，例如 `MouseMotionFilter filter(0.01f, 0.01f);`，并把限幅测试的 Y 预期改为 `-127`。在 `setup()` 中运行新测试名。

- [ ] **步骤 2：运行板载测试，确认当前实现按预期失败**

运行：

```powershell
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' test -d 'C:\Users\Cainiao\Documents\飞鼠优化\repo\firmware' -e esp32dev --upload-port COM7
```

预期：编译失败，错误指出 `MouseMotionFilter` 不接受两个灵敏度参数。该失败证明测试覆盖了尚未实现的新接口。

### 任务 2：实现独立轴增益并接入正式固件

**文件：**
- 修改：`firmware/include/MouseMotionFilter.h`
- 修改：`firmware/src/main.cpp`
- 测试：`firmware/test/test_mouse_motion/test_main.cpp`

**接口：**
- 产生：`MouseMotionFilter(float sensitivityX, float sensitivityY)`
- 保持：`MouseDelta update(int16_t gy, int16_t gz)` 和 `void reset()`。

- [ ] **步骤 1：实现最小的双灵敏度接口和 Y 方向修正**

```cpp
MouseMotionFilter(float sensitivityX, float sensitivityY)
    : sensitivityX_(sensitivityX), sensitivityY_(sensitivityY),
      residualX_(0.0f), residualY_(0.0f)
{
}

MouseDelta update(int16_t gy, int16_t gz)
{
    residualX_ += -static_cast<float>(gz) * sensitivityX_;
    residualY_ += -static_cast<float>(gy) * sensitivityY_;
    // 其余取整、余量回存和 HID 限幅逻辑保持原样。
}
```

成员字段替换为：

```cpp
float sensitivityX_;
float sensitivityY_;
float residualX_;
float residualY_;
```

- [ ] **步骤 2：在主程序中设置独立校准常量**

```cpp
constexpr float MOUSE_SENSITIVITY_X = 4.0f / 250.0f;
constexpr float MOUSE_SENSITIVITY_Y = 8.0f / 250.0f;
MouseMotionFilter mouseMotion(MOUSE_SENSITIVITY_X, MOUSE_SENSITIVITY_Y);
```

- [ ] **步骤 3：重新运行板载测试并确认全部通过**

运行与任务 1 相同的 `pio test` 命令。预期：Unity 报告全部测试通过，失败数为 0。

- [ ] **步骤 4：编译正式固件**

```powershell
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' run -d 'C:\Users\Cainiao\Documents\飞鼠优化\repo\firmware'
```

预期：命令退出码为 0，并生成 `firmware/.pio/build/esp32dev/firmware.bin`。

- [ ] **步骤 5：烧录正式固件到 COM7**

```powershell
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' run -d 'C:\Users\Cainiao\Documents\飞鼠优化\repo\firmware' -t upload --upload-port COM7
```

预期：esptool 完成所有分段写入和校验，并对 ESP32 执行硬复位。

- [ ] **步骤 6：检查差异并提交**

```powershell
git diff --check
git status --short
git add firmware/include/MouseMotionFilter.h firmware/src/main.cpp firmware/test/test_mouse_motion/test_main.cpp
git commit -m 'fix: calibrate vertical mouse motion'
```

预期：差异只包含轴向测试、滤波器双参数和主程序校准常量；提交成功。
