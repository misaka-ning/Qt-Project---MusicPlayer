cmake_minimum_required(VERSION 3.21.1)

project(MusicPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt 组件
find_package(Qt6 6.8.3 REQUIRED COMPONENTS Widgets Multimedia Network)

# 将所有源文件列出
# Core application sources (startup + main window)
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    marqueelabel.h
    marqueelabel.cpp
)

qt_add_executable(MusicPlayer
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    res.qrc
    songunit.h songunit.cpp songunit.ui
    musicplaylist.h musicplaylist.cpp musicplaylist.ui
    mediaplayerpool.h mediaplayerpool.cpp
    lrcparser.h lrcparser.cpp
    playercontroller.h playercontroller.cpp
    moremenu.h moremenu.cpp moremenu.ui
    playliststore.h playliststore.cpp
    cloudmusicservice.h cloudmusicservice.cpp
    cloudsearchwindow.h cloudsearchwindow.cpp
    config/AppConstants.h
    utils/ErrorHandlingUtils.h utils/ErrorHandlingUtils.cpp
)

# 链接 Qt 库
# Qt module links used by runtime features
target_link_libraries(MusicPlayer PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
)

# 添加当前源目录到包含路径
target_include_directories(MusicPlayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ========== 新增：根据构建类型设置编译定义 ==========
# Release 模式下禁用 qDebug / qWarning 输出
target_compile_definitions(MusicPlayer PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
    $<$<CONFIG:Release>:QT_NO_WARNING_OUTPUT>
)
# ==================================================

# 设置 macOS/iOS 相关属性
set_target_properties(MusicPlayer PROPERTIES
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

qt_finalize_executable(MusicPlayer)
