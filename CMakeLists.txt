# CMakeList.txt: asyncLog 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
project ("asyncLog")

if(UNIX)
set(CMAKE_CXX_FLAGS "${CAMKE_CXX_FLAGS} -std=c++14 -pthread -o2")
endif()


# 将源代码添加到此项目的可执行文件。
add_executable (test "test.cpp" "asynclog.cpp" "asynclog.h"  "logbuffer.h" "test.cpp")

# TODO: 如有需要，请添加测试并安装目标。
