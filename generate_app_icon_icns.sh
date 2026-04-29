#!/usr/bin/env bash
set -euo pipefail

# 根据 resources/AppIcon.png 生成 macOS 所需的 resources/AppIcon.icns。
# 需要在 macOS 上执行，依赖系统自带的 sips 和 iconutil。

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "${SCRIPT_DIR}/CMakeLists.txt" ]]; then
  ROOT_DIR="${SCRIPT_DIR}"
elif [[ -f "${SCRIPT_DIR}/../CMakeLists.txt" ]]; then
  ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
else
  echo "无法定位项目根目录：未在脚本目录或其上级目录找到 CMakeLists.txt" >&2
  exit 1
fi

PNG_PATH="${ROOT_DIR}/resources/AppIcon.png"
ICONSET_DIR="${ROOT_DIR}/resources/AppIcon.iconset"
ICNS_PATH="${ROOT_DIR}/resources/AppIcon.icns"

if [[ ! -f "${PNG_PATH}" ]]; then
  echo "未找到图标 PNG：${PNG_PATH}" >&2
  exit 1
fi

if ! command -v sips >/dev/null 2>&1; then
  echo "未找到 sips。请在 macOS 上执行该脚本。" >&2
  exit 1
fi

if ! command -v iconutil >/dev/null 2>&1; then
  echo "未找到 iconutil。请在 macOS 上执行该脚本。" >&2
  exit 1
fi

rm -rf "${ICONSET_DIR}"
mkdir -p "${ICONSET_DIR}"

sips -z 16 16       "${PNG_PATH}" --out "${ICONSET_DIR}/icon_16x16.png" >/dev/null
sips -z 32 32       "${PNG_PATH}" --out "${ICONSET_DIR}/icon_16x16@2x.png" >/dev/null
sips -z 32 32       "${PNG_PATH}" --out "${ICONSET_DIR}/icon_32x32.png" >/dev/null
sips -z 64 64       "${PNG_PATH}" --out "${ICONSET_DIR}/icon_32x32@2x.png" >/dev/null
sips -z 128 128     "${PNG_PATH}" --out "${ICONSET_DIR}/icon_128x128.png" >/dev/null
sips -z 256 256     "${PNG_PATH}" --out "${ICONSET_DIR}/icon_128x128@2x.png" >/dev/null
sips -z 256 256     "${PNG_PATH}" --out "${ICONSET_DIR}/icon_256x256.png" >/dev/null
sips -z 512 512     "${PNG_PATH}" --out "${ICONSET_DIR}/icon_256x256@2x.png" >/dev/null
sips -z 512 512     "${PNG_PATH}" --out "${ICONSET_DIR}/icon_512x512.png" >/dev/null
sips -z 1024 1024   "${PNG_PATH}" --out "${ICONSET_DIR}/icon_512x512@2x.png" >/dev/null

iconutil -c icns "${ICONSET_DIR}" -o "${ICNS_PATH}"

echo "已生成：${ICNS_PATH}"
