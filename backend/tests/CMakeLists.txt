# 认证模块单元测试（Phase 2 验收配套）
#
# 依赖 GoogleTest（Ubuntu: sudo apt install libgtest-dev）。
# 未安装时不阻断主程序构建，仅跳过测试目标。

find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(WARNING
        "未找到 GoogleTest，已跳过单元测试目标。"
        "安装方式：sudo apt install libgtest-dev 后重新执行 cmake。")
    return()
endif()

# 仅编译 utils 层（无 Drogon 依赖），单测聚焦密码哈希 / JWT / 加密工具
file(GLOB UTIL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../utils/*.cpp")

add_executable(storycanvas_tests
    test_password_utils.cpp
    test_jwt_utils.cpp
    test_encryption_utils.cpp
    test_file_utils.cpp
    ${UTIL_SOURCES}
)

target_include_directories(storycanvas_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${NLOHMANN_JSON_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIRS}
)

# 兼容不同 CMake / GoogleTest 版本的 main 目标名
if(TARGET GTest::gtest_main)
    set(SC_GTEST_MAIN GTest::gtest_main)
elseif(TARGET GTest::Main)
    set(SC_GTEST_MAIN GTest::Main)
else()
    set(SC_GTEST_MAIN ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES})
endif()

target_link_libraries(storycanvas_tests PRIVATE
    ${SC_GTEST_MAIN}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    Threads::Threads
)

# 与主程序保持一致的密码哈希后端（bcrypt 或 PBKDF2 回退）
if(BCRYPT_INCLUDE_DIR AND BCRYPT_LIBRARY)
    target_compile_definitions(storycanvas_tests PRIVATE SC_HAVE_BCRYPT)
    target_include_directories(storycanvas_tests PRIVATE ${BCRYPT_INCLUDE_DIR})
    target_link_libraries(storycanvas_tests PRIVATE ${BCRYPT_LIBRARY})
endif()

include(GoogleTest)
gtest_discover_tests(storycanvas_tests)
