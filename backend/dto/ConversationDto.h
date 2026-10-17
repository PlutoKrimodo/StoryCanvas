#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "AuthDto.h"  // FieldError

using json = nlohmann::json;

/** 分镜规划请求（POST /generation/plans）。 */
struct CreatePlanRequest {
    std::string conversationId;  // 可选：登录用户继续已有会话
    std::string text;
    int imageCount = 1;
    std::string style = "cartoon";
    std::string size = "1024x1024";

    static CreatePlanRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};

/** 用户逐格编辑后的分镜。 */
struct PlanPanelInput {
    int index = 0;
    std::string text;
};

/** PUT /generation/plans/{id}。 */
struct UpdatePlanRequest {
    std::vector<PlanPanelInput> panels;

    static UpdatePlanRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;

    json toJson() const;
};

/** POST /generation/batches/{id}/start。 */
struct StartBatchRequest {
    int seed = -1;      // -1 表示由服务端生成
    std::string bookId; // 可选：绑定绘本
    std::string style;  // 可选：以确认时的最新选择覆盖草稿
    std::string size;

    static StartBatchRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};

/** POST /conversations。 */
struct CreateConversationRequest {
    std::string title;
    std::string style;
    std::string size;

    static CreateConversationRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};
