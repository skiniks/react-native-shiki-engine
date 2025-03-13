import type { ShikiHighlighterViewProps } from '../specs'
import React, { useEffect, useState } from 'react'
import { ShikiHighlighterViewNativeComponent } from '../specs'

export function ShikiHighlighterView(props: ShikiHighlighterViewProps) {
  const { tokens, text, fontSize, fontFamily, scrollEnabled, selectable = true, ...rest } = props

  const [internalSelectable, setInternalSelectable] = useState(selectable)

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

  return (
    <ShikiHighlighterViewNativeComponent
      tokens={tokens}
      text={text}
      fontSize={fontSize}
      fontFamily={fontFamily}
      scrollEnabled={scrollEnabled}
      selectable={internalSelectable}
      {...rest}
    />
  )
}
