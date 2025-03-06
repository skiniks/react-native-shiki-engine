declare module '@shikijs/langs/*.mjs' {
  import type { LanguageRegistration } from 'shiki'

  const lang: LanguageRegistration | LanguageRegistration[]
  export default lang
}

declare module '@shikijs/langs/*' {
  import type { LanguageRegistration } from 'shiki'

  const lang: LanguageRegistration | LanguageRegistration[]
  export default lang
}

declare module '@shikijs/themes/*.mjs' {
  import type { ThemeRegistration } from 'shiki'

  const theme: ThemeRegistration
  export default theme
}

declare module '@shikijs/themes/*' {
  import type { ThemeRegistration } from 'shiki'

  const theme: ThemeRegistration
  export default theme
}
