cmake_minimum_required(VERSION 3.16)
project(storycanvas_backend VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(PostgreSQL REQUIRED)

# 添加头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DROGON_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIRS}
)

# 收集源文件
file(GLOB_RECURSE SOURCES 
    "controllers/*.cc"
    "services/*.cc"
    "repositories/*.cc"
    "models/*.cc"
    "filters/*.cc"
    "dto/*.cc"
    "utils/*.cc"
)

# 创建可执行文件
add_executable(${PROJECT_NAME} main.cc ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} 
    ${DROGON_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pq
    pthread
)

# 安装配置
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES config/config.json
    DESTINATION etc
)