import { LOGO_COLORS } from '../../theme/palette'

/** 逐字母上色（docs/17 §2.2）：首尾同为 poppy 暖色，中间走五色环 */
const LETTERS = ['S', 't', 'o', 'r', 'y', 'C', 'a', 'n', 'v', 'a', 's']

interface WordmarkProps {
  size?: 'hero' | 'nav'
  tagline?: string
  className?: string
}

/**
 * 艺术字 logo。中文「艺术感」来自配色 + 字重 + 字距 + 逐字母入场，
 * 不依赖任何外部字体；也不用 background-clip:text（html2canvas 不支持）。
 */
function Wordmark({ size = 'nav', tagline, className = '' }: WordmarkProps) {
  const letterSize = size === 'hero' ? 'text-[clamp(2rem,9vw,3.75rem)]' : 'text-xl'

  return (
    <span className={`inline-flex flex-col items-center leading-none ${className}`}>
      <span
        role="img"
        aria-label="StoryCanvas"
        className={`font-display font-bold tracking-[-0.015em] ${letterSize}`}
      >
        {LETTERS.map((letter, index) => (
          <span
            key={`${letter}-${index}`}
            aria-hidden
            className="inline-block animate-paint-in"
            style={{
              color: LOGO_COLORS[index % LOGO_COLORS.length],
              animationDelay: `${index * 40}ms`,
            }}
          >
            {letter}
          </span>
        ))}
      </span>
      {tagline ? (
        <span className="mt-3 text-center text-sm text-ink2">{tagline}</span>
      ) : null}
    </span>
  )
}

export default Wordmark
