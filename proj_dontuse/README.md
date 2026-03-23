# 测试框架 (Test Framework)

完整统一的LCD驱动和GUI框架测试模块。

## 🚀 快速开始 (3步)

### 第1步：包含头文件
```c
#include "test_all.h"
```

### 第2步：调用启动函数
```c
int main(void) {
    // 系统初始化...
    
    Test_Start();  // 启动测试框架
    
    while(1) { }
}
```

### 第3步：选择测试模式 (可选)
编辑 `test_all.c` 中的配置：
```c
#define TEST_CURRENT_MODE  TEST_MODE_LCD_CONTROL_FULL
```

✅ 完成！不需要其他配置。

---

## 📋 6种测试模式

| 模式 | 宏定义 | 说明 | 测试时间 |
|------|--------|------|---------|
| 🎨 LCD原始 | `TEST_MODE_ORIGINAL_LCD` | LCD驱动功能测试 | ~12秒 |
| 👆 触摸原始 | `TEST_MODE_ORIGINAL_TOUCH` | 触摸屏驱动测试 | ~5秒 |
| ⚙️ GUI初始化 | `TEST_MODE_LCD_CONTROL_INIT` | GUI框架初始化 | ~2秒 |
| 🔘 GUI按钮 | `TEST_MODE_LCD_CONTROL_BTN` | 按钮控件测试 | ~3秒 |
| 📑 GUI页面 | `TEST_MODE_LCD_CONTROL_PAGE` | 页面管理测试 | ~6秒 |
| ✨ GUI完整 | `TEST_MODE_LCD_CONTROL_FULL` | 完整集成测试 | ~18秒 |

---

## 📂 核心API

### 启动与初始化
```c
void Test_Start(void);                      // 启动测试框架
int Test_Init_Hardware(void);               // 硬件初始化 (自动调用)
uint8_t Test_Is_Hardware_Initialized(void); // 检查初始化状态
```

### 输出与格式化
```c
void Test_Printf(const char *format, ...);  // UART调试输出
void Test_Print_Title(const char *title);   // 打印测试标题
void Test_Print_Separator(uint8_t level);   // 打印分隔符
```

### LCD驱动测试
```c
void Test_LCD_Comprehensive(void);          // LCD综合测试
void Test_Touch_Comprehensive(void);        // 触摸屏综合测试
```

### GUI框架测试
```c
void Test_LCD_Control_Init(void);           // GUI框架初始化测试
void Test_LCD_Control_Button(void);         // GUI按钮控件测试
void Test_LCD_Control_Pages(void);          // GUI页面切换测试
void Test_LCD_Control_Full(void);           // GUI完整集成测试
```

---

## 🧪 各模式测试内容

### TEST_MODE_ORIGINAL_LCD
测试LCD驱动的基础功能：
- ✓ 颜色显示（9种颜色）
- ✓ 像素绘制（对角线、竖线、横线）
- ✓ 梯度效果（红/绿/蓝/白色梯度）
- ✓ 棋盘图案
- ✓ 全屏填充（6种颜色循环）
- ✓ 缓冲翻转
- 循环100次

### TEST_MODE_ORIGINAL_TOUCH
测试GT911触摸屏驱动：
- ✓ GT911初始化和复位
- ✓ I2C配置
- ✓ 单次触摸扫描
- ✓ I2C读写通信
- ✓ 连续触摸轮询（5秒）
- ✓ 多点触摸能力检测（5秒）

### TEST_MODE_LCD_CONTROL_INIT
验证GUI框架初始化：
- ✓ 背光PWM启动
- ✓ LCD_SDRAM_DWT_Init()完成
- ✓ 内存分配器状态
- ✓ 显示初始化成功画面

### TEST_MODE_LCD_CONTROL_BTN
测试按钮控件：
- ✓ 创建3个按钮
- ✓ 设置按钮属性（位置、大小、颜色、回调）
- ✓ 验证按钮创建成功
- ✓ 绘制按钮到LCD
- ✓ 统计点击次数

### TEST_MODE_LCD_CONTROL_PAGE
测试页面管理：
- ✓ 创建3个页面（DESKTOP, OSCILLOSCOPE, SIGNAL_GEN）
- ✓ 为各页面创建控件
- ✓ 页面切换2个循环
- ✓ 验证重绘机制

### TEST_MODE_LCD_CONTROL_FULL
完整GUI集成测试：
- ✓ GUI框架初始化
- ✓ 创建多页面控件
- ✓ 3个循环演示（每页2秒，共18秒）
- ✓ 页面之间的切换
- ✓ 注册按钮回调
- ✓ 输出测试统计

---

## 💻 代码示例

### 最小示例
```c
#include "test_all.h"

int main(void) {
    HAL_Init();
    SystemClock_Config();
    // ... 其他初始化 ...
    
    // 运行测试
    Test_Start();
    
    while(1) {
        // 应用代码
    }
}
```

### 自定义测试流程
```c
#include "test_all.h"

int main(void) {
    // 初始化硬件
    Test_Init_Hardware();
    
    // 运行特定测试
    Test_Print_Title("Custom Test");
    Test_LCD_Control_Full();
    
    while(1) { }
}
```

### 检查硬件状态
```c
#include "test_all.h"

if (Test_Is_Hardware_Initialized()) {
    Test_Printf("Hardware is ready\r\n");
} else {
    Test_Init_Hardware();
}
```

---

## 🔧 依赖与配置

### 必需的头文件
```c
#include "main.h"              // HAL及系统定义
#include "lcd_control.h"       // GUI框架
#include "bsp_lcd_single.h"    // LCD驱动
#include "bsp_touch.h"         // 触摸屏驱动
#include "bsp_dwt.h"           // DWT计时器
#include "usart.h"             // UART输出
```

### 配置选项
在 `test_all.c` 中修改：

```c
// 选择测试模式 (6项可选)
#define TEST_CURRENT_MODE  TEST_MODE_LCD_CONTROL_FULL
```

### UART配置
- **波特率**: 115200 bps
- **数据位**: 8
- **停止位**: 1
- **奇偶性**: 无
- **输出到**: USART1

---

## 🎯 使用建议

✅ **推荐流程**：
1. 在main.c中调用 `Test_Start()`
2. 修改 `test_all.c` 中的 `TEST_CURRENT_MODE` 选择测试
3. 通过串口监控查看测试输出
4. 根据输出诊断问题

✅ **调试技巧**：
- 使用 `Test_Printf()` 替代 `printf()` 输出调试信息
- 调用 `Test_Init_Hardware()` 检查硬件初始化
- 每个测试函数都有独立的标题和统计

✅ **性能参考**：
- LCD综合测试：~12秒/循环
- 触摸综合测试：~10秒
- GUI完整测试：~18秒

---

## 📝 文件结构

```
proj_test/
├── test_all.h              ← 测试框架头文件 (包含所有声明)
├── test_all.c              ← 测试框架实现 (包含所有代码)
└── README.md               ← 本文档
```

**注**：此框架为自包含模块，仅需包含 `test_all.h`，无需其他测试文件。

---

## 📞 故障排查

### 问题：编译错误 "undefined reference"
**原因**: test_all.c 未被链接到项目  
**解决**: 确保 test_all.c 在project中编译列表中

### 问题：无测试输出
**原因**: UART未初始化或TEST_CURRENT_MODE不匹配  
**解决**: 确保main.c中已调用系统初始化，检查串口配置

### 问题：测试卡在某处
**原因**: 硬件初始化失败（如背光PWM未启动）  
**解决**: 检查HAL初始化，运行 `TEST_MODE_ORIGINAL_LCD` 诊断

---

**最后更新**: 2026年3月  
**框架版本**: 2.0 (Unified Test_all)
