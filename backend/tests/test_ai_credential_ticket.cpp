#include <gtest/gtest.h>

#include <string>

#include "utils/AiCredentialTicket.h"
#include "utils/EncryptionUtils.h"

namespace {

const std::string kSecret = "test-ticket-secret-32-bytes-long!!";
const long long kNow = 1'700'000'000;

AiCredentialTicketContext makeContext() {
    AiCredentialTicketContext context;
    context.userId = "11111111-1111-1111-1111-111111111111";
    context.capability = "llm";
    context.provider = "deepseek";
    context.apiBase = "https://api.deepseek.com";
    context.model = "deepseek-chat";
    context.apiKeyHash = EncryptionUtils::sha256("sk-test-1234567890");
    return context;
}

}  // namespace

TEST(AiCredentialTicketTest, IssueAndVerifyBindsConfigurationWithoutPlaintextKey) {
    const auto context = makeContext();
    const std::string token = AiCredentialTicket::issue(kSecret, context, 600, kNow);

    std::string error;
    EXPECT_TRUE(AiCredentialTicket::verify(kSecret, token, context, error, kNow + 599))
        << error;
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(token.find("sk-test"), std::string::npos);
    EXPECT_NE(token.find('.'), std::string::npos);

    EXPECT_FALSE(AiCredentialTicket::verify(kSecret, token, context, error, kNow + 600));
    EXPECT_FALSE(error.empty());
}

TEST(AiCredentialTicketTest, RejectsTamperedSignatureAndPayload) {
    const auto context = makeContext();
    const std::string token = AiCredentialTicket::issue(kSecret, context, 600, kNow);

    std::string tampered = token;
    tampered.back() = tampered.back() == 'A' ? 'B' : 'A';
    std::string error;
    EXPECT_FALSE(AiCredentialTicket::verify(kSecret, tampered, context, error, kNow));
    EXPECT_FALSE(error.empty());

    const std::size_t split = token.rfind('.');
    ASSERT_NE(split, std::string::npos);
    std::string payloadTampered = token;
    payloadTampered[0] = payloadTampered[0] == 'A' ? 'B' : 'A';
    EXPECT_FALSE(
        AiCredentialTicket::verify(kSecret, payloadTampered, context, error, kNow));
}

TEST(AiCredentialTicketTest, RejectsDifferentUserCapabilityProviderBaseModelOrKey) {
    const auto context = makeContext();
    const std::string token = AiCredentialTicket::issue(kSecret, context, 600, kNow);

    auto checkMismatch = [&](const AiCredentialTicketContext &other) {
        std::string error;
        EXPECT_FALSE(AiCredentialTicket::verify(kSecret, token, other, error, kNow));
        EXPECT_FALSE(error.empty());
    };

    auto other = context;
    other.userId = "22222222-2222-2222-2222-222222222222";
    checkMismatch(other);

    other = context;
    other.capability = "image";
    checkMismatch(other);

    other = context;
    other.provider = "openai";
    checkMismatch(other);

    other = context;
    other.apiBase = "https://api.example.com/v1";
    checkMismatch(other);

    other = context;
    other.model = "deepseek-reasoner";
    checkMismatch(other);

    other = context;
    other.apiKeyHash = EncryptionUtils::sha256("sk-other-1234567890");
    checkMismatch(other);
}

TEST(AiCredentialTicketTest, RejectsWrongSecretAndMalformedToken) {
    const auto context = makeContext();
    const std::string token = AiCredentialTicket::issue(kSecret, context, 600, kNow);

    std::string error;
    EXPECT_FALSE(AiCredentialTicket::verify("wrong-secret", token, context, error, kNow));
    EXPECT_FALSE(error.empty());

    EXPECT_FALSE(AiCredentialTicket::verify(kSecret, "not-a-ticket", context, error, kNow));
    EXPECT_FALSE(error.empty());

    EXPECT_FALSE(AiCredentialTicket::verify(kSecret, "", context, error, kNow));
    EXPECT_FALSE(error.empty());
}

TEST(AiCredentialTicketTest, NonceMakesTokensUnique) {
    const auto context = makeContext();
    const std::string first = AiCredentialTicket::issue(kSecret, context, 600, kNow);
    const std::string second = AiCredentialTicket::issue(kSecret, context, 600, kNow);
    EXPECT_NE(first, second);
}
