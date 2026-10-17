#include <gtest/gtest.h>

#include <string>

#include "utils/AiCredentialRules.h"

namespace {
const std::string kKey = "sk-test-1234567890";
std::string error;
}  // namespace

TEST(AiCredentialRulesTest, ParsesCapability) {
    AiCapability capability = AiCapability::Unknown;
    EXPECT_TRUE(AiCredentialRules::parseCapability("llm", capability));
    EXPECT_EQ(capability, AiCapability::Llm);
    EXPECT_TRUE(AiCredentialRules::parseCapability("image", capability));
    EXPECT_EQ(capability, AiCapability::Image);
    EXPECT_FALSE(AiCredentialRules::parseCapability("tts", capability));
    EXPECT_EQ(AiCredentialRules::capabilityName(AiCapability::Image), "image");
}

TEST(AiCredentialRulesTest, CatalogKeepsSingleSourceOfDefaults) {
    const auto catalog = AiCredentialRules::catalogJson();
    ASSERT_TRUE(catalog.contains("llm"));
    ASSERT_TRUE(catalog.contains("image"));
    EXPECT_FALSE(catalog["llm"]["providers"].empty());
    EXPECT_FALSE(catalog["image"]["providers"].empty());

    EXPECT_EQ(AiCredentialRules::defaultApiBase(AiCapability::Llm, "deepseek"),
              "https://api.deepseek.com");
    EXPECT_EQ(AiCredentialRules::defaultModel(AiCapability::Llm, "deepseek"),
              "deepseek-chat");
    EXPECT_EQ(AiCredentialRules::defaultModel(AiCapability::Image, "doubao"),
              "doubao-seedream-5-0-flash-260915");
    EXPECT_EQ(AiCredentialRules::defaultModel(AiCapability::Llm, "custom"), "");
    EXPECT_FALSE(AiCredentialRules::keyUrl(AiCapability::Llm, "deepseek").empty());
}

TEST(AiCredentialRulesTest, RejectsWeakApiKey) {
    error.clear();
    EXPECT_FALSE(AiCredentialRules::validateApiKey("short", error));
    EXPECT_FALSE(error.empty());

    EXPECT_FALSE(AiCredentialRules::validateApiKey(" sk-test-1234567890", error));
    EXPECT_FALSE(AiCredentialRules::validateApiKey(std::string("sk-test\t123456"), error));
    EXPECT_TRUE(AiCredentialRules::validateApiKey(kKey, error));
}

TEST(AiCredentialRulesTest, RejectsUnsafeBaseUrls) {
    error.clear();
    EXPECT_FALSE(AiCredentialRules::validateApiBase("http://api.example.com", error));
    EXPECT_FALSE(AiCredentialRules::validateApiBase("https://localhost:8000", error));
    EXPECT_FALSE(AiCredentialRules::validateApiBase("https://127.0.0.1/v1", error));
    EXPECT_FALSE(AiCredentialRules::validateApiBase("https://169.254.169.254/latest", error));
    EXPECT_FALSE(AiCredentialRules::validateApiBase("https://user:pass@api.example.com", error));
    EXPECT_FALSE(AiCredentialRules::validateApiBase("https://api.example.com:8443", error));
    EXPECT_FALSE(AiCredentialRules::validateApiBase("https://internal", error));
    EXPECT_FALSE(AiCredentialRules::validateApiBase("https://api.example.com?debug=1", error));

    EXPECT_TRUE(AiCredentialRules::validateApiBase("https://api.example.com/v1", error));
    EXPECT_TRUE(AiCredentialRules::validateApiBase("https://api.example.com:443/v1/", error));
}

TEST(AiCredentialRulesTest, ValidatesPresetConfig) {
    error.clear();
    EXPECT_TRUE(AiCredentialRules::validateConfig(AiCapability::Llm, "deepseek", kKey,
                                                 "", "", error))
        << error;

    // 预设厂商不允许偷偷换 Base URL
    EXPECT_FALSE(AiCredentialRules::validateConfig(
        AiCapability::Llm, "deepseek", kKey, "https://evil.example.com", "", error));
    EXPECT_FALSE(AiCredentialRules::validateConfig(
        AiCapability::Image, "doubao", kKey, "https://evil.example.com", "", error));

    // 自定义厂商必须提供 Base URL 与模型
    EXPECT_FALSE(AiCredentialRules::validateConfig(AiCapability::Llm, "custom", kKey,
                                                   "", "my-model", error));
    EXPECT_FALSE(AiCredentialRules::validateConfig(AiCapability::Llm, "custom", kKey,
                                                   "https://llm.example.com/v1", "", error));
    EXPECT_TRUE(AiCredentialRules::validateConfig(AiCapability::Llm, "custom", kKey,
                                                  "https://llm.example.com/v1", "my-model",
                                                  error))
        << error;
}

TEST(AiCredentialRulesTest, QuotaExemptOnlyWhenBothConfigured) {
    EXPECT_FALSE(AiCredentialRules::shouldExemptQuota(false, false));
    EXPECT_FALSE(AiCredentialRules::shouldExemptQuota(true, false));
    EXPECT_FALSE(AiCredentialRules::shouldExemptQuota(false, true));
    EXPECT_TRUE(AiCredentialRules::shouldExemptQuota(true, true));
}
