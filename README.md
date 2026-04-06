# H743II_OSC_GEN_v1.4

这是一个基于STM32H743II的示波器生成器项目。

## 项目起源

项目由STM32CubeMX生成，初始配置为Keil MDK环境。

## 项目结构

- **proj_bsp**: 存放最底层的驱动
- **proj_level1**: 存放首次封装的函数
- **proj_page**: 存放页面
- **proj_value**: 存放静态值
- **proj_test**: 存放简易测试驱动
- **matlab_copy**: 存放FFT算法仿真

## 构建说明

项目支持多种构建方式：

### 使用CMake（推荐）

使用CMake构建项目。确保安装了必要的工具链。

1. 克隆仓库
2. 进入build目录
3. 运行CMake配置
4. 构建项目

### 使用Keil MDK

原始配置位于MDK-ARM文件夹中。

### 使用Visual Studio

build文件夹包含Visual Studio项目文件。

## 注意事项

请参考DCACHE_ATTENTION.md了解dcache相关配置。