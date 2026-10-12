import '@testing-library/jest-dom'

import { vi } from 'vitest'

// jsdom 缺这两个 API：antd 的响应式钩子（Grid/Steps）与弹层定位会在挂载时取用
class StubObserver {
  observe() {}
  unobserve() {}
  disconnect() {}
}

vi.stubGlobal('ResizeObserver', StubObserver)
vi.stubGlobal('matchMedia', (query: string) => ({
  matches: false,
  media: query,
  onchange: null,
  addListener() {},
  removeListener() {},
  addEventListener() {},
  removeEventListener() {},
  dispatchEvent: () => false,
}))
