import type { ShikiHighlighterViewProps } from '../specs'
import React, { useEffect, useState } from 'react'
import { useShikiHighlighter } from '../hooks'
import { ShikiHighlighterViewNativeComponent } from '../specs'

export function ShikiHighlighterView(props: ShikiHighlighterViewProps) {
  const {
    tokens,
    text,
    language,
    theme,
    fontSize,
    fontFamily,
    scrollEnabled,
    selectable = true,
    ...rest
  } = props

  const [internalSelectable, setInternalSelectable] = useState(selectable)
  const { getLoadedLanguages, getLoadedThemes } = useShikiHighlighter({})
  const [loadedLanguages, setLoadedLanguages] = useState<string[]>([])
  const [loadedThemes, setLoadedThemes] = useState<string[]>([])

  useEffect(() => {
    const loadAvailable = async () => {
      try {
        const languages = await getLoadedLanguages()
        const themes = await getLoadedThemes()
        setLoadedLanguages(languages)
        setLoadedThemes(themes)
      }
      catch (error) {
        console.error('Failed to load available languages and themes:', error)
      }
    }

    loadAvailable()
  }, [getLoadedLanguages, getLoadedThemes])

  useEffect(() => {
    setInternalSelectable(selectable)

    if (selectable) {
      const timer = setTimeout(() => {
        setInternalSelectable(false)
        const resetTimer = setTimeout(() => {
          setInternalSelectable(true)
        }, 50)
        return () => clearTimeout(resetTimer)
      }, 300)
      return () => clearTimeout(timer)
    }

    return () => {}
  }, [selectable])

  const validLanguage = language && loadedLanguages.includes(language) ? language : undefined
  const validTheme = theme && loadedThemes.includes(theme) ? theme : undefined

  return (
    <ShikiHighlighterViewNativeComponent
      tokens={tokens}
      text={text}
      language={validLanguage}
      theme={validTheme}
      fontSize={fontSize}
      fontFamily={fontFamily}
      scrollEnabled={scrollEnabled}
      selectable={internalSelectable}
      {...rest}
    />
  )
}
