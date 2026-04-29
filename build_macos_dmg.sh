#!/usr/bin/env bash
set -euo pipefail

APP_NAME="FileMetaCleaner"
APP_DISPLAY_NAME="FileMeta Cleaner"
VERSION="1.0.0"
DMG_NAME="FileMetaCleaner-${VERSION}-mac.dmg"
# Resolve project root whether this script is run from project root or scripts/.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "${SCRIPT_DIR}/CMakeLists.txt" ]]; then
  ROOT_DIR="${SCRIPT_DIR}"
elif [[ -f "${SCRIPT_DIR}/../CMakeLists.txt" ]]; then
  ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
else
  echo "无法定位项目根目录：未在脚本目录或其上级目录找到 CMakeLists.txt" >&2
  exit 1
fi
BUILD_DIR="${ROOT_DIR}/build"
QT_PREFIX="${QT_PREFIX:-}"

cd "${ROOT_DIR}"

if [[ -z "${QT_PREFIX}" ]]; then
  if command -v brew >/dev/null 2>&1 && brew --prefix qt >/dev/null 2>&1; then
    QT_PREFIX="$(brew --prefix qt)"
  else
    echo "未找到 Qt。请先安装 Qt，或指定 QT_PREFIX，例如：" >&2
    echo "  QT_PREFIX=/Users/you/Qt/6.7.2/macos ./build_macos_dmg.sh" >&2
    exit 1
  fi
fi


if [[ -f "resources/AppIcon.png" && ! -f "resources/AppIcon.icns" ]]; then
  echo "未发现 AppIcon.icns，开始根据 resources/AppIcon.png 生成..."
  "${ROOT_DIR}/scripts/generate_app_icon_icns.sh"
fi

if [[ ! -f "resources/AppIcon.icns" ]]; then
  echo "未找到 resources/AppIcon.icns，应用包图标可能无法生效。" >&2
  exit 1
fi

if [[ ! -x "third_party/exiftool/exiftool" || ! -d "third_party/exiftool/lib" ]]; then
  echo "未发现内置 ExifTool，开始准备..."
  "${ROOT_DIR}/scripts/prepare_exiftool.sh"
fi

if [[ ! -x "${QT_PREFIX}/bin/macdeployqt" ]]; then
  echo "未找到 macdeployqt：${QT_PREFIX}/bin/macdeployqt" >&2
  echo "请确认 QT_PREFIX 指向 Qt 安装目录。" >&2
  exit 1
fi

rm -rf "${BUILD_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${QT_PREFIX}"

cmake --build "${BUILD_DIR}" --config Release

APP_PATH="${BUILD_DIR}/${APP_NAME}.app"
BUNDLED_EXIFTOOL="${APP_PATH}/Contents/Resources/exiftool/exiftool"

if [[ ! -x "${BUNDLED_EXIFTOOL}" ]]; then
  echo "ExifTool 没有成功复制到 app bundle：${BUNDLED_EXIFTOOL}" >&2
  exit 1
fi

echo "内置 ExifTool 版本：$(${BUNDLED_EXIFTOOL} -ver)"

export PATH="${QT_PREFIX}/bin:${PATH}"
cd "${BUILD_DIR}"

# macdeployqt 会把 Qt Frameworks/plugins 拷贝进 .app，-dmg 会同时生成 DMG。
macdeployqt "${APP_NAME}.app" -dmg -verbose=2

cd "${ROOT_DIR}"
rm -f "${DMG_NAME}"
if [[ -f "${BUILD_DIR}/${APP_NAME}.dmg" ]]; then
  mv "${BUILD_DIR}/${APP_NAME}.dmg" "${DMG_NAME}"
elif [[ -f "${BUILD_DIR}/${APP_DISPLAY_NAME}.dmg" ]]; then
  mv "${BUILD_DIR}/${APP_DISPLAY_NAME}.dmg" "${DMG_NAME}"
else
  echo "未找到 macdeployqt 生成的 DMG，请检查 build 目录。" >&2
  exit 1
fi

echo "打包完成：${ROOT_DIR}/${DMG_NAME}"
