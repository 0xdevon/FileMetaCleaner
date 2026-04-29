cmake_minimum_required(VERSION 3.21)

project(FileMetaCleaner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

set(MACOSX_BUNDLE_ICON_FILE "AppIcon.icns")
set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/resources/AppIcon.icns")

set_source_files_properties(${APP_ICON_MACOS} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)

qt_add_executable(FileMetaCleaner MACOSX_BUNDLE
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    resources/resources.qrc
    ${APP_ICON_MACOS}
)

target_link_libraries(FileMetaCleaner PRIVATE Qt6::Core Qt6::Widgets)

set_target_properties(FileMetaCleaner PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.devonchan.FileMetaCleaner"
    MACOSX_BUNDLE_BUNDLE_NAME "FileMeta Cleaner"
    MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
)

# 将 ExifTool 内置进 .app：
# FileMetaCleaner.app/Contents/Resources/exiftool/exiftool
# FileMetaCleaner.app/Contents/Resources/exiftool/lib/
set(BUNDLED_EXIFTOOL_DIR "${CMAKE_SOURCE_DIR}/third_party/exiftool")

if(EXISTS "${BUNDLED_EXIFTOOL_DIR}/exiftool" AND EXISTS "${BUNDLED_EXIFTOOL_DIR}/lib")
    add_custom_command(TARGET FileMetaCleaner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_CONTENT_DIR:FileMetaCleaner>/Resources/exiftool"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${BUNDLED_EXIFTOOL_DIR}/exiftool"
                "$<TARGET_BUNDLE_CONTENT_DIR:FileMetaCleaner>/Resources/exiftool/exiftool"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${BUNDLED_EXIFTOOL_DIR}/lib"
                "$<TARGET_BUNDLE_CONTENT_DIR:FileMetaCleaner>/Resources/exiftool/lib"
        COMMAND /bin/chmod +x
                "$<TARGET_BUNDLE_CONTENT_DIR:FileMetaCleaner>/Resources/exiftool/exiftool"
        VERBATIM
    )
else()
    message(WARNING "未发现 third_party/exiftool/exiftool 或 third_party/exiftool/lib。打包前请执行 scripts/prepare_exiftool.sh。")
endif()
