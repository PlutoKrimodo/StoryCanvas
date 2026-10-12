import { CheckCircleFilled } from '@ant-design/icons'

import type { ArtStyle } from '../../types/generation'
import { ART_STYLE_META, ART_STYLE_OPTIONS } from '../../utils/constants'

interface StyleSelectorProps {
  value?: ArtStyle
  onChange?: (value: ArtStyle) => void
  disabled?: boolean
  className?: string
}

/** 硬分段笔触条：比渐变更有「调色板」感，且不需要任何图片资源 */
const strokeOf = (palette: readonly string[]) =>
  `linear-gradient(135deg, ${palette[0]} 0 30%, ${palette[1]} 30% 55%, ${palette[2]} 55% 80%, ${palette[3]} 80% 100%)`

/**
 * 艺术风格色卡（受控，可被 Form.Item 直接包裹）。
 *
 * 对外契约与原 Select 版一致：value / onChange / disabled / className，
 * 因此 `<Form.Item name="style">` 与历史「复用参数」回填都不受影响。
 */
function StyleSelector({ value, onChange, disabled, className }: StyleSelectorProps) {
  return (
    <div
      className={`grid grid-cols-2 gap-2 sm:grid-cols-3 lg:grid-cols-5 ${className ?? ''}`}
      aria-label="艺术风格"
    >
      {ART_STYLE_OPTIONS.map((option) => {
        const meta = ART_STYLE_META[option.value]
        const checked = value === option.value

        return (
          <button
            key={option.value}
            type="button"
            aria-pressed={checked}
            disabled={disabled}
            title={meta.description}
            onClick={() => onChange?.(option.value)}
            className={`relative overflow-hidden rounded-card text-left transition-[transform,box-shadow] duration-200 hover:-translate-y-1 active:scale-[0.98] disabled:opacity-50 disabled:hover:translate-y-0 ${
              checked ? 'bg-paper shadow-card ring-2 ring-clay' : 'bg-canvas2 ring-1 ring-hairline'
            }`}
          >
            {/* 上色挂在外层 span：antd 的 .anticon{color:inherit} 会盖掉同元素上的工具类 */}
            {checked ? (
              <span className="absolute right-1.5 top-1.5 z-10 text-clay">
                <CheckCircleFilled />
              </span>
            ) : null}

            <span className="block h-10 w-full" style={{ backgroundImage: strokeOf(meta.palette) }} aria-hidden />
            <span className="block px-2.5 pb-2.5 pt-2">
              <span className="block text-sm font-medium leading-tight text-ink">{meta.label}</span>
              <span className="mt-1 block text-xs leading-tight text-ink3">{meta.description}</span>
            </span>
          </button>
        )
      })}
    </div>
  )
}

export default StyleSelector
