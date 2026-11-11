CONTENT_REJECTED = "CONTENT_REJECTED"


class AIServiceException(Exception):
    """AI 服务基础异常。"""

    def __init__(self, message: str, code: int = 500, error_code: str = ""):
        self.message = message
        self.code = code
        self.error_code = error_code
        super().__init__(self.message)


class ContentPolicyException(AIServiceException):
    """儿童绘本内容策略拒绝。

    统一使用 HTTP 422 和 CONTENT_REJECTED，供上层禁止重试、禁止兜底并区分前端提示。
    """

    def __init__(self, message: str, category: str = ""):
        super().__init__(message, 422, CONTENT_REJECTED)
        self.category = category


class LLMProviderException(AIServiceException):
    """LLM 提供商异常"""

    pass


class ImageProviderException(AIServiceException):
    """图像生成提供商异常"""

    pass


class TaskTimeoutException(AIServiceException):
    """任务超时异常"""

    def __init__(self, task_id: str):
        super().__init__(f"任务 {task_id} 执行超时", 408)
