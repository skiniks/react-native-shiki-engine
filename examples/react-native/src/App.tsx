import type { ThemedToken } from '@shikijs/core'
import React, { useEffect, useState } from 'react'
import { Text, View } from 'react-native'
import { SafeAreaProvider, SafeAreaView } from 'react-native-safe-area-context'
import { isNativeEngineAvailable } from 'react-native-shiki-engine'
import { TokenDisplay } from './components/TokenDisplay'
import { HighlighterProvider } from './contexts/highlighter'
import { useHighlighter } from './hooks/useHighlighter'
import { styles } from './styles'

const code = `
use std::collections::HashMap;

// Define a custom error type for commands
#[derive(Debug)]
enum CommandError {
    UnknownCommand,
    InvalidArguments,
}

// Define a trait for executable commands
trait Command {
    fn execute(&self, args: Vec<String>) -> Result<String, CommandError>;
}

// Command to calculate the factorial of a number
struct FactorialCommand;

impl Command for FactorialCommand {
    fn execute(&self, args: Vec<String>) -> Result<String, CommandError> {
        if args.len() != 1 {
            return Err(CommandError::InvalidArguments);
        }

        let n: u64 = args[0]
            .parse()
            .map_err(|_| CommandError::InvalidArguments)?;

        let result = (1..=n).product::<u64>();
        Ok(format!("Factorial of {} is {}", n, result))
    }
}

// Command to reverse a string
struct ReverseCommand;

impl Command for ReverseCommand {
    fn execute(&self, args: Vec<String>) -> Result<String, CommandError> {
        if args.len() != 1 {
            return Err(CommandError::InvalidArguments);
        }

        let reversed = args[0].chars().rev().collect::<String>();
        Ok(format!("Reversed string: {}", reversed))
    }
}

// Command manager to register and execute commands
struct CommandManager {
    commands: HashMap<String, Box<dyn Command>>,
}

impl CommandManager {
    fn new() -> Self {
        Self {
            commands: HashMap::new(),
        }
    }

    fn register_command<C: Command + 'static>(&mut self, name: &str, command: C) {
        self.commands.insert(name.to_string(), Box::new(command));
    }

    fn execute_command(&self, name: &str, args: Vec<String>) -> String {
        match self.commands.get(name) {
            Some(command) => match command.execute(args) {
                Ok(output) => output,
                Err(err) => format!("Error executing command: {:?}", err),
            },
            None => "Unknown command".to_string(),
        }
    }
}

fn main() {
    let mut manager = CommandManager::new();

    // Register commands
    manager.register_command("factorial", FactorialCommand);
    manager.register_command("reverse", ReverseCommand);

    // Execute some commands
    println!("{}", manager.execute_command("factorial", vec!["5".to_string()]));
    println!("{}", manager.execute_command("reverse", vec!["hello".to_string()]));
    println!("{}", manager.execute_command("unknown", vec!["test".to_string()]));
    println!(
        "{}",
        manager.execute_command("factorial", vec!["not_a_number".to_string()])
    );
}
`

function ShikiDemo() {
  const [engineStatus, setEngineStatus] = useState('Initializing...')
  const [tokens, setTokens] = useState<ThemedToken[][]>([])
  const [error, setError] = useState('')
  const highlighter = useHighlighter()

  useEffect(() => {
    const initializeApp = async () => {
      try {
        const available = isNativeEngineAvailable()
        setEngineStatus(available ? 'Available' : 'Not Available')

        if (!available)
          throw new Error('Native engine not available.')

        await highlighter.initialize()

        const tokenized = highlighter.tokenize(code, {
          lang: 'rust',
          theme: 'dracula',
        })

        setTokens(tokenized)
      }
      catch (err: unknown) {
        if (err instanceof Error) {
          setError(err.message)
          console.error('Tokenization error:', err)
        }
        else {
          setError('An unknown error occurred.')
          console.error('Unknown error:', err)
        }
      }
    }

    initializeApp()

    return () => {
      highlighter.dispose()
    }
  }, [highlighter])

  return (
    <SafeAreaView style={styles.container} edges={['top', 'left', 'right']}>
      <View style={styles.header}>
        <Text style={styles.title}>React Native Shiki Engine</Text>
        <View style={styles.statusContainer}>
          <Text style={styles.statusLabel}>Engine Status:</Text>
          <Text style={styles.statusValue}>{engineStatus}</Text>
        </View>
      </View>

      <View style={styles.demoSection}>
        <Text style={styles.languageTag}>rust</Text>
        {error
          ? (
              <View style={styles.errorContainer}>
                <Text style={styles.errorText}>{error}</Text>
              </View>
            )
          : (
              <TokenDisplay tokens={tokens} />
            )}
      </View>
    </SafeAreaView>
  )
}

function App() {
  return (
    <SafeAreaProvider>
      <HighlighterProvider>
        <ShikiDemo />
      </HighlighterProvider>
    </SafeAreaProvider>
  )
}

export default App
