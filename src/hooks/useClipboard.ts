import { useState } from 'react'
import { Clipboard } from '../utils'

interface UseClipboardResult {
  copyStatus: string
  copyToClipboard: (text: string) => Promise<void>
}

export function useClipboard(): UseClipboardResult {
  const [copyStatus, setCopyStatus] = useState('Copy')

  const copyToClipboard = async (text: string) => {
    try {
      await Clipboard.setString(text)
      setCopyStatus('Copied!')
      setTimeout(() => setCopyStatus('Copy'), 2000)
    }
    catch (err) {
      console.error('Failed to copy:', err)
      setCopyStatus('Failed to copy')
      setTimeout(() => setCopyStatus('Copy'), 2000)
    }
  }

  return {
    copyStatus,
    copyToClipboard,
  }
}
