# 正式固件目录整理设计

## 目标

将仓库中的两套候选固件收敛为一套名称明确、可直接使用 PlatformIO 构建的正式固件，同时保留开发示例和项目图片。

## 最终目录结构

```text
cainiao_gun/
├── firmware/          # 唯一正式固件，来源于 new_pro_delay10ms
│   ├── .gitignore
│   ├── platformio.ini
│   └── src/
│       ├── main.cpp
│       └── combo/
├── examples/          # 历史实验和硬件验证工程，不作为正式固件
├── images/            # README 使用的项目图片
├── README.md
└── LICENSE
```

## 整理规则

1. 将 `new_pro_delay10ms/` 重命名为 `firmware/`，不修改其中的鼠标控制行为。
2. 直接删除旧的 `software/` 及其全部内容，不建立归档副本。
3. 保留 `examples/` 和 `images/` 的现有内容与目录结构。
4. 从正式固件中删除 PlatformIO 模板空壳：`.vscode/`、`include/README`、`lib/README` 和 `test/README`；空目录不保留。
5. 完整保留 `firmware/src/combo/`。其中的源码、元数据、说明和示例属于当前内置 BLE Combo 实现，本次整理不拆分该库。
6. 保留 `firmware/.gitignore`、`firmware/platformio.ini` 和全部实际源码。
7. 更新根 `README.md`，把正式源码入口从旧目录统一指向 `firmware/`，说明 `examples/` 仅用于历史实验与硬件验证，并给出 PlatformIO 构建命令。

## 构建入口

正式固件的唯一构建入口为：

```powershell
pio run -d firmware
```

示例目录可以继续各自构建，但不属于正式发布固件，也不作为本次验收对象。

## 验收标准

- 仓库根目录不存在 `software/` 和 `new_pro_delay10ms/`。
- 仓库根目录存在且仅存在一个正式固件目录 `firmware/`。
- `firmware/platformio.ini` 和 `firmware/src/main.cpp` 存在。
- `firmware/` 中不存在上述 PlatformIO 模板空壳。
- README 对正式固件入口、示例目录和构建命令的描述与实际结构一致。
- `pio run -d firmware` 成功完成；如果本机缺少 PlatformIO 或依赖下载受网络限制，应明确记录环境阻塞及已完成的静态检查，不把环境问题误报为源码失败。

## 非目标

- 不优化鼠标移动算法、BLE 连接参数或 MPU6050 数据处理。
- 不重构 `main.cpp` 或 `src/combo/`。
- 不清理、合并或重新命名 `examples/` 下的实验工程。
- 不修改硬件引脚、依赖版本或构建目标。
