# macOS 安装包构建步骤

## 目标

生成一个用户安装后即可直接使用的 macOS DMG 安装包：

```text
FileMetaCleaner-1.0.0-mac.dmg
```

生成后的 `.app` 内部包含：

```text
FileMetaCleaner.app/Contents/Resources/AppIcon.icns
FileMetaCleaner.app/Contents/Resources/exiftool/exiftool
FileMetaCleaner.app/Contents/Resources/exiftool/lib/
```

因此最终用户不需要安装 Homebrew，也不需要安装 ExifTool；应用在 Finder、Dock 和窗口中也会使用内置图标。

## 推荐方式：一键脚本

### 1. 安装构建工具

如果你使用 Homebrew：

```bash
brew install qt cmake
```

### 2. 解压源码并进入目录

```bash
unzip FileMetaCleaner_BundledExifTool_Source.zip
cd FileMetaCleaner
```

### 3. 一键构建

```bash
./build_macos_dmg.sh
```

也可以执行：

```bash
./scripts/build_macos_dmg.sh
```

如果 Qt 不是通过 Homebrew 安装，而是通过 Qt 官方安装器安装，请指定 `QT_PREFIX`：

```bash
QT_PREFIX="$HOME/Qt/6.7.2/macos" ./scripts/build_macos_dmg.sh
```

### 4. 获取 DMG

构建完成后，项目根目录会生成：

```text
FileMetaCleaner-1.0.0-mac.dmg
```

用户使用方式：

1. 双击打开 DMG；
2. 把 `FileMetaCleaner.app` 拖到“应用程序”；
3. 双击运行；
4. 直接拖入文件查看并清除元信息。

## 分步构建方式

### 1. 准备内置 ExifTool

```bash
./scripts/prepare_exiftool.sh
```

如果自动下载失败，可以手动指定 tar.gz 地址：

```bash
EXIFTOOL_TAR_URL="https://exiftool.org/Image-ExifTool-13.57.tar.gz" ./scripts/prepare_exiftool.sh
```

### 2. 图标说明

源码中已经包含：

```text
resources/AppIcon.png
resources/AppIcon.icns
resources/resources.qrc
```

`AppIcon.icns` 用于 macOS 的 Finder、Dock 和 `.app` 图标；`resources.qrc` 会把 `AppIcon.png` 编译进程序，用于 Qt 窗口运行时图标。

如果你后续替换了 `resources/AppIcon.png`，可以重新生成 `.icns`：

```bash
./scripts/generate_app_icon_icns.sh
```

### 3. 构建 app

```bash
rm -rf build

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"

cmake --build build --config Release
```

### 4. 验证 app 内置 ExifTool 与图标

```bash
ls -l build/FileMetaCleaner.app/Contents/Resources/exiftool/
build/FileMetaCleaner.app/Contents/Resources/exiftool/exiftool -ver

ls -l build/FileMetaCleaner.app/Contents/Resources/AppIcon.icns
/usr/libexec/PlistBuddy -c "Print :CFBundleIconFile"   build/FileMetaCleaner.app/Contents/Info.plist
```

### 5. 使用 macdeployqt 打包 Qt 依赖并生成 DMG

```bash
export PATH="$(brew --prefix qt)/bin:$PATH"
cd build
macdeployqt FileMetaCleaner.app -dmg -verbose=2
cd ..
mv build/FileMetaCleaner.dmg FileMetaCleaner-1.0.0-mac.dmg
```

## 可选：生成 PKG

如果你更想要 `.pkg` 安装器，可以在已经构建好 app 后执行：

```bash
rm -rf pkgroot
mkdir -p pkgroot/Applications
cp -R build/FileMetaCleaner.app pkgroot/Applications/

pkgbuild \
  --root pkgroot \
  --identifier com.devonchan.FileMetaCleaner \
  --version 1.0.0 \
  --install-location / \
  FileMetaCleaner-1.0.0.pkg
```

注意：即使使用 PKG，ExifTool 仍然应该内置在 `.app` 内，不建议安装到用户系统目录。

## 可选：签名和公证

公开分发时建议做签名与公证。你需要 Apple Developer 账号和 Developer ID 证书。

签名 app 示例：

```bash
codesign --force --deep --options runtime --timestamp \
  --sign "Developer ID Application: 你的名字或公司名 (TEAMID)" \
  build/FileMetaCleaner.app
```

验证：

```bash
codesign --verify --deep --strict --verbose=4 build/FileMetaCleaner.app
spctl --assess --type execute --verbose build/FileMetaCleaner.app
```

公证通常需要将 app 压缩后提交：

```bash
ditto -c -k --keepParent build/FileMetaCleaner.app FileMetaCleaner.zip

xcrun notarytool submit FileMetaCleaner.zip \
  --apple-id "你的 Apple ID" \
  --team-id "TEAMID" \
  --password "App 专用密码" \
  --wait

xcrun stapler staple build/FileMetaCleaner.app
```

然后再重新生成 DMG。
