project(obs-byteengine)

find_package(PkgConfig)
FIND_PACKAGE(Libcurl)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

message( STATUS "Looking for bytertc lib" )

find_package(ByteEngine MODULE REQUIRED)
if( NOT BYTEENGINE_FOUND )
	message( STATUS "No ByteEngine package was found, make an error")
	return()
endif()

find_package(Libyuv MODULE REQUIRED)
if( NOT LIBYUV_FOUND )
	message( STATUS "No Libyuv package was found, make an error")
	return()
endif()

include_directories(${BYTEENGINE_INCLUDE_DIRS})
include_directories(${LIBYUV_INCLUDE_DIRS})

if(MSVC)
	add_compile_options("$<IF:$<CONFIG:Debug>,/MTd,/MT>")
endif()

set(obs-byteengine_HEADERS
	VolcEngineSettingDlg.h
	ByteEngineWarp.h
	data.h
	)

set(obs-byteengine_SOURCES
	VolcEngineSettingDlg.cpp
	ByteEngineWarp.cpp
	obs-byteengine.c
	byteengine.cpp)

set(obs-byteengine-ui_UI
	${obs-byteengine-ui_UI}
	VolcEngineSettingDlg.ui)


if(WIN32)
	set(MODULE_DESCRIPTION "OBS bytertc streaming module")
	configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in obs-byteengine.rc)
	list(APPEND obs-byteengine_SOURCES
		obs-byteengine.rc)
endif()

message("linainfo: ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo" OR CMAKE_BUILD_TYPE MATCHES "DEBUG")
    set(WIN32_C_LIB msvcrtd)
else(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo" OR CMAKE_BUILD_TYPE MATCHES "DEBUG")
    set(WIN32_C_LIB msvcrt)
endif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo" OR CMAKE_BUILD_TYPE MATCHES "DEBUG")



qt5_wrap_ui(obs-byteengine-ui_UI_HEADERS
	${obs-byteengine-ui_UI})


add_library(obs-byteengine MODULE
	${obs-byteengine_HEADERS}
	${obs-byteengine_SOURCES}
	${obs-byteengine-ui_UI_HEADERS})

target_include_directories(obs-byteengine
		PUBLIC  "${LIBCURL_INCLUDE_DIRS}")

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/UI/obs-frontend-api")

target_link_libraries(obs-byteengine
	libobs
	obs-frontend-api
	${obs-byteengine_PLATFORM_DEPS}
	${WIN32_C_LIB}
	${LIBYUV_LIB}
	${BYTEENGINE_LIB}
    Qt5::Widgets)

If (WIN32)
	Target_Compile_Definitions(${PROJECT_NAME}
			PRIVATE
			_HAS_ITERATOR_DEBUGGING=0
			_ITERATOR_DEBUG_LEVEL=0
			)
EndIf()

target_compile_features(obs-byteengine PRIVATE cxx_std_14)
set(_CXX_STANDARD 17)
set_target_properties(obs-byteengine PROPERTIES FOLDER "plugins" CXX_STANDARD 14)

install_obs_plugin_with_data(obs-byteengine data)
