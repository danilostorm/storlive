cmake_minimum_required(VERSION 3.24)

project(StorLive VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

option(STORLIVE_WITH_LIBOBS "Build with libobs integration" ON)
set(STORLIVE_OBS_ROOT "" CACHE PATH "Optional OBS/libobs SDK root for Windows portable builds")

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.4)

set(STORLIVE_HAS_LIBOBS OFF)

if(STORLIVE_WITH_LIBOBS)
    if(WIN32 AND STORLIVE_OBS_ROOT)
        find_path(STORLIVE_LIBOBS_INCLUDE_DIR
            NAMES obs.h
            PATHS
                "${STORLIVE_OBS_ROOT}/include/obs"
                "${STORLIVE_OBS_ROOT}/include"
                "${STORLIVE_OBS_ROOT}/libobs"
            NO_DEFAULT_PATH)
        find_library(STORLIVE_LIBOBS_LIBRARY
            NAMES obs libobs
            PATHS
                "${STORLIVE_OBS_ROOT}/lib/64bit"
                "${STORLIVE_OBS_ROOT}/bin/64bit"
                "${STORLIVE_OBS_ROOT}/lib"
            NO_DEFAULT_PATH)
        if(STORLIVE_LIBOBS_INCLUDE_DIR AND STORLIVE_LIBOBS_LIBRARY)
            set(STORLIVE_HAS_LIBOBS ON)
        endif()
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(LIBOBS QUIET IMPORTED_TARGET libobs)
            if(LIBOBS_FOUND)
                set(STORLIVE_HAS_LIBOBS ON)
            endif()
        endif()
    endif()
endif()

if(STORLIVE_WITH_LIBOBS AND NOT STORLIVE_HAS_LIBOBS)
    message(FATAL_ERROR "STORLIVE_WITH_LIBOBS=ON but libobs was not found. Production builds must not fall back to the UI-only stub.")
endif()

qt_add_executable(storlive
    src/app/main.cpp
    src/core/AppController.cpp
    src/core/AppController.h
    src/core/obs/ObsEngine.cpp
    src/core/obs/ObsEngine.h
    src/core/obs/ObsPreviewProvider.cpp
    src/core/obs/ObsPreviewProvider.h
    src/core/obs/SceneManager.cpp
    src/core/obs/SceneManager.h
    src/core/streaming/StreamDestination.h
    src/core/streaming/MultiOutputManager.cpp
    src/core/streaming/MultiOutputManager.h
)

set_source_files_properties(resources/qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)

qt_add_qml_module(storlive
    URI StorLive
    VERSION 1.0
    QML_FILES
        resources/qml/Main.qml
)

target_include_directories(storlive PRIVATE src)

target_link_libraries(storlive PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

if(STORLIVE_HAS_LIBOBS)
    target_compile_definitions(storlive PRIVATE STORLIVE_HAS_LIBOBS=1)
    if(WIN32)
        target_include_directories(storlive PRIVATE "${STORLIVE_LIBOBS_INCLUDE_DIR}")
        target_link_libraries(storlive PRIVATE "${STORLIVE_LIBOBS_LIBRARY}")
    else()
        target_link_libraries(storlive PRIVATE PkgConfig::LIBOBS)
    endif()
    message(STATUS "StorLive: libobs backend enabled")
endif()

if(WIN32)
    set_target_properties(storlive PROPERTIES WIN32_EXECUTABLE TRUE)
    install(TARGETS storlive RUNTIME DESTINATION . BUNDLE DESTINATION .)
else()
    install(TARGETS storlive RUNTIME DESTINATION bin BUNDLE DESTINATION .)
    install(FILES packaging/linux/storlive.desktop DESTINATION share/applications)
    install(FILES LICENSE DESTINATION share/doc/storlive)
endif()

set(CPACK_PACKAGE_NAME "storlive")
set(CPACK_PACKAGE_VENDOR "StorLive")
set(CPACK_PACKAGE_CONTACT "danilostorm")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Desktop multi-live application for Windows and Linux")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "danilostorm")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
if(STORLIVE_HAS_LIBOBS)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "obs-plugins")
endif()
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
include(CPack)
