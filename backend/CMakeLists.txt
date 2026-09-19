cmake_minimum_required(VERSION 3.16)

project(storycanvas_backend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==================== 依赖 ====================
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

# PostgreSQL 客户端：当前阶段尚未直接使用 libpq（后续分页/复杂查询接入），
# 因此设为可选，避免因缺少 libpq-dev 阻断构建
find_package(PostgreSQL QUIET)

# 添加头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DROGON_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIRS}
)

# ==================== 源文件 ====================
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
    Threads::Threads
)

if(PostgreSQL_FOUND)
    message(STATUS "已链接 PostgreSQL 客户端库 (libpq)")
    target_link_libraries(${PROJECT_NAME} PostgreSQL::PostgreSQL)
else()
    message(STATUS "未检测到 libpq，跳过数据库客户端链接（当前代码未直接使用）")
endif()

# ==================== 密码哈希后端 ====================
# 首选 bcrypt；若系统未安装 libbcrypt，则自动回退为 PBKDF2-HMAC-SHA256（OpenSSL 实现）。
# 两种算法的哈希串自描述，验证时自动识别，可在安装 bcrypt 后平滑升级。
find_path(BCRYPT_INCLUDE_DIR bcrypt/bcrypt.h)
find_library(BCRYPT_LIBRARY NAMES bcrypt)

if(BCRYPT_INCLUDE_DIR AND BCRYPT_LIBRARY)
    message(STATUS "密码哈希：启用 bcrypt (${BCRYPT_LIBRARY})")
    target_compile_definitions(${PROJECT_NAME} PRIVATE SC_HAVE_BCRYPT)
    target_include_directories(${PROJECT_NAME} PRIVATE ${BCRYPT_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${BCRYPT_LIBRARY})
else()
    message(WARNING
        "未找到 libbcrypt，密码哈希将回退为 PBKDF2-HMAC-SHA256（OpenSSL）。"
        "如需 bcrypt，安装 libbcrypt 后重新执行 cmake 即可。")
endif()

# ==================== 安装配置 ====================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES config/config.json
    DESTINATION etc
)
