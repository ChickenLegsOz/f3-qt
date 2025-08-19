cmake_minimum_required(VERSION 3.16...3.27)
project(f3-qt 
    VERSION 3.0.0 
    DESCRIPTION "Qt GUI for the F3 - Fight Flash Fraud tool"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include current directory
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Enable automoc, autorcc, and autouic
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

if (${QT_VERSION_MAJOR} EQUAL 6)
    qt_standard_project_setup()
endif()

add_executable(f3-qt WIN32 MACOSX_BUNDLE
    aboutdialog.cpp aboutdialog.h aboutdialog.ui
    f3_launcher.cpp f3_launcher.h
    helpwindow.cpp helpwindow.h helpwindow.ui
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
)
# Set version from project version
target_compile_definitions(f3-qt PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
    QT_DISABLE_DEPRECATED_UP_TO=0x060000
)

# Icon resources
if (${QT_VERSION_MAJOR} EQUAL 6)
    qt_add_resources(f3-qt "icons"
        PREFIX
            "/icon"
        FILES
            f3.png
    )
else()
    qt5_add_resources(ICON_RC "icon.qrc")
    target_sources(f3-qt PRIVATE ${ICON_RC})
endif()

# Copy desktop file
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/f3-qt.desktop
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Link Qt libraries
target_link_libraries(f3-qt PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Installation
include(GNUInstallDirs)

install(TARGETS f3-qt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    # Install desktop file
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/f3-qt.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    
    # Install icons
    foreach(size IN ITEMS 16 22 24 32 48 64 128 256)
        install(FILES f3.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
            RENAME f3-qt.png
        )
    endforeach()
    
    # Install icon to pixmaps for legacy compatibility
    install(FILES f3.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
        RENAME f3-qt.png
    )
endif()

if (${QT_VERSION_MAJOR} EQUAL 6)
    qt_generate_deploy_app_script(
        TARGET f3-qt
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
