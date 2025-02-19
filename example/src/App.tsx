import type { Token } from 'react-native-shiki'
import rust from '@shikijs/langs/dist/rust.mjs'
import dracula from '@shikijs/themes/dist/dracula.mjs'
import React, { useEffect, useState } from 'react'
import { SafeAreaView, Text, TouchableOpacity, View } from 'react-native'
import { Clipboard, ShikiHighlighterView } from 'react-native-shiki'
import NativeShikiHighlighter from '../../src/specs/NativeShikiHighlighter'
import { styles } from './styles'

const code = `use std::collections::HashMap;

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
}`

function ShikiDemo() {
  const [isReady, setIsReady] = useState(false)
  const [highlighterStatus, setHighlighterStatus] = useState('Initializing...')
  const [tokens, setTokens] = useState<Token[]>([])
  const [error, setError] = useState('')
  const [copyStatus, setCopyStatus] = useState('Copy')

  const handleCopy = async () => {
    try {
      await Clipboard.setString(code)
      setCopyStatus('Copied!')
      setTimeout(() => setCopyStatus('Copy'), 2000)
    }
    catch (err) {
      console.error('Failed to copy:', err)
      setCopyStatus('Failed to copy')
      setTimeout(() => setCopyStatus('Copy'), 2000)
    }
  }

  useEffect(() => {
    const initializeHighlighter = async () => {
      try {
        // Load language and theme directly through the Turbo Module
        const langData = Array.isArray(rust) ? rust[0] : rust
        const langLoaded = await NativeShikiHighlighter.loadLanguage('rust', JSON.stringify(langData))
        const themeLoaded = await NativeShikiHighlighter.loadTheme('dracula', JSON.stringify(dracula))

        if (!langLoaded || !themeLoaded) {
          throw new Error('Failed to load language or theme')
        }

        setHighlighterStatus('Available')

        // Get tokens directly from the Turbo Module
        const tokenized = await NativeShikiHighlighter.codeToTokens(code, 'rust', 'dracula')

        setTokens(tokenized)
        setIsReady(true)
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

    initializeHighlighter()
  }, [])

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>React Native Shiki</Text>
        <View style={styles.statusContainer}>
          <Text style={styles.statusLabel}>Highlighter Status:</Text>
          <Text style={styles.statusValue}>{highlighterStatus}</Text>
        </View>
      </View>

      <View style={styles.demoSection}>
        {error
          ? (
              <View style={styles.errorContainer}>
                <Text style={styles.errorText}>{error}</Text>
              </View>
            )
          : (
              <>
                {isReady && tokens.length > 0 && (
                  <ShikiHighlighterView
                    style={styles.codeContainer}
                    tokens={tokens}
                    text={code}
                    fontSize={14}
                    scrollEnabled
                  />
                )}
                <TouchableOpacity
                  style={styles.copyButton}
                  onPress={handleCopy}
                >
                  <Text style={styles.copyButtonText}>
                    {copyStatus}
                  </Text>
                </TouchableOpacity>
              </>
            )}
      </View>
    </SafeAreaView>
  )
}

export default ShikiDemo
