#include "HealthController.h"
#include "../dto/ApiResponse.h"
#include <drogon/HttpResponse.h>

void HealthController::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    callback(ApiResponse::ok(ApiResponse::success("StoryCanvas Backend is running")));
}
