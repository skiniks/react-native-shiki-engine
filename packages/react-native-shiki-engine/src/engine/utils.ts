import type { IOnigMatch } from '@shikijs/vscode-textmate'

interface OnigResult {
  readonly index: number
  readonly captureIndices: readonly {
    readonly start: number
    readonly end: number
    readonly length: number
  }[]
}

export function convertToOnigMatch(result: OnigResult | null): IOnigMatch | null {
  if (!result)
    return null

  return {
    index: result.index,
    captureIndices: result.captureIndices.map(capture => ({
      start: capture.start,
      end: capture.end,
      length: capture.length,
    })),
  }
}
