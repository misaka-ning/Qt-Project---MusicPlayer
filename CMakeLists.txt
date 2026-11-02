cmake_minimum_required(VERSION 3.21.1)

project(MusicPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt 组件
find_package(Qt6 6.8.3 REQUIRED COMPONENTS Widgets Multimedia Network)

# Shared build objects let the smoke test exercise the same window and resources.
add_library(MusicPlayerClient OBJECT
    src/app/AppConstants.h
    src/player/lrcparser.cpp
    src/player/lrcparser.h
    src/player/mediaplayerpool.cpp
    src/player/mediaplayerpool.h
    src/player/playercontroller.cpp
    src/player/playercontroller.h
    src/services/ErrorHandlingUtils.cpp
    src/services/ErrorHandlingUtils.h
    src/services/cloudmusicservice.cpp
    src/services/cloudmusicservice.h
    src/services/playliststore.cpp
    src/services/playliststore.h
    src/ui/cloudsearchwindow.cpp
    src/ui/cloudsearchwindow.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/marqueelabel.cpp
    src/ui/marqueelabel.h
    src/ui/moremenu.cpp
    src/ui/moremenu.h
    src/ui/musicplaylist.cpp
    src/ui/musicplaylist.h
    src/ui/songunit.cpp
    src/ui/songunit.h
    ui/mainwindow.ui
    ui/moremenu.ui
    ui/musicplaylist.ui
    ui/songunit.ui
    resources/res.qrc
)
set_target_properties(MusicPlayerClient PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)
qt_add_executable(MusicPlayer MANUAL_FINALIZATION src/app/main.cpp)
target_link_libraries(MusicPlayer PRIVATE MusicPlayerClient)

# 链接 Qt 库
# Qt module links used by runtime features
target_link_libraries(MusicPlayerClient PUBLIC
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
)

# 添加当前源目录到包含路径
target_include_directories(MusicPlayerClient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ========== 新增：根据构建类型设置编译定义 ==========
# Release 模式下禁用 qDebug / qWarning 输出
target_compile_definitions(MusicPlayerClient PRIVATE
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

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(MusicPlayerSmoke tests/client_smoke.cpp)
    target_link_libraries(MusicPlayerSmoke PRIVATE MusicPlayerClient)
    add_test(NAME client_startup_resources COMMAND MusicPlayerSmoke)
    set_tests_properties(client_startup_resources PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 20
    )
endif()
