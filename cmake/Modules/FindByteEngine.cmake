# Once done these will be defined:
#
#  ICONV_FOUND
#  ICONV_INCLUDE_DIRS
#  ICONV_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_BYTEENGINE QUIET VolcEngineRTC)
endif()


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(BYTEENGINE_INCLUDE_DIR
	NAMES 
		bytertc_room.h
		bytertc_room_event_handler.h
		bytertc_video.h
		bytertc_video_event_handler.h
	HINTS
		ENV ByteEnginePath${_lib_suffix}
		ENV ByteEnginePath
		${_BYTEENGINE_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include)

find_library(BYTEENGINE_LIB
	NAMES ${_BYTEENGINE_LIBRARIES} VolcEngineRTC libVolcEngineRTC
	HINTS
		${_BYTEENGINE_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ByteEngine DEFAULT_MSG BYTEENGINE_LIB BYTEENGINE_INCLUDE_DIR)
mark_as_advanced(BYTEENGINE_INCLUDE_DIR BYTEENGINE_LIB)

if(BYTEENGINE_FOUND)
	message("lina-find: ${BYTEENGINE_INCLUDE_DIR},  ${BYTEENGINE_LIB}")
	set(BYTEENGINE_INCLUDE_DIRS ${BYTEENGINE_INCLUDE_DIR})
	set(BYTEENGINE_LIBRARIES ${BYTEENGINE_LIB})
else()
	message("lina-notfind: ${BYTEENGINE_INCLUDE_DIR},  ${BYTEENGINE_LIB}")
endif()
