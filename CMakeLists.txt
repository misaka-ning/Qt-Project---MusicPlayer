cmake_minimum_required(VERSION 3.16)

project(MusicPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt 组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

# 将所有源文件（包括新加的 marqueelabel 相关文件）集中列出
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    marqueelabel.h
    marqueelabel.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MusicPlayer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        res.qrc
        songunit.h songunit.cpp songunit.ui
        musicplaylist.h musicplaylist.cpp musicplaylist.ui
        mediaplayerpool.h mediaplayerpool.cpp
    )
else()
    if(ANDROID)
        add_library(MusicPlayer SHARED ${PROJECT_SOURCES})
    else()
        add_executable(MusicPlayer ${PROJECT_SOURCES})
    endif()
endif()

# 链接 Qt 库（动态使用 Qt${QT_VERSION_MAJOR} 前缀，兼容 Qt5/6）
target_link_libraries(MusicPlayer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
)

# **关键修复**：显式添加当前源目录到包含路径
# 确保编译器能找到 marqueelabel.h（尤其对于生成的 ui_mainwindow.h 中的 #include）
target_include_directories(MusicPlayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 设置 macOS/iOS 相关属性（可选）
if(QT_VERSION VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.MusicPlayer)
else()
  set(BUNDLE_ID_OPTION)  # 空值，避免未定义
endif()
set_target_properties(MusicPlayer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS MusicPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MusicPlayer)
endif()
