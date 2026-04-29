# FileMetaCleaner

一个简单的 macOS Qt Widgets 桌面应用：拖入文件后展示可识别的文件元信息，勾选需要清除的条目，点击“清除”即可移除所选元信息。

## 功能

- 拖拽本地文件到应用窗口
- 使用内置 ExifTool 读取/清除常见文件内部元数据，例如图片 EXIF、IPTC、XMP、PDF/Office 部分元信息等
- 使用 macOS 系统自带 `xattr` 读取/删除扩展属性，例如 `com.apple.quarantine`、`com.apple.metadata:*`
- 文件名、大小、路径、MIMEType 等文件系统属性会展示为只读，不支持作为普通元数据删除
- 打包后的 `.app` 会优先调用 `Contents/Resources/exiftool/exiftool`，最终用户无需安装 Homebrew 或 ExifTool
- 已内置应用图标：`resources/AppIcon.icns` 用于 macOS App 图标，`resources/resources.qrc` 用于 Qt 窗口图标

## 目录结构

```text
FileMetaCleaner/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── MainWindow.h
│   └── MainWindow.cpp
├── scripts/
│   ├── prepare_exiftool.sh
│   └── build_macos_dmg.sh
└── third_party/
    └── exiftool/
```

## 开发环境要求

构建机器需要：

- macOS
- Qt 6.x
- CMake 3.21+
- Xcode Command Line Tools
- curl、tar、perl，macOS 通常自带

最终用户只需要安装你生成的 `.dmg` / `.app`，不需要额外安装 ExifTool。

## 一键构建 DMG

如果 Qt 通过 Homebrew 安装：

```bash
brew install qt cmake
./scripts/build_macos_dmg.sh
```

如果 Qt 通过 Qt Online Installer 安装，需要手动指定 Qt 路径，例如：

```bash
QT_PREFIX="$HOME/Qt/6.7.2/macos" ./scripts/build_macos_dmg.sh
```

构建成功后，项目根目录会生成：

```text
FileMetaCleaner-1.0.0-mac.dmg
```

用户打开 DMG 后，把 `FileMetaCleaner.app` 拖到“应用程序”即可使用。

## 分步构建

### 1. 准备内置 ExifTool

```bash
./scripts/prepare_exiftool.sh
```

这个脚本会从 exiftool.org 下载完整 Perl 分发包，并整理为：

```text
third_party/exiftool/exiftool
third_party/exiftool/lib/
```

如果自动识别最新版失败，可以手动指定下载地址：

```bash
EXIFTOOL_TAR_URL="https://exiftool.org/Image-ExifTool-13.57.tar.gz" ./scripts/prepare_exiftool.sh
```

### 2. 构建 Release 版 app

Homebrew Qt：

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"

cmake --build build --config Release
```

Qt 官方安装器路径示例：

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/6.7.2/macos"

cmake --build build --config Release
```

### 3. 检查 ExifTool 是否已经进入 app

```bash
ls -l build/FileMetaCleaner.app/Contents/Resources/exiftool/
build/FileMetaCleaner.app/Contents/Resources/exiftool/exiftool -ver
```

应该能看到：

```text
exiftool
lib
```

并能输出 ExifTool 版本号。

### 4. 部署 Qt 依赖并生成 DMG

Homebrew Qt：

```bash
export PATH="$(brew --prefix qt)/bin:$PATH"
cd build
macdeployqt FileMetaCleaner.app -dmg -verbose=2
cd ..
mv build/FileMetaCleaner.dmg FileMetaCleaner-1.0.0-mac.dmg
```

Qt 官方安装器路径示例：

```bash
export PATH="$HOME/Qt/6.7.2/macos/bin:$PATH"
cd build
macdeployqt FileMetaCleaner.app -dmg -verbose=2
cd ..
mv build/FileMetaCleaner.dmg FileMetaCleaner-1.0.0-mac.dmg
```

## 本地运行测试

```bash
open build/FileMetaCleaner.app
```

拖入图片、PDF、Office 文档等文件，确认可以读取元信息并清除所选项。

## 可选：签名和公证

如果只给自己或内部同事使用，可以先不签名。如果要公开分发，建议使用 Apple Developer ID 对 `.app` 和 `.dmg` 签名并进行 notarization，否则用户首次打开时可能遇到 macOS Gatekeeper 拦截。

## 注意事项

1. 清除元数据会直接修改原文件，重要文件请先备份。
2. 不同文件格式可删除的元数据不同。某些条目虽然显示为可清除，但实际文件格式可能不支持删除，ExifTool 会返回失败信息。
3. 当前版本只处理单个拖入文件；一次拖入多个文件时，只读取第一个本地文件。
4. 源码包不直接内置 ExifTool 正文文件，避免携带第三方源码体积和版本问题；构建 DMG 时由 `scripts/prepare_exiftool.sh` 下载并内置进 `.app`。


## 应用图标

当前源码已经包含：

```text
resources/AppIcon.png
resources/AppIcon.icns
resources/resources.qrc
```

如果后续需要替换图标，建议先替换 `resources/AppIcon.png`，再在 macOS 上执行：

```bash
./scripts/generate_app_icon_icns.sh
```

然后重新执行：

```bash
./build_macos_dmg.sh
```
