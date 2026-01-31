cmake_minimum_required(VERSION 3.10)
project(DeviceDataServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_DEBUG "Enable debug flags" ON)
option(ENABLE_MYSQL "Enable MySQL support" ON)

if (ENABLE_DEBUG)
    message(STATUS "Build with debug info")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wextra -Wpedantic")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -Wpedantic")
endif()

set(SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src")

include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/utils
    ${SRC_ROOT}/thread
    ${SRC_ROOT}/net
    ${SRC_ROOT}/business
    ${SRC_ROOT}/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

file(GLOB_RECURSE SRC_FILES
    "${SRC_ROOT}/*.cpp"
)

add_executable(device_server
    ${SRC_FILES}
)

# MySQL 支持
if (ENABLE_MYSQL)
    message(STATUS "MySQL support enabled")
    
    # 尝试使用 pkg-config 查找 MySQL
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
        pkg_check_modules(MYSQL QUIET mysqlclient)
    endif()
    
    if (MYSQL_FOUND)
        message(STATUS "Found MySQL via pkg-config: ${MYSQL_LIBRARIES}")
        target_include_directories(device_server PRIVATE ${MYSQL_INCLUDE_DIRS})
        target_link_libraries(device_server ${MYSQL_LIBRARIES})
        target_compile_definitions(device_server PRIVATE ENABLE_MYSQL=1)
    else()
        # 手动查找 MySQL
        find_path(MYSQL_INCLUDE_DIR 
            NAMES mysql/mysql.h mysql.h
            PATHS 
                /usr/include
                /usr/local/include
                /usr/include/mysql
                /usr/local/mysql/include
        )
        
        find_library(MYSQL_LIBRARY 
            NAMES mysqlclient mysql
            PATHS 
                /usr/lib
                /usr/lib64
                /usr/local/lib
                /usr/local/mysql/lib
        )
        
        if (MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
            message(STATUS "Found MySQL: ${MYSQL_LIBRARY}")
            target_include_directories(device_server PRIVATE ${MYSQL_INCLUDE_DIR})
            target_link_libraries(device_server ${MYSQL_LIBRARY})
            target_compile_definitions(device_server PRIVATE ENABLE_MYSQL=1)
        else()
            message(WARNING "MySQL not found! MySQL support disabled.")
            message(WARNING "Install MySQL client library: sudo apt install libmysqlclient-dev")
        endif()
    endif()
endif()

target_link_libraries(device_server
    pthread
)
