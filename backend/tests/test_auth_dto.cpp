#include <gtest/gtest.h>

#include "dto/AuthDto.h"

TEST(AuthDtoTest, LoginAcceptsEmailWithAt) {
    auto req = LoginRequest::fromJson({
        {"email", "alice@example.com"},
        {"password", "secret"},
        {"login_type", "email"},
    });

    EXPECT_TRUE(req.validate());
}

TEST(AuthDtoTest, LoginRejectsInvalidEmailInEmailMode) {
    auto req = LoginRequest::fromJson({
        {"email", "alice"},
        {"password", "secret"},
        {"login_type", "email"},
    });

    EXPECT_FALSE(req.validate());
}

TEST(AuthDtoTest, LoginAcceptsUsernameWithoutAt) {
    auto req = LoginRequest::fromJson({
        {"email", "alice"},
        {"password", "secret"},
        {"login_type", "username"},
    });

    EXPECT_TRUE(req.validate());
}

TEST(AuthDtoTest, LoginRejectsUnknownLoginType) {
    auto req = LoginRequest::fromJson({
        {"email", "alice@example.com"},
        {"password", "secret"},
        {"login_type", "phone"},
    });

    EXPECT_FALSE(req.validate());
}

TEST(AuthDtoTest, LegacyClientWithoutLoginTypeRequiresEmailFormat) {
    auto req = LoginRequest::fromJson({
        {"email", "alice"},
        {"password", "secret"},
    });

    EXPECT_FALSE(req.validate());
}
