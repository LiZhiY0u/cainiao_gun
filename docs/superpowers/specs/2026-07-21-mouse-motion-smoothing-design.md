# 鼠标移动流畅度优化设计

## 目标

在不更换 BLE 库、不改变硬件引脚和按键功能的前提下，消除体感鼠标移动中的周期性卡顿、轻微延迟和不连续感，并通过自动化测试、PlatformIO 编译和 COM7 实机烧录验证。

## 已确认根因

当前运动链路包含以下会叠加产生不连续移动的问题：

1. 每次取得 MPU 数据后固定 `delay(9)`，发送鼠标前再固定 `delay(3)`，单轮至少阻塞约 12 ms。
2. 每轮连续执行 4 次 `Mouse.move()`，产生 BLE notification 突发，而不是均匀报告。
3. `gz / 250` 和 `gy / 250` 在写入整数变量前已经截断，小幅运动直接丢失，跨过阈值后突然跳变。
4. 即使没有取得新的 DMP 数据，主循环仍使用旧的陀螺仪值继续发送。
5. 每轮通过动态 `String` 拼接输出串口日志，引入额外调度和内存抖动。

## 方案选择

采用保守链路优化：保留 MPU6050 DMP、现有 ESP32 BLE Combo、设备名称、配对方式和硬件功能，只重构传感器数据到 HID 相对位移之间的实时路径。

本轮不迁移 NimBLE、不调整 BLE 连接参数，也不改用完全不同的姿态算法。若实机验证后仍有明显周期性卡顿，再把 BLE 连接间隔优化作为第二阶段。

## 组件设计

### 纯 C++ 位移转换器

新增 `firmware/include/MouseMotionFilter.h`，提供不依赖 Arduino 和 BLE 的状态型转换器：

- 输入：本次 MPU 原始陀螺仪 `gy`、`gz`。
- 输出：一个有符号 8 位 X/Y HID 相对位移。
- X 轴保持现有方向：`-gz`。
- Y 轴保持当前最终 HID 方向：`+gy`。
- 使用浮点残差累积不足一个 HID 单位的运动，慢速连续运动不会丢失。
- 输出限制在 `-127～127`；超过 HID 单包范围的部分不形成后续积压。
- 初始灵敏度设为 `4.0 / 250.0`，近似保留旧代码每个样本连续发送四次的总体速度。
- 提供 `reset()`，用于断开连接时清空残差，避免重连后产生陈旧位移。

### MPU 数据读取

将 `mpu_DMP()` 改为返回 `bool`：

- DMP 未就绪或没有新 FIFO 数据时返回 `false`。
- 取得新数据并完成一次 `getMotion6()` 后返回 `true`。
- 删除函数内的 `delay(9)`。

主循环只有在 `mpu_DMP()` 返回 `true` 时才进行位移转换和 HID 报告，杜绝重复发送旧样本。

### BLE 鼠标报告

每个新传感器样本最多执行一次 `Mouse.move(dx, dy)`：

- 删除发送前的 `delay(3)`。
- 删除四次重复发送循环。
- X/Y 都为 0 时不发送报告。
- 不修改 `BleComboMouse` HID 描述符、按键状态或 notification 实现。

### 调试输出

逐帧 `_print()` 从实时路径移除。保留编译期调试开关，开启后最多每 500 ms 使用 `Serial.printf()` 输出一次陀螺仪和 HID 位移，不使用动态 `String` 拼接。默认关闭。

## 数据流

```text
MPU6050 DMP 新数据
  -> getMotion6(gy, gz)
  -> MouseMotionFilter 残差累计与量化
  -> 限制到 HID -127..127
  -> 非零时发送一次 BLE Mouse.move
```

## 测试与验证

### 自动化测试

在 `firmware/test/test_mouse_motion/test_main.cpp` 中对纯 C++ 转换器执行 PlatformIO 嵌入式 Unity 测试：

- 多个不足一单位的输入可以累计成连续位移。
- X/Y 方向与现有最终鼠标方向一致。
- 正负残差均能正确累计。
- 大输入限制为 `-127～127`。
- `reset()` 清空未发送残差。

测试固件通过现有 ESP32 工具链烧录到 CH340 `COM7`；测试完成后必须再烧录正式固件。

### 正式构建和烧录

使用已安装的 VS Code PlatformIO 环境：

```powershell
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' run -d firmware
& 'C:\Users\Cainiao\.platformio\penv\Scripts\pio.exe' run -d firmware -t upload --upload-port COM7
```

### 验收

- 单元测试全部通过。
- 正式固件编译成功并烧录到 COM7。
- 正式构建 RAM/Flash 不超过当前 ESP32 分区容量。
- 鼠标运动链路不存在活动的 `delay(9)`、`delay(3)` 或四次重复 `Mouse.move()`。
- Git 工作区不包含 `.pio/`、`.vscode/` 或其他生成文件。
- 实机主观验收由用户完成：移动应比旧固件连续，且没有明显速度下降；若速度不合适，只调整单一灵敏度常量后重新烧录。

## 非目标

- 不改变 BLE 设备名和配对信息。
- 不迁移到 NimBLE。
- 不调整 BLE 连接间隔。
- 不重构按键、RGB、电磁后坐或摇杆逻辑。
- 不改变 MPU6050 偏移、硬件引脚和 PlatformIO 依赖版本。
