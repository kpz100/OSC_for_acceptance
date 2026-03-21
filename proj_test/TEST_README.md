# LCD 控制模块测试指南

## 项目结构

```
proj_test/
├── test_utils.h              ✨ 统一的测试工具库 (新增)
├── test_utils.c              ✨ 统一工具库实现 (新增)
│
├── test.h                    # 原始LCD/触摸测试 (精简)
├── test.c                    # 原始LCD/触摸实现 (精简)
│
├── test_lcd_control.h        # LCD控制框架测试头 (精简)
├── test_lcd_control.c        # LCD控制框架实现 (精简)
│
├── test_selector.h           # 测试选择器 (兼容包装)
├── test_selector.c           # 测试选择实现 (简化)
│
├── TEST_README.md            # 本文件
├── OPTIMIZATION_SUMMARY.md   # 优化总结文档 (新增)
└── README (this file)
```

**核心变化**:
- ✨ 新增 `test_utils.h/c` - 统一的测试框架
- 📝 所有其他文件已精简和优化
- 🔄 保持完整的向后兼容性

## 快速开始

### 1. 在main.c中集成测试

在 `main.c` 的 `main()` 函数中添加：

```c
#include "test_utils.h"

int main(void)
{
    // 系统初始化代码...
    
    // 运行相应的测试
    Test_Start();
    
    // 进入主循环
    while(1)
    {
        // 应用代码...
    }
}
```

### 2. 选择测试类型

编辑 `test_utils.h`，修改 `TEST_CURRENT_MODE` 宏：

```c
#define TEST_CURRENT_MODE TEST_MODE_LCD_CONTROL_FULL  // 选择要运行的测试
```

## 可用的测试模块

### TEST_MODE_ORIGINAL_LCD (0)
- **文件**: test.c
- **功能**: 原始LCD驱动测试
- **包含内容**:
  - LCD颜色显示
  - 像素绘制
  - 梯度效果
  - 棋盘图案
  - 填充操作
  - 缓冲翻转

### TEST_MODE_ORIGINAL_TOUCH (1)
- **文件**: test.c
- **功能**: 原始触摸屏驱动测试
- **包含内容**:
  - GT911初始化
  - 触摸扫描
  - I2C通信
  - 多点触摸
  - 连续轮询

### TEST_MODE_LCD_CONTROL_INIT (2)
- **文件**: test_lcd_control.c
- **功能**: LCD控制框架初始化测试
- **测试项**:
  - ✓ LCD_SDRAM_DWT_Init() 初始化
  - ✓ 页面结构体初始化
  - ✓ 内存分配器状态

**预期输出**: "LCD Init Test Successful"

### TEST_MODE_LCD_CONTROL_BTN (3)
- **文件**: test_lcd_control.c
- **功能**: 按钮创建与交互测试
- **测试项**:
  - ✓ 按钮创建 (3个按钮)
  - ✓ 按钮属性设置
  - ✓ 点击回调执行
  - ✓ 视觉状态转换
  - ✓ 点击计数统计

**预期输出**: 
```
Creating Buttons on PAGE_DESKTOP...
All Buttons Created Successfully
Buttons Drawn on LCD
Button 1 Clicked!
Button 2 Clicked!
...
```

### TEST_MODE_LCD_CONTROL_PAGE (4)
- **文件**: test_lcd_control.c
- **功能**: 页面切换管理测试
- **测试项**:
  - ✓ 多页面创建
  - ✓ 页面切换 (GUI_Set_Active_Page)
  - ✓ 重绘标志位机制
  - ✓ 页面隔离验证

**预期输出**:
```
Testing Page Management...
Switched to PAGE_DESKTOP
Switched to PAGE_OSCILLOSCOPE
Switched to PAGE_SIGNAL_GEN
...
Page Switch Test Complete
```

### TEST_MODE_LCD_CONTROL_FULL (5) ⭐ 推荐
- **文件**: test_lcd_control.c
- **功能**: 完整功能集成测试
- **测试项**:
  - ✓ 系统初始化
  - ✓ 多页面控件创建
  - ✓ 页面循环展示 (3个循环)
  - ✓ 按钮交互模拟
  - ✓ 长时间运行稳定性
  - ✓ 统计信息输出

**预期持续时间**: 约18秒 (3个循环 × 3个页面 × 2秒)

**预期输出**:
```
========================================
=== LCD Control Full Integration Test ===
========================================
Step 1: Initializing System...
Step 2: Creating Page Controls...
Step 3: Running Demo Loop...
--- Cycle 1 ---
  PAGE_DESKTOP
  PAGE_OSCILLOSCOPE
  PAGE_SIGNAL_GEN
...
========================================
=== Test Statistics ===
Total Cycles Completed: 3
Total Button Clicks: X
Status: Page switching framework operational
========================================
```

## 测试流程

### 推荐的测试顺序

1. **第一步**: 测试原始驱动
   - 确保LCD和触摸屏硬件正常
   - 运行 `TEST_MODE_ORIGINAL_LCD` 和 `TEST_MODE_ORIGINAL_TOUCH`

2. **第二步**: 测试框架初始化
   - 确保框架可以正确初始化
   - 运行 `TEST_MODE_LCD_CONTROL_INIT`

3. **第三步**: 测试单个功能
   - 按钮: `TEST_MODE_LCD_CONTROL_BTN`
   - 页面: `TEST_MODE_LCD_CONTROL_PAGE`

4. **第四步**: 集成测试
   - 运行 `TEST_MODE_LCD_CONTROL_FULL` 验证整体功能

## 调试技巧

### 查看串口输出

通过USART1查看测试输出 (波特率: 115200 bps)

通过 `Test_Printf()` 输出的所有信息都有统一的格式标记：
- `[INIT]` - 初始化步骤
- `[DONE]` - 步骤完成
- `[INFO]` - 信息提示
- `[ERROR]` - 错误信息
- `[MODE]` - 测试模式
- `[SUCCESS]` - 成功标记

### 快速切换测试

修改 `test_utils.h` 中的一行代码即可切换测试：

```c
// 只需修改这一行
#define TEST_CURRENT_MODE TEST_MODE_LCD_CONTROL_FULL
```

无需修改 main.c，Test_Start() 会自动执行对应的测试。

### 常见问题排查

#### 问题: LCD不显示
- ✓ 检查背光是否打开
- ✓ 检查 `HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1)` 调用
- ✓ 验证LCD初始化代码 (bsp_lcd_single.c)

#### 问题: 触摸不响应
- ✓ 检查I2C初始化
- ✓ 运行原始触摸测试验证硬件
- ✓ 检查GT911复位序列

#### 问题: 页面切换无效
- ✓ 确认 `GUI_Draw_Active_Page()` 在主循环中定期调用
- ✓ 检查页面ID是否有效 (0-2)
- ✓ 验证重绘标志位

#### 问题: 内存不足 (GUI_Malloc 返回NULL)
- ✓ 检查是否过多创建控件
- ✓ 增加 GUI_MEM_POOL_SIZE (在lcd_control.h中)
- ✓ 检查SDRAM是否正确初始化

## API测试用例

### 创建按钮的正确方式

```c
LCD_Button_Struct* btn = GUI_Create_Button(
    PAGE_DESKTOP,           // 页面ID
    100, 100,               // x, y 坐标
    150, 60,                // 宽度, 高度
    "Click Me",             // 按钮文本
    My_Callback_Function    // 回调函数
);

if (btn == NULL) {
    // 内存分配失败
}
```

### 页面切换的正确方式

```c
// 切换都主页面
GUI_Set_Active_Page(PAGE_DESKTOP);

// 在主循环中调用绘制和事件处理
while(1) {
    GUI_Draw_Active_Page();      // 绘制当前页面
    GUI_Process_Touch();         // 处理触摸输入
}
```

## 性能指标

| 测试项 | 预期表现 |
|--------|---------|
| 页面切换时间 | < 100ms |
| 按钮响应时间 | < 50ms |
| 整体稳定性 | 连续运行无崩溃 |
| 内存占用 | < 1MB (10MB池中) |

## 扩展与定制

### 添加新的测试

1. 在 `test_selector.h` 中定义新的 `TEST_MODE_*` 宏
2. 在 `test_selector.c` 中添加对应的处理分支
3. 创建新的测试函数实现

### 自定义GUI控件

编辑 `proj_level1/lcd_control.c`：
- 实现 `GUI_Create_Waveform()` 函数
- 实现 `GUI_Create_Text()` 函数
- 实现 `Draw_Waveform()` 和 `Draw_Text()` 绘制函数

## 注意事项

⚠️ **重要提示**:

1. 测试运行前确保所有硬件初始化完毕
2. LCD_SDRAM_DWT_Init() 应在 main() 中调用一次
3. GUI_Draw_Active_Page() 应在主循环中定期调用
4. 不支持中断上下文中的GUI操作
5. SDRAM内存池大小固定为10MB，不能动态调整

## 文件对应关系

```
main.c (包含此处)
    └─ #include "test_utils.h"      ← 统一入口
         ├─ Test_Start()            ← 统一启动函数
         ├─ Test_Printf()           ← 统一输出函数
         ├─ Test_Init_Hardware()    ← 统一初始化函数
         └─ TEST_CURRENT_MODE       ← 统一配置宏

// 调用各具体测试模块
Test_Start() 
    └─ 根据 TEST_CURRENT_MODE 选择:
        ├─ test.c: Test_LCD_Comprehensive() / Test_Touch_Comprehensive()
        └─ test_lcd_control.c: Test_LCD_Control_*()
```

**向后兼容路由**:
- 旧代码 `#include "test.h"` → 自动映射到 `test_utils.h`
- 旧代码 `#include "test_selector.h"` → 自动映射到 `test_utils.h`

## 支持与反馈

如有问题，请查看：
- LCD驱动: `bsp_lcd_single.h/c`
- 触摸驱动: `bsp_touch.h/c`
- SDRAM驱动: `bsp_sdram.h/c`
- 定时器配置: `tim.h/c`

---

**最后更新**: 2026-03-21

## 📖 详细文档

- 💾 **代码优化说明**: 参见 [OPTIMIZATION_SUMMARY.md](OPTIMIZATION_SUMMARY.md)
  - 代码重复消除：140行 → 30行
  - 接口统一：3个入口 → 1个入口
  - 宏定义统一：3套 → 1套
