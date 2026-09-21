import dayjs from 'dayjs'

export function formatDateTime(value?: string | null, pattern = 'YYYY-MM-DD HH:mm'): string {
  if (!value) {
    return '-'
  }

  const normalized = value
    .replace(' ', 'T')                  // 2026-09-21T15:30:55.463578+00
    .replace(/(\.\d{3})\d+/, '$1')      // 微秒截断为毫秒
    .replace(/([+-]\d{2})$/, '$1:00')   // +00 → +00:00

  const parsed = dayjs(normalized)
  return parsed.isValid() ? parsed.format(pattern) : value
}
