cmake_minimum_required(VERSION 3.10)
project(mempool)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
add_compile_options(-Wall -O2)

# 查找pthread库
find_package(Threads REQUIRED)

# 设置目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

# 源文件
file(GLOB SOURCES "${SRC_DIR}/*.cpp")

# 添加头文件目录
include_directories(${INC_DIR} ${TEST_DIR})

# 创建单元测试可执行文件
add_executable(unit_test 
    ${SOURCES}
    ${TEST_DIR}/UnitTest.cpp
)

# 链接pthread库
target_link_libraries(unit_test PRIVATE Threads::Threads)


# 添加测试命令
add_custom_target(test
    COMMAND unit_test
    DEPENDS unit_test
)

# 添加 include 目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 收集所有源文件
file(GLOB MEMPOOL_SOURCES "src/*.cpp")

# 生成静态库
add_library(mempool STATIC ${MEMPOOL_SOURCES})

# 安装头文件和库（可选，便于外部项目 find_package）
# install(DIRECTORY include/ DESTINATION include)
# install(TARGETS mempool ARCHIVE DESTINATION lib)

