"""GenerationService 尺寸解析测试。"""

from app.services.generation_service import GenerationService


def test_resolve_size_upscales_below_1k():
    width, height = GenerationService._resolve_size({"size": "1280x768"})

    assert width * height >= 1024 * 1024
    assert width % 8 == 0
    assert height % 8 == 0


def test_resolve_size_supports_width_height():
    width, height = GenerationService._resolve_size({"width": 768, "height": 1280})

    assert width * height >= 1024 * 1024
    assert width % 8 == 0
    assert height % 8 == 0


def test_resolve_size_keeps_1024_square():
    assert GenerationService._resolve_size({"size": "1024x1024"}) == (1024, 1024)
