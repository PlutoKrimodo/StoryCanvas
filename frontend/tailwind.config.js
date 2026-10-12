/** @type {import('tailwindcss').Config} */
const withAlpha = (name) => `rgb(var(${name}) / <alpha-value>)`

export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      // 令牌值见 src/styles/globals.css 的 :root 与 src/theme/palette.ts
      colors: {
        canvas: withAlpha('--sc-canvas'),
        canvas2: withAlpha('--sc-canvas-2'),
        paper: withAlpha('--sc-paper'),
        ink: withAlpha('--sc-ink'),
        ink2: withAlpha('--sc-ink-2'),
        ink3: withAlpha('--sc-ink-3'),
        hairline: withAlpha('--sc-hairline'),
        hairline2: withAlpha('--sc-hairline-2'),
        clay: {
          DEFAULT: withAlpha('--sc-clay'),
          hover: withAlpha('--sc-clay-hover'),
          active: withAlpha('--sc-clay-active'),
        },
        amber: withAlpha('--sc-amber'),
        // logo 五色环：仅用于字母上色、选中态与状态点，不用于小字
        accent: {
          poppy: '#D2433B',
          amber: '#E2A13C',
          leaf: '#6FA96B',
          sky: '#4E86BE',
          plum: '#8E5B9F',
        },
      },
      borderRadius: {
        card: '16px',
        chip: '10px',
      },
      boxShadow: {
        card: '0 1px 2px rgba(43,38,33,.04), 0 8px 24px -14px rgba(43,38,33,.10)',
        lift: '0 18px 40px -18px rgba(43,38,33,.22)',
      },
      fontFamily: {
        sans: ['var(--sc-font-sans)'],
        display: ['var(--sc-font-display)'],
      },
      keyframes: {
        fadeUp: {
          '0%': { opacity: '0', transform: 'translateY(8px)' },
          '100%': { opacity: '1', transform: 'translateY(0)' },
        },
        fadeIn: {
          '0%': { opacity: '0' },
          '100%': { opacity: '1' },
        },
        paintIn: {
          '0%': { opacity: '0', transform: 'translateY(6px) scale(.94)' },
          '100%': { opacity: '1', transform: 'translateY(0) scale(1)' },
        },
      },
      animation: {
        enter: 'fadeUp .5s cubic-bezier(.22,.61,.36,1) both',
        fade: 'fadeIn .35s ease-out both',
        'paint-in': 'paintIn .45s ease-out both',
      },
    },
  },
  plugins: [],
}
