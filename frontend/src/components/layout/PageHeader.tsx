import { ArrowLeftOutlined } from '@ant-design/icons'
import { Button, Typography } from 'antd'
import type { ReactNode } from 'react'

const { Title, Paragraph } = Typography

interface PageHeaderProps {
  title: ReactNode
  description?: ReactNode
  /** 右侧操作区，如「新建绘本」 */
  extra?: ReactNode
  onBack?: () => void
  backLabel?: string
}

/** 页面级标题条：取代各页自带头部按钮组的散落排版 */
function PageHeader({ title, description, extra, onBack, backLabel = '返回' }: PageHeaderProps) {
  return (
    <header className="animate-enter mb-6 flex flex-wrap items-start justify-between gap-3">
      <div className="min-w-0">
        <div className="flex flex-wrap items-center gap-2">
          {onBack ? (
            <Button type="text" size="small" icon={<ArrowLeftOutlined />} onClick={onBack}>
              {backLabel}
            </Button>
          ) : null}
          <Title level={2} className="!mb-0">
            {title}
          </Title>
        </div>
        {description ? (
          <Paragraph type="secondary" className="!mb-0 mt-2 max-w-2xl">
            {description}
          </Paragraph>
        ) : null}
      </div>
      {extra ? <div className="flex flex-wrap items-center gap-2">{extra}</div> : null}
    </header>
  )
}

export default PageHeader
