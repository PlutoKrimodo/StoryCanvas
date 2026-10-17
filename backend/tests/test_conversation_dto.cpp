#include <gtest/gtest.h>

#include "dto/ConversationDto.h"
#include "dto/PageDto.h"

TEST(ConversationDtoTest, AcceptsValidPlanRequest) {
    auto request = CreatePlanRequest::fromJson({
        {"text", "小女孩在雨后的森林里发现一只小鸟"},
        {"image_count", 3},
        {"style", "watercolor"},
        {"size", "1024x1024"},
    });

    EXPECT_TRUE(request.validate());
    EXPECT_EQ(request.imageCount, 3);
}

TEST(ConversationDtoTest, RejectsInvalidCountOrStyle) {
    auto countTooLarge = CreatePlanRequest::fromJson({
        {"text", "故事"}, {"image_count", 7}, {"style", "watercolor"}, {"size", "1024x1024"},
    });
    EXPECT_FALSE(countTooLarge.validate());

    auto badStyle = CreatePlanRequest::fromJson({
        {"text", "故事"}, {"image_count", 1}, {"style", "pixel"}, {"size", "1024x1024"},
    });
    EXPECT_FALSE(badStyle.validate());

    auto badSize = CreatePlanRequest::fromJson({
        {"text", "故事"}, {"image_count", 1}, {"style", "cartoon"}, {"size", "big"},
    });
    EXPECT_FALSE(badSize.validate());
}

TEST(ConversationDtoTest, RequiresSequentialPanelIndices) {
    json okBody = json::object();
    okBody["panels"] = json::array({
        {{"index", 0}, {"text", "第一格"}},
        {{"index", 1}, {"text", "第二格"}},
    });
    auto ok = UpdatePlanRequest::fromJson(okBody);
    EXPECT_TRUE(ok.validate());
    EXPECT_EQ(ok.panels.size(), 2u);

    json brokenBody = json::object();
    brokenBody["panels"] = json::array({
        {{"index", 0}, {"text", "第一格"}},
        {{"index", 2}, {"text", "第三格"}},
    });
    auto broken = UpdatePlanRequest::fromJson(brokenBody);
    EXPECT_FALSE(broken.validate());
}

TEST(PageDtoTest, BatchRequiresOneToSixImages) {
    SaveBatchPagesRequest empty;
    EXPECT_FALSE(empty.validate());

    SaveBatchPagesRequest tooMany;
    tooMany.imageIds = {"1", "2", "3", "4", "5", "6", "7"};
    EXPECT_FALSE(tooMany.validate());

    SaveBatchPagesRequest valid;
    valid.imageIds = {"1", "2", "3"};
    EXPECT_TRUE(valid.validate());
}
