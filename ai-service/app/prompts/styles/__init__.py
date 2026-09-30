"""风格模板注册表。

新增风格只需：1) 新建模板文件；2) 在此登记；3) 在 `ArtStyle` 枚举补值。
"""

from __future__ import annotations

from typing import Dict

from ...models.schemas import ArtStyle
from .anime import ANIME_STYLE
from .cartoon import CARTOON_STYLE
from .oil_painting import OIL_PAINTING_STYLE
from .pencil_sketch import PENCIL_SKETCH_STYLE
from .watercolor import WATERCOLOR_STYLE

STYLE_TEMPLATES: Dict[ArtStyle, dict] = {
    ArtStyle.CARTOON: CARTOON_STYLE,
    ArtStyle.WATERCOLOR: WATERCOLOR_STYLE,
    ArtStyle.OIL_PAINTING: OIL_PAINTING_STYLE,
    ArtStyle.ANIME: ANIME_STYLE,
    ArtStyle.PENCIL_SKETCH: PENCIL_SKETCH_STYLE,
}


def get_style_template(style: ArtStyle | str) -> dict:
    """按风格取模板，未知风格回退到卡通。"""
    if isinstance(style, str):
        try:
            style = ArtStyle(style)
        except ValueError:
            return CARTOON_STYLE
    return STYLE_TEMPLATES.get(style, CARTOON_STYLE)


def list_styles() -> list[dict]:
    """列出全部风格（供接口 / 文档展示）。"""
    return [
        {"key": template["key"], "name": template["name"], "description": template["description"]}
        for template in STYLE_TEMPLATES.values()
    ]


__all__ = ["STYLE_TEMPLATES", "get_style_template", "list_styles"]
