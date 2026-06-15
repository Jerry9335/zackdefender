cmake_minimum_required(VERSION 3.16)
project(ZackDefender VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Qt6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Widgets)

qt_standard_project_setup()

# ── Separately create executable and QML module (matching Gallery pattern) ───
qt_add_executable(appzackdefender MANUAL_FINALIZATION
    src/main.cpp
    src/backend/DefenderScanner.h
    src/backend/DefenderScanner.cpp
    src/backend/ProtectionMonitor.h
    src/backend/ProtectionMonitor.cpp
    src/backend/ThreatHistory.h
    src/backend/ThreatHistory.cpp
    src/backend/QuarantineManager.h
    src/backend/QuarantineManager.cpp
    src/backend/TrayManager.h
    src/backend/TrayManager.cpp
    src/backend/ThemeManager.h
    src/backend/ThemeManager.cpp
    src/backend/UpdateManager.h
    src/backend/UpdateManager.cpp
)

if(WIN32)
    target_sources(appzackdefender PRIVATE src/assets/app_icon.rc)
endif()

# Qt Quick Controls 2 configuration (font, style)
target_sources(appzackdefender PRIVATE src/resources.qrc)

qt_add_qml_module(appzackdefender
    URI "zack.defender"
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/pages/DashboardPage.qml
        src/qml/pages/ScanPage.qml
        src/qml/pages/ProtectionPage.qml
        src/qml/pages/QuarantinePage.qml
        src/qml/pages/SettingsPage.qml
)

set_target_properties(appzackdefender PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# ── MD3 Core (vendor from sibling repo) ──────────────────────────────────────
set(MD3_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../material-components-qml")
add_subdirectory("${MD3_SOURCE_DIR}/src/Core" md3Core_build)

# ── Link ─────────────────────────────────────────────────────────────────────
target_link_libraries(appzackdefender PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Widgets)

if(MINGW OR (UNIX AND NOT APPLE))
    target_link_libraries(appzackdefender
        PRIVATE
            -Wl,--whole-archive
            md3Core
            -Wl,--no-whole-archive
    )
else()
    target_link_libraries(appzackdefender PRIVATE md3Core)
endif()

if(WIN32)
    target_link_libraries(appzackdefender PRIVATE advapi32)
endif()

qt_import_qml_plugins(appzackdefender)
qt_finalize_executable(appzackdefender)

install(TARGETS appzackdefender
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ── Copy qtquickcontrols2.conf alongside executable ────────────
add_custom_command(TARGET appzackdefender POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/qtquickcontrols2.conf"
    "$<TARGET_FILE_DIR:appzackdefender>/qtquickcontrols2.conf"
    COMMENT "Copying qtquickcontrols2.conf to build dir"
)
