#include <gtest/gtest.h>

#include <string>

#include "utils/EncryptionUtils.h"
#include "utils/SecretBox.h"

namespace {
const std::string kUser = "11111111-1111-1111-1111-111111111111";
const std::string kOtherUser = "22222222-2222-2222-2222-222222222222";
const std::string kKey = EncryptionUtils::generateKey();
}  // namespace

TEST(SecretBoxTest, RoundTripRestoresApiKey) {
    const std::string sealed = SecretBox::seal(kUser, "llm", "sk-test-1234567890", kKey);

    EXPECT_EQ(sealed.find("sk-test"), std::string::npos);
    EXPECT_EQ(SecretBox::open(sealed, kUser, "llm", kKey), "sk-test-1234567890");
}

TEST(SecretBoxTest, RejectsCiphertextBoundToAnotherUser) {
    const std::string sealed = SecretBox::seal(kUser, "llm", "sk-test-1234567890", kKey);

    EXPECT_THROW(SecretBox::open(sealed, kOtherUser, "llm", kKey), std::runtime_error);
}

TEST(SecretBoxTest, RejectsCiphertextBoundToAnotherCapability) {
    const std::string sealed = SecretBox::seal(kUser, "llm", "sk-test-1234567890", kKey);

    EXPECT_THROW(SecretBox::open(sealed, kUser, "image", kKey), std::runtime_error);
}

TEST(SecretBoxTest, RejectsTamperedCiphertext) {
    std::string sealed = SecretBox::seal(kUser, "image", "sk-test-1234567890", kKey);
    sealed[sealed.size() / 2] = sealed[sealed.size() / 2] == 'A' ? 'B' : 'A';

    EXPECT_THROW(SecretBox::open(sealed, kUser, "image", kKey), std::runtime_error);
}

TEST(SecretBoxTest, RequiresEncryptionKey) {
    EXPECT_THROW(SecretBox::seal(kUser, "llm", "sk-test-1234567890", ""), std::runtime_error);
    EXPECT_FALSE(SecretBox::hasKey(""));
    EXPECT_FALSE(SecretBox::hasKey("short-secret"));
    EXPECT_FALSE(SecretBox::hasKey("change_me_please"));
    EXPECT_TRUE(SecretBox::hasKey(kKey));
}

TEST(SecretBoxTest, HintMasksMiddle) {
    EXPECT_EQ(SecretBox::hint("sk-1234567890abcdef"), "sk-1****cdef");
    EXPECT_EQ(SecretBox::hint("short"), "****");
    EXPECT_EQ(SecretBox::hint(""), "");
}
