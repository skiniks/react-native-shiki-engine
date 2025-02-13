declare module '@shikijs/langs/dist/*.mjs' {
  import type { LanguageRegistration } from 'shiki'

  const lang: LanguageRegistration | LanguageRegistration[]
  export default lang
}

declare module '@shikijs/themes/dist/*.mjs' {
  import type { ThemeRegistration } from 'shiki'

  const theme: ThemeRegistration
  export default theme
}
