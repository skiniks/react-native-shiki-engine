import type { IOnigMatch } from '@shikijs/vscode-textmate'

export type PackedOnigMatch = readonly number[]

export function convertToOnigMatch(result: PackedOnigMatch | null): IOnigMatch | null {
  if (!result || result.length < 1) return null

  const index = result[0]
  if (typeof index !== 'number') return null

  const captureIndices: IOnigMatch['captureIndices'] = []
  for (let i = 1; i + 1 < result.length; i += 2) {
    const start = result[i]
    const end = result[i + 1]
    if (typeof start !== 'number' || typeof end !== 'number') continue
    captureIndices.push({ start, end, length: end - start })
  }

  return { index, captureIndices }
}
