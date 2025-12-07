import { Platform, StyleSheet } from 'react-native'

const monospaceFontFamily = Platform.select({
  ios: 'Menlo',
  android: 'monospace',
  web: 'monospace',
})

export const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#2e3440',
  },
  header: {
    marginTop: 20,
    marginBottom: 30,
    marginHorizontal: 24,
  },
  title: {
    fontSize: 32,
    fontWeight: '700',
    marginBottom: 16,
    color: '#ECEFF4',
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
  platformBadge: {
    backgroundColor: '#5E81AC',
    paddingVertical: 6,
    paddingHorizontal: 12,
    borderRadius: 6,
    marginLeft: 12,
  },
  platformText: {
    fontSize: 14,
    color: '#ECEFF4',
    fontWeight: '600',
  },
  demoSection: {
    flex: 1,
    paddingBottom: 20,
  },
  codeContainer: {
    padding: 16,
    marginVertical: 16,
    marginHorizontal: 24,
    borderRadius: 12,
    backgroundColor: '#3B4252',
    flexShrink: 1,
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
  languageTag: {
    position: 'absolute',
    top: 8,
    right: 12,
    backgroundColor: '#4C566A',
    color: '#ECEFF4',
    fontSize: 12,
    paddingVertical: 2,
    paddingHorizontal: 8,
    borderRadius: 6,
    fontWeight: '600',
    zIndex: 1,
  },
})
