# SecureVision

一个基于 RV1126 芯片的智能监控系统，支持多种视频源接入、实时监控、异常检测和音视频篡改检测等功能。

## 项目概述

SecureVision 是一个专为嵌入式 ARM 平台（特别是 RV1126）设计的安全监控解决方案。该系统集成了多种视频采集方式，提供实时监控、智能检测和用户友好的界面。

### 主要特性

- **多源视频接入**：支持 MIPI 摄像头、RTSP 网络摄像头、USB 摄像头
- **实时视频流**：基于 RKMedia 的高性能视频处理
- **智能检测**：音视频篡改检测功能
- **跨平台界面**：基于 Qt5 的现代化用户界面
- **嵌入式优化**：针对 ARM 平台的性能优化

## 系统架构

```
SecureVision/
├── main/               # 主程序入口
├── ui/                 # 用户界面模块
│   ├── MonitorListPage     # 监控设备列表
│   ├── ShowMonitorPage     # 实时监控显示
│   ├── DetectionListPage   # 检测功能列表
│   ├── VideoDetectionPage  # 视频检测页面
│   └── AudioDetectionPage  # 音频检测页面
├── capture/            # 视频采集模块
│   ├── CaptureThread       # MIPI 摄像头采集
│   ├── RtspThread         # RTSP 流采集
│   └── USBCaptureThread   # USB 摄像头采集
├── core/               # 核心业务逻辑
├── widgets/            # 自定义 UI 组件
└── Resource/           # 资源文件
```

## 硬件要求

### 核心平台
- **SoC**: RV1126 (ARM Cortex-A7)
- **内存**: 至少 512MB RAM
- **存储**: 至少 1GB 可用空间

### 摄像头支持
- **MIPI CSI**: 支持 720x1280 分辨率
- **USB 摄像头**: 标准 UVC 设备（/dev/video45）
- **网络摄像头**: 支持 RTSP 协议的 IP 摄像头

### 显示设备
- 支持 1280x720 分辨率的显示屏
- 支持屏幕旋转（270° 逆时针）

## 软件依赖

### 系统依赖
- **操作系统**: Linux (Buildroot)
- **工具链**: arm-linux-gnueabihf-gcc
- **内核**: 支持 V4L2 的 Linux 内核

### 第三方库
- **Qt5**: 图形界面框架
  - Qt5Core, Qt5Gui, Qt5Widgets
  - Qt5Network, Qt5Multimedia
- **FFmpeg 4.1.3**: 视频编解码
  - libavformat, libavcodec, libavutil, libswscale
- **OpenCV 3.4.12**: 图像处理
- **RKMedia**: 瑞芯微媒体框架
  - easymedia, third_media, rockchip_mpp

## 编译构建

### 环境准备

1. **安装交叉编译工具链**
```bash
# 确保工具链路径正确
export TOOLCHAIN_DIR=/opt/atk-dlrv1126-toolchain
export PATH=$TOOLCHAIN_DIR/usr/bin:$PATH
```

2. **设置构建环境**
```bash
# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-linux.cmake ..
```

### 编译步骤

```bash
# 编译项目
make -j$(nproc)

# 安装到目标目录
make install
```

### CMake 配置说明

项目使用 CMake 构建系统，主要配置包括：

- **交叉编译设置**: 自动配置 ARM 工具链
- **库路径配置**: 自动查找 Qt5、OpenCV、FFmpeg
- **资源文件处理**: 自动生成 Qt 资源文件
- **安装规则**: 配置运行时库路径

## 使用说明

### 启动应用

```bash
# 在目标设备上运行
./SecureVision
```

### 功能模块

#### 1. 监控模块
- **设备管理**: 添加、配置多种类型的摄像头
- **实时预览**: 支持多路视频流同时显示
- **设备切换**: 一键切换不同摄像头源

#### 2. 异常信息
- **事件记录**: 记录系统异常和检测结果
- **历史查询**: 查看历史异常信息
- **状态监控**: 实时系统状态展示

#### 3. 篡改检测
- **视频检测**: 检测视频内容的篡改痕迹
- **音频检测**: 分析音频信号的异常特征
- **智能分析**: 基于算法的深度检测

### 界面操作

#### 主界面布局
- **左侧导航栏**: 功能模块切换
- **右侧内容区**: 对应功能页面
- **全屏模式**: 检测页面支持全屏显示

#### 设备配置
```cpp
// 添加 MIPI 摄像头
m_deviceManager.addDevice("MIPI 01");

// 添加 RTSP 摄像头
m_deviceManager.addDevice("IP 01", 
    QUrl("rtsp://admin:password@192.168.1.100:554/stream"));

// 添加 USB 摄像头
m_deviceManager.addUSBDevice("USB", "/dev/video45");
```

## 开发指南

### 代码结构

#### 主要类说明
- **SecureVision**: 主窗口类，管理整体界面布局
- **MonitorListPage**: 监控设备列表管理
- **ShowMonitorPage**: 实时监控显示页面
- **CaptureThread**: MIPI 摄像头采集线程
- **RtspThread**: RTSP 流采集线程
- **USBCaptureThread**: USB 摄像头采集线程

#### 线程管理
```cpp
// 启动 MIPI 采集线程
mipiThread = new CaptureThread(this);
mipiThread->setThreadStart(true);
mipiThread->start();

// 启动 RTSP 采集线程
rtspThread1 = new RtspThread(this);
rtspThread1->setRtspUrl("rtsp://...");
rtspThread1->setThreadStart(true);
rtspThread1->start();
```

#### 信号槽连接
```cpp
// 连接视频流信号
connect(captureThread, &CaptureThread::resultReady, 
        this, [=](QImage image) {
    videoLabel->setPixmap(QPixmap::fromImage(image)
        .scaled(videoLabel->size(), Qt::KeepAspectRatio));
});
```

### 扩展开发

#### 添加新的摄像头类型
1. 继承基础采集线程类
2. 实现特定的采集逻辑
3. 在设备管理器中注册
4. 更新 UI 显示逻辑

#### 添加新的检测算法
1. 在 core 模块中实现算法
2. 创建对应的 UI 页面
3. 集成到检测列表中
4. 配置信号槽连接

## 故障排除

### 常见问题

#### 编译错误
```bash
# 检查工具链路径
echo $TOOLCHAIN_DIR

# 验证库文件存在
ls $SYSROOT_PATH/usr/lib/libQt5*
```

#### 运行时错误
```bash
# 检查库文件路径
export LD_LIBRARY_PATH=/oem/usr/lib:$LD_LIBRARY_PATH

# 检查设备权限
ls -l /dev/video*
```

#### MIPI 摄像头无法工作
```bash
# 检查摄像头驱动
dmesg | grep -i camera

# 验证设备节点
v4l2-ctl --list-devices
```

#### RTSP 连接失败
```bash
# 测试网络连通性
ping 192.168.1.100

# 使用 FFmpeg 测试 RTSP 流
ffplay rtsp://admin:password@192.168.1.100:554/stream
```

### 调试技巧

#### 启用调试输出
```cpp
// 在代码中添加调试信息
qDebug() << "Thread started successfully";
```

#### 性能监控
```cpp
// 监控线程性能
QTime timer;
timer.start();
// ... 执行操作 ...
qDebug() << "Operation took:" << timer.elapsed() << "ms";
```

## 性能优化

### 内存优化
- 合理设置缓冲区大小
- 及时释放不用的资源
- 使用智能指针管理内存

### CPU 优化
- 多线程并行处理
- 避免在主线程进行重计算
- 使用硬件加速功能

### 功耗优化
- 动态调整帧率
- 按需启动线程
- 合理配置休眠策略

## 版本历史

### v1.0.0 (当前版本)
- 基础监控功能
- 多路视频接入
- Qt5 用户界面

### 后续规划
- [ ] 增加 AI 智能检测
- [ ] 支持云端数据同步
- [ ] 移动端 APP 控制
- [ ] 更多摄像头协议支持


---

**注意**: 本项目专为 RV1126 平台优化，在其他平台上可能需要适当修改。
