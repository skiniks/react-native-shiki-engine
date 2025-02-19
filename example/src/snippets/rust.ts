export const rustSnippet = `use std::collections::HashMap;

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
