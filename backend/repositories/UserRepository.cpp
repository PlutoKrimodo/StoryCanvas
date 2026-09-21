#include "UserRepository.h"
#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

bool UserRepository::create(User &user){
    try{
        auto db = drogon::app().getDbClient();
        spdlog::info("执行用户插入: username={}, email={}", user.username, user.email);
        auto result = db->execSqlSync(
            "INSERT INTO users (username, email, password_hash, avatar) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING id::text, created_at::text, updated_at::text",
            user.username,
            user.email,
            user.passwordHash,
            user.avatar.value_or("")
        );
        spdlog::info("插入成功, rows={}", result.size());

        if(!result.empty()){
            spdlog::info("读取 id 列...");
            auto idView = result[0]["id"].as<std::string_view>();
            spdlog::info("id={}", std::string(idView));
            user.id = std::string(idView);
            spdlog::info("读取 created_at 列...");
            auto catView = result[0]["created_at"].as<std::string_view>();
            user.createdAt = std::string(catView);
            spdlog::info("读取 updated_at 列...");
            auto uatView = result[0]["updated_at"].as<std::string_view>();
            user.updatedAt = std::string(uatView);
            spdlog::info("用户创建完成: id={}", user.id);
            return true;
        }
        return false;
    }catch(const std::exception &e){
        spdlog::error("用户创建失败：{}",e.what());
        return false;
    }
}

std::optional<User> UserRepository::findById(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "SELECT id::text, username, email, password_hash, avatar, "
            "is_active, created_at::text, updated_at::text FROM users WHERE id = $1",
            id
        );

        if (!result.empty()) {
            return User::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查找用户失败: {}", e.what());
        return std::nullopt;
    }
}

std::optional<User> UserRepository::findByUsername(const std::string &username) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "SELECT id::text, username, email, password_hash, avatar, "
            "is_active, created_at::text, updated_at::text FROM users WHERE username = $1",
            username
        );

        if (!result.empty()) {
            return User::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查找用户失败: {}", e.what());
        return std::nullopt;
    }
}

std::optional<User> UserRepository::findByEmail(const std::string &email) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "SELECT id::text, username, email, password_hash, avatar, "
            "is_active, created_at::text, updated_at::text FROM users WHERE email = $1",
            email
        );

        if (!result.empty()) {
            return User::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查找用户失败: {}", e.what());
        return std::nullopt;
    }
}

bool UserRepository::update(const User &user) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE users SET username = $1, avatar = $2, is_active = $3 "
            "WHERE id = $4",
            user.username,
            user.avatar.value_or(""),
            user.isActive,
            user.id
        );
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("更新用户失败: {}", e.what());
        return false;
    }
}

bool UserRepository::updatePassword(const std::string &userId, const std::string &newHash) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE users SET password_hash = $1 WHERE id = $2",
            newHash,
            userId
        );
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("更新密码失败: {}", e.what());
        return false;
    }
}

bool UserRepository::existsByUsername(const std::string &username) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "SELECT 1 FROM users WHERE username = $1 LIMIT 1",
            username
        );
        return !result.empty();
    } catch (const std::exception &e) {
        spdlog::error("检查用户名失败: {}", e.what());
        return false;
    }
}

bool UserRepository::existsByEmail(const std::string &email) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "SELECT 1 FROM users WHERE email = $1 LIMIT 1",
            email
        );
        return !result.empty();
    } catch (const std::exception &e) {
        spdlog::error("检查邮箱失败: {}", e.what());
        return false;
    }
}