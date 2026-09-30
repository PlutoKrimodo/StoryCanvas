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

    // 判断用户是否为「活跃的」影子游客（认领前的归属校验）
    static bool isGuestUser(const std::string &id);

    // 把游客的生成任务/图片/绘本/角色归属转移到真实用户（认领）
    static void transferGuestResources(const std::string &guestId, const std::string &userId);

    // 停用影子游客账号（认领后不再可用）
    static bool deactivateGuest(const std::string &guestId);
};