import type { ViewConfigOptions } from '../highlighter/viewConfig'
import { useMemo } from 'react'
import { createViewConfig } from '../highlighter/viewConfig'

/**
 * Hook to create and manage view configuration
 *
 * @param options View configuration options
 * @returns The processed view configuration with defaults applied
 *
 * @example
 * ```tsx
 * const viewConfig = useViewConfig({
 *   fontSize: 16,
 *   fontFamily: 'Fira Code',
 *   showLineNumbers: true,
 * });
 *
 * return (
 *   <ShikiHighlighterView
 *     tokens={tokens}
 *     text={code}
 *     {...viewConfig}
 *   />
 * );
 * ```
 */
export function useViewConfig(options: ViewConfigOptions = {}) {
  return useMemo(() => createViewConfig(options), [options])
}
