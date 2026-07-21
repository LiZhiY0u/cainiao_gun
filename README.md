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
