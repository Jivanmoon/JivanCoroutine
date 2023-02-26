cmake_minimum_required(VERSION 3.10)

project(JivanCoroutine)

include (cmake/utils.cmake)

#显示详细的原始编译信息，主要用于定位一些链接错误，看看库路径什么的是否配置对。
set(CMAKE_VERBOSE_MAKEFILE ON)

# 指定编译选项
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror")

# -rdynamic: 将所有符号都加入到符号表中，便于使用dlopen或者backtrace追踪到符号
# -fPIC: 生成位置无关的代码，便于动态链接
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")

# -Wno-unused-function: 不要警告未使用函数
# -Wno-builtin-macro-redefined: 不要警告内置宏重定义，用于重定义内置的__FILE__宏
# -Wno-deprecated: 不要警告过时的特性
# -Wno-deprecated-declarations: 不要警告使用带deprecated属性的变量，类型，函数
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")

set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++11 -O3 -Wall")

include_directories(${PROJECT_SOURCE_DIR}/include)

option(BUILD_TEST "ON for complile test" ON)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC
        code/log.cc
        code/util.cc
        code/mutex.cc
        code/env.cc
        code/config.cc
        code/thread.cc
        code/fiber.cc
        code/scheduler.cc
        code/iomanager.cc
        code/timer.cc
        code/fd_manager.cc
        code/hook.cc
        )

add_library(coroutine SHARED ${LIB_SRC})

force_redefine_file_macro_for_sources(coroutine)

set(LIBS
        coroutine
        pthread
        dl
        yaml-cpp
        )

if(BUILD_TEST)
    jivan_add_executable(test_log "tests/test_log.cc" coroutine "${LIBS}")
    jivan_add_executable(test_util "tests/test_util.cc" coroutine "${LIBS}")
    jivan_add_executable(test_env "tests/test_env.cc" coroutine "${LIBS}")
    jivan_add_executable(test_config "tests/test_config.cc" coroutine "${LIBS}")
    jivan_add_executable(test_thread "tests/test_thread.cc" coroutine "${LIBS}")
    jivan_add_executable(test_fiber "tests/test_fiber.cc" coroutine "${LIBS}")
    jivan_add_executable(test_fiber2 "tests/test_fiber2.cc" coroutine "${LIBS}")
    jivan_add_executable(test_scheduler "tests/test_scheduler.cc" coroutine "${LIBS}")
    jivan_add_executable(test_iomanager "tests/test_iomanager.cc" coroutine "${LIBS}")
    jivan_add_executable(test_timer "tests/test_timer.cc" coroutine "${LIBS}")
    jivan_add_executable(test_hook "tests/test_hook.cc" coroutine "${LIBS}")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)