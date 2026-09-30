import { imagesApi } from '../api/images'
import type { BookPage } from '../types/page'
import { MAX_PAGES_PER_BOOK } from './constants'

/**
 * 绘本 PDF 导出（v2.2「轻量多页」）。
 *
 * 方案（对照 docs/07 §12）：逐页把「图片 + 页标题」用 html2canvas 渲染成 canvas，
 * 再嵌入 jsPDF，从而规避 jsPDF 内置字体缺少中文导致的标题乱码。
 * 全程在浏览器完成，**不调用后端导出接口、不落盘**。
 *
 * 注意：`jspdf` / `html2canvas` 体积较大，改为**动态 import**，
 * 只在用户真正点击「导出 PDF」时才加载，避免拖慢首屏。
 */

type CanvasRenderer = (
  element: HTMLElement,
  options: Record<string, unknown>,
) => Promise<HTMLCanvasElement>

const isBrowser = typeof document !== 'undefined'

/** 生成下载文件名（去掉路径分隔符等非法字符）。 */
export function buildPdfFilename(bookTitle: string): string {
  const safe = (bookTitle || 'storycanvas').replace(/[\\/:*?"<>|]/g, '_').trim()
  return `${safe || 'storycanvas'}.pdf`
}

function blobToDataUrl(blob: Blob): Promise<string> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = () => resolve(String(reader.result))
    reader.onerror = () => reject(new Error('图片读取失败'))
    reader.readAsDataURL(blob)
  })
}

/** 把一页渲染成 canvas（离屏元素，渲染后立即移除）。 */
async function renderPage(
  render: CanvasRenderer,
  width: number,
  height: number,
  title: string,
  dataUrl?: string,
): Promise<HTMLCanvasElement> {
  const container = document.createElement('div')
  container.style.cssText = `position:fixed;left:-10000px;top:0;width:${width}px;height:${height}px;background:#ffffff;padding:36px;box-sizing:border-box;display:flex;flex-direction:column;font-family:-apple-system,"PingFang SC","Microsoft YaHei",sans-serif;`

  const heading = document.createElement('div')
  heading.textContent = title
  heading.style.cssText =
    'font-size:30px;font-weight:600;color:#1f2937;text-align:center;margin-bottom:24px;'

  container.appendChild(heading)

  if (dataUrl) {
    const img = document.createElement('img')
    img.src = dataUrl
    img.style.cssText = 'flex:1;min-height:0;max-width:100%;object-fit:contain;margin:0 auto;'
    container.appendChild(img)
  }

  document.body.appendChild(container)
  try {
    if (dataUrl) {
      const img = container.querySelector('img')
      // decode() 保证图片已解码，避免 canvas 出现空白
      await img?.decode?.().catch(() => undefined)
    }
    return await render(container, {
      scale: 2,
      backgroundColor: '#ffffff',
      logging: false,
      useCORS: true,
    })
  } finally {
    container.remove()
  }
}

/**
 * 导出绘本为多页 PDF 并触发浏览器下载。
 *
 * @throws 当页为空或超过上限时抛出可展示的错误信息
 */
export async function exportBookToPdf(bookTitle: string, pages: BookPage[]): Promise<void> {
  if (!isBrowser) {
    throw new Error('PDF 导出仅支持在浏览器中进行')
  }
  if (pages.length === 0) {
    throw new Error('该绘本还没有页面，请先把生成结果保存到绘本')
  }
  if (pages.length > MAX_PAGES_PER_BOOK) {
    throw new Error(`页数超过上限（最多 ${MAX_PAGES_PER_BOOK} 页）`)
  }

  const [{ default: html2canvas }, { jsPDF }] = await Promise.all([
    import('html2canvas'),
    import('jspdf'),
  ])

  const pdf = new jsPDF({ orientation: 'portrait', unit: 'pt', format: 'a4' })
  const pageWidth = pdf.internal.pageSize.getWidth()
  const pageHeight = pdf.internal.pageSize.getHeight()

  const ordered = [...pages].sort((a, b) => a.page_number - b.page_number)

  for (let index = 0; index < ordered.length; index += 1) {
    const page = ordered[index]
    if (index > 0) {
      pdf.addPage()
    }

    let dataUrl: string | undefined
    if (page.image_id) {
      const blob = await imagesApi.fetchBlob(page.image_id)
      dataUrl = await blobToDataUrl(blob)
    }

    const canvas = await renderPage(
      html2canvas as unknown as CanvasRenderer,
      pageWidth,
      pageHeight,
      page.title || `第 ${page.page_number} 页`,
      dataUrl,
    )
    pdf.addImage(canvas.toDataURL('image/jpeg', 0.92), 'JPEG', 0, 0, pageWidth, pageHeight)
  }

  pdf.save(buildPdfFilename(bookTitle))
}
