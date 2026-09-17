#include "HealthController.h"
#include "../dto/ApiResponse.h"
#include <drogon/HttpResponse.h>

void HealthController::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto resp = HttpResponse::newHttpJsonResponse(
        ApiResponse::success("StoryCanvas Backend is running")
    );
    resp->setStatusCode(k200OK);
    callback(resp);
}
