# NVIDIA Image Generation Integration Guide

## 概述

本文档介绍如何将 NVIDIA AI 图像生成功能集成到《云海山庄》游戏中。

## 已创建的文件

### 核心组件

1. **ImageGenerationManager** - 图像生成管理器
   - 头文件: `include/CloudSeamanor/engine/ImageGenerationManager.hpp`
   - 实现文件: `src/engine/ImageGenerationManager.cpp`
   - 功能: 管理图像生成请求、缓存、异步处理

2. **PixelImageGeneratorPanel** - UI 面板
   - 头文件: `include/CloudSeamanor/engine/PixelImageGeneratorPanel.hpp`
   - 实现文件: `src/engine/PixelImageGeneratorPanel.cpp`
   - 功能: 提供游戏内图像生成界面

3. **PromptTemplateLibrary** - Prompt 模板库
   - 头文件: `include/CloudSeamanor/prompt_templates/PromptTemplateLibrary.hpp`
   - 功能: 预定义角色立绘、游戏场景、物品图标的 prompt 模板

### 配置文件

- `configs/nvidia_image_generation.cfg` - 图像生成配置文件

### 示例代码

- `examples/NvidiaImageIntegrationExamples.cpp` - 基础使用示例
- `examples/GameImageGenerationIntegration.cpp` - 游戏集成示例

## 快速开始

### 1. 安装依赖

**Windows:**
```bash
# 安装 vcpkg (如果还没有)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装依赖
.\vcpkg install curl:x64-windows
.\vcpkg install jsoncpp:x64-windows
```

**Linux:**
```bash
sudo apt-get install libcurl4-openssl-dev libjsoncpp-dev
```

### 2. 配置 API Key

编辑 `configs/nvidia_image_generation.cfg`:
```
nvidia_api_key="nvapi-your-actual-api-key-here"
```

### 3. 构建项目

```bash
cmake -DBUILD_NVIDIA_EXAMPLES=ON ..
cmake --build .
```

## 使用示例

### 基础使用

```cpp
#include "CloudSeamanor/engine/ImageGenerationManager.hpp"

// 初始化
ImageGenerationManager::Instance().Initialize("nvapi-xxxxx");

// 生成角色立绘
std::string request_id = ImageGenerationManager::Instance().SubmitCharacterPortraitRequest(
    "阿茶", "武侠仙侠风格"
);

// 检查状态
auto status = ImageGenerationManager::Instance().GetStatus(request_id);

// 获取纹理
auto texture = ImageGenerationManager::Instance().GetTexture(request_id);

// 保存图像
ImageGenerationManager::Instance().SaveImageToFile(request_id, "output.png");
```

### 在游戏中集成

```cpp
// 在 GameApp 初始化时
void GameApp::InitializeImageGeneration() {
    GameConfig config;
    config.LoadFromFile("configs/nvidia_image_generation.cfg");
    std::string api_key = config.GetString("nvidia_api_key", "");
    ImageGenerationManager::Instance().Initialize(api_key);
}

// 在游戏循环中更新
void GameApp::Update(float delta) {
    ImageGenerationManager::Instance().ProcessAsyncResults();
    // ... 其他更新逻辑
}

// 在清理时关闭
void GameApp::Shutdown() {
    ImageGenerationManager::Instance().Shutdown();
}
```

### 使用 UI 面板

```cpp
// 创建面板
PixelImageGeneratorPanel panel(sf::FloatRect({100, 50}, {1000, 700}));
panel.SetApiKey("nvapi-xxxxx");
panel.SetVisible(true);

// 在游戏循环中更新和渲染
panel.Update(delta_seconds);
window.draw(panel);

// 开始生成
panel.StartGeneration("portrait_wuxia");

// 保存图像
panel.SaveCurrentImage("output.png");
```

## 可用的 Prompt 模板

### 角色立绘模板
- `武侠仙侠风格` - 精致汉服女性角色
- `活泼可爱风格` - 彩色传统服饰
- `冷艳高傲风格` - 冷艳女性角色
- `男性剑客风格` - 武侠男性角色
- `儿童角色风格` - 可爱儿童角色

### 游戏场景模板
- `茶园场景` - 云海茶园
- `主屋大厅场景` - 云海山庄主建筑
- `温泉场景` - 仙气缭绕的温泉
- `灵兽园场景` - 仙兽栖息之地
- `观云台日落场景` - 壮观云海景观
- `工坊场景` - 温馨工坊
- `客栈场景` - 热闹客栈

### 物品图标模板
- `灵茶图标` - 仙气缭绕的灵茶
- `装备图标` - 中国风宝剑
- `药材图标` - 珍贵草药

## 性能优化建议

1. **异步生成**: 使用异步模式避免阻塞游戏主线程
2. **缓存管理**: 设置合理的缓存大小（默认100张）
3. **批量生成**: 支持批量生成多个图像
4. **预加载**: 在游戏启动时预生成常用图像

## 注意事项

1. **API Key 安全**: 不要将 API Key 提交到版本控制系统
2. **网络连接**: 需要稳定的网络连接到 NVIDIA API
3. **生成时间**: 单张图像生成约需 30-60 秒
4. **费用控制**: 注意 NVIDIA API 的使用费用

## 故障排除

### 常见问题

1. **编译错误: 找不到 curl/jsoncpp**
   - 确保已安装依赖库
   - 检查 CMake 配置路径

2. **API 调用失败**
   - 检查 API Key 是否正确
   - 检查网络连接
   - 查看 NVIDIA API 服务状态

3. **图像生成超时**
   - 增加 `default_timeout_seconds` 配置
   - 检查网络延迟

## 更多资源

- [NVIDIA NIM 文档](https://docs.nvidia.com/nim/)
- [Stable Diffusion API 参考](https://docs.api.nvidia.com/nim/reference/stabilityai-stable-diffusion-3-medium-infer)
- 项目示例代码: `examples/` 目录

## 更新日志

- 2026-05-02: 初始版本，完成基础集成功能