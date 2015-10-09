cmake_minimum_required (VERSION 3.1)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

project(libvmm CXX)

set(VERSION "1.0")


# address space limit
# can be set full 32bit address (should give instability to lj)
# for full 32bit call cmake with  -DXW_USE_FULL_32:bool=on
if ( XW_USE_FULL_32 )
    message(STATUS "Using 32bit addresses! VAS manager will try to reserve 3.5GB of bottom VAS.")
else()
    message(STATUS "Using 31bit addresses! VAS manager will try to reserve 2GB of bottom VAS.")
    add_definitions(-D__LIMIT_TO_31_bit__)
endif()


add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

macro(set_lib_names target)
    SET_TARGET_PROPERTIES( ${target} PROPERTIES
        OUTPUT_NAME "${target}"
    )
    
    if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
        #we need avoid linux "lib" prefix
        SET_TARGET_PROPERTIES( ${target} PROPERTIES PREFIX "")
    endif()
    
endmacro()

# ---------- Set MS Visual version -----------------------
set (XW_3RDP_ROOT ${CMAKE_INSTALL_PREFIX})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")

    if( "${VC_VERSION}" STREQUAL "" )
    
        set ( VC_VERSION 10 )
    
    endif()

endif()

# ---------- Find Boost Headers/Libraries -----------------------
set (XW_BOOST_MAJOR_VERSION 1 )
set (XW_BOOST_MINOR_VERSION 59)
set (XW_BOOST_VERSION "${XW_BOOST_MAJOR_VERSION}.${XW_BOOST_MINOR_VERSION}" ) 

set (XW_BOOST_DEPENDS system filesystem)

#handle boost as part of local build system if not defined before
if ("${BOOST_ROOT}" STREQUAL "") 
    set (BOOST_ROOT ${XW_3RDP_ROOT}/include/boost-${XW_BOOST_MAJOR_VERSION}_${XW_BOOST_MINOR_VERSION})
    set (BOOST_LIBRARYDIR ${XW_3RDP_ROOT}/lib)
endif ()

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    SET (Boost_COMPILER -gcc48)
endif()

SET (Boost_FIND_REQUIRED ON)
SET (Boost_FIND_QUIETLY ON)
SET (Boost_DEBUG ON)
set (Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_LIBS OFF)
set (Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost ${XW_BOOST_VERSION} COMPONENTS ${XW_BOOST_DEPENDS} REQUIRED)

if(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
endif()

#============================================================================
# libvmm 
#============================================================================

set(libvmm_HDRS_complie
    ${CMAKE_CURRENT_SOURCE_DIR}/xw-incl/cntrs/bit-array.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xw-incl/os/os_mem_functions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xw-incl/cpp/phoenix.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xw-incl/cpp/terminable_base.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xw-incl/cpp/type_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mem_core.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dllmain.h
)    

set(libvmm_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/libvmm.h
)

set(libvmm_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/dllmain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mem_core.cpp
)

SOURCE_GROUP("src" FILES ${libvmm_HDRS_complie} ${libvmm_HDRS} ${libvmm_SRCS})

add_library(libvmm SHARED ${libvmm_HDRS_complie} ${libvmm_HDRS} ${libvmm_SRCS})

target_link_libraries (libvmm
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}       
)

set_lib_names(libvmm)

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL )
    install(TARGETS libvmm
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" )
endif()
if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL )
    install(FILES ${libvmm_HDRS} DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
endif()