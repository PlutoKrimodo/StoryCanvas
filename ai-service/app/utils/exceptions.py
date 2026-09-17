class AIServiceException(Exception):
    """AI 服务基础异常"""
    def __init__(self, message: str, code: int = 500):
        self.message = message
        self.code = code
        super().__init__(self.message)


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
