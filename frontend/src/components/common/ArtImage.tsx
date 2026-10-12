import { PictureOutlined, ReloadOutlined } from '@ant-design/icons'
import { Button, Typography } from 'antd'
import { useEffect, useRef, useState } from 'react'

const { Text } = Typography

type LoadState = 'loading' | 'loaded' | 'error'

interface ArtImageProps {
  src: string
  alt: string
  className?: string
  wrapperClassName?: string
  /** 骨架屏与图片共用的限高，避免出图瞬间卡片跳动 */
  maxHeight?: number | string
  rounded?: boolean
}

/**
 * 结果图（docs/17 §4.3）：骨架屏 → 淡入 → 失败可重试。
 *
 * 淡入只挂在本组件显式输出的 `.sc-artimg` 类上。utils/pdf.ts 会往 body 里塞
 * 裸 <img> 交给 html2canvas 截图，任何元素级动画都会让它在 opacity<1 时被抓取，
 * 导出 PDF 出现空白页——所以这里不碰 img {} 全局选择器（docs/17 §8.1）。
 */
function ArtImage({
  src,
  alt,
  className = '',
  wrapperClassName = '',
  maxHeight = 360,
  rounded = true,
}: ArtImageProps) {
  const [state, setState] = useState<LoadState>('loading')
  const [retryToken, setRetryToken] = useState(0)
  const imgRef = useRef<HTMLImageElement>(null)

  const heightCss = typeof maxHeight === 'number' ? `${maxHeight}px` : maxHeight

  useEffect(() => {
    setState('loading')
  }, [src, retryToken])

  // 命中缓存的图片可能在 effect 挂载前就已 complete，onLoad 不会补发
  useEffect(() => {
    if (state !== 'loading') {
      return
    }
    const node = imgRef.current
    if (node?.complete) {
      setState(node.naturalWidth > 0 ? 'loaded' : 'error')
    }
  }, [src, retryToken, state])

  return (
    <div
      className={`relative overflow-hidden bg-canvas2 ${wrapperClassName}`}
      style={state === 'loaded' ? undefined : { height: heightCss }}
    >
      <img
        ref={imgRef}
        key={`${src}-${retryToken}`}
        src={src}
        alt={alt}
        onLoad={() => setState('loaded')}
        onError={() => setState('error')}
        style={{ maxHeight: heightCss }}
        className={`mx-auto block w-auto ${state === 'loaded' ? 'sc-artimg' : 'invisible'} ${
          rounded ? 'rounded-card' : ''
        } ${className}`}
      />

      {state === 'loading' ? <div className="sc-skeleton absolute inset-0" aria-hidden /> : null}

      {state === 'error' ? (
        <div className="absolute inset-0 flex flex-col items-center justify-center gap-2 px-4 text-center">
          <PictureOutlined className="text-2xl text-ink3" />
          <Text type="secondary" className="text-sm">
            图片加载失败
          </Text>
          <Button
            size="small"
            icon={<ReloadOutlined />}
            onClick={() => setRetryToken((token) => token + 1)}
          >
            重试
          </Button>
        </div>
      ) : null}
    </div>
  )
}

export default ArtImage
