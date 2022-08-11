# Once done these will be defined:
#
#  LIBYUV_FOUND
#  LIBYUV_INCLUDE_DIRS
#  LIBYUV_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_LIBYUV QUIET libyuv)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()



find_path(LIBYUV_INCLUDE_DIR
	NAMES libyuv.h
	HINTS
		ENV LibyuvPath${_lib_suffix}
		ENV LibyuvPath
		${_LIBYUV_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include)

find_library(LIBYUV_LIB
	NAMES ${_LIBYUV_LIBRARIES} libyuv yuv
	HINTS
		${_LIBYUV_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libyuv DEFAULT_MSG LIBYUV_LIB LIBYUV_INCLUDE_DIR)
mark_as_advanced(LIBYUV_INCLUDE_DIR LIBYUV_LIB)

if(LIBYUV_FOUND)
	set(LIBYUV_INCLUDE_DIRS ${LIBYUV_INCLUDE_DIR})
	set(LIBYUV_LIBRARIES ${LIBYUV_LIB})
endif()
