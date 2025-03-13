import type { ContentInset } from '../specs/ShikiHighlighterViewNativeComponent'

/**
 * Configuration options for the ShikiHighlighterView component
 */
export interface ViewConfigOptions {
  /**
   * Font size for the code. Default: 14
   */
  fontSize?: number

  /**
   * Font family for the code. Default: 'Menlo'
   */
  fontFamily?: string

  /**
   * Font weight for the code. Default: 'regular'
   */
  fontWeight?: 'regular' | 'bold' | 'light' | 'medium' | 'semibold' | string

  /**
   * Font style for the code. Default: 'normal'
   */
  fontStyle?: 'normal' | 'italic' | string

  /**
   * Whether to show line numbers. Default: false
   */
  showLineNumbers?: boolean

  /**
   * Whether the view is scrollable. Default: true
   */
  scrollEnabled?: boolean

  /**
   * Whether the text is selectable. Default: true
   */
  selectable?: boolean

  /**
   * Content insets for the view
   */
  contentInset?: ContentInset
}

/**
 * Default view configuration
 */
export const DEFAULT_VIEW_CONFIG: Required<ViewConfigOptions> = {
  fontSize: 14,
  fontFamily: 'Menlo',
  fontWeight: 'regular',
  fontStyle: 'normal',
  showLineNumbers: false,
  scrollEnabled: true,
  selectable: true,
  contentInset: {
    top: 0,
    right: 0,
    bottom: 0,
    left: 0,
  },
}

/**
 * Create a view configuration with custom options
 */
export function createViewConfig(options: ViewConfigOptions = {}): Required<ViewConfigOptions> {
  return {
    ...DEFAULT_VIEW_CONFIG,
    ...options,
    contentInset: {
      ...DEFAULT_VIEW_CONFIG.contentInset,
      ...options.contentInset,
    },
  }
}
