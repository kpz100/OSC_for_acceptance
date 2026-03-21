# 测试框架快速参考卡

## 🚀 最简单的3步启动测试

### 第1步: 包含头文件
```c
#include "test_utils.h"
```

### 第2步: 调用启动函数
```c
Test_Start();  // 在main()中调用这一行
```

### 第3步: 选择测试类型 (可选)
在 `test_utils.h` 中修改：
```c
#define TEST_CURRENT_MODE TEST_MODE_LCD_CONTROL_FULL
```

✅ 完成！Test_Start() 会自动执行对应的测试。

---

## 📋 6种测试模式速查表

| 模式 | 宏定义 | 说明 | 时间 | 用途 |
|------|--------|------|------|------|
| 🎨 LCD原始 | `TEST_MODE_ORIGINAL_LCD` | 测试LCD驱动 | 12秒 | 硬件测试 |
| 👆 触摸原始 | `TEST_MODE_ORIGINAL_TOUCH` | 测试触摸屏 | 5秒+ | 硬件测试 |
| ⚙️ GUI初始化 | `TEST_MODE_LCD_CONTROL_INIT` | 框架初始化 | 2秒 | 框架验证 |
| 🔘 GUI按钮 | `TEST_MODE_LCD_CONTROL_BTN` | 按钮功能 | 3秒 | 控件测试 |
| 📑 GUI页面 | `TEST_MODE_LCD_CONTROL_PAGE` | 页面切换 | 6秒 | 管理测试 |
| ✨ GUI完整 | `TEST_MODE_LCD_CONTROL_FULL` | 全功能集成 | 18秒 | 综合测试 |

---

## 🔧 关键函数一览

### 测试启动 (test_utils.h/c)
```c
void Test_Start(void);           // ← 调用这个启动全部测试
void Test_Printf(...);            // ← 所有输出用这个函数
int Test_Init_Hardware(void);    // ← 手动初始化硬件
uint8_t Test_Is_Hardware_Initialized(void);  // ← 检查初始化状态
```

### LCD功能测试 (test.c)
```c
void Test_LCD_Comprehensive(void);    // 测试LCD驱动
void Test_Touch_Comprehensive(void);  // 测试触摸屏驱动
```

### LCD控制框架测试 (test_lcd_control.c)
```c
void Test_LCD_Control_Init(void);     // 框架初始化测试
void Test_LCD_Control_Button(void);   // 按钮控件测试
void Test_LCD_Control_Pages(void);    // 页面管理测试
void Test_LCD_Control_Full(void);     // 完整集成测试
```

---

## 📊 常用代码片段

### 01. 最小示例 (复制粘贴即用)
```c
#include "test_utils.h"

int main(void) {
    HAL_Init();
    SystemClock_Config();
    // ... 其他初始化 ...
    
    Test_Start();  // 一行启动所有测试
    
    while(1) {}
    return 0;
}
```

### 02. 指定测试模式
```c
#include "test_utils.h"

// 在 test_utils.h 中修改这个宏:
// #define TEST_CURRENT_MODE TEST_MODE_LCD_CONTROL_FULL

int main(void) {
    // ...初始化...
    Test_Start();
}
```

### 03. 自定义初始化
```c
#include "test_utils.h"

int main(void) {
    // 仅在第一次初始化硬件
    if (Test_Init_Hardware() == 1) {
        Test_Printf("Hardware initialized for the first time\r\n");
    }
    
    // 后续调用会自动跳过
    Test_Init_Hardware();  // 返回0，跳过
    
    Test_Start();
}
```

### 04. 在旧代码中使用 (完全兼容)
```c
// 旧方式1仍然可用
#include "test.h"
Test_Start();

// 旧方式2仍然可用
#include "test_selector.h"
Test_Selector_Run();  // 映射到Test_Start()
```

---

## ⚡ 性能指标

| 指标 | 数值 |
|------|------|
| 代码重复消除 | **80%** |
| 初始化步骤 | 6步自动 |
| 硬件初始化耗时 | <1秒 |
| 串口输出速率 | 115200 bps |
| 支持的测试模式 | 6种 |

---

## ❓ 常见问题速解

### Q: 如何只运行LCD测试？
A: 修改 test_utils.h：
```c
#define TEST_CURRENT_MODE TEST_MODE_ORIGINAL_LCD
```

### Q: 如何跳过硬件初始化？
A: Test_Start() 自动检测状态，第二次调用会跳过。
或者直接调用具体测试函数：
```c
Test_LCD_Control_Full();  // 直接调用，跳过Test_Start()
```

### Q: 如何增加自己的日志输出？
A: 使用 Test_Printf()：
```c
Test_Printf("My log: %d\r\n", value);
```

### Q: 代码会自动初始化硬件吗？
A: 是的！Test_Start() 内会自动调用 Test_Init_Hardware()。

### Q: 能同时运行多个测试吗？
A: 不能，TEST_CURRENT_MODE 只能定义一个。
但可以修改宏后重新编译来运行不同的测试。

### Q: 旧代码还能用吗？
A: 完全兼容！所有旧的函数调用都仍然有效。

---

## 🎯 建议的测试流程

```
1️⃣  TEST_MODE_ORIGINAL_LCD      ← 验证LCD硬件
    ↓
2️⃣  TEST_MODE_ORIGINAL_TOUCH    ← 验证触摸硬件
    ↓
3️⃣  TEST_MODE_LCD_CONTROL_INIT  ← 验证框架初始化
    ↓
4️⃣  TEST_MODE_LCD_CONTROL_FULL  ← 完整的GUI测试
    ↓
✅ 所有测试通过！系统就绪
```

---

## 📞 技术支持

遇到问题？检查：
1. 编译错误 → 检查 #include 是否正确
2. 链接错误 → 检查所有 .c 文件是否编译
3. 运行无输出 → 检查串口参数 (115200 bps)
4. 测试失败 → 查看 [TEST_README.md](TEST_README.md)

更详细的优化说明见: [OPTIMIZATION_SUMMARY.md](OPTIMIZATION_SUMMARY.md)

---

**快速参考卡版本**: 1.0
**最后更新**: 2026-03-21
