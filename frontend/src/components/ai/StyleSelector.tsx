import { Select } from 'antd'

import { ART_STYLE_OPTIONS } from '../../utils/constants'
import type { ArtStyle } from '../../types/generation'

interface StyleSelectorProps {
  value?: ArtStyle
  onChange?: (value: ArtStyle) => void
  disabled?: boolean
  className?: string
}

/** 艺术风格选择器（受控，可被 Form.Item 直接包裹）。 */
function StyleSelector({ value, onChange, disabled, className }: StyleSelectorProps) {
  const options = ART_STYLE_OPTIONS.map((option) => ({
    value: option.value,
    label: option.label,
  }))

  return (
    <Select
      value={value}
      onChange={onChange}
      options={options}
      disabled={disabled}
      className={className}
      placeholder="选择艺术风格"
    />
  )
}

export default StyleSelector
