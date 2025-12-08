# Contributing

Contributions are always welcome, no matter how large or small!

We want this community to be friendly and respectful to each other. Please follow it in all your interactions with the project. Before contributing, please read the [code of conduct](./CODE_OF_CONDUCT.md).

## Development workflow

This project is a monorepo managed using [pnpm workspaces](https://pnpm.io/workspaces). It contains the following packages:

- The library package in `packages/react-native-shiki-engine`.
- Example apps in the `examples/` directory.

To get started with the project, run `pnpm install` in the root directory to install the required dependencies for each package:

```sh
pnpm install
```

> This project uses pnpm workspaces and includes a `pnpm-lock.yaml` lockfile. While npm or yarn may work, we strongly recommend using pnpm to ensure consistent dependency resolution and to take advantage of pnpm-specific features like the workspace catalog.

The example apps in the `examples/` directory demonstrate usage of the library. You need to run them to test any changes you make.

They are configured to use the local version of the library, so any changes you make to the library's source code will be reflected in the example apps. Changes to the library's JavaScript code will be reflected in the example apps without a rebuild, but native code changes will require a rebuild of the example app.

If you want to use Android Studio or XCode to edit the native code, you can open the `examples/react-native/android` or `examples/react-native/ios` directories respectively in those editors. To edit the Objective-C or Swift files, open `examples/react-native/ios/*.xcworkspace` in XCode and find the source files at `Pods > Development Pods > react-native-shiki-engine`.

To edit the Java or Kotlin files, open `examples/react-native/android` in Android Studio and find the source files at `react-native-shiki-engine` under `Android`.

You can use various commands to work with the example apps. Navigate to the example directory first:

```sh
cd examples/react-native
```

To start the Metro bundler:

```sh
pnpm start
```

To run the example app on Android:

```sh
pnpm android
```

To run the example app on iOS:

```sh
pnpm ios
```

To confirm that the app is running with the new architecture, you can check the Metro logs for a message like this:

```sh
Running "ShikiEngineExample" with {"fabric":true,"initialProps":{"concurrentRoot":true},"rootTag":1}
```

Note the `"fabric":true` and `"concurrentRoot":true` properties.

Make sure your code passes TypeScript and ESLint. Run the following to verify:

```sh
pnpm typecheck
pnpm lint
```

To fix formatting errors, run the following:

```sh
pnpm lint:fix
```

### Commit message convention

We follow the [conventional commits specification](https://www.conventionalcommits.org/en) for our commit messages:

- `fix`: bug fixes, e.g. fix crash due to deprecated method.
- `feat`: new features, e.g. add new method to the module.
- `refactor`: code refactor, e.g. migrate from class components to hooks.
- `docs`: changes into documentation, e.g. add usage example for the module..
- `chore`: tooling changes, e.g. change CI config.

Our pre-commit hooks verify that your commit message matches this format when committing.

### Linting

[ESLint](https://eslint.org/), [TypeScript](https://www.typescriptlang.org/)

We use [TypeScript](https://www.typescriptlang.org/) for type checking and [ESLint](https://eslint.org/) for linting and formatting the code.

Our pre-commit hooks verify that the linter and tests pass when committing.

### Publishing to npm

We use [release-it](https://github.com/release-it/release-it) to make it easier to publish new versions. It handles common tasks like bumping version based on semver, creating tags and releases etc.

To publish new versions, run the following:

```sh
pnpm release
```

### Scripts

The root `package.json` file contains various scripts for common tasks:

- `pnpm install`: setup project by installing dependencies.
- `pnpm typecheck`: type-check files with TypeScript across all packages.
- `pnpm lint`: lint files with ESLint across all packages.
- `pnpm build`: build all packages.
- `pnpm release`: publish a new version of the library.

For running example apps, navigate to the specific example directory (e.g., `cd examples/react-native`) and use the scripts defined in that package's `package.json`.

### Sending a pull request

> **Working on your first pull request?** You can learn how from this _free_ series: [How to Contribute to an Open Source Project on GitHub](https://app.egghead.io/playlists/how-to-contribute-to-an-open-source-project-on-github).

When you're sending a pull request:

- Prefer small pull requests focused on one change.
- Verify that linters are passing.
- Review the documentation to make sure it looks good.
- Follow the pull request template when opening a pull request.
- For pull requests that change the API or implementation, discuss with maintainers first by opening an issue.
