export const typescriptSnippet = `// Advanced TypeScript Example
// Demonstrates generics, interfaces, decorators, and more

// Generic interface for data models
interface Model<T> {
  id: string;
  data: T;
  createdAt: Date;
  updatedAt: Date;
}

// Type for user data
interface UserData {
  username: string;
  email: string;
  preferences: {
    theme: 'light' | 'dark' | 'system';
    notifications: boolean;
    language: string;
  };
}

// Decorator factory for logging method calls
function log(target: any, propertyKey: string, descriptor: PropertyDescriptor) {
  const originalMethod = descriptor.value;

  descriptor.value = function(...args: any[]) {
    console.log(\`Calling \${propertyKey} with arguments: \${JSON.stringify(args)}\`);
    const result = originalMethod.apply(this, args);
    console.log(\`Method \${propertyKey} returned: \${JSON.stringify(result)}\`);
    return result;
  };

  return descriptor;
}

// Utility type helpers
type Nullable<T> = T | null;
type ReadOnly<T> = { readonly [P in keyof T]: T[P] };
type DeepPartial<T> = {
  [P in keyof T]?: T[P] extends object ? DeepPartial<T[P]> : T[P];
};

// Repository class for handling user data
class UserRepository {
  private users: Map<string, Model<UserData>> = new Map();

  @log
  async findById(id: string): Promise<Nullable<Model<UserData>>> {
    // Simulate async database query
    return new Promise((resolve) => {
      setTimeout(() => {
        resolve(this.users.get(id) || null);
      }, 100);
    });
  }

  @log
  async create(userData: UserData): Promise<Model<UserData>> {
    const id = \`user_\${Date.now()}\`;
    const now = new Date();

    const user: Model<UserData> = {
      id,
      data: userData,
      createdAt: now,
      updatedAt: now
    };

    this.users.set(id, user);
    return user;
  }

  @log
  async update(id: string, updates: DeepPartial<UserData>): Promise<Nullable<Model<UserData>>> {
    const user = this.users.get(id);
    if (!user) return null;

    // Deep merge the updates
    const updatedData = this.deepMerge(user.data, updates);

    const updatedUser: Model<UserData> = {
      ...user,
      data: updatedData,
      updatedAt: new Date()
    };

    this.users.set(id, updatedUser);
    return updatedUser;
  }

  private deepMerge<T>(target: T, source: DeepPartial<T>): T {
    const output = { ...target };

    if (isObject(target) && isObject(source)) {
      Object.keys(source).forEach(key => {
        if (isObject(source[key])) {
          if (!(key in target)) {
            Object.assign(output, { [key]: source[key] });
          } else {
            output[key] = this.deepMerge(target[key], source[key]);
          }
        } else {
          Object.assign(output, { [key]: source[key] });
        }
      });
    }

    return output;
  }
}

// Type guard for objects
function isObject(item: any): item is Record<string, any> {
  return item && typeof item === 'object' && !Array.isArray(item);
}

// Example usage
async function main() {
  const repo = new UserRepository();

  // Create a new user
  const user = await repo.create({
    username: 'johndoe',
    email: 'john@example.com',
    preferences: {
      theme: 'dark',
      notifications: true,
      language: 'en-US'
    }
  });

  console.log('Created user:', user);

  // Update user preferences
  const updated = await repo.update(user.id, {
    preferences: {
      theme: 'light',
      notifications: false
    }
  });

  console.log('Updated user:', updated);

  // Retrieve the user
  const retrieved = await repo.findById(user.id);
  console.log('Retrieved user:', retrieved);
}

// Run the example
main().catch(console.error);
`
