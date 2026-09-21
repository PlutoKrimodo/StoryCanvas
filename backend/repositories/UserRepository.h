#pragma once

#include <optional>
#include <string>
#include "../models/User.h"

//用户数据访问
class UserRepository{
public:
    //创建用户
    static bool create(User &user); 
    //按ID查找
    static std::optional<User> findById(const std::string &id); 
    //按用户名查找
    static std::optional<User> findByUsername(const std::string &username); 
    //按邮箱查找（登录用）
    static std::optional<User> findByEmail(const std::string &email);
    //更新用户信息
    static bool update(const User &user);
    //更新密码
    static bool updatePassword(const std::string &userId, const std::string &newHash);
    //检查用户名是否已存在
    static bool existsByUsername(const std::string &username);
    //检查邮箱是否已存在 
    static bool existsByEmail(const std::string &email);
};