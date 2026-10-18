import { useEffect, useRef } from 'react'
import { createPortal } from 'react-dom'
import { CloseOutlined } from '@ant-design/icons'

export interface ImageLightboxProps {
  open: boolean
  src: string
  alt?: string
  onClose: () => void
}

/**
 * 生成图全屏查看器。
 *
 * 通过 Portal 挂到 body，避免被对话滚动容器裁剪；打开时锁定页面滚动，
 * 支持 Esc、点击背景或右上角关闭。
 */
function ImageLightbox({ open, src, alt = '生成图片', onClose }: ImageLightboxProps) {
  const closeRef = useRef<HTMLButtonElement>(null)

  useEffect(() => {
    if (!open) return
    const previousOverflow = document.body.style.overflow
    document.body.style.overflow = 'hidden'

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') onClose()
    }
    window.addEventListener('keydown', handleKeyDown)
    closeRef.current?.focus()

    return () => {
      document.body.style.overflow = previousOverflow
      window.removeEventListener('keydown', handleKeyDown)
    }
  }, [open, onClose])

  if (!open) return null

  return createPortal(
    <div
      role="dialog"
      aria-modal="true"
      aria-label="图片预览"
      className="animate-fade fixed inset-0 z-[1200] flex items-center justify-center bg-ink/85 p-4 backdrop-blur-sm"
      onClick={onClose}
    >
      <button
        ref={closeRef}
        type="button"
        aria-label="关闭预览"
        onClick={(event) => {
          event.stopPropagation()
          onClose()
        }}
        className="absolute right-4 top-4 flex h-10 w-10 items-center justify-center rounded-full bg-paper/90 text-ink shadow-card transition-transform duration-200 hover:scale-105 focus:outline-none focus-visible:ring-2 focus-visible:ring-amber"
      >
        <CloseOutlined />
      </button>

      <img
        src={src}
        alt={alt}
        onClick={(event) => event.stopPropagation()}
        className="max-h-[88vh] max-w-[92vw] rounded-card object-contain shadow-lift"
      />

      <span className="pointer-events-none absolute bottom-5 left-1/2 -translate-x-1/2 rounded-full bg-ink/60 px-3 py-1 text-xs text-paper">
        Esc 或点击背景关闭
      </span>
    </div>,
    document.body,
  )
}

export default ImageLightbox
