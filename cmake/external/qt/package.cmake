# This file is part of Desktop App Toolkit,
# a set of libraries for developing nice desktop applications.
#
# For license and copyright information please follow this link:
# https://github.com/desktop-app/legal/blob/master/LEGAL


if (NOT DESKTOP_APP_USE_PACKAGED)
    if (DEFINED ENV{QT})
        set(qt_requested "$ENV{QT}" CACHE STRING "Qt version" FORCE)
        set(qt_requested "$ENV{QT}")
    endif()

    if (NOT LINUX AND NOT DEFINED qt_requested)
        message(FATAL_ERROR "Qt version is unknown, set `QT' environment variable")
    endif()

    if (WIN32)
        set(qt_loc ${libs_loc}/Qt-${qt_requested})

        if (qt_requested GREATER 6)
            set(OPENSSL_FOUND 1)
            set(OPENSSL_INCLUDE_DIR ${libs_loc}/openssl3/include)
            set(LIB_EAY_DEBUG ${libs_loc}/openssl3/out.dbg/libcrypto.lib)
            set(SSL_EAY_DEBUG ${libs_loc}/openssl3/out.dbg/libssl.lib)
            set(LIB_EAY_RELEASE ${libs_loc}/openssl3/out/libcrypto.lib)
            set(SSL_EAY_RELEASE ${libs_loc}/openssl3/out/libssl.lib)
            set(JPEG_FOUND 1)
            set(JPEG_INCLUDE_DIR ${libs_loc}/mozjpeg)
            set(JPEG_LIBRARY_DEBUG ${libs_loc}/mozjpeg/Debug/jpeg-static.lib)
            set(JPEG_LIBRARY_RELEASE ${libs_loc}/mozjpeg/Release/jpeg-static.lib)
            set(ZLIB_FOUND 1)
            set(ZLIB_INCLUDE_DIR ${libs_loc}/zlib)
            set(ZLIB_LIBRARY_DEBUG ${libs_loc}/zlib/Debug/zlibstaticd.lib)
            set(ZLIB_LIBRARY_RELEASE ${libs_loc}/zlib/Release/zlibstatic.lib)
            set(WebP_INCLUDE_DIR ${libs_loc}/libwebp/src)
            set(WebP_demux_INCLUDE_DIR ${libs_loc}/libwebp/src)
            set(WebP_mux_INCLUDE_DIR ${libs_loc}/libwebp/src)
            if (CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
                set(qt_arch ARM64)
            elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(qt_arch x64)
            else()
                set(qt_arch x86)
            endif()
            set(WebP_LIBRARY ${libs_loc}/libwebp/out/release-static/${qt_arch}/lib/webp.lib)
            set(WebP_demux_LIBRARY ${libs_loc}/libwebp/out/release-static/${qt_arch}/lib/webpdemux.lib)
            set(WebP_mux_LIBRARY ${libs_loc}/libwebp/out/release-static/${qt_arch}/lib/webpmux.lib)
            set(LCMS2_FOUND 1)
            set(LCMS2_INCLUDE_DIR ${libs_loc}/liblcms2/include)
            set(LCMS2_LIBRARIES ${libs_loc}/liblcms2/out/Release/src/liblcms2.a)
        endif()
    elseif (APPLE)
        set(qt_loc ${libs_loc}/local/Qt-${qt_requested})
    endif()
    message(STATUS "DEBUG: QT ENV: $ENV{QT}")
    message(STATUS "DEBUG: libs_loc: ${libs_loc}")
    message(STATUS "DEBUG: qt_loc: ${qt_loc}")
    set(CMAKE_PREFIX_PATH ${qt_loc} ${libs_loc}/local ${CMAKE_PREFIX_PATH})
endif()


if (DEFINED QT_VERSION_MAJOR)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
else()
    if (qt_requested VERSION_GREATER_EQUAL 6)
        find_package(Qt6 COMPONENTS Core REQUIRED)
    else()
        find_package(Qt5 COMPONENTS Core REQUIRED)
    endif()
endif()

if (NOT LINUX AND NOT DESKTOP_APP_USE_PACKAGED AND NOT qt_requested EQUAL QT_VERSION)
    message(FATAL_ERROR "Configured Qt version ${QT_VERSION} does not match requested version ${qt_requested}. Please reconfigure.")
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets Network Svg REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Quick QuickWidgets QUIET)

if (QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS OpenGL OpenGLWidgets REQUIRED)
endif()

if (QT_VERSION VERSION_GREATER_EQUAL 6.10)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS GuiPrivate WidgetsPrivate REQUIRED)
endif()

if (LINUX)
    find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS DBus WaylandCompositor QUIET)
endif()

set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "(gen)")
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "(gen)")
