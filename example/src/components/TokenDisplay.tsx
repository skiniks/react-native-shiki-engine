import type { ViewStyle } from 'react-native'
import type { Token } from 'react-native-shiki'
import { ScrollView, StyleSheet, Text, View } from 'react-native'

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  codeWrapper: {
    padding: 16,
  },
  codeText: {
    fontFamily: 'Menlo',
    fontSize: 12,
    lineHeight: 20,
  },
})

interface TokenDisplayProps {
  tokens: Token[]
  code: string
  style?: ViewStyle
}

export function TokenDisplay({ tokens, code, style }: TokenDisplayProps) {
  return (
    <ScrollView style={[styles.container, style]}>
      <ScrollView horizontal showsHorizontalScrollIndicator>
        <View style={styles.codeWrapper}>
          <Text style={styles.codeText}>
            {tokens.map((token) => {
              const content = code.slice(token.start, token.start + token.length)
              const tokenKey = `${token.start}-${token.length}-${token.scope}`
              return (
                <Text
                  key={tokenKey}
                  style={{
                    color: token.style?.color,
                    backgroundColor: token.style?.backgroundColor,
                    fontWeight: token.style?.bold ? 'bold' : undefined,
                    fontStyle: token.style?.italic ? 'italic' : undefined,
                    textDecorationLine: token.style?.underline ? 'underline' : undefined,
                  }}
                >
                  {content}
                </Text>
              )
            })}
          </Text>
        </View>
      </ScrollView>
    </ScrollView>
  )
}
