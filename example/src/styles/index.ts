import { Platform, StyleSheet } from 'react-native'

const monospaceFontFamily = Platform.select({
  ios: 'Menlo',
  android: 'monospace',
})

export const styles = StyleSheet.create({
  container: {
    flex: 1,
    paddingHorizontal: 24,
    paddingTop: 60,
    paddingBottom: 20,
    backgroundColor: '#2e3440',
  },
  header: {
    marginTop: 40,
    marginBottom: 30,
    marginHorizontal: 16,
  },
  title: {
    fontSize: 32,
    fontWeight: '700',
    marginBottom: 16,
    color: '#ECEFF4',
  },
  backButton: {
    fontSize: 16,
    color: '#88C0D0',
    marginBottom: 16,
  },
  statusContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#3B4252',
    paddingVertical: 12,
    paddingHorizontal: 16,
    borderRadius: 8,
    alignSelf: 'flex-start',
  },
  statusLabel: {
    fontSize: 16,
    color: '#D8DEE9',
    marginRight: 10,
  },
  statusValue: {
    fontSize: 16,
    color: '#ECEFF4',
    fontWeight: '600',
  },
  demoSection: {
    flex: 1,
  },
  codeContainer: {
    position: 'relative',
    backgroundColor: '#282A36',
    borderRadius: 8,
    marginTop: 16,
    flex: 1,
  },
  copyButton: {
    position: 'absolute',
    top: 8,
    right: 8,
    backgroundColor: '#44475A',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    zIndex: 1,
  },
  copyButtonText: {
    color: '#F8F8F2',
    fontSize: 14,
    fontWeight: '500',
  },
  codeScrollContainer: {
    flexDirection: 'column',
    minWidth: '100%',
  },
  codeLine: {
    fontSize: 16,
    fontFamily: monospaceFontFamily,
    lineHeight: 24,
    marginBottom: 6,
    flexDirection: 'row',
  },
  codeText: {
    fontSize: 16,
    fontFamily: monospaceFontFamily,
    lineHeight: 24,
  },
  errorContainer: {
    padding: 16,
    backgroundColor: '#bf616a22',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#bf616a',
    marginHorizontal: 16,
  },
  errorText: {
    color: '#bf616a',
    fontSize: 16,
  },
  advancedButton: {
    backgroundColor: '#6272a4',
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderRadius: 8,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: 20,
    alignSelf: 'flex-start',
    marginHorizontal: 16,
  },
  advancedButtonText: {
    color: '#F8F8F2',
    fontSize: 16,
    fontWeight: '500',
  },
})
