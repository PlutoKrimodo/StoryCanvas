import { useState } from 'react'
import { Button, message } from 'antd'
import { FilePdfOutlined } from '@ant-design/icons'

import type { BookPage } from '../../types/page'
import { exportBookToPdf } from '../../utils/pdf'

interface ExportPdfButtonProps {
  bookTitle: string
  pages: BookPage[]
  disabled?: boolean
}

/** 导出绘本为多页 PDF（前端生成，不落盘、不走后端导出接口）。 */
function ExportPdfButton({ bookTitle, pages, disabled }: ExportPdfButtonProps) {
  const [exporting, setExporting] = useState(false)

  const handleExport = async () => {
    setExporting(true)
    try {
      await exportBookToPdf(bookTitle, pages)
      message.success('PDF 已开始下载')
    } catch (error) {
      console.error('导出 PDF 失败', error)
      message.error(error instanceof Error ? error.message : '导出 PDF 失败')
    } finally {
      setExporting(false)
    }
  }

  return (
    <Button
      icon={<FilePdfOutlined />}
      loading={exporting}
      disabled={disabled || pages.length === 0}
      onClick={handleExport}
    >
      导出 PDF
    </Button>
  )
}

export default ExportPdfButton
